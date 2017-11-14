#pragma once

#include <vector>

namespace at3 {

  struct KernelEntry {
    int xOffset, yOffset, weight;
  };

  struct Kernel {
    std::vector<KernelEntry> contributingCells;
    
  };

  struct ImageFloatMono {
    std::vector<float> values;
    size_t resX, resY;
    float & at (int x, int y);
  };

}
