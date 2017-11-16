#pragma once

#include <vector>
#include <glm/glm.hpp>

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
   * A CRTP base class for anything to which a kernel can be applied
   * @tparam Derived
   */
  template <typename Derived>
  class Kernelable {
    private:

      Derived & img() {
        return *static_cast<Derived*>(this);
      }

    public:

      size_t resX, resY;

      void applyKernelAtLocation(size_t x, size_t y, Kernel &kernel) {
        double sumWeights = 0;
        auto sumValues = img().zeroValue();
        int imgIndex = y * resX + x;
        for (unsigned i = 0; i < kernel.kermits.size(); ++i) {
          Kermit k = kernel.kermits[i];
          if (x + k.offsetX >= 0 && x + k.offsetX < resX && y + k.offsetY >= 0 && y + k.offsetY < resY) {
            sumValues += k.weight * img().at(x + k.offsetX, y + k.offsetY);
            sumWeights += abs(k.weight);
          }
        }
        auto finalValue = img().zeroValue();
        if (sumWeights > 0) {
          finalValue = sumValues / sumWeights;
        }
        img().at(x, y) = finalValue;
      }

      void applyKernel(Kernel & kernel) {
        for (size_t y = 0; y < resY; ++y) {
          for (size_t x = 0; x < resX; ++x) {
            applyKernelAtLocation(x, y, kernel);
          }
        }
      }

  };

  /**
   * A monochromatic image with floating point values
   */
  struct ImageFloatMono : public Kernelable<ImageFloatMono> {
    std::vector<float> values;
    size_t size();
    float & at (int x, int y);
    float zeroValue();
  };

  /**
   * A bichromatic image with floating point values
   */
  struct ImageFloatBi : public Kernelable<ImageFloatBi> {
    std::vector<glm::vec2> values;
    size_t size();
    glm::vec2 & at(int x, int y);
    glm::vec2 zeroValue();
  };

  Kernel gaussianBlur(double sigma, size_t sideLength);

}
