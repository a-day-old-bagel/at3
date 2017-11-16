#include <cassert>
#include <SDL_stdinc.h>
#include <iostream>
#include "imageProcessing.h"

namespace at3 {

//  float ImageFloatMono::zeroValue() {
//    return 0;
//  }
//  float *ImageFloatMono::data() {
//    return values.data();
//  }
//  size_t ImageFloatMono::size() {
//    return values.size();
//  }
//  float &ImageFloatMono::at(int x, int y) {
//    assert(x >= 0 && y >= 0);
//    return values.at(y * resX + x);
//  }
//  void ImageFloatMono::setValues(std::vector<float> &values, size_t resX, size_t resY) {
//    this->values = values;
//    this->resX = resX;
//    this->resY = resY;
//  }

  Kernel::Kernel(std::vector<double> weights) {
    auto matrixSideLength = (size_t)sqrt(weights.size());
    if (weights.size() != matrixSideLength * matrixSideLength) {
      throw std::runtime_error("Kernel must have a square number of kermits to form a square convolution matrix");
    }
    if (matrixSideLength % 2 == 0) {
      throw std::runtime_error("Kernel convolution matrix must have an odd side length");
    }
    int cornerIndex = (int)matrixSideLength / 2;
    for (int y = cornerIndex; y >= -cornerIndex; --y) {
      for (int x = -cornerIndex; x <= cornerIndex; ++x) {
        size_t kermitIndex = (x + cornerIndex) + (cornerIndex - y) * matrixSideLength;
        if (weights.at(kermitIndex)) {
          this->kermits.push_back({x, y, weights.at(kermitIndex)});
        }
      }
    }
  }

  Kernel gaussianBlur(double sigma, size_t sideLength) {
    std::vector<double> weights;
    double mean = sideLength / 2;
    for (int y = 0; y < sideLength; ++y) {
      for (int x = 0; x < sideLength; ++x) {
        weights.push_back(exp(-0.5 * (pow((x - mean) / sigma, 2.0) + pow((y - mean) / sigma, 2.0)))
                          / (M_PI * 2 * sigma * sigma));
      }
    }
    return Kernel(weights);
  }

}
