
#include <algorithm>
#include "network.hpp"
#include "playerSet.hpp"

using namespace SLNet;
using namespace rtu;

namespace at3 {

  NetworkSystem::NetworkSystem(State *state) : System(state) {
    name = "Net Sync System";
    // Static subscriptions will only apply to the first instance of this class created. But usually only one exists.
    RTU_STATIC_SUB(setNetInterfaceSub, "set_network_interface", NetworkSystem::setNetInterface, this);
    RTU_STATIC_SUB(setEcsInterfaceSub, "set_ecs_interface", NetworkSystem::setEcsInterface, this);
  }

  bool NetworkSystem::onInit() {
    registries[1].discoverHandler = RTU_MTHD_DLGT(&NetworkSystem::onDiscoverNetworkedPhysics, this);
    RTU_STATIC_SUB(switchToMouseCtrlSub, "switch_to_mouse_controls", NetworkSystem::switchToMouseCtrl, this);
    RTU_STATIC_SUB(switchToWalkCtrlSub, "switch_to_walking_controls", NetworkSystem::switchToWalkCtrl, this);
    RTU_STATIC_SUB(switchToPyramidCtrlSub, "switch_to_pyramid_controls", NetworkSystem::switchToPyramidCtrl, this);
    RTU_STATIC_SUB(switchToTrackCtrlSub, "switch_to_track_controls", NetworkSystem::switchToTrackCtrl, this);
    RTU_STATIC_SUB(switchToFreeCtrlSub, "switch_to_free_controls", NetworkSystem::switchToFreeCtrl, this);
    return true;
  }

  void NetworkSystem::onTick(float dt) {

    receiveAdministrativePackets();

    // switch (network->getRole()) {
    //   case settings::network::SERVER: {
    //     handleNewClients();
    //     receiveAdministrativePackets();
    //   } break;
    //   case settings::network::CLIENT: {
    //     receiveAdministrativePackets();
    //   } break;
    //   default: return;
    // }
    // if (ecs->physicsHistory.findTheTruth()) {
    //   // TODO: send truth update to clients
    // }
  }

  void NetworkSystem::send(PacketPriority priority, PacketReliability reliability, char channel) {
    network->send(outStream, priority, reliability, channel);
    outStream.Reset();
  }

  void NetworkSystem::sendTo(const AddressOrGUID &target, PacketPriority priority,
                             PacketReliability reliability, char channel) {
    network->sendTo(outStream, target, priority, reliability, channel);
    outStream.Reset();
  }

//  bool NetworkSystem::writePhysicsSyncs(float dt) {
//    timeAccumulator += dt;
//    if (timeAccumulator < 0.1f) { return false; }
//    timeAccumulator = 0;
//
//    outStream.Write((MessageID) ID_SYNC_PHYSICS);
//    serializePhysicsSync(true, outStream);
//    return true;
//  }

  bool NetworkSystem::sendPeriodicPhysicsUpdate() {
    // Only send physics state update every 6 steps, which is 10 times per second if physics framerate is 60 fps.
    if (++physicsStepAccumulator < 6) { return false; }
    physicsStepAccumulator = 0;

    outStream.Write((MessageID) ID_SYNC_PHYSICS);
    outStream.WriteBitsFromIntegerRange(ecs->physicsHistory.getLatestIndex(), (uint8_t)0,
                                        (uint8_t)(ezecs::Physics::maxStoredStates - 1));
    outStream.Write(ecs->physicsHistory.getInputChecksum(ecs->physicsHistory.getLatestIndex()));
    serializePhysicsSync(true, outStream);
    send(LOW_PRIORITY, UNRELIABLE_SEQUENCED, CH_PHYSICS_SYNC);
    return true;
  }

