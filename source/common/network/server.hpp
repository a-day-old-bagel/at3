#pragma once

#include <cstdint>

#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakNetTypes.h"

namespace at3 {
  class Server {
      SLNet::RakPeerInterface *peer;
      void receive();
    public:
      Server();
      virtual ~Server();
      void tick();
  };
}
