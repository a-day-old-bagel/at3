#pragma once

#include <experimental/filesystem>
#include "definitions.hpp"
namespace fs = std::experimental::filesystem;

// Thanks MSVC. I always love your shoddy implementations.
#ifdef _WIN32
# define PATH_TYPE const fs::_Directory_iterator<std::true_type>::value_type
#else
# define PATH_TYPE const std::experimental::filesystem::v1::__cxx11::directory_entry
#endif

std::string getFileNameOnly(PATH_TYPE &path);
std::string getFileExtOnly(PATH_TYPE &path);
std::string getFileNameRelative(PATH_TYPE &path);

bool fileExists(const std::string &filePath);