  bool NetworkSystem::writeControlSync() {
    // TODO: check for physics base index - NO, just send base index with initial state so never without
    if ( ! mouseControlId) { // In case no controls are currently assigned (no type of control *doesn't* use the mouse)
      return false;
    }
    switch (network->getRole()) {
      case settings::network::SERVER: {
        inputs.emplace_back();
        serializeControlSyncShortType(true, inputs.back().data, keyControlMessageId);
        inputs.back().data.Write(mouseControlId);
        inputs.back().data.WriteBitsFromIntegerRange(ecs->physicsHistory.getLatestIndex(), (uint8_t)0,
                                                     (uint8_t)(ezecs::Physics::maxStoredStates - 1));
        serializeControlSync(true, inputs.back().data, mouseControlId, keyControlId, keyControlMessageId);

        // ecs->physicsHistory.addToInput(inputs.back().data, ecs->physicsHistory.getLatestIndex(), SLNet::RakNetGUID(1));
        ecs->physicsHistory.addToInput(inputs.back().data, ecs->physicsHistory.getLatestIndex());
        ecs->physicsHistory.addToInputChecksum(ecs->physicsHistory.getLatestIndex(), 1); // 1 signifies server input

        inputs.back().data.SetReadOffset(0);

      } break;
      case settings::network::CLIENT: {
        outStream.Write(keyControlMessageId);
        outStream.Write(mouseControlId);
        outStream.WriteBitsFromIntegerRange(ecs->physicsHistory.getLatestIndex(), (uint8_t)0,
                                            (uint8_t)(ezecs::Physics::maxStoredStates - 1));
        serializeControlSync(true, outStream, mouseControlId, keyControlId, keyControlMessageId);

        printf("\nSending control type %u at index %u\n", keyControlMessageId, ecs->physicsHistory.getLatestIndex());
      } break;
      default: break;
    }
    return true;
  }

  // FIXME: TODO: use a hashmap of guid->stream so that duplicates don't get sent out
  void NetworkSystem::sendAllControlSyncs() {
    // FIXME: TODO: send to all clients, not just the senders, also, do this inside the new classes or whatever.
//    for (auto & receiveingClient : inputs) {
//      BitStream tempStream;
//      uint8_t numEntries = 0; // TODO: use robust type (templates in new classes or whatever)
//      for (auto & inputClient : inputs) {
//        if ( ! (receiveingClient.id == inputClient.id)) {
//          BitSize_t head = inputClient.data.GetReadOffset();
//          tempStream.Write(inputClient.data);
//          inputClient.data.SetReadOffset(head);
//          ++numEntries;
//        }
//      }
//      outStream.Write((MessageID) ID_SYNC_ALL_CONTROLS);
//      outStream.Write(numEntries);
//      outStream.Write(tempStream);
//      sendTo(receiveingClient.id, HIGH_PRIORITY, RELIABLE_ORDERED, CH_CONTROL_SYNC);
//    }
//    inputs.clear();


    BitStream dataStream, checksumStream;
    std::unordered_map<uint8_t, uint32_t> indexedChecksums;
    uint8_t numEntries = 0; // TODO: use robust type (templates in new classes or whatever)
    uint8_t numIndices = 0;
    for (auto & input : inputs) {
      indexedChecksums[input.index] += RakNetGUID::ToUint32(input.id.rakNetGuid);
      BitSize_t head = input.data.GetReadOffset();
      dataStream.Write(input.data);
      input.data.SetReadOffset(head);
      ++numEntries;
    }
    numIndices = (uint8_t)indexedChecksums.size();
    for (auto & index : indexedChecksums) {
      checksumStream.WriteBitsFromIntegerRange(index.first, (uint8_t)0, (uint8_t)(ezecs::Physics::maxStoredStates - 1));
      checksumStream.Write(index.second);
    }

    outStream.Write((MessageID) ID_SYNC_MULTIPLE_CONTROLS);
    outStream.Write(numEntries);
    outStream.Write(numIndices);

    outStream.Write(checksumStream);
    outStream.Write(dataStream);

    send(HIGH_PRIORITY, RELIABLE_ORDERED, CH_CONTROL_SYNC);
    inputs.clear();


  }

