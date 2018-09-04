
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
        if (RakNetGUID::ToUint32(server->getGuid()) < 2) {
          fprintf(stderr, "Attempted server creation using reserved GUID - recreating.\n");
          assumeRole(); // Recurse until guid is acceptable.
        }
      } break;
      case settings::network::Role::CLIENT: {
        server.reset();
        client = std::make_unique<Client>();
        if (RakNetGUID::ToUint32(client->getGuid()) < 2) {
          fprintf(stderr, "Attempted client creation using reserved GUID - recreating.\n");
          assumeRole(); // Recurse until guid is acceptable.
        }
      } break;
      default: {
        server.reset();
        client.reset();
      }
    }
  }

  void NetInterface::discardPacketCollection(std::vector<SLNet::Packet *> &packets) {
    for (auto pack : packets) {
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
    packets.clear();
  }

  uint32_t NetInterface::getRole() const {
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
        server->tick(requestPackets, syncPackets, freshConnections);
      } break;
      case settings::network::Role::CLIENT: {
        client->tick(requestPackets, syncPackets);
      } break;
      default: break;
    }
  }

  void NetInterface::send(const BitStream &stream, PacketPriority priority,
                          PacketReliability reliability, char channel) {
    switch(role) {
      case settings::network::Role::SERVER: {
        server->send(stream, priority, reliability, channel);
      } break;
      case settings::network::Role::CLIENT: {
        client->send(stream, priority, reliability, channel);
      } break;
      default: break;
    }
  }

  void NetInterface::sendTo(const SLNet::BitStream &stream, const SLNet::AddressOrGUID &target, PacketPriority priority,
                            PacketReliability reliability, char channel) {
    switch(role) {
      case settings::network::Role::SERVER: {
        server->sendTo(stream, target, priority, reliability, channel);
      } break;
      case settings::network::Role::CLIENT: {
        client->sendTo(stream, target, priority, reliability, channel);
      } break;
      default: break;
    }
  }

  const std::vector<SLNet::Packet *> & NetInterface::getRequestPackets() const {
    return requestPackets;
  }

  void NetInterface::discardRequestPackets() {
    discardPacketCollection(requestPackets);
  }

  const std::vector<SLNet::Packet *> & NetInterface::getSyncPackets() const {
    return syncPackets;
  }

  void NetInterface::discardSyncPackets() {
    discardPacketCollection(syncPackets);
  }

  const std::vector<SLNet::AddressOrGUID> &NetInterface::getFreshConnections() const {
    return freshConnections;
  }

  void NetInterface::discardFreshConnections() {
    freshConnections.clear();
  }

  const DataStructures::List<SLNet::SystemAddress> & NetInterface::getClientAddresses() const {
    switch(role) {
      case settings::network::Role::SERVER: { return server->getClientAddresses(); }
      case settings::network::Role::CLIENT:
      default: { return emptyAddresses; }
    }
  }

  const DataStructures::List<SLNet::RakNetGUID> & NetInterface::getClientGuids() const {
    switch(role) {
      case settings::network::Role::SERVER: { return server->getClientGuids(); }
      case settings::network::Role::CLIENT:
      default: { return emptyGuids; }
    }
  }

  uint32_t NetInterface::getClientSum() const {
    switch(role) {
      case settings::network::Role::SERVER: { return server->getClientSum(); }
      case settings::network::Role::CLIENT:
      default: { return 0; }
    }
  }

}
