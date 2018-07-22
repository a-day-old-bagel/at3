
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
        writeControlSyncs();
        send(IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0);  // TODO: Is immediate priority even helpful here? Or bad?
        if (writePhysicsSyncs(dt)) {
          send(LOW_PRIORITY, UNRELIABLE_SEQUENCED, 1);
        }
        receiveSyncPackets();
      } break;
      case settings::network::CLIENT: {
        writeControlSyncs();
        send(IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0);
        receiveSyncPackets();
      } break;
      default: break;
    }
  }

  void NetSyncSystem::send(PacketPriority priority, PacketReliability reliability, char channel) {
    network->send(outStream, priority, reliability, channel);
    outStream.Reset();
  }

  bool NetSyncSystem::writePhysicsSyncs(float dt) {
    timeAccumulator += dt;
    if (timeAccumulator < 0.1f) { return false; }
    timeAccumulator = 0;

    outStream.Write((MessageID)ID_SYNC_PHYSICS);
    serializePhysicsSync(true, outStream);
    return true;
  }

  void NetSyncSystem::writeControlSyncs() {
    outStream.Write(keyControlMessageId);
    outStream.Write(mouseControlId);
    serializeControlSync(true, outStream, mouseControlId, keyControlId, keyControlMessageId);
  }

  void NetSyncSystem::receiveSyncPackets() {
    for (auto pack : network->getSyncPackets()) {
      BitStream stream(pack->data, pack->length, false);
      MessageID syncType;
      stream.Read(syncType);
      switch (syncType) {
        case ID_SYNC_PHYSICS: {
          serializePhysicsSync(false, stream);
        } break;
        case ID_SYNC_WALKCONTROLS:
        case ID_SYNC_PYRAMIDCONTROLS:
        case ID_SYNC_TRACKCONTROLS:
        case ID_SYNC_FREECONTROLS: {
          entityId mouseId = 0;
          stream.Read(mouseId);
          serializeControlSync(false, stream, mouseId, 0, syncType);
        } break;
        default: {
          fprintf(stderr, "Client received bad sync packet!\n");
        } break;
      }
    }
    network->discardSyncPackets();
  }

  void NetSyncSystem::serializePhysicsSync(bool rw, SLNet::BitStream &stream) {
    for (auto id : registries[1].ids) {
      Physics *physics;
      state->get_Physics(id, &physics);
      bool include;
      if (rw) {
        include = physics->useCase != Physics::WHEEL &&
                  physics->useCase != Physics::STATIC_MESH &&
                  physics->rigidBody->isActive();
      }
      stream.SerializeCompressed(rw, include);
      if (include) {
        btVector3 pos, lin, ang;
        btQuaternion rot;
        btTransform transform;
        if (rw) {
          physics->rigidBody->getMotionState()->getWorldTransform(transform);
          pos = transform.getOrigin();
          rot = transform.getRotation();
          lin = physics->rigidBody->getLinearVelocity();
          ang = physics->rigidBody->getAngularVelocity();
        }
        stream.Serialize(rw, pos);
        stream.Serialize(rw, rot);
        stream.Serialize(rw, lin);
        stream.Serialize(rw, ang);
        if ( ! rw) {
          transform.setOrigin(pos);
          transform.setRotation(rot);
          physics->rigidBody->setCenterOfMassTransform(transform);
          physics->rigidBody->setLinearVelocity(lin);
          physics->rigidBody->setAngularVelocity(ang);
        }
      }
    }
  }

  void NetSyncSystem::serializeControlSync(bool rw, SLNet::BitStream &stream, entityId mId, entityId cId, MessageID syncType) {
    MouseControls *mouseControls;
    state->get_MouseControls(mId, &mouseControls);
    stream.Serialize(rw, mouseControls->yaw);
    stream.Serialize(rw, mouseControls->pitch);
    stream.SerializeCompressed(rw, mouseControls->invertedX);
    stream.SerializeCompressed(rw, mouseControls->invertedY);
    stream.Serialize(rw, cId);
    switch (syncType) {
      case ID_SYNC_WALKCONTROLS: {
        PlayerControls *playerControls;
        state->get_PlayerControls(cId, &playerControls);
        stream.Serialize(rw, playerControls->accel);
        stream.SerializeCompressed(rw, playerControls->jumpRequested);
        stream.SerializeCompressed(rw, playerControls->jumpInProgress);
        stream.SerializeCompressed(rw, playerControls->isGrounded);
        stream.SerializeCompressed(rw, playerControls->isRunning);
      } break;
      case ID_SYNC_PYRAMIDCONTROLS: {
        PyramidControls *pyramidControls;
        state->get_PyramidControls(cId, &pyramidControls);
        stream.Serialize(rw, pyramidControls->accel);
        stream.SerializeCompressed(rw, pyramidControls->turbo);
      } break;
      case ID_SYNC_TRACKCONTROLS: {
        TrackControls *trackControls;
        state->get_TrackControls(cId, &trackControls);
        stream.Serialize(rw, trackControls->control);
        stream.Serialize(rw, trackControls->brakes);
        stream.SerializeCompressed(rw, trackControls->flipRequested);
        trackControls->hasDriver = true; // FIXME: this permanently sets it to true - find way to set to false
      } break;
      case ID_SYNC_FREECONTROLS: {
        FreeControls *freeControls;
        state->get_FreeControls(cId, &freeControls);
        stream.Serialize(rw, freeControls->control);
        stream.SerializeBitsFromIntegerRange(rw, freeControls->x10, 0, 3);
      } break;
      default: break;
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