  void NetworkSystem::receiveAdministrativePackets() {
    for (auto pack : network->getRequestPackets()) {
      BitStream stream(pack->data, pack->length, false);
      MessageID reqType;
      stream.Read(reqType);
      switch (reqType) {
        case ID_USER_PACKET_ECS_REQUEST_ENUM: {
          uint8_t request;
          stream.ReadBitsFromIntegerRange(request, (uint8_t) 0, (uint8_t) (REQ_END_ENUM - 1), false);
          switch (request) {
            case REQ_ENTITY_OP: {
              respondToEntityRequest(stream);
            } break;
            case REQ_COMPONENT_OP: {

            } break;
            default:
              break;
          }
        } break;
        case ID_USER_PACKET_ECS_RESPONSE_ENUM: {

        } break;
        case ID_USER_PACKET_ADMIN_COMMAND: {
          uint8_t command;
          stream.ReadBitsFromIntegerRange(command, (uint8_t) 0, (uint8_t) (CMD_END_ENUM - 1), false);
          switch (command) {
            case CMD_ASSIGN_PLAYER_ID: {
              fprintf(stderr, "poop POOP\n");
              serializePlayerAssignment(false, stream, 0);
            } break;
            default:
              break;
          }
        } break;
        default: {
          fprintf(stderr, "Received bad request/response packet!\n");
        } break;
      }
    }
    network->discardRequestPackets();
  }

  bool NetworkSystem::receiveSyncPackets() {
    bool atLeastOneClientCtrl = false;
    for (auto pack : network->getSyncPackets()) {
      AddressOrGUID sender(pack->guid);
      BitStream stream(pack->data, pack->length, false);
      MessageID syncType;
      stream.Read(syncType);
      switch (syncType) {
        case ID_SYNC_PHYSICS: {
          if (network->getRole() == settings::network::CLIENT) { // Only clients should receive these

            // These should contain the current network-wide physics state index number, so receiving at least one of
            // these is required to proceed with normal execution. This only applies to clients.

            // TODO: just write stream to history. This should only be called when reading history.

            uint8_t index;
            stream.ReadBitsFromIntegerRange(index, (uint8_t)0, (uint8_t)(ezecs::Physics::maxStoredStates - 1));
            uint32_t inputChecksum;
            stream.Read(inputChecksum);
            ecs->physicsHistory.setInputChecksum(index, inputChecksum);
            ecs->physicsHistory.addToState(stream, index, true);


            printf("\nReceived Physics with input checksum %u = %u\n", index, inputChecksum);


//            serializePhysicsSync(false, stream);   // ACTUALLY APPLY IT
          }
        } break;
        case ID_SYNC_WALKCONTROLS:
        case ID_SYNC_PYRAMIDCONTROLS:
        case ID_SYNC_TRACKCONTROLS:
        case ID_SYNC_FREECONTROLS: {
          if (network->getRole() == settings::network::SERVER) { // Only servers should receive these
            atLeastOneClientCtrl = true;
            inputs.emplace_back();
            serializeControlSyncShortType(true, inputs.back().data, syncType);
            BitSize_t head = stream.GetReadOffset();
            inputs.back().data.Write(stream);
            inputs.back().id = sender;
            stream.SetReadOffset(head);

            entityId mouseId = 0;
            stream.Read(mouseId);

            uint8_t index;
            stream.ReadBitsFromIntegerRange(index, (uint8_t)0, (uint8_t)(Physics::maxStoredStates - 1));
            ecs->physicsHistory.addToInput(inputs.back().data, index, pack->guid);
            inputs.back().data.SetReadOffset(0);

            inputs.back().index = index;

            printf("\nReceived input to %u from %lu\n", index, RakNetGUID::ToUint32(pack->guid));

//            serializeControlSync(false, stream, mouseId, 0, syncType);  // ACTUALLY APPLY IT
          }
        } break;
        case ID_SYNC_MULTIPLE_CONTROLS: {
          if (network->getRole() == settings::network::CLIENT) { // Only clients should receive these

//            applyCombinedInputPacket(stream);  // ACTUALLY APPLY IT



            uint8_t numEntries;
            stream.Read(numEntries);
            uint8_t numIndices;
            stream.Read(numIndices);
            for (uint8_t i = 0; i < numIndices; ++i) {
              uint8_t index;
              stream.Read(index);
              uint32_t checksumPart;
              stream.Read(checksumPart);
              ecs->physicsHistory.addToInputChecksum(index, checksumPart);

              printf("\nRecieved input checksum %u += %u\n", index, checksumPart);
            }
            for (uint8_t i = 0; i < numEntries; ++i) {
              MessageID controlType;
              serializeControlSyncShortType(false, stream, controlType);
              entityId mouseId = 0;
              stream.Read(mouseId);
              uint8_t index;
              stream.ReadBitsFromIntegerRange(index, (uint8_t) 0, (uint8_t) (Physics::maxStoredStates - 1));
              ecs->physicsHistory.addToInput(stream, index);

              printf("\nRecieved multi-controls %u\n", index);
            }
          }
        } break;
        default: {
          fprintf(stderr, "Received bad sync packet! type was %u.\n", syncType);
        } break;
      }
    }
    network->discardSyncPackets();
    return atLeastOneClientCtrl;
  }

