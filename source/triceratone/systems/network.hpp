
#pragma once

#include <queue>

#include "ezecs.hpp"
#include "topics.hpp"
#include "netInterface.hpp"
#include "serialization.hpp"
#include "interface.hpp"

using namespace ezecs;

namespace at3 {
  class NetworkSystem : public System<NetworkSystem> {
      std::shared_ptr<NetInterface> network;
      std::shared_ptr<EntityComponentSystemInterface> ecs;
      entityId mouseControlId = 0;
      entityId keyControlId = 0;
      SLNet::MessageID keyControlMessageId = ID_USER_PACKET_END_ENUM;
      SLNet::BitStream outStream;
      float timeAccumulator = 0;
      bool strictWarp = false;

      uint8_t maxStoredStates = 31;
      uint8_t storedStateIndexCounter = 0;
      std::queue<SLNet::BitStream> physicsStates;

      void setNetInterface(void *netInterface);
      void setEcsInterface(void *ecs);
      void switchToMouseCtrl(void *id);
      void switchToWalkCtrl(void* id);
      void switchToPyramidCtrl(void *id);
      void switchToTrackCtrl(void *id);
      void switchToFreeCtrl(void *id);

      bool writePhysicsSyncs(float dt);
      void writeControlSyncs();
      void send(PacketPriority priority, PacketReliability reliability, char channel);
      void sendTo(const SLNet::AddressOrGUID & target, PacketPriority priority,
                  PacketReliability reliability, char channel);

      void receiveAdministrativePackets();
      void receiveSyncPackets();
      void handleNewClients();

      void respondToEntityRequest(SLNet::BitStream &);
      void respondToComponentRequest(SLNet::BitStream &);

      void serializePhysicsSync(bool rw, SLNet::BitStream &, bool includeAll = false);
      void serializeControlSync(bool rw, SLNet::BitStream &, entityId mId, entityId cId,
                                SLNet::MessageID syncType = ID_USER_PACKET_END_ENUM);

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
      void onAfterBulletPhysicsStep();
      void rewindPhysics();
  };
}
