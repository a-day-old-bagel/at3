
#include <algorithm>
#include "ecsSystem_netSync.hpp"

using namespace SLNet;

namespace at3 {

  NetSyncSystem::NetSyncSystem(State * state)
      : System(state),
        setNetInterfaceSub("set_network_interface", RTU_MTHD_DLGT(&NetSyncSystem::setNetInterface, this)),
        switchToMouseCtrlSub("switch_to_mouse_controls", RTU_MTHD_DLGT(&NetSyncSystem::switchToMouseCtrl, this)),
        switchToWalkCtrlSub("switch_to_walking_controls", RTU_MTHD_DLGT(&NetSyncSystem::switchToWalkCtrl, this)),
        switchToPyramidCtrlSub("switch_to_pyramid_controls", RTU_MTHD_DLGT(&NetSyncSystem::switchToPyramidCtrl, this)),
        switchToTrackCtrlSub("switch_to_track_controls", RTU_MTHD_DLGT(&NetSyncSystem::switchToTrackCtrl, this)),
        switchToFreeCtrlSub("switch_to_free_controls", RTU_MTHD_DLGT(&NetSyncSystem::switchToFreeCtrl, this))
  {
    name = "Net Sync System";
  }
  bool NetSyncSystem::onInit() {
    return true;
  }
  void NetSyncSystem::onTick(float dt) {
    switch(network->getRole()) {
      case settings::network::SERVER: {
        sendPlacementSync();
        sendPhysicsSync();
        receiveSyncPacketsFromClients();
      } break;
      case settings::network::CLIENT: {
        receiveSyncPacketsFromServer();
      } break;
      default: break;
    }
  }

  void NetSyncSystem::beginSyncStream(BitStream &stream, MessageID messageType, uint16_t numComponents) {
    stream.Write(messageType);
    stream.Write(numComponents);
  }

  void NetSyncSystem::sendPlacementSync() { // TODO: exclude entities that have physics components, use those instead.
    BitStream stream;
    uint16_t numComps = std::max((size_t) 1, registries[0].ids.size() / 10);  // each component synced every 10 frames
    beginSyncStream(stream, ID_SYNC_PLACEMENT, numComps);
    for (uint32_t i = 0; i < numComps; ++i, ++currentPlacementIndex) {
      if (currentPlacementIndex >= registries[0].ids.size()) { currentPlacementIndex = 0; }
      entityId id = registries[0].ids[currentPlacementIndex];
      Placement *placement;
      state->get_Placement(id, &placement);
      stream.Write(id);
      stream.Write(placement->mat);
    }
    network->send(stream);
  }

  void NetSyncSystem::sendPhysicsSync() { // FIXME: numComps assumes that most objects are dynamic and active
    BitStream stream;
    uint16_t numComps = std::max((size_t) 1, registries[1].ids.size() / 10);
    entityId antiWrapId = 0;
    beginSyncStream(stream, ID_SYNC_PHYSICS, numComps);
    for (uint32_t i = 0; i < numComps; ++i, ++currentPhysicsIndex) {
      if (currentPhysicsIndex >= registries[1].ids.size()) { currentPhysicsIndex = 0; }
      entityId id = registries[1].ids[currentPhysicsIndex];
      if (antiWrapId) {
        // prevents wrapping around when fewer than 1/10th of objects are moving
        if (antiWrapId == id) { break; } // FIXME: IN SOME CASES, THE PACKET WILL HAVE FEWER THAN numComps COMPONENTS!!!
      } else {
        antiWrapId = id;
      }
      Physics *physics;
      state->get_Physics(id, &physics);
      if ( physics->useCase != Physics::WHEEL &&
           physics->useCase != Physics::STATIC_MESH &&
           physics->rigidBody->isActive() ) {
        stream.Write(id);
        stream.Write(physics->rigidBody->getLinearVelocity());
      } else {
        ++numComps; // skip this component but still write as many as we originally intended
      }
    }
    network->send(stream);
  }

