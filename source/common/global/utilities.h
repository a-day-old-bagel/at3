#pragma once

#include <experimental/filesystem>
#include "configuration.h"
namespace fs = std::experimental::filesystem;

std::string getFileNameOnly(const fs::_Directory_iterator<std::true_type>::value_type &path);
std::string getFileNameRelative(const fs::_Directory_iterator<std::true_type>::value_type &path);
