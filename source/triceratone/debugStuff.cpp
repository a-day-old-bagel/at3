
#include "configuration.hpp"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/gtc/matrix_transform.hpp>
#include "debugStuff.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

using namespace ezecs;

namespace at3 {

  void debugGenerateFloorGrid(float xMin, float xMax, unsigned xRes, float yMin, float yMax, unsigned yRes) {
    float sclX = (xMax - xMin) * .5f;
    float sclY = (yMax - yMin) * .5f;
    float ctrX = xMin + sclX;
    float ctrY = yMin + sclY;
    float xStep = 2.f / xRes;
    for (unsigned x = 0; x <= xRes; ++x) {
      Debug_::drawLine(
          glm::vec3(ctrX + (x * xStep * sclX - sclX), ctrY - sclY, 0.f),
          glm::vec3(ctrX + (x * xStep * sclX - sclX), ctrY + sclY, 0.f),
          glm::vec3(1.f, 0.f, 1.f) );
    }
    float yStep = 2.f / yRes;
    for (unsigned y = 0; y <= yRes; ++y) {
      Debug_::drawLine(
          glm::vec3(ctrX - sclX, ctrY + (y * yStep * sclY - sclY), 0.f),
          glm::vec3(ctrX + sclX, ctrY + (y * yStep * sclY - sclY), 0.f),
          glm::vec3(1.f, 0.f, 1.f) );
    }
  }

  void debugGenerateVirus() {
    float delta = 0.3f;
    Debug_::drawLine(
        glm::vec3(10 * delta, 0.f, 4.f),
        glm::vec3(11 * delta, 0.f, 2.f),
        glm::vec3(0.f, 1.f, 0.f));
    Debug_::drawLine(
        glm::vec3(11 * delta, 0.f, 2.f),
        glm::vec3(12 * delta, 0.f, 4.f),
        glm::vec3(0.f, 1.f, 0.f));

    Debug_::drawLine(
        glm::vec3(13 * delta, 0.f, 2.f),
        glm::vec3(13 * delta, 0.f, 4.f),
        glm::vec3(0.f, 1.f, 0.f));

    Debug_::drawLine(
        glm::vec3(14 * delta, 0.f, 2.f),
        glm::vec3(14 * delta, 0.f, 4.f),
        glm::vec3(0.f, 1.f, 0.f));
    Debug_::drawLine(
        glm::vec3(14 * delta, 0.f, 4.f),
        glm::vec3(15 * delta, 0.f, 3.5f),
        glm::vec3(0.f, 1.f, 0.f));
    Debug_::drawLine(
        glm::vec3(15 * delta, 0.f, 3.5f),
        glm::vec3(14 * delta, 0.f, 3.f),
        glm::vec3(0.f, 1.f, 0.f));
    Debug_::drawLine(
        glm::vec3(14 * delta, 0.f, 3.f),
        glm::vec3(15 * delta, 0.f, 2.f),
        glm::vec3(0.f, 1.f, 0.f));

    Debug_::drawLine(
        glm::vec3(16 * delta, 0.f, 2.f),
        glm::vec3(16 * delta, 0.f, 4.f),
        glm::vec3(0.f, 1.f, 0.f));
    Debug_::drawLine(
        glm::vec3(16 * delta, 0.f, 2.f),
        glm::vec3(17 * delta, 0.f, 2.f),
        glm::vec3(0.f, 1.f, 0.f));
    Debug_::drawLine(
        glm::vec3(17 * delta, 0.f, 2.f),
        glm::vec3(17 * delta, 0.f, 4.f),
        glm::vec3(0.f, 1.f, 0.f));

    Debug_::drawLine(
        glm::vec3(19 * delta, 0.f, 4.f),
        glm::vec3(18 * delta, 0.f, 3.3f),
        glm::vec3(0.f, 1.f, 0.f));
    Debug_::drawLine(
        glm::vec3(18 * delta, 0.f, 3.3f),
        glm::vec3(19 * delta, 0.f, 2.6f),
        glm::vec3(0.f, 1.f, 0.f));
    Debug_::drawLine(
        glm::vec3(19 * delta, 0.f, 2.6f),
        glm::vec3(18 * delta, 0.f, 2.f),
        glm::vec3(0.f, 1.f, 0.f));
  }

