
#pragma once

#include <array>
#include <queue>

#include "ezecs.hpp"
#include "topics.hpp"
#include "netInterface.hpp"
#include "serialization.hpp"
#include "interface.hpp"

using namespace ezecs;

namespace at3 {

  template <typename T, typename indexType, indexType numSlots>
  class Ouroboros {
    public:

      typedef rtu::Delegate<T&(T&, const indexType&)> Ctor;
      typedef rtu::Delegate<void(T&, const indexType&)> Dtor;
      static Ctor defaultCtor;
      static Dtor defaultDtor;
      explicit Ouroboros(const Ctor & ctor = defaultCtor, const Dtor & dtor = defaultDtor);
      const indexType getNumSlots() const;
      const bool isEmpty() const;
      const bool isFull() const;
      const bool isValid() const;
      const bool isValid(indexType index) const;
      T & capitate();
      T & decaudate();
      T & decaudate(indexType upToButNot);
      T & operator [] (indexType index);
      const T & operator [] (indexType index) const;

    private:

      std::array<T, numSlots> raw;
      Ctor ctor;
      Dtor dtor;
      indexType nextHead = 0;
      indexType currentTail = 0;
      bool emptyFlag = true;
      bool fullFlag = false;
      bool errorFlag = false;

      static T& defaultCtorImpl(T&, const indexType&);
      static void defaultDtorImpl(T&, const indexType&);
      indexType advanceHead();
      indexType advanceTail();
      void haveBadTime();
  };

  template <typename playerIdType>
  class SnapShot {
    public:

      bool addToInputState(const playerIdType &id, const SLNet::BitStream &input);
      void addToPhysicsState(const SLNet::BitStream &input);

      static rtu::Delegate<SnapShot&(SnapShot&, const playerIdType&)> init;
      static rtu::Delegate<void(SnapShot&, const playerIdType&)> clear;

    private:

      SLNet::BitStream inputState, physicsState;

      static SnapShot & initImpl(SnapShot &snapShot, const playerIdType &index);
      static void clearImpl(SnapShot &snapShot, const playerIdType &index);
  };

  class PhysicsHistory {
    public:
      PhysicsHistory();
    private:
      Ouroboros<SnapShot<uint8_t>, uint8_t, 32> snapShots;

      // TODO: store tail-time whenever a collapse happens, then set the bullet pre-step to walk through the control
      // states as it steps through the (currentTime(ms) - tailTime(ms)) / 16.67(ms) steps
      // should verify that correct number of steps are taken?
      // TODO: Should switch control system to only fire on physics pre-step?
      // Maybe just the key-pressing part, not the mouse. OTherwise key-held events have more power depending on
      // framerate and those in-between-bullet-step updates are probably lost in networking anyway.
  };

  struct SingleClientInput {
    SingleClientInput() = default;
    SingleClientInput(const SingleClientInput &) {}
    SLNet::AddressOrGUID id;
    SLNet::BitStream data;
  };

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

      PhysicsHistory physicsHistory;

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

      bool writePhysicsSyncs(float dt);

      void writeControlSyncs();
      void sendAllControlSyncs();
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
      void onAfterBulletPhysicsStep();
      void rewindPhysics();
  };
}