  void NetworkSystem::applyCombinedInputPacket(BitStream &stream) {
    uint8_t numEntries;
    stream.Read(numEntries);
    for (uint8_t i = 0; i < numEntries; ++i) {
      MessageID controlType;
      serializeControlSyncShortType(false, stream, controlType);
      entityId mouseId = 0;
      stream.Read(mouseId);

      uint8_t index;
      stream.ReadBitsFromIntegerRange(index, (uint8_t) 0, (uint8_t) (Physics::maxStoredStates - 1));

      serializeControlSync(false, stream, mouseId, 0, controlType);
//      printf("%u %u %u %u\n", mouseControlId, mouseId, controlType, ID_SYNC_FREECONTROLS);
    }
  }

  void NetworkSystem::handleNewClients() {
    if (!network->getFreshConnections().empty()) {
      // All existing entities sent here
      // TODO: revert to last complete snapshot before sending this, and then send that snapShot index
      // TODO: Maybe just use whatever's in the state currently like this instead and let the client correct itself.
      for (const auto &id : registries[0].ids) {
        writeEntityRequestHeader(outStream);
        serializeEntityCreationRequest(true, outStream, *state, id);
        send(LOW_PRIORITY, RELIABLE_ORDERED, CH_ECS_REQUEST);
      }
      // Any new player and avatar components sent here, along with the player ID assignments
      for (const auto &client : network->getFreshConnections()) {
        outStream.Write((MessageID) ID_USER_PACKET_ADMIN_COMMAND);
        outStream.WriteBitsFromIntegerRange((uint8_t) CMD_ASSIGN_PLAYER_ID, (uint8_t) 0,
                                            (uint8_t) (CMD_END_ENUM - 1));
        serializePlayerAssignment(true, outStream, PlayerSet::create(*state, *ecs, network->getRole()));
        sendTo(client, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_ECS_REQUEST);
      }
      network->discardFreshConnections();
    }
  }

  void NetworkSystem::respondToEntityRequest(BitStream &stream) {
    uint8_t operation;
    stream.ReadBitsFromIntegerRange(operation, (uint8_t) 0, (uint8_t) (OP_END_ENUM - 1), false);
    switch (operation) {
      case OP_CREATE: {
        serializeEntityCreationRequest(false, stream, *state);
//        if (network->getRole() == settings::network::SERVER) {
//          // Recreate the correct header on a new copy of the bitstream, since resetting the read head doesn't work FSR.
//          BitStream fullStream;
//          writeEntityRequestHeader(fullStream);
//          fullStream.Write(stream);
//          network->send(fullStream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_ECS_REQUEST);
//        }
        switch (network->getRole()) {
          case settings::network::SERVER: {
            stream.SetReadOffset(0);
            network->send(stream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_ECS_REQUEST);
          } break;
          case settings::network::CLIENT: { } break;
          default: break;
        }
      } break;
      case OP_DESTROY: {
        serializeEntityDeletionRequest(false, stream, *state);
        switch (network->getRole()) {
          case settings::network::SERVER: {
            stream.SetReadOffset(0);
            network->send(stream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_ECS_REQUEST);
          } break;
          case settings::network::CLIENT: { } break;
          default: break;
        }
      } break;
      default:
        break;
    }
  }

  void NetworkSystem::respondToComponentRequest(BitStream &stream) {

  }

