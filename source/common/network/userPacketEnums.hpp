#pragma once

#include "MessageIdentifiers.h"

namespace at3 {
  enum PacketEnums {
      ID_USER_PACKET_ECS_REQUEST_ENUM = ID_USER_PACKET_ENUM,
      ID_USER_PACKET_ECS_RESPONSE_ENUM,
      ID_USER_PACKET_ADMIN_COMMAND,
      ID_USER_PACKET_CLIENT_READY,

      ID_USER_PACKET_SYNC_ENUM
  };
}
