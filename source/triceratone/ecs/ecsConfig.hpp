
#ifndef EZECS_ECSCONFIG_HPP
#define EZECS_ECSCONFIG_HPP

// BEGIN INCLUDES

#include <vector>

#include "definitions.hpp"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "glm/gtx/transform.hpp"

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

// END INCLUDES

using namespace ezecs;

namespace {

  // BEGIN DECLARATIONS

  // NOTE: The parsing done on this file isn't currently able to understand template arguments unless they're
  // typedef'ed here as a single symbol. Sorry. This is on an absurdly long laundry list of things to fix.
  typedef rtu::Delegate<glm::mat4(const glm::mat4&, const glm::mat4&, uint32_t time)> transformFunc;
  typedef std::vector<float>* floatVecPtr;
  typedef std::vector<uint32_t>* uintVecPtr;

  struct Placement : public Component<Placement> {
    glm::mat4 mat = glm::mat4(1.f);
    glm::mat4 absMat = glm::mat4(1.f);
    bool forceLocalRotationAndScale = false;
    Placement(glm::mat4 mat);
    glm::vec3 getTranslation(bool abs = false);
    glm::vec3 getLookAt(bool abs = false);
    float getHorizRot(bool abs = false);
    glm::mat3 getHorizRotMat(bool abs = false);
    glm::quat getQuat(bool abs = false);
  };

  struct TransformFunction : public Component<TransformFunction> {
    glm::mat4 transformed = glm::mat4(1.f);
    transformFunc func;
    TransformFunction(transformFunc func);
  };
  EZECS_COMPONENT_DEPENDENCIES(TransformFunction, Placement)

  struct Perspective : public Component<Perspective> {
    float fovy, prevFovy,
        near, far;
    Perspective(float fovy, float near, float far);
  };
  EZECS_COMPONENT_DEPENDENCIES(Perspective, Placement)

  struct WheelInfo {
    entityId parentVehicle, myId;
    int bulletWheelId;
    float leftOrRight;
  };
  struct WheelInitInfo {
    WheelInfo wi;
    btVector3 connectionPoint, direction, axle;
    float suspensionRestLength, wheelRadius;
    bool isFrontWheel;
  };
  struct TriangleMeshInfo {
    floatVecPtr vertices;
    uintVecPtr indices;
    uint32_t vertexStride;
  };
  struct Physics : public Component<Physics> {
    enum UseCase {
      NONE, PLANE, SPHERE, BOX, DYNAMIC_CONVEX_MESH, STATIC_MESH, WHEEL, CHARA
    };
    int useCase;
    float mass;
    btRigidBody* rigidBody;
    void* initData;
    void* customData = nullptr;
    Physics(float mass, void* initData, UseCase useCase);
    ~Physics();
    void setTransform(glm::mat4 &newTrans);
    void beStill();
  };
  EZECS_COMPONENT_DEPENDENCIES(Physics, Placement)

  struct PyramidControls : public Component<PyramidControls> {
    glm::vec3 accel = glm::vec3(0, 0, 0);
    glm::vec3 force = glm::vec3(0, 0, 0);
    glm::vec3 up = glm::vec3(0, 0, 1);
    bool turbo = false;
    PyramidControls();
  };
  EZECS_COMPONENT_DEPENDENCIES(PyramidControls, Physics)

  struct TrackControls : public Component<TrackControls> {
    glm::vec2 control = glm::vec2(0, 0);
    glm::vec2 brakes = glm::vec2(0, 0);
    glm::vec2 torque = glm::vec2(0, 0);
    bool hasDriver = false;
    std::vector<WheelInfo> wheels;
    btRaycastVehicle *vehicle;
    btRaycastVehicle::btVehicleTuning tuning;
    TrackControls();
  };
  EZECS_COMPONENT_DEPENDENCIES(TrackControls, Physics)

