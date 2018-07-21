
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
        // TODO: maybe send all of these in the same BitStream (does RakNet take care of packet splitting?)
        sendPlacementSyncs();
        sendPhysicsSyncs();
        receiveSyncPacketsFromClients();
      } break;
      case settings::network::CLIENT: {
        sendControlsSyncs();
        receiveSyncPacketsFromServer();
      } break;
      default: break;
    }
  }

  void NetSyncSystem::beginSyncStream(BitStream &stream, MessageID messageType, uint16_t numComponents) {
    stream.Write(messageType);
    stream.Write(numComponents);
  }

  // TODO: instead of splitting the ids to spread out updates, split them by size of packet and send as many as possible
  // TODO: if you want all ids to go through every n units of time, just send them all every update even if multiple packets.
  // TODO: Check of RakNet automatically splits bitstreams into packets.

  void NetSyncSystem::sendPlacementSyncs() {
    Time currentTime = GetTime();
    if (currentTime - lastPlacementSyncTime < 10) { return; }
    lastPlacementSyncTime = currentTime;

    BitStream stream;
    uint16_t actualWritten = 0;
    entityId antiWrapId = 0;
    uint16_t numComps = std::max((size_t) 1, registries[0].ids.size() / 10);
    beginSyncStream(stream, ID_SYNC_PLACEMENT, numComps);
    for (uint32_t i = 0; i < numComps; ++i, ++currentPlacementIndex) {
      if (currentPlacementIndex >= registries[0].ids.size()) { currentPlacementIndex = 0; }
      entityId id = registries[0].ids[currentPlacementIndex];

      if (antiWrapId) { // prevent wrapping around when fewer than 1/10th of placements don't belong to physics
        if (antiWrapId == id) { // FIXME: None of this works. It just doesn't seem to send correctly. Maybe get rid of numComps after all.
          stream.SetWriteOffset(sizeof(MessageID));
          stream.Write(actualWritten);
          fprintf(stderr, "Placement packet written shorter than expected, if client crashed, this is maybe why.\n");
          break;
        }
      } else {
        antiWrapId = id;
      }

      if (state->getComponents(id) & PHYSICS) {
        ++numComps; // skip this component but still try to write as many as we originally intended
      } else {
        Placement *placement;
        state->get_Placement(id, &placement);
        stream.Write(id);
        stream.Write(placement->mat);
        ++actualWritten;
      }
    }
    network->send(stream);
  }

  void NetSyncSystem::sendPhysicsSyncs() { // FIXME: numComps assumes that most objects are dynamic and active
    Time currentTime = GetTime();
    if (currentTime - lastPhysicsSyncTime < 10) { return; }
    lastPhysicsSyncTime = currentTime;

    BitStream stream;
    uint16_t actualWritten = 0;
    entityId antiWrapId = 0;
    uint16_t numComps = std::max((size_t) 1, registries[1].ids.size() / 10);
    beginSyncStream(stream, ID_SYNC_PHYSICS, numComps);
    for (uint32_t i = 0; i < numComps; ++i, ++currentPhysicsIndex) {
      if (currentPhysicsIndex >= registries[1].ids.size()) { currentPhysicsIndex = 0; }
      entityId id = registries[1].ids[currentPhysicsIndex];

      if (antiWrapId) { // prevent wrapping around when fewer than 1/10th of objects are dynamic and active
        if (antiWrapId == id) {  // FIXME: None of this works. It just doesn't seem to send correctly. Maybe get rid of numComps after all.
          stream.SetWriteOffset(sizeof(MessageID));
          stream.Write(actualWritten);
          fprintf(stderr, "Physics packet written shorter than expected, if client crashed, this is maybe why.\n");
          break;
        }
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
        btTransform transform;
        physics->rigidBody->getMotionState()->getWorldTransform(transform);
        stream.Write(transform);
        ++actualWritten;
      } else {
        ++numComps; // skip this component but still try to write as many as we originally intended
      }
    }
    network->send(stream);
  }

  void NetSyncSystem::sendControlsSyncs() {
    BitStream stream;
    stream.Write(keyControlMessageId);
    MouseControls *mouseControls;
    state->get_MouseControls(mouseControlId, &mouseControls);
    stream.Write(mouseControlId);
    stream.Write(*mouseControls);
    stream.Write(keyControlId);
    switch(keyControlMessageId) { // FIXME: change all of these to only send the initial control stuff (accel, not force, etc.)
      case ID_SYNC_WALKCONTROLS: {
        PlayerControls *playerControls;
        state->get_PlayerControls(keyControlId, &playerControls);
        stream.Write(*playerControls);
      } break;
      case ID_SYNC_PYRAMIDCONTROLS: {
        PyramidControls *pyramidControls;
        state->get_PyramidControls(keyControlId, &pyramidControls);
        stream.Write(*pyramidControls);
      } break;
      case ID_SYNC_TRACKCONTROLS: { // FIXME: Trackcontrols break the receiving end - pointers?
        TrackControls *trackControls;
        state->get_TrackControls(keyControlId, &trackControls);
        stream.Write(*trackControls);
      } break;
      case ID_SYNC_FREECONTROLS: {
        FreeControls *freeControls;
        state->get_FreeControls(keyControlId, &freeControls);
        stream.Write(*freeControls);
      } break;
      default: break;
    }
    network->send(stream, IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED);
  }

  void NetSyncSystem::receiveSyncPacketsFromClients() { // TODO: also send server controls to clients for better sim
    for (auto pack : network->getSyncPackets()) {
      BitStream stream(pack->data, pack->length, false);
      MessageID syncType;
      stream.Read(syncType);
      entityId mouseId;
      stream.Read(mouseId);
      MouseControls *mouseControls;
      state->get_MouseControls(mouseId, &mouseControls);
      stream.Read(*mouseControls);
      entityId controlId;
      stream.Read(controlId);
      switch (syncType) {
        case ID_SYNC_WALKCONTROLS: {
          PlayerControls *playerControls;
          state->get_PlayerControls(controlId, &playerControls);
          stream.Read(*playerControls);
        } break;
        case ID_SYNC_PYRAMIDCONTROLS: {
          PyramidControls *pyramidControls;
          state->get_PyramidControls(controlId, &pyramidControls);
          stream.Read(*pyramidControls);
        } break;
        case ID_SYNC_TRACKCONTROLS: { // FIXME: Trackcontrols break the receiving end - pointers?
          TrackControls *trackControls;
          state->get_TrackControls(controlId, &trackControls);
          stream.Read(*trackControls);
        } break;
        case ID_SYNC_FREECONTROLS: {
          FreeControls *freeControls;
          state->get_FreeControls(controlId, &freeControls);
          stream.Read(*freeControls);
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

      Placement *placement;
      state->get_Placement(id, &placement);
      placement->mat = mat;
    }
  }

  void NetSyncSystem::receivePhysicsSyncs(BitStream &stream, uint16_t count) {
    for (uint16_t i = 0; i < count; ++i) {
      entityId id;
      btVector3 vel;
      btTransform transform;
      stream.Read(id);
      stream.Read(vel);
      stream.Read(transform);

      if ( ! state->getComponents(id)) { continue; } // FIXME: this temporarily just ignores new objects on the server

      // TODO: find more interpolation methods within the bullet stuff instead of setting things directly?
      // TODO: Or write own "smooth sync" interpolation stuff somehow.
      Physics *physics;
      state->get_Physics(id, &physics);
      physics->rigidBody->setLinearVelocity(vel);
      physics->rigidBody->setCenterOfMassTransform(transform);
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
    keyControlMessageId = ID_SYNC_WALKCONTROLS;
  }

  void NetSyncSystem::switchToPyramidCtrl(void *id) {
    keyControlId = *(entityId*)id;
    keyControlMessageId = ID_SYNC_PYRAMIDCONTROLS;
  }

  void NetSyncSystem::switchToTrackCtrl(void *id) {
    keyControlId = *(entityId*)id;
    keyControlMessageId = ID_SYNC_TRACKCONTROLS;
  }

  void NetSyncSystem::switchToFreeCtrl(void *id) {
    keyControlId = *(entityId*)id;
    keyControlMessageId = ID_SYNC_FREECONTROLS;
  }
}
