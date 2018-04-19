#pragma once
#include <cstdint>
#include "vkh.h"
#include "vkh_mesh.h"

std::vector<vkh::MeshAsset> loadMesh(const char* filepath, bool combineSubMeshes, vkh::VkhContext& ctxt);
