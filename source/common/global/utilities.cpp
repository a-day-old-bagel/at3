#include "utilities.h"

std::string getFileNameOnly(PATH_TYPE &path) {
  return path.path().stem().string();
}
std::string getFileExtOnly(PATH_TYPE &path) {
  return path.path().extension().string();
}
std::string getFileNameRelative(PATH_TYPE &path) {
  return path.path().string();
}

bool fileExists(const std::string &filePath) {
  return fs::exists(filePath);
}
