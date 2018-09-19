

#include <iostream>




#include <algorithm>
#include "network.hpp"
#include "playerSet.hpp"

using namespace SLNet;
using namespace rtu;

// How the physics and multi-control packets are sent from server (they use the same channel)
#define SERVER_SIM_SYNC_PACKET_SEQUENCING RELIABLE_ORDERED // UNRELIABLE_SEQUENCED

namespace at3 {

  static uint32_t theThing = 0;

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
    outStream.WriteBitsFromIntegerRange(ecs->physicsHistory.getNewestIndex(), (uint8_t)0,
                                        (uint8_t)(Physics::maxStoredStates - 1));
    // outStream.Write(ecs->physicsHistory.getInputChecksum(ecs->physicsHistory.getLatestIndex()));
    serializePhysicsSync(true, outStream);
    send(LOW_PRIORITY, SERVER_SIM_SYNC_PACKET_SEQUENCING, CH_SIMULATION_UPDATE);
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
        inputs.back().data.WriteBitsFromIntegerRange(ecs->physicsHistory.getNewestIndex(), (uint8_t)0,
                                                     (uint8_t)(Physics::maxStoredStates - 1));
        serializeControlSync(true, inputs.back().data, mouseControlId, keyControlId, keyControlMessageId);

        inputs.back().id = SLNet::RakNetGUID(1); // 1 is server id
        inputs.back().index = ecs->physicsHistory.getNewestIndex();

        // ecs->physicsHistory.addToInput(inputs.back().data, ecs->physicsHistory.getLatestIndex(), SLNet::RakNetGUID(1));
        ecs->physicsHistory.addToInput(inputs.back().data, inputs.back().index);
        ecs->physicsHistory.addToInputChecksum(ecs->physicsHistory.getNewestIndex(), theThing); // 1 signifies server input
        if (theThing) { theThing = 0; }

        printf("\nServer input %u with mouse %u at %u (%u)\n", keyControlMessageId, mouseControlId,
               ecs->physicsHistory.getNewestIndex(), inputs.back().data.GetNumberOfBitsUsed());

