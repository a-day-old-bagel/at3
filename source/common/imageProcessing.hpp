#pragma once

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>

#include <vector>

namespace at3 {

  /**
   * A single entry in the convolution matrix
   */
  struct Kermit {
    int offsetX, offsetY;
    double weight;
  };
  /**
   * A convolution matrix, or kernel
   */
  struct Kernel {
    std::vector<Kermit> kermits;
    explicit Kernel(std::vector<double> weights);
  };

  /**
   * An image to which can be applied a kernel
   * @tparam Pixel : The type of a single pixel
   */
  template <typename Pixel>
  class Image {
    private:

      std::vector<Pixel> vecs[2];
      size_t resX, resY;
      int whichVec = 0;

      std::vector<Pixel> & rVec() {
        return vecs[whichVec];
      }
      std::vector<Pixel> & wVec() {
        return vecs[1 - whichVec];
      }
      void swapRW() {
        whichVec = 1 - whichVec;
      }
      Pixel & wAt (size_t x, size_t y) {
        return wVec().at(y * resX + x);
      }
      void applyKernelAtLocation(size_t x, size_t y, Kernel &kernel) {
        auto sumValues = static_cast<Pixel>(0);
        int64_t imgIndex = y * resX + x;
        for (unsigned i = 0; i < kernel.kermits.size(); ++i) {
          Kermit k = kernel.kermits[i];
          if (x + k.offsetX >= 0 && x + k.offsetX < resX && y + k.offsetY >= 0 && y + k.offsetY < resY) {
            float contributingValue = at(x + k.offsetX, y + k.offsetY);
            sumValues += k.weight * contributingValue;
          }
        }
        wAt(x, y) = sumValues;
      }

    public:

      Image() : resX(0), resY(0) { }
      Image(size_t x, size_t y) : resX(x), resY(y) {
        rVec().resize(x * y);
      }

      Pixel & at (size_t x, size_t y) {
        return rVec().at(y * resX + x);
      }
      Pixel* data() {
        return rVec().data();
      }
      size_t size() {
        return rVec().size();
      }
      size_t getResX() { return resX; }
      size_t getResY() { return resY; }
      void resize(size_t x, size_t y) {
        resX = x;
        resY = y;
        rVec().resize(x * y);
      }
      void setValues(std::vector<Pixel> *values, size_t resX, size_t resY) {
        this->rVec() = *values;
        this->resX = resX;
        this->resY = resY;
      }
      std::vector<Pixel> *getValues() {
        return &rVec();
      }
      void applyKernel(Kernel & kernel) {
        wVec().resize(rVec().size());
        for (size_t y = 0; y < resY; ++y) {
          for (size_t x = 0; x < resX; ++x) {
            applyKernelAtLocation(x, y, kernel);
          }
        }
        swapRW();
        wVec().clear();
        wVec().shrink_to_fit();
      }

  };

  Kernel gaussianBlur(double sigma, size_t sideLength);
  Image<float> sobelGradientX(Image<float> &in, bool use5x5 = false, bool useBlur = false);
  Image<float> sobelGradientY(Image<float> &in, bool use5x5 = false, bool useBlur = false);
  Image<float> sobelEdgeMono(Image<float> &gradX, Image<float> &gradY, float threshold);
  Image<float> sobelEdgeMono(Image<float> &in, float threshold, bool use5x5 = false, bool useBlur = false);

}
