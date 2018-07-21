
#include <string>
#include "client.hpp"
#include "settings.hpp"

using namespace SLNet;

namespace at3 {
  Client::Client() {
    peer = RakPeerInterface::GetInstance();
    SocketDescriptor sock;
    StartupResult startResult = peer->Startup(1, &sock, 1);
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
    printf("Client is connecting to %s:%d\n", settings::network::serverAddress.c_str(), settings::network::serverPort);
    ConnectionAttemptResult connectionResult = peer->Connect
        (settings::network::serverAddress.c_str(),static_cast<uint8_t>(settings::network::serverPort), nullptr, 0);
    if (connectionResult != CONNECTION_ATTEMPT_STARTED) {
      printf("Client failed to connect to the server.\n");
    }
  }

  void Client::receive(std::vector<Packet*> & buffer) {
    Packet *packet;
    for (packet=peer->Receive(); packet; packet=peer->Receive()) {
      switch (packet->data[0]) {
        case ID_CONNECTION_REQUEST_ACCEPTED: {
          printf("Client has connnected to server.\n");
        } break;
        case ID_CONNECTION_ATTEMPT_FAILED: {
          fprintf(stderr, "Client could not connect to server!\n");
        }
        default: {
          if ((MessageID)packet->data[0] >= ID_USER_PACKET_ENUM) {
            buffer.emplace_back(packet);
            continue; // Do not deallocate - pointer to packet now in buffer
          }
        }
      }
      deallocatePacket(packet);
    }
  }

  void Client::tick(std::vector<Packet*> & buffer) {
    receive(buffer);
  }

  void Client::send(BitStream &stream) {
    peer->Send(&stream, LOW_PRIORITY, UNRELIABLE_SEQUENCED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
  }

  void Client::deallocatePacket(Packet *packet) {
    peer->DeallocatePacket(packet);
  }
}
