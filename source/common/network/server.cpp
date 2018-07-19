
#include "server.hpp"

using namespace SLNet;

constexpr uint32_t serverPort = 22022;
constexpr uint32_t maxConns = 150;

namespace at3 {
  Server::Server() {
    peer = RakPeerInterface::GetInstance();
    peer->SetMaximumIncomingConnections(maxConns);
    SocketDescriptor socket(serverPort, nullptr);
    StartupResult startResult = peer->Startup(maxConns, &socket, 1);
    if (startResult != RAKNET_STARTED) {
      printf("Server failed to start.\n");
    }
    peer->SetOccasionalPing(true);
    printf("Server is ready to receive connections.\n");
  }
  Server::~Server() {
    printf("Server is being destroyed.\n");
    RakPeerInterface::DestroyInstance(peer);
  }

  void Server::receive() {
    Packet *packet;
    for (packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive()) {
      switch (packet->data[0]) {
        case ID_NEW_INCOMING_CONNECTION: {
          printf("A remote system has successfully connected.\n");
        } break;
        case ID_DISCONNECTION_NOTIFICATION: {
          printf("A remote system has disconnected.\n");
        } break;
        case ID_CONNECTION_LOST: {
          printf("A remote system lost the connection.\n");
        } break;
        default: {
          printf("Server received unknown packet.\n");
        }
      }
    }
  }

  void Server::tick() {
    receive();
  }
}
