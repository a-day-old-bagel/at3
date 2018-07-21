#pragma once

#include <cstdint>
#include <vector>

#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "GetTime.h"
#include "RakNetTypes.h"

namespace at3 {
  class Server {
      SLNet::RakPeerInterface *peer;
      void receive(std::vector<SLNet::Packet*> & buffer);
    public:
      Server();
      virtual ~Server();
      void tick(std::vector<SLNet::Packet*> & buffer);
      void send(SLNet::BitStream &stream, PacketPriority priority, PacketReliability reliability);
      void deallocatePacket(SLNet::Packet * packet);
  };
}
