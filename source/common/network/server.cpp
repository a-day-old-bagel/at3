
#include "server.hpp"
#include "settings.hpp"

using namespace SLNet;

namespace at3 {
  Server::Server() {
    peer = RakPeerInterface::GetInstance();
    peer->SetMaximumIncomingConnections(static_cast<uint16_t>(settings::network::maxServerConns));
    SocketDescriptor sock(static_cast<uint16_t >(settings::network::serverPort), nullptr);
    // TODO: this priority won't work on Linux.
    StartupResult startResult = peer->Startup(settings::network::maxServerConns, &sock, 1, THREAD_PRIORITY_NORMAL);
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

# define CASE_REPORT_PACKET(e, s) case ID_ ##e: fprintf(s, "Received %s\n", #e); break
  void Server::receive(std::vector<Packet*> & buffer) {
    Packet *packet;
    for (packet=peer->Receive(); packet; packet=peer->Receive()) {
      switch (packet->data[0]) {
          CASE_REPORT_PACKET(NEW_INCOMING_CONNECTION, stdout);
          CASE_REPORT_PACKET(DISCONNECTION_NOTIFICATION, stdout);
          CASE_REPORT_PACKET(CONNECTION_LOST, stderr);
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
# undef CASE_REPORT_PACKET

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
