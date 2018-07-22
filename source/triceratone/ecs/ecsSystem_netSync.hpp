
#pragma once

#include "ezecs.hpp"
#include "netInterface.hpp"
#include "topics.hpp"

using namespace ezecs;

namespace at3 {

  enum FurtherPacketEnums {
      ID_SYNC_PHYSICS = ID_USER_PACKET_FURTHER_ENUM,
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
      float timeAccumulator = 0;

      SLNet::BitStream outStream;

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

      bool writePhysicsSyncs(float dt);
      void writeControlSyncs();
      void send(PacketPriority priority, PacketReliability reliability, char channel);

      void receiveSyncPackets();

      void serializePhysicsSync(bool rw, SLNet::BitStream &);
      void serializeControlSync(bool rw, SLNet::BitStream &, entityId mId, entityId cId,
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
