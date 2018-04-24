#include "utilities.h"

std::string getFileNameOnly(PATH_TYPE &path) {
  return path.path().stem().string();
}
std::string getFileNameRelative(PATH_TYPE &path) {
  return path.path().string();
}