  void NetworkSystem::serializePhysicsSync(bool rw, BitStream &stream, bool includeAll) {
    // FIXME: Don't do this by iterating over IDs!
    // Include some specifier of ID range or something instead, so that this doesn't cause problems.
    for (auto id : registries[1].ids) {
      Physics *physics;
      state->getPhysics(id, &physics);

      bool include;
      if (rw) {
        // never write info for wheels or static meshes
        include = physics->useCase != Physics::WHEEL &&
                  //                  physics->useCase != Physics::STATIC_MESH &&
                  //                  physics->rigidBody->isActive();
                  physics->useCase != Physics::STATIC_MESH;
        if ( ! includeAll) { // if not saving entire physics state
          // FIXME: Why on earth is 'include &= physics->rigidBody->isActive()' crashing while this version works?
          include = include && physics->rigidBody->isActive(); // object must be active to be written
        }
      } // else it's read from the stream
      stream.SerializeCompressed(rw, include);

//      // If includeAll is true, data is still written for an object that isn't included in networked physics messages.
//      // This is useful for keeping a complete physics state history but also remembering which ones to network.
//      bool saveStateInclude = false;
//      if (includeAll) {
//        saveStateInclude = physics->useCase != Physics::WHEEL && physics->useCase != Physics::STATIC_MESH;
//      }

//      if (netInclude || saveStateInclude) {

      if (include) {

        bool needsRotation;
        if (rw) {
          needsRotation = physics->useCase != Physics::CHARA && physics->useCase != Physics::SPHERE;
        } // else it's written to by the stream
        stream.SerializeCompressed(rw, needsRotation);

        glm::vec3 pos, lin, ang;  // glm conversion because bullet uses four floats for its vec3's for *SOME* reason.
        btQuaternion rot;
        btTransform transform;
        if (rw) {
          physics->rigidBody->getMotionState()->getWorldTransform(transform);
          pos = bulletToGlm(transform.getOrigin());
          lin = bulletToGlm(physics->rigidBody->getLinearVelocity());
          if (needsRotation) {
            rot = transform.getRotation();
            ang = bulletToGlm(physics->rigidBody->getAngularVelocity());
          }
        }
        stream.Serialize(rw, pos);
        stream.Serialize(rw, lin);
        if (needsRotation) {
          stream.Serialize(rw, rot);
          stream.Serialize(rw, ang);
        }
        if (!rw) {
          if (needsRotation) {
            transform.setRotation(rot);
            physics->rigidBody->setAngularVelocity(glmToBullet(ang));
          } else {
            physics->rigidBody->getMotionState()->getWorldTransform(transform);
          }
          RTU_STATIC_SUB(strictWarpToggler, "key_down_f1", NetworkSystem::toggleStrictWarp, this);
          if (strictWarp) { // just hard warp to position
            transform.setOrigin(glmToBullet(pos));
          } else { // "Smart" interpolate between current position and networked truth position
            Networking *networking;
            state->getNetworking(id, &networking);
            ((NetworkedPhysicsData *) networking->nonpersistentCustomData.get())->truthPos = glmToBullet(pos);
            btTransform localTrans;
            physics->rigidBody->getMotionState()->getWorldTransform(localTrans);
            glm::vec3 currentPos = bulletToGlm(localTrans.getOrigin());
            glm::vec3 correction = (pos - bulletToGlm(localTrans.getOrigin()));
            float correctionStiffness = 2.f;
            if (glm::length(correction) < 5.f) { // if it's just a little bit off
              transform.setOrigin(localTrans.getOrigin()); // keep local position
              lin += correction * correctionStiffness;  // only adjust using velocity
            } else { // Just hard warp if too distant
              transform.setOrigin(glmToBullet(pos));
            }
          }
          physics->rigidBody->setCenterOfMassTransform(transform);
          physics->rigidBody->setLinearVelocity(glmToBullet(lin));
        }
      }
    }
  }