  DebugStuff::DebugStuff(Scene_ &scene, PhysicsSystem* physicsSystem) : scene(&scene),
    lineDrawRequestSubscription("draw_debug_line", RTU_MTHD_DLGT(&DebugStuff::fulfillDrawLineRequest, this))
  {
    // a floor grid
    debugGenerateFloorGrid(-10.5f, 10.5f, 21, -10.5f, 10.5f, 21);
    // a virus
    debugGenerateVirus();
    // a bullet physics debug-drawing thing
    bulletDebug = std::make_shared<BulletDebug_>();
    bulletDebug->setDebugMode(btIDebugDraw::DBG_DrawAabb | btIDebugDraw::DBG_DrawWireframe);
    physicsSystem->setDebugDrawer(bulletDebug);
    addToScene();
  }
  DebugStuff::~DebugStuff() {
    Debug_::reset();
  }
  void DebugStuff::addToScene() {
    scene->addObject(bulletDebug);
    scene->addObject(Debug_::instance());
  }
  void DebugStuff::queueMusic() {
    Uint8 *gameMusic[4];
    Uint32 gameMusicLength[4];
    int count = SDL_GetNumAudioDevices(0);
    fprintf(stderr, "Number of audio devices: %d\n", count);
    for (int i = 0; i < count; ++i) {
      fprintf(stderr, "Audio device %d: %s\n", i, SDL_GetAudioDeviceName(i, 0));
    }
    SDL_AudioSpec want, have;
    SDL_AudioDeviceID dev;
    SDL_memset(&want, 0, sizeof(want));
    want.freq = 4800;
    want.format = AUDIO_F32;
    want.channels = 2;
    want.samples = 4096;
    want.callback = NULL;

    int i = 0;
    SDL_LoadWAV(
        "./assets/audio/TitleScreen.wav",
        &want,
        &gameMusic[i],
        &gameMusicLength[i]);
    i += 1;
    SDL_LoadWAV(
        "./assets/audio/IndustrialTechno_2.wav",
        &want,
        &gameMusic[i],
        &gameMusicLength[i]);
    i += 1;
    SDL_LoadWAV(
        "./assets/audio/YouLose.wav",
        &want,
        &gameMusic[i],
        &gameMusicLength[i]);
    i += 1;
    SDL_LoadWAV(
        "./assets/audio/YouWin.wav",
        &want,
        &gameMusic[i],
        &gameMusicLength[i]);
    i += 1;

    dev = SDL_OpenAudioDevice(
        NULL,  // device
        0,  // is capture
        &want,  // desired
        &have,  // obtained
        0  // allowed changes
    );
    if (dev == 0) {
      fprintf(stderr, "Failed to open SDL audio device: %s\n", SDL_GetError());
    } else {
      for (int i = 0; i < 4; ++i) {
        SDL_QueueAudio(
            dev,
            gameMusic[i],
            gameMusicLength[i]);
      }
      SDL_PauseAudioDevice(dev, 0);  // start audio
    }
  }
  lineDrawFuncType DebugStuff::getLineDrawFunc() {
    return std::bind(&BulletDebug_::drawLineGlm, bulletDebug,
                     std::placeholders::_1,
                     std::placeholders::_2,
                     std::placeholders::_3);
  }

  void DebugStuff::fulfillDrawLineRequest(void *args) {
    auto threeVec3s = (float*)args;
    bulletDebug->drawLine(
        btVector3(threeVec3s[0], threeVec3s[1], threeVec3s[2]),
        btVector3(threeVec3s[3], threeVec3s[4], threeVec3s[5]),
        btVector3(threeVec3s[6], threeVec3s[7], threeVec3s[8]));
  }
}
#pragma clang diagnostic pop
