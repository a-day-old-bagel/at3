
#include <string>
#include "client.hpp"
#include "settings.hpp"
#include "userPacketEnums.hpp"
#include "topics.hpp"

using namespace SLNet;

namespace at3 {
  Client::Client() {
    peer = RakPeerInterface::GetInstance();
    SocketDescriptor sock(static_cast<uint16_t>(settings::network::clientPort), nullptr);
    StartupResult startResult = peer->Startup(1, &sock, 1, AT3_NET_THREAD_PRIORITY);
    if (startResult == RAKNET_STARTED) {
      connect();
    } else {
      fprintf(stderr, "Peer interface failed to start!\n");
    }
  }

  Client::~Client() {
    if (peer->IsActive()) {
      peer->Shutdown(100);
    }
    printf("Client is being destroyed.\n");
    RakPeerInterface::DestroyInstance(peer);
  }

  void Client::connect() {
    printf("Connecting to server %s:%d ...\n", settings::network::serverAddress.c_str(), settings::network::serverPort);
    PublicKey publicKey = {PKM_INSECURE_CONNECTION, nullptr, nullptr, nullptr};
    ConnectionAttemptResult connectionResult = peer->Connect
        (settings::network::serverAddress.c_str(), static_cast<uint16_t>(settings::network::serverPort),
         nullptr, 0, // password char* and password length
         &publicKey, 0, // public key pointer and socket index (sock in constructor can be array pointer)
         12, 800, 0);  // num tries, ms between tries, timeout once connected (0 picks default)
    printf("Returned: %s.\n", getConnectionAttemptResultString(connectionResult).c_str());
  }

# define CASE_REPORT_PACKET(e, s) case ID_ ##e: fprintf(s, "Received %s\n", #e); break
  void Client::receive(std::vector<SLNet::Packet*> & requestBuffer, std::vector<SLNet::Packet*> & syncBuffer) {
    Packet *packet;
    for (packet=peer->Receive(); packet; packet=peer->Receive()) {
      switch (packet->data[0]) {
        CASE_REPORT_PACKET(CONNECTION_REQUEST_ACCEPTED, stdout);
        case ID_CONNECTION_ATTEMPT_FAILED: {
          fprintf(stderr, "Client connection attempt failed!\n");
          rtu::topics::publish("client_could_not_connect");
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

# define CASE_EMIT_RESULT(e) case e: return #e
  std::string Client::getConnectionAttemptResultString(SLNet::ConnectionAttemptResult result) {
    switch (result) {
      CASE_EMIT_RESULT(CONNECTION_ATTEMPT_STARTED);
      CASE_EMIT_RESULT(INVALID_PARAMETER);
      CASE_EMIT_RESULT(CANNOT_RESOLVE_DOMAIN_NAME);
      CASE_EMIT_RESULT(ALREADY_CONNECTED_TO_ENDPOINT);
      CASE_EMIT_RESULT(CONNECTION_ATTEMPT_ALREADY_IN_PROGRESS);
      CASE_EMIT_RESULT(SECURITY_INITIALIZATION_FAILED);
      default: return "unknown error";
    }
  }
# undef CASE_EMIT_RESULT

  void Client::tick(std::vector<SLNet::Packet*> & requestBuffer, std::vector<SLNet::Packet*> & syncBuffer) {
    receive(requestBuffer, syncBuffer);
  }

  void Client::send(const BitStream &stream, PacketPriority priority, PacketReliability reliability, char channel) {
    peer->Send(&stream, priority, reliability, channel, UNASSIGNED_SYSTEM_ADDRESS, true);
  }

  void Client::sendTo(const BitStream &stream, const AddressOrGUID &target, PacketPriority priority,
                      PacketReliability reliability, char channel) {
    peer->Send(&stream, priority, reliability, channel, target, false);
  }

  void Client::deallocatePacket(Packet *packet) {
    peer->DeallocatePacket(packet);
  }

  SLNet::RakNetGUID Client::getGuid() {
    return peer->GetMyGUID();
  }
}
