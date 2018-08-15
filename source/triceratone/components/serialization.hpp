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
  enum CommandSpecifierEnums {
    CMD_KICK = 0,
    CMD_ASSIGN_PLAYER_ID,

    CMD_END_ENUM
  };
  enum ChannelEnums {
    CH_ZERO_UNUSED = 0,
    CH_ADMIN_MESSAGE,
    CH_ECS_REQUEST,
    CH_CONTROL_SYNC,
    CH_PHYSICS_SYNC,
  };

  void writeEntityRequestHeader(SLNet::BitStream &stream);
  ezecs::entityId serializeEntityCreationRequest(
      bool rw, SLNet::BitStream &stream, ezecs::State &state, ezecs::entityId id = 0,
      std::vector<std::unique_ptr<SLNet::BitStream>> *compStreams = nullptr);
  void serializeComponentCreationRequest(
      bool rw, SLNet::BitStream &stream, ezecs::State &state, ezecs::entityId id = 0,
      std::vector<std::unique_ptr<SLNet::BitStream>> *compStreams = nullptr);

  void serializePlacement(bool rw, SLNet::BitStream *stream, ezecs::State &state, ezecs::entityId id,
                          std::vector<std::unique_ptr<SLNet::BitStream>> *compStreams = nullptr,
                          const glm::mat4 *mat = nullptr);
  void serializeSceneNode(bool rw, SLNet::BitStream *stream, ezecs::State &state, ezecs::entityId id,
                          std::vector<std::unique_ptr<SLNet::BitStream>> *compStreams = nullptr,
                          const ezecs::entityId *parentId = nullptr);
  void serializeTransformFunction(bool rw, SLNet::BitStream *stream, ezecs::State &state, ezecs::entityId id,
                                  std::vector<std::unique_ptr<SLNet::BitStream>> *compStreams = nullptr,
                                  const uint8_t *transFuncId = nullptr);
  void serializeMesh(bool rw, SLNet::BitStream *stream, ezecs::State &state, ezecs::entityId id,
                     std::vector<std::unique_ptr<SLNet::BitStream>> *compStreams = nullptr,
                     const std::string *meshFileName = nullptr,
                     const std::string *textureFileName = nullptr);
  void serializeCamera(bool rw, SLNet::BitStream *stream, ezecs::State &state, ezecs::entityId id,
                       std::vector<std::unique_ptr<SLNet::BitStream>> *compStreams = nullptr,
                       const float *fovY = nullptr,
                       const float *nearPlane = nullptr,
                       const float *farPlane = nullptr);
  void serializePhysics(bool rw, SLNet::BitStream *stream, ezecs::State &state, ezecs::entityId id,
                        std::vector<std::unique_ptr<SLNet::BitStream>> *compStreams = nullptr,
                        float *mass = nullptr,
                        std::shared_ptr<void> *data = nullptr,
                        int *useCase = nullptr);
  void serializeNetworking(bool rw, SLNet::BitStream *stream, ezecs::State &state, ezecs::entityId id,
                           std::vector<std::unique_ptr<SLNet::BitStream>> *compStreams = nullptr);
  void serializePyramidControls(bool rw, SLNet::BitStream *stream, ezecs::State &state, ezecs::entityId id,
                                std::vector<std::unique_ptr<SLNet::BitStream>> *compStreams = nullptr,
                                const ezecs::entityId *mouseCtrlId = nullptr);
  void serializeTrackControls(bool rw, SLNet::BitStream *stream, ezecs::State &state, ezecs::entityId id,
                              std::vector<std::unique_ptr<SLNet::BitStream>> *compStreams = nullptr);
  void serializeWalkControls(bool rw, SLNet::BitStream *stream, ezecs::State &state, ezecs::entityId id,
                             std::vector<std::unique_ptr<SLNet::BitStream>> *compStreams = nullptr,
                             const ezecs::entityId *mouseCtrlId = nullptr);
  void serializeFreeControls(bool rw, SLNet::BitStream *stream, ezecs::State &state, ezecs::entityId id,
                             std::vector<std::unique_ptr<SLNet::BitStream>> *compStreams = nullptr,
                             const ezecs::entityId *mouseCtrlId = nullptr);
  void serializeMouseControls(bool rw, SLNet::BitStream *stream, ezecs::State &state, ezecs::entityId id,
                              std::vector<std::unique_ptr<SLNet::BitStream>> *compStreams = nullptr,
                              const bool *invertedX = nullptr,
                              const bool *invertedY = nullptr);
  void serializePlayer(bool rw, SLNet::BitStream *stream, ezecs::State &state, ezecs::entityId id,
                       std::vector<std::unique_ptr<SLNet::BitStream>> *compStreams = nullptr,
                       ezecs::entityId *free = nullptr,
                       ezecs::entityId *walk = nullptr,
                       ezecs::entityId *pyramid = nullptr,
                       ezecs::entityId *track = nullptr);
}
