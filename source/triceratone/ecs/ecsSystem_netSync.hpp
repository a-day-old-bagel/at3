
#pragma once

#include "ezecs.hpp"
#include "netInterface.hpp"
#include "topics.hpp"

using namespace ezecs;

namespace at3 {

  enum FurtherPacketEnums {
      ID_SYNC_PLACEMENT = ID_USER_PACKET_FURTHER_ENUM,
      ID_SYNC_PHYSICS,
      ID_SYNC_WALKCONTROLS,
      ID_SYNC_PYRAMIDCONTROLS,
      ID_SYNC_TRACKCONTROLS,
      ID_SYNC_FREECONTROLS,

      ID_USER_PACKET_END_ENUM
  };

  class NetSyncSystem : public System<NetSyncSystem> {
      std::shared_ptr<NetInterface> network;
      entityId mouseControlId = 0;
      entityId keyControlId = 0;
      SLNet::MessageID keyControlMessageId = ID_USER_PACKET_END_ENUM;
      uint32_t currentPlacementIndex = 0;
      uint32_t currentPhysicsIndex = 0;
      SLNet::Time lastPlacementSyncTime = SLNet::GetTime();
      SLNet::Time lastPhysicsSyncTime = SLNet::GetTime();

      rtu::topics::Subscription setNetInterfaceSub;
      rtu::topics::Subscription switchToMouseCtrlSub;
      rtu::topics::Subscription switchToWalkCtrlSub;
      rtu::topics::Subscription switchToPyramidCtrlSub;
      rtu::topics::Subscription switchToTrackCtrlSub;
      rtu::topics::Subscription switchToFreeCtrlSub;

      void setNetInterface(void *netInterface);
      void switchToMouseCtrl(void *id);
      void switchToWalkCtrl(void* id);
      void switchToPyramidCtrl(void *id);
      void switchToTrackCtrl(void *id);
      void switchToFreeCtrl(void *id);

      void beginSyncStream(SLNet::BitStream &stream, SLNet::MessageID messageType, uint16_t numComponents);
      void sendPlacementSyncs(SLNet::BitStream &stream);
      void sendPhysicsSyncs(SLNet::BitStream &stream);
      void sendControlsSyncs(SLNet::BitStream &stream);

      void receiveSyncPacketsFromClients();
      void receiveSyncPacketsFromServer();
      void receivePlacementSyncs(SLNet::BitStream &stream);
      void receivePhysicsSyncs(SLNet::BitStream &stream);
      void receiveControlSyncs(SLNet::BitStream &stream, SLNet::MessageID syncType);

      void serializePlacementSync(SLNet::BitStream &, entityId id, bool rw);
      void serializePhysicsSync(SLNet::BitStream &, entityId id, bool rw);
      void serializeControlSync(
          SLNet::BitStream &, entityId id, bool rw,
          SLNet::MessageID syncType = ID_USER_PACKET_END_ENUM);

    public:
      std::vector<compMask> requiredComponents = {
        PLACEMENT,
        PHYSICS,
      };
      NetSyncSystem(State * state);
      bool onInit();
      void onTick(float dt);
  };
}
