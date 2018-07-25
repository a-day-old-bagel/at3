#pragma once

#include "netInterface.hpp"
#include "userPacketEnums.hpp"
#include "ezecs.hpp"

namespace at3 {
  enum FurtherPacketEnums {
      ID_SYNC_PHYSICS = ID_USER_PACKET_SYNC_ENUM,
      ID_SYNC_WALKCONTROLS,
      ID_SYNC_PYRAMIDCONTROLS,
      ID_SYNC_TRACKCONTROLS,
      ID_SYNC_FREECONTROLS,

      ID_USER_PACKET_END_ENUM
  };
  enum RequestSpecifierEnums {
      REQ_ENTITY_OP = 0,
      REQ_COMPONENT_OP,

      REQ_END_ENUM
  };
  enum OperationSpecifierEnums {
      OP_CREATE = 0,
      OP_DESTROY,

      OP_END_ENUM
  };
  enum ChannelEnums {
      CH_CONTROL_SYNC = 0,
      CH_PHYSICS_SYNC,
      CH_ECS_REQUEST
  };

  ezecs::entityId serializeEntityCreationRequest(
      bool rw, SLNet::BitStream &stream, ezecs::State &state,
      ezecs::entityId id = 0);
  ezecs::entityId serializeComponentCreationRequest(
      bool rw, SLNet::BitStream &stream, ezecs::State &state,
      ezecs::entityId id = 0);
}
