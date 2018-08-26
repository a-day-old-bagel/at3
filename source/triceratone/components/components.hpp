
#ifndef EZECS_ECSCONFIG_HPP
#define EZECS_ECSCONFIG_HPP

// BEGIN INCLUDES

#include <vector>
#include <memory>

#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <BitStream.h>
#include <StringCompressor.h>

#include "math.hpp"
#include "delegate.hpp"

// END INCLUDES

using namespace ezecs;

namespace {

  // BEGIN DECLARATIONS

  // NOTE: parsing C++ is always a terrible idea, but in this case it allows for components to have things like
  // helper methods and avoids writing a new pseudo-language for component descriptions. Of course, since only a
  // tiny subset of C++ can be used here anyway, it's basically a pseudo-language. Whatever.
  // Before you get mad that I'm using regex to parse C++ (which is provably impossible), understand that I'm not
  // intending to parse it correctly. I just want to parse enough of it to achieve the ECS generation effect.

  // NOTE: constructors aren't currently able to accept any arguments qualified by const or any other keyword.
  // Basically you must follow this simple syntax: Constructor(type0 name0, type1 name1, ... , typeN nameN).

  // An object to pass to transform functions when ECS state is needed to perform computations
  struct TransFuncEcsContext {
    void *ecs; // Type cannot be known at this juncture
    entityId id;
  };

  // NOTE: The parsing done on this file isn't currently able to understand template arguments unless they're
  // typedef'ed here as a single symbol. Sorry. This is on an absurdly long laundry list of things to fix.
  typedef rtu::Delegate<glm::mat4(const glm::mat4&, const glm::mat4&, uint32_t, TransFuncEcsContext*)> transformFunc;
  typedef std::vector<float>* floatVecPtr;
  typedef std::vector<uint32_t>* uintVecPtr;
  typedef std::shared_ptr<void> sharedVoidPtr;

  // An object to hold a reference to a transformFunc by an ID that can be shared amongst networked instances
  struct TransformFunctionDescriptor {
    uint8_t registrationId = 0;
    transformFunc func;
  };

  struct Placement : public Component<Placement> {
    glm::mat4 mat = glm::mat4(1.f);
    glm::mat4 absMat = glm::mat4(1.f);
    bool forceLocalRotationAndScale = false;
    Placement(glm::mat4 mat);
    glm::vec3 getTranslation(bool abs = false);
    void setTranslation(glm::vec3 &pos);
    glm::vec3 getLookAt(bool abs = false);
    float getHorizRot(bool abs = false);
    glm::mat3 getHorizRotMat(bool abs = false);
    glm::quat getQuat(bool abs = false);
    void setQuat(glm::quat &quat);
  };

  struct SceneNode : public Component<SceneNode> {
    entityId parentId;
    SceneNode(entityId parentId);
  };

  struct TransformFunction : public Component<TransformFunction> {
    glm::mat4 transformed = glm::mat4(1.f);
    uint8_t transFuncId;
    TransformFunction(uint8_t transFuncId);
  };
  EZECS_COMPONENT_DEPENDENCIES(TransformFunction, Placement)

  struct Mesh : public Component<Mesh> {
    std::string meshFileName, textureFileName;
    Mesh(std::string meshFileName, std::string textureFileName);
  };
  EZECS_COMPONENT_DEPENDENCIES(TransformFunction, Placement)

