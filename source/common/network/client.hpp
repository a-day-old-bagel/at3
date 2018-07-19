#pragma once

#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakNetTypes.h"
#include "StringCompressor.h"

namespace at3 {
  class Client {
      SLNet::RakPeerInterface *peer;
      void connect();
      void receive();
    public:
      Client();
      virtual ~Client();
      void tick();
  };
}
