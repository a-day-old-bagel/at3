
#include "server.hpp"
#include "settings.hpp"

using namespace SLNet;

namespace at3 {
  Server::Server() {
    peer = RakPeerInterface::GetInstance();
    peer->SetMaximumIncomingConnections(static_cast<uint8_t>(settings::network::maxServerConnections));
    SocketDescriptor sock(static_cast<uint8_t>(settings::network::serverPort), nullptr);
    StartupResult startResult = peer->Startup(static_cast<uint8_t>(settings::network::maxServerConnections), &sock, 1);
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

  void Server::receive(std::vector<Packet*> & buffer) {
    Packet *packet;
    for (packet=peer->Receive(); packet; packet=peer->Receive()) {
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
          if ((MessageID)packet->data[0] >= ID_USER_PACKET_ENUM) {
            buffer.emplace_back(packet);
            continue; // Do not deallocate - pointer to packet now in buffer
          }
        }
      }
      deallocatePacket(packet);
    }
  }

  void Server::tick(std::vector<Packet*> & buffer) {
    receive(buffer);
  }

  void Server::send(BitStream &stream, PacketPriority priority, PacketReliability reliability, char channel) {
    peer->Send(&stream, priority, reliability, channel, UNASSIGNED_SYSTEM_ADDRESS, true);
  }

  void Server::deallocatePacket(Packet *packet) {
    peer->DeallocatePacket(packet);
  }
}
