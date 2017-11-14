#include <cassert>
#include "imageProcessing.h"

namespace at3 {
  float &ImageFloatMono::at(int x, int y) {
    assert(x >= 0 && y >= 0);
    return values.at(y * resX + x);
  }
}