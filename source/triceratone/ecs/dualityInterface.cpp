

#include "dualityInterface.h"

using namespace ezecs;

namespace at3 {

  DualityInterface::DualityInterface(State *state) : state(state) {

  }

  entityId DualityInterface::createEntity() {
    entityId id;
    CompOpReturn status = this->state->createEntity(&id);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return id;
  }

  void DualityInterface::destroyEntity(const entityId &id) {
    CompOpReturn status = state->deleteEntity(id);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
  }

  void DualityInterface::addTransform(const entityId &id, const glm::mat4 &transform) {
    CompOpReturn status = state->add_Placement(id, transform);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == SUCCESS);
  }

  bool DualityInterface::hasTransform(const entityId &id) {
    compMask compsPresent = state->getComponents(id);
    assert(compsPresent);
    return (bool)(compsPresent & PLACEMENT);
  }

  glm::mat4 DualityInterface::getTransform(const entityId &id) {
    Placement* placement;
    ezecs::CompOpReturn status = state->get_Placement(id, &placement);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return placement->mat;
  }

  void DualityInterface::setTransform(const ezecs::entityId &id, const glm::mat4 &transform) {
    Placement* placement;
    ezecs::CompOpReturn status = state->get_Placement(id, &placement);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    placement->mat = transform;
  }

  bool DualityInterface::hasTransformFunction(const entityId &id) {
    compMask compsPresent = state->getComponents(id);
    assert(compsPresent);
    return (bool)(compsPresent & TRANSFORMFUNCTION);
  }

  glm::mat4 DualityInterface::getTransformFunction(const entityId &id) {
    TransformFunction* transformFunction;
    ezecs::CompOpReturn status = state->get_TransformFunction(id, &transformFunction);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return transformFunction->transformed;
  }

  void DualityInterface::addPerspective(const ezecs::entityId &id, const float fovy, const float near, const float far){
    ezecs::CompOpReturn status = this->state->add_Perspective(id, fovy, near, far);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
  }

  float DualityInterface::getFovy(const ezecs::entityId &id) {
    ezecs::Perspective* perspective;
    ezecs::CompOpReturn status = state->get_Perspective(id, &perspective);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return perspective->fovy;
  }

  float DualityInterface::getFovyPrev(const ezecs::entityId &id) {
    ezecs::Perspective* perspective;
    ezecs::CompOpReturn status = state->get_Perspective(id, &perspective);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return perspective->prevFovy;
  }

  float DualityInterface::getNear(const ezecs::entityId &id) {
    ezecs::Perspective* perspective;
    ezecs::CompOpReturn status = state->get_Perspective(id, &perspective);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return perspective->near;
  }

  float DualityInterface::getFar(const ezecs::entityId &id) {
    ezecs::Perspective* perspective;
    ezecs::CompOpReturn status = state->get_Perspective(id, &perspective);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
    return perspective->far;
  }

  void DualityInterface::setFar(const ezecs::entityId &id, const float far) {
    ezecs::Perspective* perspective;
    ezecs::CompOpReturn status = state->get_Perspective(id, &perspective);
    assert(status == ezecs::SUCCESS);
    perspective->far = far;
  }

  void DualityInterface::addTerrain(const ezecs::entityId &id, const glm::mat4 &transform, floatVecPtr heights,
                                const size_t resX, const size_t resY, const float sclX, const float sclY,
                                const float sclZ, const float minZ, const float maxZ) {
    addTransform(id, transform);

    ezecs::CompOpReturn status;
    status = this->state->add_Terrain(id, heights, resX, resY, sclX, sclY, sclZ, minZ, maxZ);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);

    status = this->state->add_Physics(id, 0.f, NULL, Physics::TERRAIN);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == ezecs::SUCCESS);
  }

  void DualityInterface::addMouseControl(const ezecs::entityId &id) {
    CompOpReturn status = state->add_MouseControls(id, false, false);
    EZECS_CHECK_PRINT(EZECS_ERR(status));
    assert(status == SUCCESS);
  }
};
