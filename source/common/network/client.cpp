
#include <string>
#include "client.hpp"

using namespace SLNet;

constexpr uint32_t serverPort = 22022;
constexpr auto serverAddr = "127.0.0.1";

namespace at3 {
  Client::Client() {
    peer = RakPeerInterface::GetInstance();
    SocketDescriptor socket;
    StartupResult startResult = peer->Startup(1, &socket, 1);
    if (startResult != RAKNET_STARTED) {
      printf("Client failed to start.\n");
    }
    connect();
  }

  Client::~Client() {
    printf("Client is being destroyed.\n");
    RakPeerInterface::DestroyInstance(peer);
  }

  void Client::connect() {
    printf("Client is connecting to %s:%d\n", serverAddr, serverPort);
    ConnectionAttemptResult connectionResult = peer->Connect(serverAddr, serverPort, nullptr, 0);
    if (connectionResult != CONNECTION_ATTEMPT_STARTED) {
      printf("Client failed to connect to the server.\n");
    }
  }

  void Client::receive() {
    Packet *packet;
    for (packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive()) {
      switch (packet->data[0]) {
        case ID_CONNECTION_REQUEST_ACCEPTED:
          printf("The connection to the server has been accepted.\n");
          break;
        default: {
          printf("Client received unknown packet.\n");
        }
      }
    }
  }

  void Client::tick() {
    receive();
  }

}
