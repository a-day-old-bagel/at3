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

  AiSystem::AiSystem(State *state, float minX, float maxX, float minY, float maxY, float spawnHeight)
      : System(state), minX(minX), maxX(maxX), minY(minY), maxY(maxY), spawnHeight(spawnHeight) {

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
      std::vector<entityId> countedTargets;
      for (int i = 0; i < participants.size(); ++i) {
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

        auto &pairs = sweeperAi->ghostObject->getOverlappingPairs();
        int numPairs = sweeperAi->ghostObject->getNumOverlappingObjects();
        if (numPairs > 2) {
          for (int j = 2; j < numPairs; ++j) {
            entityId touched = (entityId) pairs.at(j)->getUserIndex();
            compMask compsPresent = state->getComponents(touched);
            if (compsPresent & SWEEPERTARGET) {
//              if (std::find(countedTargets.begin(), countedTargets.end(), touched) == countedTargets.end()) {
              if (true) {
                for (auto id : countedTargets) {
                  std::cout << id << " ";
                }
                std::cout << "(" << countedTargets.size() << ") " << std::endl;
                countedTargets.push_back(touched);
                ++sweeperAi->fitness;
                Physics *physics;
                state->get_Physics(touched, &physics);
                glm::mat4 newTransform = randTransformWithinDomain(rayFunc, 200.f, 5.f);
                physics->setTransform(newTransform);
                physics->beStill();
                physics->rigidBody->activate();
                physics->rigidBody->applyCentralImpulse({0.f, 0.f, 1.f});
                std::cout << i << " counted " << touched << std::endl;
              } else {
                std::cout << i << " ALREADY COUNTED " << touched << "!" << std::endl;
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

      Physics *physics;
      for (int i = 0; i < registries[0].ids.size(); ++i) {
        state->get_Physics(registries[0].ids.at(i), &physics);
        state->get_SweeperAi(registries[0].ids.at(i), &sweeperAi);
        population.at(i).dFitness = std::min(sweeperAi->fitness, sweeperAi->fitness / (sweeperAi->distance * 0.1f));
        if (i == 0) {
          std::cout << "0's FIT: " << sweeperAi->fitness << " -> " << population.at(i).dFitness << std::endl;
        }
        sweeperAi->fitness = 0;
        glm::mat4 newTransform = randTransformWithinDomain(rayFunc, 200.f, 5.f);
        physics->setTransform(newTransform);
        physics->beStill();
      }

      //run the GA to create a new population
      population = geneticAlgorithm->Epoch(population);

      for (int i = 0; i < registries[0].ids.size(); ++i) {
        state->get_SweeperAi(registries[0].ids.at(i), &sweeperAi);
        sweeperAi->net.PutWeights(population.at(i).vecWeights);
      }
    }
  }

  bool AiSystem::handleEvent(SDL_Event &event) {
    switch (event.type) {
      case SDL_KEYDOWN:
        switch (event.key.keysym.scancode) {
          default:
            return false; // could not handle it here
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
    for (int i = 0; i < participants.size(); ++i) {
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
}

#pragma clang diagnostic pop