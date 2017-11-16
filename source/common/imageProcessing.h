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
   * An image to which can be applied a kernel
   * @tparam Pixel : The type of a single pixel
   */
  template <typename Pixel>
  class Image {
    private:
      std::vector<Pixel> *values;
      std::vector<Pixel> *temp;
      std::vector<Pixel> vec0;
      std::vector<Pixel> vec1;
      size_t resX, resY;

      void applyKernelAtLocation(size_t x, size_t y, Kernel &kernel) {
        double sumWeights = 0;
        auto sumValues = static_cast<Pixel>(0);
        int64_t imgIndex = y * resX + x;
        for (unsigned i = 0; i < kernel.kermits.size(); ++i) {
          Kermit k = kernel.kermits[i];
          if (x + k.offsetX >= 0 && x + k.offsetX < resX && y + k.offsetY >= 0 && y + k.offsetY < resY) {
            float contributingValue = at(x + k.offsetX, y + k.offsetY);
            sumValues += k.weight * contributingValue;
            sumWeights += abs(k.weight);
          }
        }
        auto finalValue = static_cast<Pixel>(0);
        if (sumWeights > 0) {
          finalValue = sumValues / sumWeights;
//          if (finalValue) {
//            std::cout << "bar\n";
//          }
        }
        tempAt(x, y) = finalValue;
      }

      Pixel & tempAt (size_t x, size_t y) {
        return temp->at(y * resX + x);
      }

    public:

      Image() : values(&vec0), temp(&vec1), resX(0), resY(0) { }

      Pixel & at (size_t x, size_t y) {
        return values->at(y * resX + x);
      }
      Pixel* data() {
        return values->data();
      }
      size_t size() {
        return values->size();
      }
      void setValues(std::vector<Pixel> *values, size_t resX, size_t resY) {
        *(this->values) = *values;
        temp->resize(values->size());
        this->resX = resX;
        this->resY = resY;
      }
      std::vector<Pixel> *getValues() {
        return values;
      }
      void applyKernel(Kernel & kernel) {
        for (size_t y = 0; y < resY; ++y) {
          for (size_t x = 0; x < resX; ++x) {
//            if (y == 100 && x == 100) {
//              std::cout << "foo\n";
//            }
            applyKernelAtLocation(x, y, kernel);
          }
        }
        std::vector<Pixel> *copy = values;
        values = temp;
        temp = copy;
      }

  };



//  /**
//   * A CRTP base class for anything to which a kernel can be applied
//   * @tparam Derived
//   */
//  template <typename Derived>
//  class Kernelable {
//    private:
//
//      Derived & img() {
//        return *static_cast<Derived*>(this);
//      }
//
//    public:
//
//      size_t resX, resY;
//
//      void applyKernelAtLocation(size_t x, size_t y, Kernel &kernel) {
//        double sumWeights = 0;
//        auto sumValues = img().zeroValue();
//        int imgIndex = y * resX + x;
//        for (unsigned i = 0; i < kernel.kermits.size(); ++i) {
//          Kermit k = kernel.kermits[i];
//          if (x + k.offsetX >= 0 && x + k.offsetX < resX && y + k.offsetY >= 0 && y + k.offsetY < resY) {
//            sumValues += k.weight * img().at(x + k.offsetX, y + k.offsetY);
//            sumWeights += abs(k.weight);
//          }
//        }
//        auto finalValue = img().zeroValue();
//        if (sumWeights > 0) {
//          finalValue = sumValues / sumWeights;
//        }
//        img().at(x, y) = finalValue;
//      }
//
//      void applyKernel(Kernel & kernel) {
//        for (size_t y = 0; y < resY; ++y) {
//          for (size_t x = 0; x < resX; ++x) {
//            applyKernelAtLocation(x, y, kernel);
//          }
//        }
//      }
//
//  };
//
//  /**
//   * An image
//   */
//  template <typename Pixel>
//  class Image : public typename Kernelable<Image<Pixel>> {
//    private:
//      std::vector<Pixel> values;
//    public:
//      Pixel zeroValue() {
//        return static_cast<Pixel>(0);
//      }
//      Pixel* data() {
//        return values.data();
//      }
//      size_t size() {
//        return values.size();
//      }
//      Pixel & at (int x, int y) {
//        return values.at(y * resX + x);
//      }
//      void setValues(std::vector<Pixel> &values, size_t resX, size_t resY) {
//        this->values = values;
//        this->resX = resX;
//        this->resY = resY;
//      }
//  };

//  /**
//   * A monochromatic image with floating point values
//   */
//  class ImageFloatMono : public Kernelable<ImageFloatMono> {
//    private:
//      std::vector<float> values;
//    public:
//      float zeroValue();
//      float* data();
//      size_t size();
//      float & at (int x, int y);
//      void setValues(std::vector<float> &values, size_t resX, size_t resY);
//  };

  Kernel gaussianBlur(double sigma, size_t sideLength);

}
