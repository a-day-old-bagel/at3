

#include "interface.hpp"
#include "serialization.hpp"

using namespace ezecs;
using namespace SLNet;

namespace at3 {

  EntityComponentSystemInterface::EntityComponentSystemInterface(ezecs::State *state)
      : state(state),
        setNetInterfaceSub("set_network_interface",
                           RTU_MTHD_DLGT(&EntityComponentSystemInterface::setNetInterface, this))
  {
    compStreams.resize(numCompTypes - 1);
    for (auto & stream : compStreams) {
      stream = std::make_unique<BitStream>();
    }
  }

  void EntityComponentSystemInterface::setNetInterface(void *netInterface) {
    network = *(std::shared_ptr<NetInterface>*) netInterface;
  }

  void EntityComponentSystemInterface::openEntityRequest() {
    if (entityRequestOpen) {
      fprintf(stderr, "Entity request is already open. Cannot open again until finalized!\n");
    } else {
      entityRequestOpen = true;
      if (network->getRole() == settings::network::NONE) {
        state->createEntity(&openRequestId);
      } else {
        stream.Reset();
        for (auto &stream : compStreams) {
          stream->Reset();
        }
      }
    }
  }

  EntityComponentSystemInterface::EcsId EntityComponentSystemInterface::closeEntityRequest() {
    EcsId id = 0;
    if (entityRequestOpen) {
      entityRequestOpen = false;
      if (network->getRole() == settings::network::NONE) {
        return openRequestId;
      }
      writeEntityRequestHeader(stream);
      switch (network->getRole()) {
        case settings::network::SERVER: {
          BitStream headerless;
          serializeEntityCreationRequest(true, headerless, *state, 0, &compStreams); // ID 0 = request without action
          id = serializeEntityCreationRequest(false, headerless, *state); // treat it as if it came from a client
          stream.Write(headerless);
        } break;
        case settings::network::CLIENT: {
          serializeEntityCreationRequest(true, stream, *state, 0, &compStreams); // ID 0 = request without action
        } break;
        default: break;
      }
      network->send(stream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_ECS_REQUEST);
    } else {
      fprintf(stderr, "Entity request is not open. Cannot finalize until opened!\n");
    }
    // TODO: for a client request, instead of returning 0, return a local-end ID that can later be replaced.
    return id;
  }

  void EntityComponentSystemInterface::broadcastManualEntity(const entityId &id) {
    // Solo's don't need to do this, and clients should not do this. This might be a redundant check, though.
    if (network->getRole() == settings::network::SERVER) {
      stream.Reset();
      writeEntityRequestHeader(stream);
      serializeEntityCreationRequest(true, stream, *state, id);
      network->send(stream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_ECS_REQUEST);
    }
  }

  void EntityComponentSystemInterface::requestEntityDeletion(const ezecs::entityId &id) {
    switch (network->getRole()) {
      case settings::network::SERVER: {
        stream.Reset();
        serializeEntityDeletionRequest(true, stream, *state, id);
        network->send(stream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_ECS_REQUEST);

        ezecs::CompOpReturn status = state->deleteEntity(id);
        EZECS_CHECK_PRINT(EZECS_ERR(status));
//        printf("Entity %u deleted.\n", id);
      } break;
      case settings::network::CLIENT: {
        // TODO: Along with entity creation, decide if clients should be able to send these at all.
//        stream.Reset();
//        serializeEntityDeletionRequest(true, stream, *state, id);
//        network->send(stream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_ECS_REQUEST);
      } break;
      default: {
        state->deleteEntity(id);
      } break;
    }
  }

  void EntityComponentSystemInterface::requestPlacement(const glm::mat4 &mat) {
    if (network->getRole() == settings::network::NONE) {
      state->addPlacement(openRequestId, mat);
    } else {
      serializePlacement(true, nullptr, *state, 0, &compStreams, &mat);
    }
  }

  void EntityComponentSystemInterface::requestSceneNode(EntityComponentSystemInterface::EcsId parentId) {
    if (network->getRole() == settings::network::NONE) {
      state->addSceneNode(openRequestId, parentId);
    } else {
      serializeSceneNode(true, nullptr, *state, 0, &compStreams, &parentId);
    }
  }

  void EntityComponentSystemInterface::requestTransformFunction(uint8_t transFuncId) {
    if (network->getRole() == settings::network::NONE) {
      state->addTransformFunction(openRequestId, transFuncId);
    } else {
      serializeTransformFunction(true, nullptr, *state, 0, &compStreams, &transFuncId);
    }
  }

  void EntityComponentSystemInterface::requestMesh(std::string meshFileName, std::string textureFileName){
    if (network->getRole() == settings::network::NONE) {
      state->addMesh(openRequestId, meshFileName, textureFileName);
    } else {
      serializeMesh(true, nullptr, *state, 0, &compStreams, &meshFileName, &textureFileName);
    }
  }

  void EntityComponentSystemInterface::requestCamera(float fovY, float nearPlane, float farPlane) {
    if (network->getRole() == settings::network::NONE) {
      state->addCamera(openRequestId, fovY, nearPlane, farPlane);
    } else {
      serializeCamera(true, nullptr, *state, 0, &compStreams, &fovY, &nearPlane, &farPlane);
    }
  }

