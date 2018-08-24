
#include "server.hpp"
#include "settings.hpp"
#include "userPacketEnums.hpp"

using namespace SLNet;

namespace at3 {
  Server::Server() {
    peer = RakPeerInterface::GetInstance();
    peer->SetMaximumIncomingConnections(static_cast<uint16_t>(settings::network::maxServerConns));
    SocketDescriptor sock(static_cast<uint16_t>(settings::network::serverPort), nullptr);
    StartupResult startResult = peer->Startup(settings::network::maxServerConns, &sock, 1, AT3_NET_THREAD_PRIORITY);
    if (startResult == RAKNET_STARTED) {
      peer->SetOccasionalPing(true);
      printf("Server is ready to receive connections.\n");
    } else {
      printf("Server failed to start.\n");
    }
  }
  Server::~Server() {
    if (peer->IsActive()) {
      peer->Shutdown(100);
    }
    printf("Server is being destroyed.\n");
    RakPeerInterface::DestroyInstance(peer);
  }

# define CASE_REPORT_PACKET(e, s) case ID_ ##e: fprintf(s, "Received %s\n", #e); break
  void Server::receive(std::vector<SLNet::Packet*> & requestBuffer, std::vector<SLNet::Packet*> & syncBuffer,
                       std::vector<SLNet::AddressOrGUID> & connectionBuffer) {
    connectionListsDirty = true;
    Packet *packet;
    for (packet=peer->Receive(); packet; packet=peer->Receive()) {
      switch (packet->data[0]) {
          CASE_REPORT_PACKET(DISCONNECTION_NOTIFICATION, stdout);
          CASE_REPORT_PACKET(CONNECTION_LOST, stderr);
        case ID_NEW_INCOMING_CONNECTION: {
          connectionBuffer.emplace_back(AddressOrGUID(packet));
          printf("Client Connected: %s\n", connectionBuffer.back().systemAddress.ToString());
        } break;
        default: {
          if ((MessageID)packet->data[0] >= ID_USER_PACKET_SYNC_ENUM) {
            syncBuffer.emplace_back(packet);
            continue; // Do not deallocate - pointer to packet now in buffer
          } else if ((MessageID)packet->data[0] >= ID_USER_PACKET_ECS_REQUEST_ENUM) {
            requestBuffer.emplace_back(packet);
            continue; // Do not deallocate - pointer to packet now in buffer
          }
        }
      }
      deallocatePacket(packet);
    }
  }
# undef CASE_REPORT_PACKET

  void Server::updateConnectionLists() {
    if (connectionListsDirty) {
      peer->GetSystemList(addresses, guids);
      connectionListsDirty = false;
    }
  }

  void Server::tick(std::vector<SLNet::Packet*> & requestBuffer, std::vector<SLNet::Packet*> & syncBuffer,
                    std::vector<SLNet::AddressOrGUID> & connectionBuffer) {
    receive(requestBuffer, syncBuffer, connectionBuffer);
  }

  void Server::send(const BitStream &stream, PacketPriority priority, PacketReliability reliability, char channel) {
    peer->Send(&stream, priority, reliability, channel, UNASSIGNED_SYSTEM_ADDRESS, true);
  }

  void Server::sendTo(const BitStream &stream, const AddressOrGUID &target, PacketPriority priority,
                      PacketReliability reliability, char channel) {
    peer->Send(&stream, priority, reliability, channel, target, false);
  }

  void Server::deallocatePacket(Packet *packet) {
    peer->DeallocatePacket(packet);
  }

  const DataStructures::List<SLNet::SystemAddress> &Server::getClientAddresses() {
    updateConnectionLists();
    return addresses;
  }

  const DataStructures::List<SLNet::RakNetGUID> &Server::getClientGuids() {
    updateConnectionLists();
    return guids;
  }
}
