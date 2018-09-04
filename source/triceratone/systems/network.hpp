
#pragma once

#include "ezecs.hpp"
#include "topics.hpp"
#include "ouroboros.hpp"
#include "netInterface.hpp"
#include "serialization.hpp"
#include "interface.hpp"

using namespace ezecs;

namespace at3 {

  struct SingleClientInput {
    SingleClientInput() = default;
    SingleClientInput(const SingleClientInput &) {}
    SLNet::AddressOrGUID id = SLNet::RakNetGUID(0);
    uint8_t index = 0;
    SLNet::BitStream data;
  };

  class NetworkSystem : public System<NetworkSystem> {
      std::shared_ptr<NetInterface> network;
      std::shared_ptr<EntityComponentSystemInterface> ecs;
      entityId mouseControlId = 0;
      entityId keyControlId = 0;
      SLNet::MessageID keyControlMessageId = ID_USER_PACKET_END_ENUM;
      SLNet::BitStream outStream;

//      float timeAccumulator = 0;
      uint8_t physicsStepAccumulator = 0;

      bool strictWarp = true;

      bool initialStep = false;

//      uint8_t maxStoredStates = 31;
//      uint8_t storedStateIndexCounter = 0;
//      std::queue<SLNet::BitStream> physicsStates;

//      PhysicsHistory physicsHistory;

      // TODO: use hashmap of guid->stream instead, and keep a list of all clients to check against so that no poopy
      // entries are inserted into the hashmap (use [] to insert upon join, use at() and check first to put streams in)
      std::vector<SingleClientInput> inputs;

      void setNetInterface(void *netInterface);
      void setEcsInterface(void *ecs);
      void switchToMouseCtrl(void *id);
      void switchToWalkCtrl(void* id);
      void switchToPyramidCtrl(void *id);
      void switchToTrackCtrl(void *id);
      void switchToFreeCtrl(void *id);

      bool sendPeriodicPhysicsUpdate();

      bool writeControlSync();
      void sendAllControlSyncs();
      void send(PacketPriority priority, PacketReliability reliability, char channel);
      void sendTo(const SLNet::AddressOrGUID & target, PacketPriority priority,
                  PacketReliability reliability, char channel);

      void receiveAdministrativePackets();
      bool receiveSyncPackets();
      void applyCombinedInputPacket(SLNet::BitStream &);
      void handleNewClients();

      void respondToEntityRequest(SLNet::BitStream &);
      void respondToComponentRequest(SLNet::BitStream &);

      void serializePhysicsSync(bool rw, SLNet::BitStream &, bool includeAll = false);
      void serializeControlSync(bool rw, SLNet::BitStream &, entityId mId, entityId cId,
                                SLNet::MessageID syncType = ID_USER_PACKET_END_ENUM);
      bool serializeControlSyncShortType(bool rw, SLNet::BitStream &, SLNet::MessageID &type);

      void serializePlayerAssignment(bool rw, SLNet::BitStream &, entityId);

    public:
      std::vector<compMask> requiredComponents = {
        NETWORKING,
        NETWORKING | PHYSICS,
      };
      explicit NetworkSystem(State * state);
      bool onInit();
      void onTick(float dt);
      bool onDiscoverNetworkedPhysics(const entityId &id);
      void toggleStrictWarp();
      void onBeforePhysicsStep();
      void onAfterPhysicsStep();
      void rewindPhysics();
  };
}
