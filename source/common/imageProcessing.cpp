#include <cassert>
#include <SDL_stdinc.h>
#include <iostream>
#include "imageProcessing.h"

namespace at3 {

  Kernel::Kernel(std::vector<double> weights) {
    auto matrixSideLength = (size_t)sqrt(weights.size());
    if (weights.size() != matrixSideLength * matrixSideLength) {
      throw std::runtime_error("Kernel must have a square number of kermits to form a square convolution matrix");
    }
    if (matrixSideLength % 2 == 0) {
      throw std::runtime_error("Kernel convolution matrix must have an odd side length");
    }
    int cornerIndex = (int)matrixSideLength / 2;
    double sumWeights = 0;
    for (int y = cornerIndex; y >= -cornerIndex; --y) {
      for (int x = -cornerIndex; x <= cornerIndex; ++x) {
        size_t kermitIndex = (x + cornerIndex) + (cornerIndex - y) * matrixSideLength;
        if (weights.at(kermitIndex)) {
          this->kermits.push_back({x, y, weights.at(kermitIndex)});
          sumWeights += abs(weights.at(kermitIndex));
        }
      }
    }
    // normalize the kernel
    for (auto & kermit : kermits) {
      kermit.weight /= sumWeights;
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

  Image<float> sobelGradientX(Image<float> &in, bool use5x5 /*= false*/, bool useBlur /*= false*/) {
    Image<float> gradX;
    gradX.setValues(in.getValues(), in.getResX(), in.getResY());
    if (useBlur) {
      Kernel gauss15 = gaussianBlur(1, 5);
      gradX.applyKernel(gauss15);
    }
    if (use5x5) {
      Kernel sobelX5
          ({
              1,  2, 0,  -2, -1,
              4,  8, 0,  -8, -4,
              6, 12, 0, -12, -6,
              4,  8, 0,  -8, -4,
              1,  2, 0,  -2, -1,
          });
      gradX.applyKernel(sobelX5);
    } else {
      Kernel sobelX3
          ({
               3,  0,  -3,
              10,  0, -10,
               3,  0,  -3
          });
      gradX.applyKernel(sobelX3);
    }
    return gradX;
  }

  Image<float> sobelGradientY(Image<float> &in, bool use5x5 /*= false*/, bool useBlur /*= false*/) {
    Image<float> gradY;
    gradY.setValues(in.getValues(), in.getResX(), in.getResY());
    if (useBlur) {
      Kernel gauss15 = gaussianBlur(1, 5);
      gradY.applyKernel(gauss15);
    }
    if (use5x5) {
      Kernel sobelY5
          ({
               -1, -4,  -6, -4, -1,
               -2, -8, -12, -8, -2,
                0,  0,   0,  0,  0,
                2,  8,  12,  8,  2,
                1,  4,   6,  4,  1,
           });
      gradY.applyKernel(sobelY5);
    } else {
      Kernel sobelY3
          ({
               -3, -10, -3,
                0,   0,  0,
                3,  10,  3
           });
      gradY.applyKernel(sobelY3);
    }
    return gradY;
  }

  Image<float> sobelEdgeMono(Image<float> &gradX, Image<float> &gradY, float threshold) {
    size_t resX = gradX.getResX();
    size_t resY = gradX.getResY();
    if (resX != gradY.getResX() || resY != gradY.getResY()) {
      throw std::runtime_error("Sobel edge detection gradient x/y image sizes don't match");
    }
    Image<float> edges(resX, resY);
    for (size_t y = 0; y < resY; ++y) {
      for (size_t x = 0; x < resX; ++x) {
        float x2 = gradX.at(x,y) * gradX.at(x,y);
        float y2 = gradY.at(x,y) * gradY.at(x,y);
        float edgeX2 = x2 > threshold ? x2 : 0.f;
        float edgeY2 = y2 > threshold ? y2 : 0.f;
        edges.at(x, y) = sqrt(edgeX2 + edgeY2);
      }
    }
    return edges;
  }

  Image<float> sobelEdgeMono(Image<float> &in, float threshold, bool use5x5 /*= false*/, bool useBlur /*= false*/) {

    size_t resX = in.getResX();
    size_t resY = in.getResY();
    Image<float> edgeX;
    Image<float> edgeY;
    edgeX.setValues(in.getValues(), resX, resY);

    // Don't needlessly blur both x and y images, just copy
    if (useBlur) {
      Kernel gauss15 = gaussianBlur(1, 5);
      edgeX.applyKernel(gauss15);
      edgeY.setValues(edgeX.getValues(), resX, resY);
    } else {
      edgeY.setValues(in.getValues(), resX, resY);
    }

    edgeX = sobelGradientX(edgeX, use5x5, false);
    edgeY = sobelGradientY(edgeY, use5x5, false);

    return sobelEdgeMono(edgeX, edgeY, threshold);
  }

}