        // inputs.back().data.SetReadOffset(0);

      } break;
      case settings::network::CLIENT: {
        // outStream.Write(keyControlMessageId);
        // outStream.Write(mouseControlId);
        // outStream.WriteBitsFromIntegerRange(ecs->physicsHistory.getLatestIndex(), (uint8_t)0,
        //                                     (uint8_t)(Physics::maxStoredStates - 1));
        // serializeControlSync(true, outStream, mouseControlId, keyControlId, keyControlMessageId);

        BitStream commonStream, historyStream;
        commonStream.Write(mouseControlId);
        commonStream.WriteBitsFromIntegerRange(ecs->physicsHistory.getNewestIndex(), (uint8_t)0,
                                            (uint8_t)(Physics::maxStoredStates - 1));
        serializeControlSync(true, commonStream, mouseControlId, keyControlId, keyControlMessageId);

        serializeControlSyncShortType(true, historyStream, keyControlMessageId);
        historyStream.Write(commonStream);
        ecs->physicsHistory.addToInput(historyStream, ecs->physicsHistory.getNewestIndex());

        outStream.Write(keyControlMessageId);
        commonStream.SetReadOffset(0);
        outStream.Write(commonStream);

        printf("\nSending type %u with mouse %u at %u (%u)\n", keyControlMessageId, mouseControlId,
               ecs->physicsHistory.getNewestIndex(), outStream.GetNumberOfBitsUsed());
      } break;
      default: break;
    }
    return true;
  }

  void NetworkSystem::sendAllControlSyncs() {

    // TODO: READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS
    // TODO: Maybe clients don't need the checksums at all, and can just collapse to truth whenever a physics update comes, while using whatever inputs they *do* have to rebuild the current state.





    // TODO: READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS READ THIS
    // TODO: Replace entityId's with playerId's ranging from 0-127 or whatever and keep a mapping of those id's to Player components
    // TODO: These player components will contain all the necessary id's for avatar control.
    // TODO: also replace control innards with boolean representations.





    printf("\nSending multi-controls:\n");

    BitStream dataStream;//, checksumStream;
    // std::unordered_map<uint8_t, uint32_t> indexedChecksums;
    uint8_t numEntries = 0; // TODO: use robust type (templates in new classes or whatever)
    // uint8_t numIndices = 0;
    for (auto & input : inputs) {

      printf("from %lu at %u (%u)\n", RakNetGUID::ToUint32(input.id.rakNetGuid), input.index, input.data.GetNumberOfBitsUsed());

      // indexedChecksums[input.index] += RakNetGUID::ToUint32(input.id.rakNetGuid);
      // BitSize_t head = input.data.GetReadOffset(); // pointless
      dataStream.Write(input.data);
      // input.data.SetReadOffset(head); // pointless
      ++numEntries;
    }
    // numIndices = (uint8_t)indexedChecksums.size();
    // for (auto & index : indexedChecksums) {
    //   checksumStream.WriteBitsFromIntegerRange(index.first, (uint8_t)0, (uint8_t)(Physics::maxStoredStates - 1));
    //   checksumStream.Write(index.second);
    // }

    outStream.Write((MessageID) ID_SYNC_MULTIPLE_CONTROLS);
    outStream.Write(numEntries);
    // outStream.Write(numIndices);

    // outStream.Write(checksumStream);
    outStream.Write(dataStream);

    send(HIGH_PRIORITY, SERVER_SIM_SYNC_PACKET_SEQUENCING, CH_SIMULATION_UPDATE);
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
              printf("received player id\n");
              serializePlayerAssignment(false, stream, 0);
              network->discardSyncPackets();
            } break;
            case CMD_WORLD_INIT: {
              printf("received world init\n");
              outStream.Write((MessageID) ID_USER_PACKET_CLIENT_READY);
              send(IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_ECS_UPDATE);
              // network->discardSyncPackets();
            } break;
            default:
              break;
          }
        } break;
        case ID_USER_PACKET_CLIENT_READY: {
          printf("received client ready\n");
          if (freshPlayers.empty()) {
            fprintf(stderr, "No new fresh player avatar sets available for client %lu\n",
                RakNetGUID::ToUint32(pack->guid));
          } else {
            outStream.Write((MessageID) ID_USER_PACKET_ADMIN_COMMAND);
            outStream.WriteBitsFromIntegerRange((uint8_t) CMD_ASSIGN_PLAYER_ID, (uint8_t) 0,
                                                (uint8_t) (CMD_END_ENUM - 1));
            serializePlayerAssignment(true, outStream, freshPlayers.top());
            freshPlayers.pop();
            sendTo(AddressOrGUID(pack->guid), IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_ECS_UPDATE);
            uint32_t indexOfIgnorance = ignoredInputSources.GetIndexOf(pack->guid);
            if (indexOfIgnorance != MAX_UNSIGNED_LONG) {
              ignoredInputSources.RemoveAtIndexFast(indexOfIgnorance);
            }
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
            stream.ReadBitsFromIntegerRange(index, (uint8_t)0, (uint8_t)(Physics::maxStoredStates - 1));
            // uint32_t inputChecksum;
            // stream.Read(inputChecksum);
            // ecs->physicsHistory.setInputChecksum(index, inputChecksum);
            ecs->physicsHistory.addToState(stream, index, true);


            // printf("\nReceived Physics with input checksum %u = %u\n", index, inputChecksum);
            printf("\nReceived Physics %u.\n", index);


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

            printf("\nReceived input to %u from %lu (%u -> %u)\n", inputs.back().index, RakNetGUID::ToUint32(inputs.back().id.rakNetGuid), stream.GetNumberOfBitsUsed(), inputs.back().data.GetNumberOfBitsUsed());

//            serializeControlSync(false, stream, mouseId, 0, syncType);  // ACTUALLY APPLY IT
          }
        } break;
        case ID_SYNC_MULTIPLE_CONTROLS: {
          if (network->getRole() == settings::network::CLIENT) { // Only clients should receive these

//            applyCombinedInputPacket(stream);  // ACTUALLY APPLY IT






            uint8_t numEntries;
            stream.Read(numEntries);

            printf("\nRecieved %u multi-controls:\n", numEntries);

            // uint8_t numIndices;
            // stream.Read(numIndices);
            // for (uint8_t i = 0; i < numIndices; ++i) {
            //   uint8_t index;
            //   stream.ReadBitsFromIntegerRange(index, (uint8_t)0, (uint8_t)(Physics::maxStoredStates - 1));
            //   uint32_t checksumPart;
            //   stream.Read(checksumPart);
            //   ecs->physicsHistory.addToInputChecksum(index, checksumPart);
            //
            //   printf("\nRecieved input checksum %u += %u\n", index, checksumPart);
            // }
            for (uint8_t i = 0; i < numEntries; ++i) {

              BitSize_t head0 = stream.GetReadOffset();

              MessageID controlType;
              serializeControlSyncShortType(false, stream, controlType);
              entityId mouseId = 0;
              stream.Read(mouseId);
              uint8_t index;
              stream.ReadBitsFromIntegerRange(index, (uint8_t) 0, (uint8_t) (Physics::maxStoredStates - 1));

              serializeControlSync(false, stream, mouseId, 0, controlType, true);

              BitSize_t head1 = stream.GetReadOffset();




              stream.SetReadOffset(head0);

              // FIXME: read the entire rest of the stream in.
              ecs->physicsHistory.addToInput(stream, index, head1 - head0);

              // FIXME: Actual control data is not read off the stream, causing too much left over.

              stream.SetReadOffset(head1);






              printf("Type %u with mouse %u at %u (%u)\n", controlType, mouseId, index, head1 - head0);
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
    // if (!network->getFreshConnections().empty()) {
    //   // All existing entities sent here
    //   // TODO: revert to last complete snapshot before sending this, and then send that snapShot index
    //   // TODO: Maybe just use whatever's in the state currently like this instead and let the client correct itself.
    //   for (const auto &id : registries[0].ids) {
    //     writeEntityRequestHeader(outStream);
    //     serializeEntityCreationRequest(true, outStream, *state, id);
    //     send(LOW_PRIORITY, RELIABLE_ORDERED, CH_ECS_UPDATE);
    //   }
    //   // Any new player and avatar components sent here, along with the player ID assignments
    //   for (const auto &client : network->getFreshConnections()) {
    //     outStream.Write((MessageID) ID_USER_PACKET_ADMIN_COMMAND);
    //     outStream.WriteBitsFromIntegerRange((uint8_t) CMD_ASSIGN_PLAYER_ID, (uint8_t) 0,
    //                                         (uint8_t) (CMD_END_ENUM - 1));
    //     serializePlayerAssignment(true, outStream, PlayerSet::create(*state, *ecs, network->getRole()));
    //     sendTo(client, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_ECS_UPDATE);
    //   }
    //   network->discardFreshConnections();
    // }


    if ( ! network->getFreshConnections().empty()) {
      // All existing entities sent here
      // TODO: revert to last complete snapshot before sending this, and then send that snapShot index
      // TODO: Maybe just use whatever's in the state currently like this instead and let the client correct itself.
      BitStream stream;
      for (const auto &id : registries[0].ids) {
        writeEntityRequestHeader(stream);
        serializeEntityCreationRequest(true, stream, *state, id);
        for (const auto &client : network->getFreshConnections()) {
          network->sendTo(stream, client, LOW_PRIORITY, RELIABLE_ORDERED, CH_ECS_UPDATE);
        }
        stream.Reset();
      }
      // New player avatar sets send out to everyone here
      for (const auto &client : network->getFreshConnections()) {
        freshPlayers.push(PlayerSet::create(*state, *ecs));
        ignoredInputSources.Push(client.rakNetGuid, __FILE__, __LINE__);
      }
      stream.Write((MessageID) ID_USER_PACKET_ADMIN_COMMAND);
      stream.WriteBitsFromIntegerRange((uint8_t) CMD_WORLD_INIT, (uint8_t) 0, (uint8_t) (CMD_END_ENUM - 1));
      for (const auto &client : network->getFreshConnections()) {
        printf("sent world init to %lu\n", (long unsigned)RakNetGUID::ToUint32(client.rakNetGuid));
        network->sendTo(stream, client, LOW_PRIORITY, RELIABLE_ORDERED, CH_ECS_UPDATE);
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
//          network->send(fullStream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_ECS_UPDATE);
//        }
        switch (network->getRole()) {
          case settings::network::SERVER: {
            stream.SetReadOffset(0);
            network->send(stream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_ECS_UPDATE);
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
            network->send(stream, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_ECS_UPDATE);
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


    printf("\nSerialize Physics: %s\n", rw ? "Writing" : "Reading");
    // bool debugStupidCrap = true;


    // FIXME: Don't do this by iterating over IDs!
    // Include some specifier of ID range or something instead, so that this doesn't cause problems.
    for (auto id : registries[1].ids) {


      printf("%u: ", id);


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
      if ( ! stream.SerializeCompressed(rw, include)) {
        printf("break ");
        break; // Stop if the stream is empty
      } else {
        printf("good ");
      }

//      // If includeAll is true, data is still written for an object that isn't included in networked physics messages.
//      // This is useful for keeping a complete physics state history but also remembering which ones to network.
//      bool saveStateInclude = false;
//      if (includeAll) {
//        saveStateInclude = physics->useCase != Physics::WHEEL && physics->useCase != Physics::STATIC_MESH;
//      }

//      if (netInclude || saveStateInclude) {


      if (include) {

        printf("included\n");

        // if (! rw) { debugStupidCrap = false; }

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


          std::cout << "PHYS: " << physics->rigidBody->getCenterOfMassPosition().length();


          physics->rigidBody->setCenterOfMassTransform(transform);
          physics->rigidBody->setLinearVelocity(glmToBullet(lin));


          std::cout << " -> " << physics->rigidBody->getCenterOfMassPosition().length() << std::endl;


        }
      } else {
        printf("zeroed\n");
      }
    }



    // if (stream.GetNumberOfBitsUsed()) {
    //   stream.SetReadOffset(0);
    //   char streamStr[stream.GetNumberOfBitsUsed() * 2];
    //   // stream.PrintBits(streamStr);
    //   stream.PrintHex(streamStr);
    //   std::string streamStrCpp(streamStr);
    //   std::cout << streamStrCpp << std::endl;
    // }



    // if ( ! rw && debugStupidCrap) {
    //   stream.SetReadOffset(0);
    //   char streamStr[stream.GetNumberOfBitsUsed()];
    //   stream.PrintBits(streamStr);
    //   std::cout << streamStr << std::endl;
    // }


  }

  void NetworkSystem::serializeControlSync(bool rw, BitStream &stream, entityId mId, entityId cId,
                                           MessageID syncType, bool useDummies) {
    MouseControls *mouseControls;
    if (useDummies) { mouseControls = &dummies.mouse; }
    else { state->getMouseControls(mId, &mouseControls); }
    if (!mouseControls) { return; } // a client may not yet have the component being referred to.
    stream.Serialize(rw, mouseControls->yaw);
    stream.Serialize(rw, mouseControls->pitch);
    stream.SerializeCompressed(rw, mouseControls->invertedX);
    stream.SerializeCompressed(rw, mouseControls->invertedY);
    stream.Serialize(rw, cId);
    switch (syncType) {
      case ID_SYNC_WALKCONTROLS: {
        WalkControls *walkControls;
        if (useDummies) { walkControls = &dummies.walk; }
        else { state->getWalkControls(cId, &walkControls); }
        if (!walkControls) { return; } // a client may not yet have the component being referred to.
        stream.Serialize(rw, walkControls->accel);
        stream.SerializeCompressed(rw, walkControls->jumpRequested);
        stream.SerializeCompressed(rw, walkControls->jumpInProgress);
        stream.SerializeCompressed(rw, walkControls->isGrounded);
        stream.SerializeCompressed(rw, walkControls->isRunning);
      } break;
      case ID_SYNC_PYRAMIDCONTROLS: {
        PyramidControls *pyramidControls;
        if (useDummies) { pyramidControls = &dummies.pyramid; }
        else { state->getPyramidControls(cId, &pyramidControls); }
        if (!pyramidControls) { return; } // a client may not yet have the component being referred to.
        stream.Serialize(rw, pyramidControls->accel);
        stream.SerializeCompressed(rw, pyramidControls->turbo);
        stream.SerializeCompressed(rw, pyramidControls->shoot);
        stream.SerializeCompressed(rw, pyramidControls->drop);
        stream.SerializeCompressed(rw, pyramidControls->pop);
      } break;
      case ID_SYNC_TRACKCONTROLS: {
        TrackControls *trackControls;
        if (useDummies) { trackControls = &dummies.track; }
        else { state->getTrackControls(cId, &trackControls); }
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
      outStream.Write(ecs->physicsHistory.getNewestIndex());
      printf("Sent initial index %u and playerId %u\n", ecs->physicsHistory.getNewestIndex(), playerId);
      // outStream.Write(ecs->physicsHistory.getInputChecksum(ecs->physicsHistory.getLatestIndex()));
    } else {
      uint8_t index;
      stream.Read(index);
      // uint32_t inputChecksum;
      // stream.Read(inputChecksum);
      ecs->physicsHistory.setEmptyIndex(index);

      printf("Set initial index: %u\n", index);

      ecs->physicsHistory.appendIndex();
      // ecs->physicsHistory.setInputChecksum(index, inputChecksum);
      topics::publish<entityId>("set_player_id", playerId);
      initialStep = true;
    }
  }

  void NetworkSystem::serializeWorldInit(bool rw, SLNet::BitStream &) {

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

    if (ecs->physicsHistory.isReplaying()) {

      // TODO: load next replay inputs until replay is done.

    } else if (ecs->physicsHistory.truthIsAvailable()) {

      // TODO: load truth state to begin replay, make sure that original truth state is the one being sent

      serializePhysicsSync(false, ecs->physicsHistory.getNewestCompleteState());
      serializePhysicsSync(false, ecs->physicsHistory.getNewestCompleteAuthState());

      ecs->physicsHistory.beginReplayFromTruth();



      // char streamStr[ecs->physicsHistory.getLatestCompleteAuthState().GetNumberOfBitsUsed() * 2];
      // ecs->physicsHistory.getLatestCompleteAuthState().PrintBits(streamStr);
      // // ecs->physicsHistory.getLatestCompleteAuthState().PrintHex(streamStr);
      // printf("\nReplay %u: %s\n", ecs->physicsHistory.getLatestCompleteIndex(), streamStr);
      // ecs->physicsHistory.getLatestCompleteAuthState().SetReadOffset(0);



      switch (network->getRole()) {
        case settings::network::SERVER: {
          // Send truth state to clients
          outStream.Write((MessageID) ID_SYNC_PHYSICS);
          outStream.WriteBitsFromIntegerRange(ecs->physicsHistory.getNewestIndex(), (uint8_t)0,
                                              (uint8_t)(Physics::maxStoredStates - 1));
          serializePhysicsSync(true, outStream);
          send(LOW_PRIORITY, SERVER_SIM_SYNC_PACKET_SEQUENCING, CH_SIMULATION_UPDATE);
        } break;
        default: break;
      }

      { // TODO: get rid of this section once replay is working
        ecs->physicsHistory._turnOffReplay();
        onBeforePhysicsStep();
      }

    } else {

      if (ecs->physicsHistory.isFull()) { // FIXME
        uint8_t tail = ecs->physicsHistory.getNewestCompleteIndex();
        std::vector<RakNetGUID> slowClients = ecs->physicsHistory.getUnfulfilledInputs(tail);
        for (const auto &client : slowClients) {
          fprintf(stderr, "\nSlow Client: %lu\n", RakNetGUID::ToUint32(client));
        }
      }

      BitStream physics;
      // TODO: get which indices will be used for server physics update and only store complete state in those.
      serializePhysicsSync(true, physics, true); // copy of entire local state, but not authorized truth state

      uint8_t newStateIndex;
      if (initialStep) {
        newStateIndex = ecs->physicsHistory.getNewestIndex();
        initialStep = false;
      } else {
        newStateIndex = ecs->physicsHistory.appendIndex();
      }



      // char streamStr[physics.GetNumberOfBitsUsed() * 2];
      // physics.PrintBits(streamStr);
      // // physics.PrintHex(streamStr);
      // printf("\n%u: %s\n", newStateIndex, streamStr);
      // physics.SetReadOffset(0);



      switch (network->getRole()) {
        case settings::network::SERVER: {

          handleNewClients();

          // TODO: don't set sources to getClientIds, rather wait for new clients to be ready before adding them.
          ecs->physicsHistory.setInputSources(network->getClientGuids(), newStateIndex);
          if (ignoredInputSources.Size()) {
            ecs->physicsHistory.excludeInputSources(ignoredInputSources, newStateIndex);
          }
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
            send(IMMEDIATE_PRIORITY, RELIABLE_ORDERED, CH_SIMULATION_UPDATE);
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

    // if (network->getRole() == settings::network::SERVER) {
    //   sendPeriodicPhysicsUpdate(); // TODO: get rid of this and the function once history is working
    // }

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

    theThing = 1;

//    uint8_t storedStateIndex;
//    physicsStates.front().ReadBitsFromIntegerRange(storedStateIndex, (uint8_t) 0, maxStoredStates);
//    serializePhysicsSync(false, physicsStates.front(), true);
  }



}
