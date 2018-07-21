
#pragma once

#include "ezecs.hpp"
#include "netInterface.hpp"
#include "topics.hpp"

using namespace ezecs;

namespace at3 {

  enum FurtherPacketEnums {
      ID_SYNC_PLACEMENT = ID_USER_PACKET_FURTHER_ENUM,
      ID_SYNC_PHYSICS,
      ID_SYNC_MOUSECONTROLS,
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
      uint32_t currentPlacementIndex = 0;
      uint32_t currentPhysicsIndex = 0;

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
      void sendPlacementSync();
      void sendPhysicsSync();

      void receiveSyncPacketsFromClients();
      void receiveSyncPacketsFromServer();
      void receivePlacementSyncs(SLNet::BitStream &stream, uint16_t count);
      void receivePhysicsSyncs(SLNet::BitStream &stream, uint16_t count);

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