  struct PlayerControls : public Component<PlayerControls> {
    glm::vec2 horizControl = glm::vec2(0, 0);
    glm::vec3 forces = glm::vec3(0, 0, 0);
    glm::vec3 up = glm::vec3(0, 0, 0);
    float equilibriumOffset = 0.f;
    bool jumpRequested = false;
    bool jumpInProgress = false;
    bool isGrounded = false;
    bool isRunning = false;
    PlayerControls();
  };
  EZECS_COMPONENT_DEPENDENCIES(PlayerControls, Physics)

  struct FreeControls : public Component<FreeControls> {
    glm::vec3 control = glm::vec3(0, 0, 0);
    int x10 = 0;
    FreeControls();
  };
  EZECS_COMPONENT_DEPENDENCIES(FreeControls, Placement)

  struct MouseControls : public Component<MouseControls> {
    enum Style {
        NONE, FLAT_Z_UP, CYLINDRICAL_ABOUT_Y
    };
    Style style;
    float yaw = 0, pitch = 0;
    bool invertedX, invertedY;
    MouseControls(bool invertedX, bool invertedY, Style style);
  };
  EZECS_COMPONENT_DEPENDENCIES(MouseControls, Placement)

  // END DECLARATIONS

  // BEGIN DEFINITIONS

  Placement::Placement(glm::mat4 mat)
      : mat(mat) { }

  glm::vec3 Placement::getTranslation(bool abs) {
    return abs ? glm::vec3(absMat[3][0], absMat[3][1], absMat[3][2]) :
                 glm::vec3(   mat[3][0],    mat[3][1],    mat[3][2]);
  }

  glm::vec3 Placement::getLookAt(bool abs) {
    glm::vec4 lookAt = (abs ? absMat : mat) * glm::vec4(0.f, 1.f, 0.f, 0.f);
    return glm::vec3(lookAt.x, lookAt.y, lookAt.z);
  }

  float Placement::getHorizRot(bool abs) {
    glm::quat latestQuat = glm::quat_cast( abs ? absMat : mat );
    glm::vec3 latestLook = latestQuat * glm::vec3(0.f, 1.0, 0.f);
    glm::vec3 latestHorizLook(latestLook.x, latestLook.y, 0.f);
    latestHorizLook = glm::normalize(latestHorizLook);
    return acosf(glm::dot({0.f, 1.f, 0.f}, latestHorizLook)) * (latestLook.x < 0.f ? 1.f : -1.f);
  }

  glm::mat3 Placement::getHorizRotMat(bool abs) {
    return glm::mat3(glm::rotate(getHorizRot(abs), glm::vec3(0.f, 0.f, 1.f)));
  }

  glm::quat Placement::getQuat(bool abs) {
    return glm::quat_cast( abs ? absMat : mat );
  }

  TransformFunction::TransformFunction(transformFunc func)
      : func(func) { }

  Perspective::Perspective(float fovy, float near, float far)
      : fovy(fovy), prevFovy(fovy), near(near), far(far) { }

  Physics::Physics(float mass, void* initData, Physics::UseCase useCase)
      : useCase(useCase), mass(mass), initData(initData) { }
  Physics::~Physics() {
    if (customData && useCase == WHEEL) {
      switch (useCase) {
        case WHEEL: {
          delete ((WheelInfo*)customData);
        } break;
        default: break; // TODO: Make this deconstructor safer, or do deallocation elsewhere
      }
    }
  }
  void Physics::setTransform(glm::mat4 &newTrans) {
    btTransform transform = rigidBody->getCenterOfMassTransform();
    transform.setFromOpenGLMatrix((btScalar *)&newTrans);
    rigidBody->setCenterOfMassTransform(transform);
  }
  void Physics::beStill() {
    rigidBody->setLinearVelocity( {0.f, 0.f, 0.f} );
    rigidBody->setAngularVelocity( {0.f, 0.f, 0.f} );
  }

  PyramidControls::PyramidControls() { }

  TrackControls::TrackControls() { }

  PlayerControls::PlayerControls() { }

  FreeControls::FreeControls() { }

  MouseControls::MouseControls(bool invertedX, bool invertedY, MouseControls::Style style)
      : invertedX(invertedX), invertedY(invertedY), style(style) { }

  // END DEFINITIONS

}

#endif //EZECS_ECSCONFIG_HPP