  void EntityComponentSystemInterface::requestPhysics(float mass, std::shared_ptr<void> &initData, int useCase) {
    if (network->getRole() == settings::network::NONE) {
      state->addPhysics(openRequestId, mass, initData, (Physics::UseCase)useCase);
    } else {
      serializePhysics(true, nullptr, *state, 0, &compStreams, &mass, &initData, &useCase);
    }
  }
  void EntityComponentSystemInterface::requestStaticMeshPhysics(const std::string &meshFileName) {
    std::shared_ptr<void> meshFileNamePtr = std::make_shared<std::string>(meshFileName);
    requestPhysics(0, meshFileNamePtr, Physics::STATIC_MESH);
  }

  void EntityComponentSystemInterface::requestNetworking() {
    if (network->getRole() == settings::network::NONE) {
      state->addNetworking(openRequestId);
    } else {
      serializeNetworking(true, nullptr, *state, 0, &compStreams);
    }
  }

  void EntityComponentSystemInterface::requestPyramidControls(EntityComponentSystemInterface::EcsId mouseCtrlId){
    if (network->getRole() == settings::network::NONE) {
      state->addPyramidControls(openRequestId, mouseCtrlId);
    } else {
      serializePyramidControls(true, nullptr, *state, 0, &compStreams, &mouseCtrlId);
    }
  }

  void EntityComponentSystemInterface::requestTrackControls() {
    if (network->getRole() == settings::network::NONE) {
      state->addTrackControls(openRequestId);
    } else {
      serializeTrackControls(true, nullptr, *state, 0, &compStreams);
    }
  }

  void EntityComponentSystemInterface::requestWalkControls(EntityComponentSystemInterface::EcsId mouseCtrlId) {
    if (network->getRole() == settings::network::NONE) {
      state->addWalkControls(openRequestId, mouseCtrlId);
    } else {
      serializeWalkControls(true, nullptr, *state, 0, &compStreams, &mouseCtrlId);
    }
  }

  void EntityComponentSystemInterface::requestFreeControls(EntityComponentSystemInterface::EcsId mouseCtrlId) {
    if (network->getRole() == settings::network::NONE) {
      state->addFreeControls(openRequestId, mouseCtrlId);
    } else {
      serializeFreeControls(true, nullptr, *state, 0, &compStreams, &mouseCtrlId);
    }
  }

  void EntityComponentSystemInterface::requestMouseControls(bool invertedX, bool invertedY, bool independent) {
    if (network->getRole() == settings::network::NONE) {
      state->addMouseControls(openRequestId, invertedX, invertedY, independent);
    } else {
      serializeMouseControls(true, nullptr, *state, 0, &compStreams, &invertedX, &invertedY, &independent);
    }
  }

  void EntityComponentSystemInterface::requestPlayer(EcsId free, EcsId walk, EcsId pyramid, EcsId track) {
    if (network->getRole() == settings::network::NONE) {
      state->addPlayer(openRequestId, free, walk, pyramid, track);
    } else {
      serializePlayer(true, nullptr, *state, 0, &compStreams, &free, &walk, &pyramid, &track);
    }
  }

  bool EntityComponentSystemInterface::hasTransform(const entityId &id) {
    compMask compsPresent = state->getComponents(id);
    assert(compsPresent);
    return (bool) (compsPresent & PLACEMENT);
  }

  glm::mat4 EntityComponentSystemInterface::getTransform(const entityId &id) {
    Placement *placement;
    ezecs::CompOpReturn status = state->getPlacement(id, &placement);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return placement->mat;
  }

  glm::mat4 EntityComponentSystemInterface::getAbsTransform(const entityId &id) {
    Placement *placement;
    ezecs::CompOpReturn status = state->getPlacement(id, &placement);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return placement->absMat;
  }

  void EntityComponentSystemInterface::setAbsTransform(const ezecs::entityId &id, const glm::mat4 &transform) {
    Placement *placement;
    ezecs::CompOpReturn status = state->getPlacement(id, &placement);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    placement->absMat = transform;
  }
  bool EntityComponentSystemInterface::hasLocalMat3Override(const ezecs::entityId &id) {
    Placement *placement;
    ezecs::CompOpReturn status = state->getPlacement(id, &placement);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return placement->forceLocalRotationAndScale;
  }

  bool EntityComponentSystemInterface::hasCustomModelTransform(const entityId &id) {
    compMask compsPresent = state->getComponents(id);
    assert(compsPresent);
    return (bool) (compsPresent & TRANSFORMFUNCTION);
  }

  glm::mat4 EntityComponentSystemInterface::getCustomModelTransform(const entityId &id) {
    TransformFunction *transformFunction;
    ezecs::CompOpReturn status = state->getTransformFunction(id, &transformFunction);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return transformFunction->transformed;
  }

  void EntityComponentSystemInterface::notifyOfSceneTreeRemoval(const EntityComponentSystemInterface::EcsId &id) {
    // We can just access the state directly since this function should only be called when a parent object has
    // been deleted, which deletion should have already been networked if networking was required. This should
    // guarantee that the correct children are deleted on both server and client.
    state->deleteEntity(id);
//    printf("ECS notified of scene tree removal of %u\n", id);
  }
};