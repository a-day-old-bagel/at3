/*
 * Copyright (c) 2016 Galen Cochrane
 * Galen Cochrane <galencochrane@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "ecsSystem_ai.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "IncompatibleTypes"
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

#define TRACK_TORQUE 100.f

namespace at3 {

  void nullDraw(glm::vec3&, glm::vec3&, glm::vec3&) {
    return;
  }

  AiSystem::AiSystem(State *state, float minX, float maxX, float minY, float maxY, float spawnHeight)
      : System(state), minX(minX), maxX(maxX), minY(minY), maxY(maxY), spawnHeight(spawnHeight) {
    srand(133701337);
  }

  AiSystem::~AiSystem() {
    if (geneticAlgorithm) {
      delete geneticAlgorithm;
    }
  }

  bool AiSystem::onInit() {
    registries[0].discoverHandler = DELEGATE(&AiSystem::onDiscover, this);
    registries[0].forgetHandler = DELEGATE(&AiSystem::onForget, this);
    return true;
  }

  void AiSystem::onTick(float dt) {
    if (!simulationStarted) { return; }

    Placement *placement;
    SweeperAi *sweeperAi;

    std::vector<SVector2D> vecTargets(registries[1].ids.size());
    for (auto target : registries[1].ids) {
      state->get_Placement(target, &placement);
      vecTargets.push_back(SVector2D(placement->mat[3][0], placement->mat[3][1]));
    }

    uint32_t nowTime = SDL_GetTicks();
    uint32_t deltaTime = nowTime - lastTime;

    if (deltaTime < CParams::iNumTicks) {
      for (size_t i = 0; i < participants.size(); ++i) {
        state->get_SweeperAi(participants[i], &sweeperAi);
        state->get_Placement(participants[i], &placement);

        double closest_so_far = 99999;
        float distanceToTarget;
        glm::vec2 toTargetNormalized(0, 0);
        // cycle through mines to find closest
        for (auto target : registries[1].ids) {
          Placement *targetPlacement;
          state->get_Placement(target, &targetPlacement);
          glm::vec3 toTarget3 = targetPlacement->getTranslation() - placement->getTranslation();
          glm::vec2 toTarget = glm::vec2(toTarget3.x, toTarget3.y);
          distanceToTarget = glm::length(toTarget);
          if (distanceToTarget < closest_so_far) {
            closest_so_far = distanceToTarget;
            toTargetNormalized = glm::normalize(toTarget);
          }
        }
        sweeperAi->distance = distanceToTarget;

        // this will store all the inputs for the NN
        std::vector<double> inputs;
        // add in vector to closest mine
        inputs.push_back(toTargetNormalized.x);
        inputs.push_back(toTargetNormalized.y);
        // add in sweepers look at vector
        glm::vec3 lookAt = placement->getLookAt();
        glm::vec2 horizLookAt = glm::normalize(glm::vec2(lookAt.x, lookAt.y));
        inputs.push_back(horizLookAt.x);
        inputs.push_back(horizLookAt.y);

        glm::vec3 lineColor(1.f, 1.f, 0.f);
        glm::vec3 startPoint = placement->getTranslation() + glm::vec3(0.f, 0.f, 3.f);
        glm::vec3 endPoint = placement->getTranslation() + glm::vec3(horizLookAt.x, horizLookAt.y, 0.f) * 3.f
                             + glm::vec3(0.f, 0.f, 3.f);
        lineDrawFunc(startPoint, endPoint, lineColor);
        lineColor = glm::vec3(0.f, 1.f, 1.f);
        startPoint = placement->getTranslation() + glm::vec3(0.f, 0.f, 3.f);
        endPoint = placement->getTranslation() + glm::vec3(toTargetNormalized.x, toTargetNormalized.y, 0.f) * 3.f
                   + glm::vec3(0.f, 0.f, 3.f);
        lineDrawFunc(startPoint, endPoint, lineColor);

        // update the NN and position
        std::vector<double> outputs = (sweeperAi->net.Update(inputs));
        if (outputs.size() != CParams::iNumOutputs) {
          std::cout << "NN produced wrong number of outputs!\n";
          return;
        }

        TrackControls *trackControls;
        state->get_TrackControls(participants.at(i), &trackControls);
        trackControls->torque.x = (float) (outputs.at(0) * 2.f - 1.f) * TRACK_TORQUE;
        trackControls->torque.y = (float) (outputs.at(1) * 2.f - 1.f) * TRACK_TORQUE;

        trackControls->brakes.x = std::max(0.f, TRACK_TORQUE - TRACK_TORQUE * abs(trackControls->torque.x));
        trackControls->brakes.y = std::max(0.f, TRACK_TORQUE - TRACK_TORQUE * abs(trackControls->torque.y));

//          trackControls->torque.x = (float)(+outputs[0] /*+ outputs[1]*/);
//          trackControls->torque.y = (float)(-outputs[0] /*+ outputs[1]*/);
//        trackControls->brakes.x = (float)(+outputs[1] /*+ outputs[1]*/);
//        trackControls->brakes.y = (float)(-outputs[1] /*+ outputs[1]*/);

        /*trackControls->torque = glm::vec2();
        trackControls->brakes = glm::vec2();
        int numLeftWheels = 0, numRightWheels = 0;
        for (int j = 0; j < trackControls->vehicle->getNumWheels(); ++j) {
          WheelInfo wi = trackControls->wheels.at(j);
          btWheelInfo btwi = trackControls->vehicle->getWheelInfo(wi.bulletWheelId);
          float currentRot = btwi.m_deltaRotation * btwi.m_wheelsRadius;
          float desiredRot = wi.leftOrRight * (float)(wi.leftOrRight < 0 ? outputs[0] : outputs[1]);
          float appliedRot = desiredRot - currentRot;
          if (wi.leftOrRight < 0) {
            ++numLeftWheels;
            trackControls->torque.x += appliedRot * appliedRot * appliedRot;
          } else {
            ++numRightWheels;
            trackControls->torque.y += appliedRot * appliedRot * appliedRot;
          }
        }
        if (numLeftWheels) {
          trackControls->torque.x /= numLeftWheels;
          trackControls->brakes.x /= numLeftWheels;
        }
        if (numRightWheels) {
          trackControls->torque.y /= numRightWheels;
          trackControls->brakes.y /= numRightWheels;
        }*/

        // TURNINESS
        if (trackControls->torque.x) {
          float turnSlope = trackControls->torque.y / trackControls->torque.x;
          sweeperAi->turniness += abs(sweeperAi->lastTurnSlope - turnSlope) / (dt * 100000.f);
          sweeperAi->lastTurnSlope = turnSlope;
        }

        auto &pairs = sweeperAi->ghostObject->getOverlappingPairs();
        int numPairs = sweeperAi->ghostObject->getNumOverlappingObjects();
        if (numPairs > 2) {
          for (int j = 2; j < numPairs; ++j) {
            entityId touched = (entityId) pairs.at(j)->getUserIndex();
            compMask compsPresent = state->getComponents(touched);
            if (compsPresent & SWEEPERTARGET) {
              SweeperTarget *target;
              state->get_SweeperTarget(touched, &target);
              if (!target->coolDown) {
                ++sweeperAi->fitness;
                target->coolDown = true;
                target->lastTime = SDL_GetTicks();
                Physics *physics;
                state->get_Physics(touched, &physics);
                glm::mat4 newTransform = randTransformWithinDomain(rayFunc, 200.f, 20.f);
                physics->setTransform(newTransform);
                physics->beStill();
                physics->rigidBody->activate();
                physics->rigidBody->applyCentralImpulse({0.f, 0.f, 1.f});
              } else {
                if (SDL_GetTicks() - target->lastTime >= 1000) { // 1 second
                  target->coolDown = false;
                }
              }
            }
          }
        }
      }
    } else {

      //increment the generation counter
      ++generationCount;
      std::cout << "--== GENERATION " << generationCount << " ==--" << std::endl;

      lastTime = nowTime;

      float bestFitness = 0;
      float turninessOfBest = 0;
      float baseFitnessOfBest = 0;
      int indexOfBest = 0;
      entityId idOfBest = 0;
      Physics *physics;
      for (size_t i = 0; i < registries[0].ids.size(); ++i) {
        state->get_Physics(registries[0].ids.at(i), &physics);
        state->get_SweeperAi(registries[0].ids.at(i), &sweeperAi);
        float fitness2 = sweeperAi->fitness * sweeperAi->fitness;
        float trueFitness = sweeperAi->turniness *
                            std::min(fitness2, fitness2 / (sweeperAi->distance * 0.1f));
        population.at(i).dFitness = trueFitness;
        if (trueFitness > bestFitness) {
          bestFitness = trueFitness;
          turninessOfBest = sweeperAi->turniness;
          baseFitnessOfBest = sweeperAi->fitness;
          idOfBest = registries[0].ids.at(i);
          indexOfBest = i;
        }
        sweeperAi->fitness = 0;
        glm::mat4 newTransform = randTransformWithinDomain(rayFunc, 200.f, 5.f);
        physics->setTransform(newTransform);
        physics->beStill();
      }
      std::cout << "BEST FIT (#" << indexOfBest << " ID" << idOfBest << "): " << baseFitnessOfBest
                << ", " << turninessOfBest << " -> " << bestFitness << std::endl;

      //run the GA to create a new population
      population = geneticAlgorithm->Epoch(population);

      for (size_t i = 0; i < registries[0].ids.size(); ++i) {
        state->get_SweeperAi(registries[0].ids.at(i), &sweeperAi);
        sweeperAi->net.PutWeights(population.at(i).vecWeights);
      }
    }
  }

  bool AiSystem::handleEvent(SDL_Event &event) {
    switch (event.type) {
      case SDL_KEYDOWN:
        switch (event.key.keysym.scancode) {
          default: return false; // could not handle it here
        }
        break;
      default:
        return false; // could not handle it here
    }
    return true; // handled it here
  }

  void AiSystem::beginSimulation(rayFuncType &rayFunc) {
    assert((participants.size() > CParams::iNumElite) && "INSUFFICIENT NUMBER OF AI SIMULATION PARTICIPANTS");
    simulationStarted = true;

    this->rayFunc = rayFunc;
    this->minX = minX;
    this->maxX = maxX;
    this->minY = minY;
    this->maxY = maxY;
    this->spawnHeight = spawnHeight;

    SweeperAi *sweeperAi;
    state->get_SweeperAi(participants[0], &sweeperAi);
    numWeightsInNN = sweeperAi->net.GetNumberOfWeights();
    geneticAlgorithm = new CGenAlg(
        (int) participants.size(),
        CParams::dMutationRate,
        CParams::dCrossoverRate,
        numWeightsInNN
    );
    population = geneticAlgorithm->GetChromos();
    for (size_t i = 0; i < participants.size(); ++i) {
      state->get_SweeperAi(participants[i], &sweeperAi);
      sweeperAi->net.PutWeights(population[i].vecWeights);
    }

    lastTime = SDL_GetTicks();
  }

  bool AiSystem::onDiscover(const entityId &id) {
    if (!simulationStarted) {
      participants.push_back(id);
    } else {
      lateComers.push_back(id);
    }
    return true;
  }

  bool AiSystem::onForget(const entityId &id) {
    auto it = std::find(lateComers.begin(), lateComers.end(), id);
    if (it != lateComers.end()) {
      lateComers.erase(it);
    }
    it = std::find(participants.begin(), participants.end(), id);
    if (it != participants.end()) {
      assert(!simulationStarted && "DO NOT REMOVE AN ACTIVE AI PARTICIPANT - NOT SUPPORTED YET");
      participants.erase(it);
    }
    return true;
  }

  glm::vec2 AiSystem::randVec2WithinDomain() {
    glm::vec2 result(minX + RandFloat() * (maxX - minX), minY + RandFloat() * (maxY - minY));
    return result;
  }

  glm::vec2 AiSystem::randVec2WithinDomain(float scale) {
    float spanX = (maxX - minX) * scale;
    float spanY = (maxY - minY) * scale;
    float lowX = minX + ((maxX - minX) - spanX) * 0.5f;
    float lowY = minY + ((maxY - minY) - spanY) * 0.5f;
    glm::vec2 result(lowX + RandFloat() * spanX, lowY + RandFloat() * spanY);
    return result;
  }

  glm::mat4 AiSystem::randTransformWithinDomain(rayFuncType& rayFunc, float rayLen, float offsetHeight,
                                                float scale /*= 1.f*/) {
    glm::vec2 loc = randVec2WithinDomain(scale);
    btVector3 startPoint (loc.x, loc.y, spawnHeight);
    btVector3 endPoint (loc.x, loc.y, spawnHeight - rayLen);
    btCollisionWorld::ClosestRayResultCallback ray = rayFunc(startPoint, endPoint);
    glm::mat4 result;
    if (ray.hasHit()) {
      result = glm::translate(result, {loc.x, loc.y, ray.m_hitPointWorld.z() + offsetHeight});
    } else {
      result = glm::translate(result, {loc.x, loc.y, spawnHeight});
    }
    result = glm::rotate(result, (float)RandFloat() * (float)M_PI * 2.f, glm::vec3(0.f, 0.f, 1.f));
    return result;
  }
  void AiSystem::setLineDrawFunc(lineDrawFuncType &&func) {
    lineDrawFunc = func;
  }
}

#pragma clang diagnostic pop