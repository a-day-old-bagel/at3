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
      DataStructures::List<SLNet::SystemAddress> addresses;
      DataStructures::List<SLNet::RakNetGUID> guids;
      uint32_t clientSum = 0;
      bool connectionListsDirty = true;
      void receive(std::vector<SLNet::Packet*> & requestBuffer, std::vector<SLNet::Packet*> & syncBuffer,
                   std::vector<SLNet::AddressOrGUID> & connectionBuffer);
      void updateConnectionLists();
    public:
      Server();
      virtual ~Server();
      void tick(std::vector<SLNet::Packet*> & requestBuffer, std::vector<SLNet::Packet*> & syncBuffer,
                std::vector<SLNet::AddressOrGUID> & connectionBuffer);
      void send(const SLNet::BitStream &stream, PacketPriority priority, PacketReliability reliability, char channel);
      void sendTo(const SLNet::BitStream &stream, const SLNet::AddressOrGUID &target, PacketPriority priority,
                  PacketReliability reliability, char channel);
      void deallocatePacket(SLNet::Packet * packet);
      const DataStructures::List<SLNet::SystemAddress> & getClientAddresses();
      const DataStructures::List<SLNet::RakNetGUID> & getClientGuids();
      uint32_t getClientSum();
      SLNet::RakNetGUID getGuid();
  };
}
