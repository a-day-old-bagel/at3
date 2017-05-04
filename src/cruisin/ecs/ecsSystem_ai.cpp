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

#define TRACK_TORQUE 50.f

namespace at3 {

  AiSystem::AiSystem(State *state) : System(state) {

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

    std::vector<SVector2D> vecTargets(registries[1].ids.size());
    Placement* placement;
    for (int i = 0; i < registries[1].ids.size(); ++i) {
      state->get_Placement(registries[1].ids.at((size_t)i), &placement);
      vecTargets.push_back(SVector2D(placement->mat[3][0], placement->mat[3][1]));
    }

    SweeperAi *sweeperAi;

    if (ticks++ < CParams::iNumTicks)
    {
      for (int i = 0; i < participants.size(); ++i)
      {
        state->get_SweeperAi(participants[i], &sweeperAi);
        state->get_Placement(participants[i], &placement);

        double      closest_so_far = 99999;
        glm::vec2   closestTarget(0, 0);
        //cycle through mines to find closest
        for (auto target : registries[1].ids) {
          Placement *targetPlacement;
          state->get_Placement(target, &targetPlacement);
          glm::vec3 toTarget3 = targetPlacement->getTranslation() - placement->getTranslation();
          glm::vec2 toTarget = glm::vec2(toTarget3.x, toTarget3.y);
          double len_to_object = glm::length(toTarget);
          if (len_to_object < closest_so_far) {
            closest_so_far	= len_to_object;
            closestTarget	= glm::normalize(toTarget);
          }
        }

        //this will store all the inputs for the NN
        vector<double> inputs;
        //add in vector to closest mine
        inputs.push_back(closestTarget.x);
        inputs.push_back(closestTarget.y);
        //add in sweepers look at vector
        glm::vec3 lookAt = placement->getLookAt();
        glm::vec2 horizLookAt = glm::normalize(glm::vec2(lookAt.x, lookAt.y));
        inputs.push_back(horizLookAt.x);
        inputs.push_back(horizLookAt.y);

        //update the NN and position
        std::vector<double> outputs = (sweeperAi->net.Update(inputs));
        if (outputs.size() != CParams::iNumOutputs) {
          std::cout << "NN produced wrong number of outputs!\n";
          return;
        }

        TrackControls *trackControls;
        state->get_TrackControls(participants.at((size_t)i), &trackControls);
        trackControls->torque.x = (float)outputs.at(0) * TRACK_TORQUE;
        trackControls->torque.y = (float)outputs.at(1) * TRACK_TORQUE;

//        for (auto val : outputs) {
//          std::cout << val << ", ";
//        }
//        std::cout << std::endl;


//        //see if it's found a mine
//        int GrabHit = m_vecSweepers[i].CheckForMine(m_vecMines,
//                                                    CParams::dMineScale);

//        if (GrabHit >= 0)
//        {
//          //we have discovered a mine so increase fitness
//          m_vecSweepers[i].IncrementFitness();
//
//          //mine found so replace the mine with another at a random
//          //position
//          m_vecMines[GrabHit] = SVector2D(RandFloat() * cxClient,
//                                          RandFloat() * cyClient);
//        }
//
//        //update the chromos fitness score
//        m_vecThePopulation[i].dFitness = m_vecSweepers[i].Fitness();
      }
    } else {
//      // Another generation has been completed.
//      // Time to run the GA and update the sweepers with their new NNs
//      //update the stats to be used in our stat window
//      m_vecAvFitness.push_back(m_pGA->AverageFitness());
//      m_vecBestFitness.push_back(m_pGA->BestFitness());
//
//      //increment the generation counter
//      ++m_iGenerations;
//
      //reset cycles
      ticks = 0;

      Physics *physics;
      for (auto id : registries[0].ids) {
        state->get_Physics(id, &physics);
        btTransform transform = physics->rigidBody->getCenterOfMassTransform();
        transform.setOrigin({0.f, -150.f, 0.f});
        transform.setRotation(btQuaternion());
        physics->rigidBody->setCenterOfMassTransform(transform);
        physics->rigidBody->setLinearVelocity({0.f, 0.f, -1.f});
      }
//
//      //run the GA to create a new population
//      m_vecThePopulation = m_pGA->Epoch(m_vecThePopulation);
//
//      //insert the new (hopefully)improved brains back into the sweepers
//      //and reset their positions etc
//      for (int i=0; i<m_NumSweepers; ++i)
//      {
//        m_vecSweepers[i].PutWeights(m_vecThePopulation[i].vecWeights);
//
//        m_vecSweepers[i].Reset();
//      }
    }
  }
  bool AiSystem::handleEvent(SDL_Event &event) {
    switch (event.type) {
      case SDL_KEYDOWN:
        switch (event.key.keysym.scancode) {
          default: return false; // could not handle it here
        } break;
      default: return false; // could not handle it here
    }
    return true; // handled it here
  }

  void AiSystem::beginSimulation() {
    assert(participants.size() && "AI SIMULATION MUST HAVE PARTICIPANTS");
    simulationStarted = true;
    SweeperAi *sweeperAi;
    state->get_SweeperAi(participants[0], &sweeperAi);
    numWeightsInNN = sweeperAi->net.GetNumberOfWeights();
    geneticAlgorithm = new CGenAlg(
        (int)participants.size(),
        CParams::dMutationRate,
        CParams::dCrossoverRate,
        numWeightsInNN
    );
    population = geneticAlgorithm->GetChromos();
    for (int i = 0; i < participants.size(); ++i) {
      state->get_SweeperAi(participants[i], &sweeperAi);
      sweeperAi->net.PutWeights(population[i].vecWeights);
    }
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
}

#pragma clang diagnostic pop