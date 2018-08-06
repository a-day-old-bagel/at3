

#include "ecsInterface.hpp"
#include "ecsNetworking.hpp"

using namespace ezecs;
using namespace SLNet;

namespace at3 {

  EntityComponentSystemInterface::EntityComponentSystemInterface(ezecs::State *state)
      : state(state),
        setNetInterfaceSub("set_network_interface",
                           RTU_MTHD_DLGT(&EntityComponentSystemInterface::setNetInterface, this))
  {
    compStreams.resize(numCompTypes);
    for (auto & stream : compStreams) {
      stream = std::make_unique<BitStream>();
    }
  }

  void EntityComponentSystemInterface::setNetInterface(void *netInterface) {
    network = *(std::shared_ptr<NetInterface>*) netInterface;
  }

  entityId EntityComponentSystemInterface::createEntity() {

    switch (network->getRole()) {
      case settings::network::SERVER: {

      } break;
      case settings::network::CLIENT: {
        BitStream stream;
        stream.Write((MessageID)ID_USER_PACKET_ECS_REQUEST_ENUM);
        stream.WriteBitsFromIntegerRange((uint8_t)REQ_ENTITY_OP, (uint8_t)0, (uint8_t)(REQ_END_ENUM - 1), false);
        stream.WriteBitsFromIntegerRange((uint8_t)OP_CREATE, (uint8_t)0, (uint8_t)(OP_END_ENUM - 1), false);
        serializeEntityCreationRequest(true, stream, *state, 0); // 0 ID indicates a request rather than a command
        network->send(stream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_ECS_REQUEST);
      } break;
      default: break;
    }

    entityId id;
    CompOpReturn status = this->state->createEntity(&id);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return id;
  }

  void EntityComponentSystemInterface::destroyEntity(const entityId &id) {
    CompOpReturn status = state->deleteEntity(id);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
  }

  void EntityComponentSystemInterface::openEntityRequest() {
    if (entityRequestOpen) {
      fprintf(stderr, "Entity request is already open. Cannot open again until finalized!\n");
    } else {
      entityRequestOpen = true;
      requestStream.Reset();
      for (auto & stream : compStreams) {
        stream->Reset();
      }
    }
  }

  EntityComponentSystemInterface::EcsId EntityComponentSystemInterface::closeEntityRequest(BitStream &stream) {
    EcsId id = 0;
    if (entityRequestOpen) {
      entityRequestOpen = false;
      switch (network->getRole()) {
        case settings::network::SERVER: {

        } break;
        case settings::network::CLIENT: {

        } break;
        default: break;
      }
    } else {
      fprintf(stderr, "Entity request is not open. Cannot finalize until opened!\n");
    }
    return id;
  }

  void EntityComponentSystemInterface::addTransform(const entityId &id, const glm::mat4 &transform) {
    CompOpReturn status = state->add_Placement(id, transform);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == SUCCESS);
  }

  bool EntityComponentSystemInterface::hasTransform(const entityId &id) {
    compMask compsPresent = state->getComponents(id);
    assert(compsPresent);
    return (bool) (compsPresent & PLACEMENT);
  }

  glm::mat4 EntityComponentSystemInterface::getTransform(const entityId &id) {
    Placement *placement;
    ezecs::CompOpReturn status = state->get_Placement(id, &placement);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return placement->mat;
  }

  glm::mat4 EntityComponentSystemInterface::getAbsTransform(const entityId &id) {
    Placement *placement;
    ezecs::CompOpReturn status = state->get_Placement(id, &placement);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return placement->absMat;
  }

  void EntityComponentSystemInterface::setAbsTransform(const ezecs::entityId &id, const glm::mat4 &transform) {
    Placement *placement;
    ezecs::CompOpReturn status = state->get_Placement(id, &placement);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    placement->absMat = transform;
  }
  bool EntityComponentSystemInterface::hasLocalMat3Override(const ezecs::entityId &id) {
    Placement *placement;
    ezecs::CompOpReturn status = state->get_Placement(id, &placement);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return placement->forceLocalRotationAndScale;
  }
  void EntityComponentSystemInterface::setLocalMat3Override(const ezecs::entityId &id, bool value) {
    Placement *placement;
    ezecs::CompOpReturn status = state->get_Placement(id, &placement);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    placement->forceLocalRotationAndScale = value;
  }

  bool EntityComponentSystemInterface::hasCustomModelTransform(const entityId &id) {
    compMask compsPresent = state->getComponents(id);
    assert(compsPresent);
    return (bool) (compsPresent & TRANSFORMFUNCTION);
  }

  glm::mat4 EntityComponentSystemInterface::getCustomModelTransform(const entityId &id) {
    TransformFunction *transformFunction;
    ezecs::CompOpReturn status = state->get_TransformFunction(id, &transformFunction);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return transformFunction->transformed;
  }

  void EntityComponentSystemInterface::addCamera(const ezecs::entityId &id, const float fovy,
                                                      const float nearPlane, const float farPlane) {
    ezecs::CompOpReturn status = this->state->add_Camera(id, fovy, nearPlane, farPlane);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
  }

  void EntityComponentSystemInterface::addMouseControl(const ezecs::entityId &id) {
    CompOpReturn status = state->add_MouseControls(id, false, false);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == SUCCESS);
  }
};
