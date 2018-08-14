#pragma once

#include <string>
#include <vector>

#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakNetTypes.h"
#include "StringCompressor.h"

namespace at3 {
  class Client {
      SLNet::RakPeerInterface *peer;
      void connect();
      void receive(std::vector<SLNet::Packet*> & requestBuffer, std::vector<SLNet::Packet*> & syncBuffer);
      std::string getConnectionAttemptResultString(SLNet::ConnectionAttemptResult result);
    public:
      Client();
      virtual ~Client();
      void tick(std::vector<SLNet::Packet*> & requestBuffer, std::vector<SLNet::Packet*> & syncBuffer);
      void send(const SLNet::BitStream &stream, PacketPriority priority, PacketReliability reliability, char channel);
      void sendTo(const SLNet::BitStream &stream, const SLNet::AddressOrGUID &target, PacketPriority priority,
                  PacketReliability reliability, char channel);
      void deallocatePacket(SLNet::Packet * packet);
  };
}
