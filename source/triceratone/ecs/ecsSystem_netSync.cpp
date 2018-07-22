
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

        BitStream controlStream;
        sendControlsSyncs(controlStream);
        network->send(controlStream);

//        BitStream placementStream;
//        sendPlacementSyncs(placementStream);
//        network->send(placementStream);

        BitStream physicsStream;
        sendPhysicsSyncs(physicsStream);
        network->send(physicsStream);

        receiveSyncPacketsFromClients();
      } break;
      case settings::network::CLIENT: {
        BitStream controlStream;
        sendControlsSyncs(controlStream);
        network->send(controlStream, IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED);

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

  void NetSyncSystem::sendPlacementSyncs(BitStream &stream) {
    Time currentTime = GetTime();
    if (currentTime - lastPlacementSyncTime < 10) { return; }
    lastPlacementSyncTime = currentTime;

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
        serializePlacementSync(stream, id, true);
        ++actualWritten;
      }
    }
  }

  void NetSyncSystem::sendPhysicsSyncs(BitStream &stream) { // FIXME: numComps assumes that most objects are dynamic and active
    Time currentTime = GetTime();
    if (currentTime - lastPhysicsSyncTime < 10) { return; }
    lastPhysicsSyncTime = currentTime;

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
        serializePhysicsSync(stream, id, true);
        ++actualWritten;
      } else {
        ++numComps; // skip this component but still try to write as many as we originally intended
      }
    }
  }

  // TODO: copy the read/write of each component into some kind of single functions so that we don't have to match them.

  void NetSyncSystem::sendControlsSyncs(BitStream &stream) {
    stream.Write(keyControlMessageId);
    MouseControls *mouseControls;
    state->get_MouseControls(mouseControlId, &mouseControls);
    stream.Write(mouseControlId);
    stream.Write(mouseControls->yaw);
    stream.Write(mouseControls->pitch);
    stream.Write(mouseControls->invertedX);
    stream.Write(mouseControls->invertedY);
    stream.Write(keyControlId);
    switch(keyControlMessageId) { // FIXME: change all of these to only send the initial control stuff (accel, not force, etc.)
      case ID_SYNC_WALKCONTROLS: {
        PlayerControls *playerControls;
        state->get_PlayerControls(keyControlId, &playerControls);
        stream.Write(playerControls->accel);
        stream.Write(playerControls->jumpRequested);
        stream.Write(playerControls->jumpInProgress);
        stream.Write(playerControls->isGrounded);
        stream.Write(playerControls->isRunning);
      } break;
      case ID_SYNC_PYRAMIDCONTROLS: {
        PyramidControls *pyramidControls;
        state->get_PyramidControls(keyControlId, &pyramidControls);
        stream.Write(pyramidControls->accel);
        stream.Write(pyramidControls->turbo);
      } break;
      case ID_SYNC_TRACKCONTROLS: { // FIXME: Trackcontrols break the receiving end - pointers?
        TrackControls *trackControls;
        state->get_TrackControls(keyControlId, &trackControls);
        stream.Write(trackControls->control);
        stream.Write(trackControls->brakes);
        stream.Write(trackControls->flipRequested);
      } break;
      case ID_SYNC_FREECONTROLS: {
        FreeControls *freeControls;
        state->get_FreeControls(keyControlId, &freeControls);
        stream.Write(freeControls->control);
        stream.Write(freeControls->x10);
      } break;
      default: break;
    }
  }

  void NetSyncSystem::receiveSyncPacketsFromClients() { // TODO: also send server controls to clients for better sim
    for (auto pack : network->getSyncPackets()) {
      BitStream stream(pack->data, pack->length, false);
      MessageID syncType;
      stream.Read(syncType);

      switch (syncType) {
        case ID_SYNC_WALKCONTROLS:
        case ID_SYNC_PYRAMIDCONTROLS:
        case ID_SYNC_TRACKCONTROLS:
        case ID_SYNC_FREECONTROLS: {
          receiveControlSyncs(stream, syncType);
        } break;
        default: {
          fprintf(stderr, "Client received bad sync packet!\n");
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
      switch (syncType) {
        case ID_SYNC_PLACEMENT: {
          receivePlacementSyncs(stream);
        } break;
        case ID_SYNC_PHYSICS: {
          receivePhysicsSyncs(stream);
        } break;
        case ID_SYNC_WALKCONTROLS:
        case ID_SYNC_PYRAMIDCONTROLS:
        case ID_SYNC_TRACKCONTROLS:
        case ID_SYNC_FREECONTROLS: {
          receiveControlSyncs(stream, syncType);
        } break;
        default: {
          fprintf(stderr, "Client received bad sync packet!\n");
        } break;
      }
    }
    network->discardSyncPackets();
  }

  void NetSyncSystem::receivePlacementSyncs(BitStream &stream) {
    uint16_t numComps;
    stream.Read(numComps);
    for (uint16_t i = 0; i < numComps; ++i) {
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

  void NetSyncSystem::receivePhysicsSyncs(BitStream &stream) {
    uint16_t numComps;
    stream.Read(numComps);
    for (uint16_t i = 0; i < numComps; ++i) {
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

  void NetSyncSystem::receiveControlSyncs(SLNet::BitStream &stream, MessageID syncType) {
    entityId mouseId;
    stream.Read(mouseId);
    MouseControls *mouseControls;
    state->get_MouseControls(mouseId, &mouseControls);
    stream.Read(mouseControls->yaw);
    stream.Read(mouseControls->pitch);
    stream.Read(mouseControls->invertedX);
    stream.Read(mouseControls->invertedY);
    entityId controlId;
    stream.Read(controlId);
    switch (syncType) {
      case ID_SYNC_WALKCONTROLS: {
        PlayerControls *playerControls;
        state->get_PlayerControls(controlId, &playerControls);
        stream.Read(playerControls->accel);
        stream.Read(playerControls->jumpRequested);
        stream.Read(playerControls->jumpInProgress);
        stream.Read(playerControls->isGrounded);
        stream.Read(playerControls->isRunning);
      } break;
      case ID_SYNC_PYRAMIDCONTROLS: {
        PyramidControls *pyramidControls;
        state->get_PyramidControls(controlId, &pyramidControls);
        stream.Read(pyramidControls->accel);
        stream.Read(pyramidControls->turbo);
      } break;
      case ID_SYNC_TRACKCONTROLS: {
        TrackControls *trackControls;
        state->get_TrackControls(controlId, &trackControls);
        stream.Read(trackControls->control);
        stream.Read(trackControls->brakes);
        stream.Read(trackControls->flipRequested);
        trackControls->hasDriver = true; // FIXME: this permanently sets it to true - find way to set to false
      } break;
      case ID_SYNC_FREECONTROLS: {
        FreeControls *freeControls;
        state->get_FreeControls(controlId, &freeControls);
        stream.Read(freeControls->control);
        stream.Read(freeControls->x10);
      } break;
      default: break;
    }
  }

  void NetSyncSystem::serializePlacementSync(SLNet::BitStream &stream, entityId id, bool rw) {
    Placement *placement;
    state->get_Placement(id, &placement);
    stream.Serialize(rw, id);
    stream.Serialize(rw, placement->mat);
  }

  void NetSyncSystem::serializePhysicsSync(SLNet::BitStream &stream, entityId id, bool rw) {
    Physics *physics;
    state->get_Physics(id, &physics);
    stream.Serialize(rw, id);
    stream.Serialize(rw, physics->rigidBody->getLinearVelocity());
    btTransform transform;
    physics->rigidBody->getMotionState()->getWorldTransform(transform);
    stream.Serialize(rw, transform);
  }

  void NetSyncSystem::serializeControlSync(SLNet::BitStream &stream, entityId id, bool rw, MessageID syncType) {

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