  void NetworkSystem::serializeControlSync(bool rw, BitStream &stream, entityId mId, entityId cId,
                                           MessageID syncType) {
    MouseControls *mouseControls;
    state->getMouseControls(mId, &mouseControls);
    if (!mouseControls) { return; } // a client may not yet have the component being referred to.
    stream.Serialize(rw, mouseControls->yaw);
    stream.Serialize(rw, mouseControls->pitch);
    stream.SerializeCompressed(rw, mouseControls->invertedX);
    stream.SerializeCompressed(rw, mouseControls->invertedY);
    stream.Serialize(rw, cId);
    switch (syncType) {
      case ID_SYNC_WALKCONTROLS: {
        WalkControls *walkControls;
        state->getWalkControls(cId, &walkControls);
        if (!walkControls) { return; } // a client may not yet have the component being referred to.
        stream.Serialize(rw, walkControls->accel);
        stream.SerializeCompressed(rw, walkControls->jumpRequested);
        stream.SerializeCompressed(rw, walkControls->jumpInProgress);
        stream.SerializeCompressed(rw, walkControls->isGrounded);
        stream.SerializeCompressed(rw, walkControls->isRunning);
      } break;
      case ID_SYNC_PYRAMIDCONTROLS: {
        PyramidControls *pyramidControls;
        state->getPyramidControls(cId, &pyramidControls);
        if (!pyramidControls) { return; } // a client may not yet have the component being referred to.
        stream.Serialize(rw, pyramidControls->accel);
        stream.SerializeCompressed(rw, pyramidControls->turbo);
        stream.SerializeCompressed(rw, pyramidControls->shoot);
        stream.SerializeCompressed(rw, pyramidControls->drop);
        stream.SerializeCompressed(rw, pyramidControls->pop);
      } break;
      case ID_SYNC_TRACKCONTROLS: {
        TrackControls *trackControls;
        state->getTrackControls(cId, &trackControls);
        if (!trackControls) { return; } // a client may not yet have the component being referred to.
        stream.Serialize(rw, trackControls->control);
        stream.Serialize(rw, trackControls->brakes);
        stream.SerializeCompressed(rw, trackControls->flipRequested);
        trackControls->hasDriver = true; // FIXME: this permanently sets it to true - find way to set to false
      } break;
      case ID_SYNC_FREECONTROLS: {
        // Don't bother syncing these - they're not tied to physics anyway, making the sync difficult
      } break;
      default:
        break;
    }
  }

  bool NetworkSystem::serializeControlSyncShortType(bool rw, BitStream &stream, MessageID &type) {
    return stream.SerializeBitsFromIntegerRange(rw, type,
                                                (MessageID)ID_SYNC_WALKCONTROLS,
                                                (MessageID)ID_SYNC_FREECONTROLS);
  }

  void NetworkSystem::serializePlayerAssignment(bool rw, BitStream &stream, entityId playerId) {
    stream.Serialize(rw, playerId);
    if (rw) {
      outStream.Write(ecs->physicsHistory.getLatestIndex());
      outStream.Write(ecs->physicsHistory.getInputChecksum(ecs->physicsHistory.getLatestIndex()));
    } else {
      uint8_t index;
      stream.Read(index);
      uint32_t inputChecksum;
      stream.Read(inputChecksum);
      ecs->physicsHistory.setEmptyIndex(index);
      ecs->physicsHistory.appendIndex();
      ecs->physicsHistory.setInputChecksum(index, inputChecksum);
      topics::publish<entityId>("set_player_id", playerId);
      initialStep = true;
    }
  }

  void NetworkSystem::setNetInterface(void *netInterface) {
    network = *(std::shared_ptr<NetInterface> *) netInterface;
  }

  void NetworkSystem::setEcsInterface(void *ecs) {
    this->ecs = *(std::shared_ptr<EntityComponentSystemInterface> *) ecs;
  }

  void NetworkSystem::switchToMouseCtrl(void *id) {
    mouseControlId = *(entityId *) id;
  }

  void NetworkSystem::switchToWalkCtrl(void *id) {
    keyControlId = *(entityId *) id;
    keyControlMessageId = ID_SYNC_WALKCONTROLS;
  }

  void NetworkSystem::switchToPyramidCtrl(void *id) {
    keyControlId = *(entityId *) id;
    keyControlMessageId = ID_SYNC_PYRAMIDCONTROLS;
  }

  void NetworkSystem::switchToTrackCtrl(void *id) {
    keyControlId = *(entityId *) id;
    keyControlMessageId = ID_SYNC_TRACKCONTROLS;
  }

  void NetworkSystem::switchToFreeCtrl(void *id) {
    keyControlId = *(entityId *) id;
    keyControlMessageId = ID_SYNC_FREECONTROLS;
  }

