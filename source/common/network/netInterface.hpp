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
      uint32_t role = settings::network::role;
      void assumeRole();
      void discardPacketCollection(std::vector<SLNet::Packet*> & packets);
    public:
      NetInterface();
      uint32_t getRole();
      void switchToServer();
      void switchToClient();
      void tick();
      void send(const SLNet::BitStream &stream, PacketPriority priority = LOW_PRIORITY,
                PacketReliability reliability = UNRELIABLE, char channel = 0);
      void sendTo(const SLNet::BitStream &stream, const SLNet::AddressOrGUID &target,
                  PacketPriority priority = LOW_PRIORITY, PacketReliability reliability = UNRELIABLE, char channel = 0);
      const std::vector<SLNet::Packet*> & getRequestPackets();
      void discardRequestPackets();
      const std::vector<SLNet::Packet*> & getSyncPackets();
      void discardSyncPackets();
  };
}
