
#include "sweepers.h"

namespace at3 {
//---------------------------------------constructor---------------------
//
//	initilaize the sweepers, their brains and the GA factory
//
//-----------------------------------------------------------------------
  CController::CController() : m_NumSweepers(CParams::iNumSweepers),
                               m_pGA(NULL),
                               m_bFastRender(false),
                               m_iTicks(0),
                               m_NumMines(CParams::iNumMines),
                               m_iGenerations(0),
                               cxClient(CParams::WindowWidth),
                               cyClient(CParams::WindowHeight) {
    //let's create the mine sweepers
    for (int i = 0; i < m_NumSweepers; ++i) {
      m_vecSweepers.push_back(CMinesweeper());
    }

    //get the total number of weights used in the sweepers
    //NN so we can initialise the GA
    m_NumWeightsInNN = m_vecSweepers[0].GetNumberOfWeights();

    //initialize the Genetic Algorithm class
    m_pGA = new CGenAlg(m_NumSweepers,
                        CParams::dMutationRate,
                        CParams::dCrossoverRate,
                        m_NumWeightsInNN);

    //Get the weights from the GA and insert into the sweepers brains
    m_vecThePopulation = m_pGA->GetChromos();

    for (int i = 0; i < m_NumSweepers; i++)

      m_vecSweepers[i].PutWeights(m_vecThePopulation[i].vecWeights);

    //initialize mines in random positions within the application window
    for (int i = 0; i < m_NumMines; ++i) {
      m_vecMines.push_back(SVector2D(RandFloat() * cxClient,
                                     RandFloat() * cyClient));
    }
  }


//--------------------------------------destructor-------------------------------------
//
//--------------------------------------------------------------------------------------
  CController::~CController() {
    if (m_pGA) {
      delete m_pGA;
    }
  }


//---------------------WorldTransform--------------------------------
//
//	sets up the translation matrices for the mines and applies the
//	world transform to each vertex in the vertex buffer passed to this
//	method.
//-------------------------------------------------------------------
  void CController::WorldTransform(std::vector<SPoint> &VBuffer, SVector2D vPos) {
    //create the world transformation matrix
    C2DMatrix matTransform;

    //scale
    matTransform.Scale(CParams::dMineScale, CParams::dMineScale);

    //translate
    matTransform.Translate(vPos.x, vPos.y);

    //transform the ships vertices
    matTransform.TransformSPoints(VBuffer);
  }


//-------------------------------------Update-----------------------------
//
//	This is the main workhorse. The entire simulation is controlled from here.
//
//	The comments should explain what is going on adequately.
//-------------------------------------------------------------------------
  bool CController::Update() {
    //run the sweepers through CParams::iNumTicks amount of cycles. During
    //this loop each sweepers NN is constantly updated with the appropriate
    //information from its surroundings. The output from the NN is obtained
    //and the sweeper is moved. If it encounters a mine its fitness is
    //updated appropriately,
    if (m_iTicks++ < CParams::iNumTicks) {
      for (int i = 0; i < m_NumSweepers; ++i) {
        //update the NN and position
        if (!m_vecSweepers[i].Update(m_vecMines)) {
          //error in processing the neural net
          printf("\"Wrong amount of NN inputs!\"\n");

          return false;
        }

        //see if it's found a mine
        int GrabHit = m_vecSweepers[i].CheckForMine(m_vecMines,
                                                    CParams::dMineScale);

        if (GrabHit >= 0) {
          //we have discovered a mine so increase fitness
          m_vecSweepers[i].IncrementFitness();

          //mine found so replace the mine with another at a random
          //position
          m_vecMines[GrabHit] = SVector2D(RandFloat() * cxClient,
                                          RandFloat() * cyClient);
        }

        //update the chromos fitness score
        m_vecThePopulation[i].dFitness = m_vecSweepers[i].Fitness();

      }
    }

      //Another generation has been completed.

      //Time to run the GA and update the sweepers with their new NNs
    else {
      //update the stats to be used in our stat window
      m_vecAvFitness.push_back(m_pGA->AverageFitness());
      m_vecBestFitness.push_back(m_pGA->BestFitness());

      //increment the generation counter
      ++m_iGenerations;

      //reset cycles
      m_iTicks = 0;

      //run the GA to create a new population
      m_vecThePopulation = m_pGA->Epoch(m_vecThePopulation);

      //insert the new (hopefully)improved brains back into the sweepers
      //and reset their positions etc
      for (int i = 0; i < m_NumSweepers; ++i) {
        m_vecSweepers[i].PutWeights(m_vecThePopulation[i].vecWeights);

        m_vecSweepers[i].Reset();
      }
    }

    return true;
  }
}