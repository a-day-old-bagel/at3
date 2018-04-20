
#include "dataStore.h"
#include "mesh_loading.h"
#include "vkh.h"
#include "config.h"
#include "state.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

/*
	Single threaded. Try to keep as much equal as possible, save for the experimental changes
	Test - binding once with uniform buffers, binding once with ssbo, binding per object uniform data

	- Read in all textures, store in a global array of textures, give materials push constant indices
	- meshes should be read in, buffers stored in a global place, but not combined
	- materials created by hand (since for this demo, we'll likely only have the one material, with a lot of instances)

*/

void logFPSAverage(double avg);
void mainLoop(VkbbState &state);

int main(int argc, char **argv) {

  VkbbState state;

  std::vector<at3::EMeshVertexAttribute> meshLayout;
  meshLayout.push_back(at3::EMeshVertexAttribute::POSITION);
  meshLayout.push_back(at3::EMeshVertexAttribute::UV0);
  meshLayout.push_back(at3::EMeshVertexAttribute::NORMAL);
  at3::Mesh::setGlobalVertexLayout(meshLayout);

//load a test obj mesh
#if BISTRO_TEST
  state.testMesh = loadMesh("./meshes/exterior.obj", false, state.context);
  auto interior = loadMesh("./meshes/interior.obj", false, state.context);
  state.testMesh.insert(state.testMesh.end(), interior.begin(), interior.end());
#else
  state.testMesh = loadMesh("./meshes/sponza.obj", false, state.context);
#endif

  state.uboIdx.resize(state.testMesh.size());
  printf("Num meshes: %lu\n", state.testMesh.size());
  ubo_store::init(state.context);

  for (uint32_t i = 0; i < state.testMesh.size(); ++i) {
    bool didAcquire = ubo_store::acquire(state.uboIdx[i]);
    checkf(didAcquire, "Error acquiring ubo index");
  }

  initRendering(state.context, (uint32_t)state.testMesh.size());
  mainLoop(state);

  return 0;
}

void logFPSAverage(double avg) {
  printf("AVG FRAMETIME FOR LAST %i FRAMES: %f ms\n", FPS_DATA_FRAME_HISTORY_SIZE, avg);
}

void mainLoop(VkbbState &state) {
  FPSData fpsData = {0};
  fpsData.logCallback = logFPSAverage;
  startTimingFrame(fpsData);
  while (state.running) {
    double dt = endTimingFrame(fpsData);
    startTimingFrame(fpsData);
    sdl::pollEvents();
    state.tick(dt);
    render(state.context, state.worldCamera, state.testMesh, state.uboIdx);
  }
}

#pragma clang diagnostic pop