  struct Camera : public Component<Camera> {
    float fovY, prevFovY, nearPlane, farPlane;
    Camera(float fovY, float nearPlane, float farPlane);
  };
  EZECS_COMPONENT_DEPENDENCIES(Camera, Placement)

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
      INVALID = 0, PLANE, SPHERE, BOX, DYNAMIC_CONVEX_MESH, STATIC_MESH, WHEEL, CHARA, MAX_USE_CASE
    };
    int useCase;

    float mass;
    sharedVoidPtr initData;
    void* customData = nullptr;
    btRigidBody* rigidBody;

    Physics(float mass, sharedVoidPtr initData, UseCase useCase);
    void setTransform(glm::mat4 &newTrans);
    void beStill();

    // TODO: if writing to the component, it will have to be re-created in Bullet for changes to happen.
    void serialize(bool rw, SLNet::BitStream & stream);
    static void serialize(bool rw, SLNet::BitStream & stream, float & mass, sharedVoidPtr & initData, int & useCase);

    static const inline uint8_t simulationFps = 60;
    static const inline uint8_t maxStoredStates = 32;
  };
  EZECS_COMPONENT_DEPENDENCIES(Physics, Placement)

  // A possible nonpersistent data type to be held by the Networking component
  struct NetworkedPhysicsData {
//    uint32_t priorityAccumulator = 0;
    btVector3 truthPos = {0, 0, 0};
  };
  // So far this will be to indicate networked-ness of an entity and to hold data used for networked physics.
  // This networked physics data will not need to be persistent, thus the pointer.
  // I haven't worked out net physics yet - if I only send a subset of object updates, I don't want to have to include
  // the id of each object that I send, but otherwise how will the client know which it's receiving, given that my
  // frames aren't synced and clients can join at any time? GafferOnGames does not handle this...
  struct Networking : public Component<Networking> {
    std::shared_ptr<void> nonpersistentCustomData;
    Networking();
  };

  struct PyramidControls : public Component<PyramidControls> {
    glm::vec3 accel = glm::vec3(0, 0, 0);
    glm::vec3 force = glm::vec3(0, 0, 0);
    glm::vec3 up = glm::vec3(0, 0, 1);
    entityId mouseCtrlId = 0;
    bool turbo = false;
    bool shoot = false;
    bool drop = false;
    uint8_t engineActivationLevel = 0;
    PyramidControls(entityId mouseCtrlId);

    static const inline float sideAccelFactor = 2500.f;
    static const inline float upAccelFactor = 4000.f;
  };
  EZECS_COMPONENT_DEPENDENCIES(PyramidControls, Physics)

  struct TrackControls : public Component<TrackControls> {
    glm::vec2 control = glm::vec2(0, 0);
    glm::vec2 brakes = glm::vec2(0, 0);
    glm::vec2 torque = glm::vec2(0, 0);
    bool hasDriver = false;
    bool flipRequested = false;
    std::vector<WheelInfo> wheels;
    btRaycastVehicle *vehicle;
    btRaycastVehicle::btVehicleTuning tuning;
    TrackControls();

    static const inline float vacantBrake = 0.5f;
    static const inline float torqueFactor = 100.f;
    static const inline float wheelRadius = 1.5f;
  };
  EZECS_COMPONENT_DEPENDENCIES(TrackControls, Physics)

  struct WalkControls : public Component<WalkControls> {
    glm::vec3 accel = glm::vec3(0, 0, 0);
    glm::vec3 force = glm::vec3(0, 0, 0);
    glm::vec3 up = glm::vec3(0, 0, 0);
    entityId mouseCtrlId = 0;
    float equilibriumOffset = 0.f;
    bool jumpRequested = false;
    bool jumpInProgress = false;
    bool isGrounded = false;
    bool isRunning = false;
    WalkControls(entityId mouseCtrlId);

    static const inline float humanTorsoRadius = 0.5f;
    static const inline float jumpFactor = 5.f;
    static const inline float midAirFactor = 0.05f;
    static const inline float springFactor = 140.f;
    static const inline float walkFactor = 100.f;
    static const inline float runFactor = 250.f;
  };
  EZECS_COMPONENT_DEPENDENCIES(WalkControls, Physics)

  struct FreeControls : public Component<FreeControls> {
    glm::vec3 control = glm::vec3(0, 0, 0);
    entityId mouseCtrlId = 0;
    int x10 = 0;
    FreeControls(entityId mouseCtrlId);

    static const inline float speedFactor = 5.f;
  };
  EZECS_COMPONENT_DEPENDENCIES(FreeControls, Placement)

  struct MouseControls : public Component<MouseControls> {
    float yaw = 0, pitch = 0;
    glm::mat3 lastCtrlRot = glm::mat4(1.f);
    glm::mat3 lastHorizCtrlRot = glm::mat4(1.f);
    bool invertedX, invertedY, independent;
    MouseControls(bool invertedX, bool invertedY, bool independent);
  };
  EZECS_COMPONENT_DEPENDENCIES(MouseControls, Placement)

  /**
   * This is pretty weird - a Player component doesn't impart any functionality to an entity at all, it's just a
   * place to store player information, and so doesn't really "belong" in a component. But since we want all persistent
   * data to be stored in the ECS, this was the first way I thought of to achieve that. Maybe we'll change it once a
   * better way comes up, or once this causes weird problems.
   * For example, what happens if the entity this belongs to gets deleted? That player is just lost from the world?
   * Any attempts from the corresponding user to play the game would just break?
   * Right now the only persistent data being stored are id's of the different avatars each player can embody under
   * normal circumstances, and each player is just identified by the ID of the Player component itself.
   * So if you were to re-log into a simulation, you'd have to be given the correct ID by the server.
   *
   * Maybe a better way to do this would be to not store id's of entities that *actually* represent a player's avatar in
   * here, but to rather give each avatar entity one of these components with information like health, status, and user
   * identification of some kind, so that a user can only correspond to one entity with "his/her" Player component at a
   * time. This assumes that a system will keep a 1-to-1 map from [user id] to [Player component id] in it somewhere.
   */
  struct Player : public Component<Player> { // TODO: Use a persistent ID like a username or salted hash or something
    entityId free = 0, walk = 0, pyramid = 0, track = 0;
    Player(entityId free, entityId walk, entityId pyramid, entityId track);
  };

  // END DECLARATIONS

  // BEGIN DEFINITIONS

  Placement::Placement(glm::mat4 mat)
      : mat(mat) { }

  glm::vec3 Placement::getTranslation(bool abs) {
    return abs ? glm::vec3(absMat[3][0], absMat[3][1], absMat[3][2]) :
                 glm::vec3(   mat[3][0],    mat[3][1],    mat[3][2]);
  }

  void Placement::setTranslation(glm::vec3 &pos) {
    mat[3][0] = pos.x;
    mat[3][1] = pos.y;
    mat[3][2] = pos.z;
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

  void Placement::setQuat(glm::quat &quat) {
    glm::vec3 pos = getTranslation(false);
    mat = glm::mat4(glm::mat3_cast(quat));
    setTranslation(pos);
  }

  SceneNode::SceneNode(entityId parentId)
      : parentId(parentId) { }

  TransformFunction::TransformFunction(uint8_t transFuncId)
      : transFuncId(transFuncId) { }

  Mesh::Mesh(std::string meshFileName, std::string textureFileName)
      : meshFileName(meshFileName), textureFileName(textureFileName) { }

  Camera::Camera(float fovY, float nearPlane, float farPlane)
      : fovY(fovY), prevFovY(fovY), nearPlane(nearPlane), farPlane(farPlane) { }

  Physics::Physics(float mass, sharedVoidPtr initData, Physics::UseCase useCase)
      : useCase(useCase), mass(mass), initData(initData) { }
  void Physics::setTransform(glm::mat4 &newTrans) {
    btTransform transform = rigidBody->getCenterOfMassTransform();
    transform.setFromOpenGLMatrix((btScalar *)&newTrans);
    rigidBody->setCenterOfMassTransform(transform);
  }
  void Physics::beStill() {
    rigidBody->setLinearVelocity( {0.f, 0.f, 0.f} );
    rigidBody->setAngularVelocity( {0.f, 0.f, 0.f} );
  }
  // TODO: if writing to the component, it will have to be re-created in Bullet for changes to happen.
  void Physics::serialize(bool rw, SLNet::BitStream & stream) {
    serialize(rw, stream, mass, initData, useCase);
  }
  void Physics::serialize(bool rw, SLNet::BitStream & stream, float & mass, sharedVoidPtr & initData,
                                 int & useCase) {
    stream.Serialize(rw, mass);
    stream.SerializeBitsFromIntegerRange(rw, useCase, INVALID + 1, MAX_USE_CASE - 1);
    switch(useCase) {
      case Physics::SPHERE: {
        if ( ! rw) { initData = std::make_shared<float>(); }
        float & radius = *((float*)initData.get());
        stream.Serialize(rw, radius);
      } break;
      case Physics::DYNAMIC_CONVEX_MESH: {
        if ( ! rw) { initData = std::make_shared<std::vector<float>>(); }
        std::vector<float> & floats = *((std::vector<float>*)initData.get());
        size_t numFloats = floats.size();
        stream.Serialize(rw, numFloats);
        if ( ! rw) {
          floats.resize(numFloats);
        }
        for (auto & flt : floats) {
          stream.Serialize(rw, flt);
        }
      } break;
      case Physics::STATIC_MESH: {
        if (rw) {
          std::string & meshFileName = *((std::string*)initData.get());
          SLNet::StringCompressor::Instance()->EncodeString(meshFileName.c_str(), 256, &stream);
        } else {
          char meshFileNameIn[256];
          SLNet::StringCompressor::Instance()->DecodeString(meshFileNameIn, 256, &stream);
          initData = std::make_shared<std::string>(meshFileNameIn);
        }
      } break;
      case Physics::WHEEL: {
        if ( ! rw) { initData = std::make_shared<WheelInitInfo>(); }
        WheelInitInfo & info = *((WheelInitInfo*)initData.get());
        stream.Serialize(rw, info);
      } break;
      case Physics::CHARA:
      case Physics::BOX:
      case Physics::PLANE:
      default: break;
    }
  }

  Networking::Networking() { }

  PyramidControls::PyramidControls(entityId mouseCtrlId)
      : mouseCtrlId(mouseCtrlId) { }

  TrackControls::TrackControls() { }

  WalkControls::WalkControls(entityId mouseCtrlId)
      : mouseCtrlId(mouseCtrlId) { }

  FreeControls::FreeControls(entityId mouseCtrlId)
      : mouseCtrlId(mouseCtrlId) { }

  MouseControls::MouseControls(bool invertedX, bool invertedY, bool independent)
      : invertedX(invertedX), invertedY(invertedY), independent(independent) { }

  Player::Player(entityId free, entityId walk, entityId pyramid, entityId track)
      : free(free), walk(walk), pyramid(pyramid), track(track) { }

  // END DEFINITIONS

}

#endif //EZECS_ECSCONFIG_HPP
