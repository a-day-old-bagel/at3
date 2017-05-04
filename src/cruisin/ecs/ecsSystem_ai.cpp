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

//    Placement* placement;
//    for (int i = 0; i < vecTargets.size(); ++i) {
//      state->get_Placement(targets->at((size_t)i)->getId(), &placement);
//      vecTargets[i] = SVector2D(placement->mat[3][0], placement->mat[3][1]);
//    }
//    //    std::vector<double>& vecTargets_dbl = (std::vector<double>&)&vecTargets;
//    std::vector<double> vecTargets_dbl;
//    vecTargets_dbl.resize(vecTargets.size() * 2);
//    memcpy(vecTargets_dbl.data(), vecTargets.data(), vecTargets_dbl.size() * sizeof(double));

    std::vector<double> vecTargets_dbl(registries[1].ids.size() * 2);
    Placement* placement;
    for (int i = 0; i < registries[1].ids.size(); ++i) {
      state->get_Placement(registries[1].ids.at((size_t)i), &placement);
      vecTargets_dbl[i * 2 + 0] = placement->mat[3][0];
      vecTargets_dbl[i * 2 + 1] = placement->mat[3][1];
    }

    SweeperAi *sweeperAi;

    //run the sweepers through CParams::iNumTicks amount of cycles. During
    //this loop each sweepers NN is constantly updated with the appropriate
    //information from its surroundings. The output from the NN is obtained
    //and the sweeper is moved. If it encounters a mine its fitness is
    //updated appropriately,
//    if (ticks++ < CParams::iNumTicks)
//    {
//      for (int i = 0; i < participants.size(); ++i)
//      {
//        state->get_SweeperAi(participants[i], &sweeperAi);
//
//        //update the NN and position
//        std::vector<double> outputs = (sweeperAi->net.Update(vecTargets_dbl));
//        if (!outputs.size()) {
//          //error in processing the neural net
//          printf("\"Wrong amount of NN inputs!\"\n");
//          return;
//        }
//
//        //see if it's found a mine
//        int GrabHit = m_vecSweepers[i].CheckForMine(m_vecMines,
//                                                    CParams::dMineScale);
//
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
//
//      }
//    }

      //Another generation has been completed.

      //Time to run the GA and update the sweepers with their new NNs
//    else
//    {
//      //update the stats to be used in our stat window
//      m_vecAvFitness.push_back(m_pGA->AverageFitness());
//      m_vecBestFitness.push_back(m_pGA->BestFitness());
//
//      //increment the generation counter
//      ++m_iGenerations;
//
//      //reset cycles
//      ticks = 0;
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
//    }
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

  void AiSystem::beginSimulation(std::vector<std::shared_ptr<MeshObject_>> *targets) {
    assert(participants.size() && "AI SIMULATION MUST HAVE PARTICIPANTS");
    simulationStarted = true;
    SweeperAi *sweeperAi;
    state->get_SweeperAi(participants[0], &sweeperAi);
    numWeightsInNN = sweeperAi->net.GetNumberOfWeights();
    geneticAlgorithm = new CGenAlg(
        participants.size(),
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