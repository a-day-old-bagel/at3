/*
 * Copyright (c) 2016 Jonathan Glines, Galen Cochrane
 * Jonathan Glines <jonathan@glines.net>
 * Galen Cochrane <galencochrane@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <cstdio>
#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"

#include "debug.h"
#include "scene.h"
#include "shaders.h"
#include "shaderProgram.h"
#include "glError.h"

#include "game.h"

#define TIME_MULTIPLIER_MS 0.001f

namespace at3 {
  Game::Game(int argc, char **argv, const char *windowTitle)
    : m_windowTitle(windowTitle), m_scene(nullptr),
    m_width(640), m_height(480), m_fovy((float)M_PI * 0.35f)
  {
    m_lastTime = 0.0f;
    if (! m_initSdl() || ! m_initGl() || ! m_initScene()) {
      exit(EXIT_FAILURE);
    }
    auto shader = Shaders::terrainShader();
    shader->use();
    assert(shader->screenSize() != -1);
    glUniform2f(shader->screenSize(), m_width, m_height);              ASSERT_GL_ERROR();

    // at 0 field of view, a normalized dot product of 1 must be achieved between the forward view vector and
    // a point for it to be considered on the screen. At PI field of view, a dot product of at least zero is
    // required.  At 2PI field of view, a dot product of at least -1 is required, meaning that everything is visible.
    float maxFieldOfView = (aspect() > 1.f) ? m_fovy * aspect() : m_fovy;
    float maxFieldViewDot = 0.9f - ((maxFieldOfView * 0.5f) / (float)(M_PI));  // 0.9 for extra margin
    assert(shader->maxFieldViewDot() != -1);
    glUniform1f(shader->maxFieldViewDot(), maxFieldViewDot);           ASSERT_GL_ERROR();
  }

  Game::~Game() {
    // Free the graphics scene
    delete m_scene;

    // TODO: Free GL resources
    // TODO: Free SDL resources
  }

  bool Game::m_initSdl() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
      fprintf(stderr, "Failed to initialize SDL: %s\n",
          SDL_GetError());
      return false;
    }
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
    m_window = SDL_CreateWindow(
        m_windowTitle,  // title
        SDL_WINDOWPOS_UNDEFINED,  // x
        SDL_WINDOWPOS_UNDEFINED,  // y
        m_width, m_height,  // w, h
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE  // flags
        );
    if (m_window == nullptr) {
      fprintf(stderr, "Failed to create SDL window: %s\n",
          SDL_GetError());
      return false;
    }
    return true;
  }

  bool Game::m_initGl() {
    // Create an OpenGL context for our window
    m_glContext = SDL_GL_CreateContext(m_window);
    if (m_glContext == nullptr) {
      fprintf(stderr, "Failed to initialize OpenGL context: %s\n",
          SDL_GetError());
      return false;
    }
    // Configure the GL
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepth(1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glViewport(0, 0, m_width, m_height);
    return true;
  }

  bool Game::m_initScene() {
    m_scene = new Scene();
    m_scene->addObject(Debug::instance(state));
    return true;
  }

  bool Game::mainLoop(ezecs::Delegate<bool(SDL_Event &)> &systemsHandler, float &dtOut) {
    SDL_GL_SwapWindow(m_window);
    if (m_lastTime == 0.0f) {
      // FIXME: Try to make sure this doesn't ever produce a dt of 0.
      m_lastTime = (float)(std::min((Uint32)0, SDL_GetTicks() - 1)) * TIME_MULTIPLIER_MS;
    }
    // Check for SDL events (user input, etc.)
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (systemsHandler(event))
        continue;   // The systems have handled this event
      if (this->handleEvent(event))
        continue;  // Our derived class handled this event
      if (this->scene()->handleEvent(event))
        continue;  // One of the scene objects handled this event
      switch (event.type) {
        case SDL_WINDOWEVENT:
          switch (event.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
              m_width = event.window.data1;
              m_height = event.window.data2;
              glViewport(0, 0, m_width, m_height);
              auto shader = Shaders::terrainShader();
              shader->use();
              assert(shader->screenSize() != -1);
              glUniform2f(shader->screenSize(), m_width, m_height);              ASSERT_GL_ERROR();
              // at 0 field of view, a normalized dot product of 1 must be achieved between the forward view vector and
              // a point for it to be considered on the screen. At PI field of view, a dot product of at least zero is
              // required.  At 2PI field of view, a dot product of at least -1 is required, meaning that everything is
              // visible.
              float maxFieldOfView = (aspect() > 1.f) ? m_fovy * aspect() : m_fovy;
              float maxFieldViewDot = 0.9f - ((maxFieldOfView * 0.5f) / (float)(M_PI)); // 0.9 for extra margin
              assert(shader->maxFieldViewDot() != -1);
              glUniform1f(shader->maxFieldViewDot(), maxFieldViewDot);           ASSERT_GL_ERROR();
              std::cout << "NEW SCREEN SIZE: " << m_width << ", " << m_height << std::endl;
              break;
            }
            default: break;
          } break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_V: {
              terrainShaderDebugLines = !terrainShaderDebugLines;
              auto shader = Shaders::terrainShader();
              shader->use();
              assert(shader->debugLines() != -1);
              glUniform1i(shader->debugLines(), terrainShaderDebugLines);       ASSERT_GL_ERROR();
              break;
            }
            default: break;
          } break;
        case SDL_QUIT:
          return false;
        default: break;
      }
    }

    // Calculate the time since the last frame was drawn TODO: SDL_GetTicks may be too granular
    float currentTime = (float)SDL_GetTicks() * TIME_MULTIPLIER_MS;
    float dt = currentTime - m_lastTime;
    m_lastTime = currentTime;

    // Draw the window
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_camera) {
      fprintf(stderr, "The scene camera was not set\n");
    } else {
      // Draw the scene
      float aspect = (float)m_width / (float)m_height;
      m_scene->draw(*m_camera, aspect);
    }

    dtOut = dt;
    return true;
  }
}
