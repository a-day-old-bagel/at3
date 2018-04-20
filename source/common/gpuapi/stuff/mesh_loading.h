#pragma once
#include <cstdint>
#include "vkh.h"
#include "vkh_mesh.h"

std::vector<at3::MeshAsset> loadMesh(const char* filepath, bool combineSubMeshes, at3::VkhContext& ctxt);