  void NetSyncSystem::receiveSyncPacketsFromClients() {
    for (auto pack : network->getSyncPackets()) {
      BitStream stream(pack->data, pack->length, false);
      MessageID syncType;
      stream.Read(syncType);
      switch (syncType) {
        case ID_SYNC_MOUSECONTROLS: {

        } break;
        case ID_SYNC_WALKCONTROLS: {

        } break;
        case ID_SYNC_PYRAMIDCONTROLS: {

        } break;
        case ID_SYNC_TRACKCONTROLS: {

        } break;
        case ID_SYNC_FREECONTROLS: {

        } break;
        default: {
          fprintf(stderr, "Server received bad sync packet!\n");
        } break;
      }
    }
    network->discardSyncPackets();
  }

  void NetSyncSystem::receiveSyncPacketsFromServer() {
    for (auto pack : network->getSyncPackets()) {
      BitStream stream(pack->data, pack->length, false);
      MessageID syncType;
      stream.Read(syncType);
      uint16_t numComps;
      stream.Read(numComps);
      switch (syncType) {
        case ID_SYNC_PLACEMENT: {
          receivePlacementSyncs(stream, numComps);
        } break;
        case ID_SYNC_PHYSICS: {
          receivePhysicsSyncs(stream, numComps);
        } break;
        default: {
          fprintf(stderr, "Client received bad sync packet!\n");
        } break;
      }
    }
    network->discardSyncPackets();
  }

  void NetSyncSystem::receivePlacementSyncs(BitStream &stream, uint16_t count) {
    for (uint16_t i = 0; i < count; ++i) {
      entityId id;
      glm::mat4 mat;
      stream.Read(id);
      stream.Read(mat);

      if ( ! state->getComponents(id)) { continue; } // FIXME: this temporarily just ignores new objects on the server

      if (state->getComponents(id) & PHYSICS) {
        Physics *physics;
        state->get_Physics(id, &physics);
        if ( physics->useCase != Physics::WHEEL &&
             physics->useCase != Physics::STATIC_MESH &&
             physics->rigidBody->isActive() ) {
          btTransform transform;
          physics->rigidBody->getMotionState()->getWorldTransform(transform);
          transform.setFromOpenGLMatrix((btScalar*)&mat);
          physics->rigidBody->setCenterOfMassTransform(transform);
          continue; // don't bother writing to the actual placement, since the physics system will do that
        }
      }
      Placement *placement;
      state->get_Placement(id, &placement);
      placement->mat = mat;
    }
  }

  void NetSyncSystem::receivePhysicsSyncs(BitStream &stream, uint16_t count) {
    for (uint16_t i = 0; i < count; ++i) {

      // TODO: MAKE SURE THE PACKET ACTUALLY CONTAINS numComps COMPONENTS. IT MIGHT NOT (SEE sendPhysicsSync) !!!!

      entityId id;
      btVector3 vel;
      stream.Read(id);
      stream.Read(vel);

      if ( ! state->getComponents(id)) { continue; } // FIXME: this temporarily just ignores new objects on the server

      // TODO: use interpolation within the bullet stuff instead of setting things directly
      // TODO: also set position for physics objects, and don't do the placement sync for them (see above)
      Physics *physics;
      state->get_Physics(id, &physics);
      physics->rigidBody->setLinearVelocity(vel);
    }
  }

  void NetSyncSystem::setNetInterface(void *netInterface) {
    network = *(std::shared_ptr<NetInterface>*) netInterface;
  }

  void NetSyncSystem::switchToMouseCtrl(void *id) {
    mouseControlId = *(entityId*)id;
  }

  void NetSyncSystem::switchToWalkCtrl(void *id) {
    keyControlId = *(entityId*)id;
  }

  void NetSyncSystem::switchToPyramidCtrl(void *id) {
    keyControlId = *(entityId*)id;
  }

  void NetSyncSystem::switchToTrackCtrl(void *id) {
    keyControlId = *(entityId*)id;
  }

  void NetSyncSystem::switchToFreeCtrl(void *id) {
    keyControlId = *(entityId*)id;
  }
}
