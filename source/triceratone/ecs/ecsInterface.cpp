

#include "ecsInterface.hpp"

using namespace ezecs;

namespace at3 {

  EntityComponentSystemInterface::EntityComponentSystemInterface(ezecs::State *state) : state(state) {

  }

  entityId EntityComponentSystemInterface::createEntity() {
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

  void EntityComponentSystemInterface::addPerspective(const ezecs::entityId &id, const float fovy, const float near,
                                                      const float far) {
    ezecs::CompOpReturn status = this->state->add_Perspective(id, fovy, near, far);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
  }

  float EntityComponentSystemInterface::getFovy(const ezecs::entityId &id) {
    ezecs::Perspective *perspective;
    ezecs::CompOpReturn status = state->get_Perspective(id, &perspective);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return perspective->fovy;
  }

  float EntityComponentSystemInterface::getFovyPrev(const ezecs::entityId &id) {
    ezecs::Perspective *perspective;
    ezecs::CompOpReturn status = state->get_Perspective(id, &perspective);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return perspective->prevFovy;
  }

  float EntityComponentSystemInterface::getNear(const ezecs::entityId &id) {
    ezecs::Perspective *perspective;
    ezecs::CompOpReturn status = state->get_Perspective(id, &perspective);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return perspective->near;
  }

  float EntityComponentSystemInterface::getFar(const ezecs::entityId &id) {
    ezecs::Perspective *perspective;
    ezecs::CompOpReturn status = state->get_Perspective(id, &perspective);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return perspective->far;
  }

  void EntityComponentSystemInterface::setFar(const ezecs::entityId &id, const float far) {
    ezecs::Perspective *perspective;
    ezecs::CompOpReturn status = state->get_Perspective(id, &perspective);
    assert(status == ezecs::SUCCESS);
    perspective->far = far;
  }

  void EntityComponentSystemInterface::addMouseControl(const ezecs::entityId &id) {
    CompOpReturn status = state->add_MouseControls(id, false, false);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == SUCCESS);
  }
};
