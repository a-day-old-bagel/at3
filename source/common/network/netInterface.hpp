#pragma once

#include "settings.hpp"
#include "server.hpp"
#include "client.hpp"

#include <memory>

namespace at3 {
  class NetInterface {
      std::unique_ptr<Server> server;
      std::unique_ptr<Client> client;
      std::vector<SLNet::Packet*> requestPackets;
      std::vector<SLNet::Packet*> syncPackets;
      std::vector<SLNet::AddressOrGUID> freshConnections;
      uint32_t role = settings::network::role;
      DataStructures::List<SLNet::SystemAddress> emptyAddresses;
      DataStructures::List<SLNet::RakNetGUID> emptyGuids;
      void assumeRole();
      void discardPacketCollection(std::vector<SLNet::Packet*> & packets);
    public:

      NetInterface();

      uint32_t getRole() const;
      void switchToServer();
      void switchToClient();

      void tick();
      void send(const SLNet::BitStream &stream, PacketPriority priority = LOW_PRIORITY,
                PacketReliability reliability = UNRELIABLE, char channel = 0);
      void sendTo(const SLNet::BitStream &stream, const SLNet::AddressOrGUID &target,
                  PacketPriority priority = LOW_PRIORITY, PacketReliability reliability = UNRELIABLE, char channel = 0);

      const std::vector<SLNet::Packet*> & getRequestPackets() const;
      void discardRequestPackets();
      const std::vector<SLNet::Packet*> & getSyncPackets() const;
      void discardSyncPackets();
      const std::vector<SLNet::AddressOrGUID> & getFreshConnections() const;
      void discardFreshConnections();

      const DataStructures::List<SLNet::SystemAddress> & getClientAddresses() const;
      const DataStructures::List<SLNet::RakNetGUID> & getClientGuids() const;
  };
}
