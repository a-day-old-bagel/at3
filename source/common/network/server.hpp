#pragma once

#include <cstdint>
#include <vector>

#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakNetTypes.h"

namespace at3 {
  class Server {
      SLNet::RakPeerInterface *peer;
      void receive(std::vector<SLNet::Packet*> & requestBuffer, std::vector<SLNet::Packet*> & syncBuffer,
                   std::vector<SLNet::AddressOrGUID> & connectionBuffer);
    public:
      Server();
      virtual ~Server();
      void tick(std::vector<SLNet::Packet*> & requestBuffer, std::vector<SLNet::Packet*> & syncBuffer,
                std::vector<SLNet::AddressOrGUID> & connectionBuffer);
      void send(const SLNet::BitStream &stream, PacketPriority priority, PacketReliability reliability, char channel);
      void sendTo(const SLNet::BitStream &stream, const SLNet::AddressOrGUID &target, PacketPriority priority,
                  PacketReliability reliability, char channel);
      void deallocatePacket(SLNet::Packet * packet);
  };
}
