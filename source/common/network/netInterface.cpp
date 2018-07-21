
#include "netInterface.hpp"

using namespace SLNet;

namespace at3 {

  NetInterface::NetInterface() {
    assumeRole();
  }

  void NetInterface::assumeRole() {
    switch(role) {
      case settings::network::Role::SERVER: {
        client.reset();
        server = std::make_unique<Server>();
      } break;
      case settings::network::Role::CLIENT: {
        server.reset();
        client = std::make_unique<Client>();
      } break;
      default: {
        server.reset();
        client.reset();
      }
    }
  }

  uint32_t NetInterface::getRole() {
    return role;
  }

  void NetInterface::switchToServer() {
    role = settings::network::Role::SERVER;
    assumeRole();
  }

  void NetInterface::switchToClient() {
    role = settings::network::Role::CLIENT;
    assumeRole();
  }

  void NetInterface::tick() {
    switch(role) {
      case settings::network::Role::SERVER: {
        server->tick(syncPackets);
      } break;
      case settings::network::Role::CLIENT: {
        client->tick(syncPackets);
      } break;
      default: break;
    }
  }

  void NetInterface::send(BitStream &stream) {
    switch(role) {
      case settings::network::Role::SERVER: {
        server->send(stream);
      } break;
      case settings::network::Role::CLIENT: {
        client->send(stream);
      } break;
      default: break;
    }
  }

  const std::vector<SLNet::Packet *> & NetInterface::getSyncPackets() {
    return syncPackets;
  }

  void NetInterface::discardSyncPackets() {
    for (auto pack : syncPackets) {
      switch(role) {
        case settings::network::Role::SERVER: {
          server->deallocatePacket(pack);
        } break;
        case settings::network::Role::CLIENT: {
          client->deallocatePacket(pack);
        } break;
        default: break;
      }
    }
    syncPackets.clear();
  }

}