  bool NetworkSystem::onDiscoverNetworkedPhysics(const entityId &id) {
    Networking *networking;
    state->getNetworking(id, &networking);
    networking->nonpersistentCustomData = std::make_shared<NetworkedPhysicsData>();
    return true;
  }

  void NetworkSystem::toggleStrictWarp() {
    strictWarp = !strictWarp;
    printf("Strict Warp %s\n", strictWarp ? "On" : "Off");
  }

  void NetworkSystem::onBeforePhysicsStep() {

    if (network->getRole() == settings::network::NONE) {
      return;
    }

    // if (ecs->physicsHistory.isReplaying() || ecs->physicsHistory.findTheTruth()) {
    //
    //   // TODO: load next replay state
    //   ecs->physicsHistory._turnOffReplay();
    //
    // } else {
    { ecs->physicsHistory.findTheTruth();

      if (ecs->physicsHistory.isFull()) {
        uint8_t tail = ecs->physicsHistory.getLatestCompleteIndex();
        std::vector<RakNetGUID> slowClients = ecs->physicsHistory.getUnfulfilledInputs(tail);
        for (const auto &client : slowClients) {
          fprintf(stderr, "\nSlow Client: %lu\n", RakNetGUID::ToUint32(client));
        }
      }

      BitStream physics;
      serializePhysicsSync(true, physics, true); // copy of entire local state, but not authorized truth state

      uint8_t newStateIndex;
      if (initialStep) {
        newStateIndex = ecs->physicsHistory.getLatestIndex();
        initialStep = false;
      } else {
        newStateIndex = ecs->physicsHistory.appendIndex();
      }

      switch (network->getRole()) {
        case settings::network::SERVER: {

          handleNewClients();

          ecs->physicsHistory.setInputSources(network->getClientGuids(), newStateIndex);
          ecs->physicsHistory.setAdditionalInputChecksum(newStateIndex, 1);

          if (writeControlSync()) { // Write server's own controls plus any client controls received
            receiveSyncPackets();
            sendAllControlSyncs();
          } else if (receiveSyncPackets()) { // Write any client controls received anyway
            sendAllControlSyncs();
          }

          // local state, authorized by server, but still not complete until all inputs have been received and applied.
          ecs->physicsHistory.addToState(physics, newStateIndex, true);
        } break;
        case settings::network::CLIENT: {

          // TODO: get desired input checksum from server

          if (writeControlSync()) {
            send(IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_CONTROL_SYNC);
            printf("sent\n");
          }
          receiveSyncPackets();

          // local state, not authorized by server, and not input-complete.
          ecs->physicsHistory.addToState(physics, newStateIndex);
        } break;
        default: break;
      }

      // TODO: find where this should really go. Maybe even after the step?
      ecs->physicsHistory.setTime(newStateIndex, SDL_GetTicks());

    }

  }

  void NetworkSystem::onAfterPhysicsStep() {

    if (network->getRole() == settings::network::NONE) {
      return;
    }

    if (network->getRole() == settings::network::SERVER) {
      sendPeriodicPhysicsUpdate(); // TODO: get rid of this and the function once history is working
    }

//    BitStream physics;
//    serializePhysicsSync(true, physics, true); // complete state copy
//
//    switch (network->getRole()) {
//      case settings::network::SERVER: {
//        ecs->physicsHistory.addToState(physics, ecs->physicsHistory.getLatestIndex(), true);
//        sendPeriodicPhysicsUpdate(); // TODO: get rid of this and the function
//      } break;
//      case settings::network::CLIENT: {
//        ecs->physicsHistory.addToState(physics, ecs->physicsHistory.getLatestIndex());
//      } break;
//      default:
//        break;
//    }


//    physicsStates.emplace();
//    physicsStates.back().WriteBitsFromIntegerRange(storedStateIndexCounter++, (uint8_t) 0, maxStoredStates);
//    serializePhysicsSync(true, physicsStates.back(), true);
//    if (physicsStates.size() > maxStoredStates) {
//      physicsStates.pop();
//    }
  }

  void NetworkSystem::rewindPhysics() {
//    uint8_t storedStateIndex;
//    physicsStates.front().ReadBitsFromIntegerRange(storedStateIndex, (uint8_t) 0, maxStoredStates);
//    serializePhysicsSync(false, physicsStates.front(), true);
  }

}
