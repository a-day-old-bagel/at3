#pragma once

#include <cstdint>
#include "vkh.h"
#include "vkh_mesh.h"
#include "camera.h"

struct DrawCall {
    vkh::MeshAsset mesh;
    uint32_t uboIdx;
};

void initRendering(vkh::VkhContext &, uint32_t num);

//void cleanupRendering(vkh::VkhContext &);

void updateUBOs(vkh::VkhContext &, Camera::Cam &cam);

void render(vkh::VkhContext &, Camera::Cam &camera, const std::vector<vkh::MeshAsset> &drawCalls,
            const std::vector<uint32_t> &uboIdx);
