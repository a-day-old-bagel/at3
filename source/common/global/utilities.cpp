#include "utilities.h"

std::string getFileNameOnly(const fs::_Directory_iterator<std::true_type>::value_type &path) {
  return path.path().stem().string();
}
std::string getFileNameRelative(const fs::_Directory_iterator<std::true_type>::value_type &path) {
  return path.path().string();
}
