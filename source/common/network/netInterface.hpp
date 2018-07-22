#pragma once

#include <memory>
#include "server.hpp"
#include "client.hpp"
#include "settings.hpp"

namespace at3 {

  enum PacketEnums {
      ID_REQUEST_NEW_ENTITY = ID_USER_PACKET_ENUM,
      ID_GRANT_NEW_ENTITY,

      ID_USER_PACKET_FURTHER_ENUM
  };

  class NetInterface {
      std::unique_ptr<Server> server;
      std::unique_ptr<Client> client;
      std::vector<SLNet::Packet*> syncPackets;
      uint32_t role = settings::network::role;
      void assumeRole();
    public:
      NetInterface();
      uint32_t getRole();
      void switchToServer();
      void switchToClient();
      void tick();
      void send(SLNet::BitStream &stream, PacketPriority priority = LOW_PRIORITY,
                PacketReliability reliability = UNRELIABLE_SEQUENCED, char channel = 0);
      const std::vector<SLNet::Packet*> & getSyncPackets();
      void discardSyncPackets();
  };
}
