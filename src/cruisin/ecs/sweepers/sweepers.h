
#ifndef AT3_SWEEPERS_H
#define AT3_SWEEPERS_H

#include <vector>
#include <sstream>
#include <string>
#include <fstream>

#include "CMineSweeper.h"
#include "CGenAlg.h"
#include "utils.h"
#include "C2DMatrix.h"
#include "SVector2D.h"
#include "CParams.h"


namespace at3 {

  class CController
  {
    private:

      //storage for the population of genomes
      vector<SGenome>	     m_vecThePopulation;

      //and the minesweepers
      vector<CMinesweeper> m_vecSweepers;

      //and the mines
      vector<SVector2D>	   m_vecMines;

      //pointer to the GA
      CGenAlg*		         m_pGA;

      int					         m_NumSweepers;

      int					         m_NumMines;

      int					         m_NumWeightsInNN;

      //stores the average fitness per generation for use
      //in graphing.
      vector<double>		   m_vecAvFitness;

      //stores the best fitness per generation
      vector<double>		   m_vecBestFitness;

      //toggles the speed at which the simulation runs
      bool				m_bFastRender;

      //cycles per generation
      int					m_iTicks;

      //generation counter
      int					m_iGenerations;

      //window dimensions
      int         cxClient, cyClient;

    public:

      CController();

      ~CController();

      void		WorldTransform(vector<SPoint> &VBuffer,
                             SVector2D      vPos);

      bool		Update();
  };

}


#endif //AT3_SWEEPERS_H
