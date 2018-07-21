#pragma once

#include <vector>

#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "GetTime.h"
#include "RakNetTypes.h"
#include "StringCompressor.h"

namespace at3 {
  class Client {
      SLNet::RakPeerInterface *peer;
      void connect();
      void receive(std::vector<SLNet::Packet*> & buffer);
    public:
      Client();
      virtual ~Client();
      void tick(std::vector<SLNet::Packet*> & buffer);
      void send(SLNet::BitStream &stream);
      void deallocatePacket(SLNet::Packet * packet);
  };
}
