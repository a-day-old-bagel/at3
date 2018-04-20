#pragma once

#include <cstdint>
#include "vkh.h"
#include "vkh_mesh.h"
#include "camera.h"

struct DrawCall {
    at3::MeshAsset mesh;
    uint32_t uboIdx;
};

void initRendering(at3::VkhContext &, uint32_t num);

//void cleanupRendering(at3::VkhContext &);

void updateUBOs(at3::VkhContext &, Camera::Cam &cam);

void render(at3::VkhContext &, Camera::Cam &camera, const std::vector<at3::MeshAsset> &drawCalls,
            const std::vector<uint32_t> &uboIdx);

void render(at3::VkhContext &ctxt, const glm::mat4 &wvMat, const std::vector<at3::MeshAsset> &drawCalls,
            const std::vector<uint32_t> &uboIdx);
