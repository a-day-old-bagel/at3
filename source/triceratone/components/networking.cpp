
#include "networking.hpp"

using namespace ezecs;
using namespace SLNet;

namespace at3 {

  /*
   *
   * WIP WIP WIP WIP WIP WIP
   * TENTATIVE PLANS.....
   *
   *
   * Create new entity if in read mode and id is non-zero (create at that id). Only a client should do this.
   *
   * Evaluate request if in read mode and id is zero (should only happe to a server), then create the entity and
   * relay the request to clients with the correct ID included so that they create it too. Maybe do this by recursing
   * with rw set to true (write mode).
   *
   * If in write mode and id is zero, send request to server but do not create entity yourself yet (clients should
   * be the only ones to do this)
   *
   * If in write mode and id is nonzero, create components assuming entity exists and send non-zero-id request to
   * clients so that they make it too (only a server should do this).
   */

  // TODO: can some of this be moved into the ezecs generation code?
  // Maybe provide the State object with callbacks to actual serialization functions (like BitStream.serialize)
  // so that ezecs can remain network-library-agnostic.


  void writeEntityRequestHeader(BitStream &stream) {
    stream.Write((MessageID)ID_USER_PACKET_ECS_REQUEST_ENUM);
    stream.WriteBitsFromIntegerRange((uint8_t)REQ_ENTITY_OP, (uint8_t)0, (uint8_t)(REQ_END_ENUM - 1), false);
    stream.WriteBitsFromIntegerRange((uint8_t)OP_CREATE, (uint8_t)0, (uint8_t)(OP_END_ENUM - 1), false);
  }

  entityId serializeEntityCreationRequest(bool rw, BitStream &stream, State &state, entityId id,
                                          std::vector<std::unique_ptr<BitStream>> *compStreams) {
    stream.Serialize(rw, id);
    if (!rw) {  // reading a request
      if (id) { // Receive a remote server request to change the local client ECS, which gets fulfilled.
        // If outgoing ECS request server messages are ordered, we shouldn't need to check for inappropriate id.
        // That means this loop should always do exactly one iteration, but if it doesn't, we need to fix it.
        int i = 0;
        for (; !state.getComponents(id) && i < 10; ++i) {  // times out after 10 loops if there IS an anomaly
          state.createEntity();
        }
        if (i != 1) { // TODO: make this an assert?
          fprintf(stderr, "Anomaly found while processing entity creation request! Check logic!\n");
        }
        serializeComponentCreationRequest(false, stream, state, id);
      } else { // Receive a client's request to update all networked ECS's. The server fulfills it and rebroadcasts.
        AT3_ASSERT(settings::network::role == settings::network::SERVER, "POOP\n");
        state.createEntity(&id);
        serializeComponentCreationRequest(false, stream, state, id);
        stream.Reset(); // Rewrite the passed-in stream to be the rebroadcasted command that includes the new ID.
        serializeEntityCreationRequest(true, stream, state, id); // Recurse to accomplish this.
      }
    } else {  // writing a request
      if (id) {
        if (state.getComponents(id)) {
          serializeComponentCreationRequest(true, stream, state, id);
        } else {  // TODO: make this an assert?
          stream.Reset();
          fprintf(stderr, "Attempted to write entity creation request using invalid ID!\n");
        }
      } else {  // This is a request made without actually adding anything to your own ECS (a client does this)
        if (compStreams) {
          serializeComponentCreationRequest(false, stream, state, 0, compStreams);
        } else {
          stream.Reset();
          fprintf(stderr, "Passed a nullptr as compStreams when creating an unfulfilled ECS request!\n");
        }
      }
    }
    return id;
  }

  void serializeComponentCreationRequest(bool rw, BitStream &stream, State &state, entityId id,
                                         std::vector<std::unique_ptr<BitStream>> *compStreams) {
    serializePlacement(rw, &stream, state, id, compStreams);
    serializeSceneNode(rw, &stream, state, id, compStreams);
    serializeTransformFunction(rw, &stream, state, id, compStreams);
    serializeMesh(rw, &stream, state, id, compStreams);
    serializeCamera(rw, &stream, state, id, compStreams);
    serializePhysics(rw, &stream, state, id, compStreams);
    serializeNetworkedPhysics(rw, &stream, state, id, compStreams);
    serializePyramidControls(rw, &stream, state, id, compStreams);
    serializeTrackControls(rw, &stream, state, id, compStreams);
    serializeWalkControls(rw, &stream, state, id, compStreams);
    serializeFreeControls(rw, &stream, state, id, compStreams);
    serializeMouseControls(rw, &stream, state, id, compStreams);
  }

  bool hasComponent(bool rw, BitStream *stream, State &state, entityId id,
                    const compMask &type) {
    bool hasComp;
    if (rw) { hasComp = (bool) (state.getComponents(id) & type); }
    stream->SerializeCompressed(rw, hasComp);
    return hasComp;
  }

  void serializePlacement(bool rw, BitStream *stream, State &state, entityId id,
                          std::vector<std::unique_ptr<BitStream>> *compStreams, const glm::mat4 *mat) {
    BitStream *inStream = compStreams ? (*compStreams)[0].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->Write(*mat);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream->WriteCompressed((bool) true);
        stream->Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream->WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, PLACEMENT)) {
      if (rw) {
        Placement *placement;
        state.get_Placement(id, &placement);
        stream->Write(placement->mat);
      } else {
        glm::mat4 inMat;
        stream->Read(inMat);
        state.add_Placement(id, inMat);
      }
    }
  }

  void serializeSceneNode(bool rw, BitStream *stream, State &state, entityId id,
                          std::vector<std::unique_ptr<BitStream>> *compStreams, const entityId *parentId) {
    BitStream *inStream = compStreams ? (*compStreams)[1].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->Write(*parentId);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream->WriteCompressed((bool) true);
        stream->Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream->WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, SCENENODE)) {
      if (rw) {
        SceneNode *sceneNode;
        state.get_SceneNode(id, &sceneNode);
        stream->Write(sceneNode->parentId);
      } else {
        entityId parentIdIn;
        stream->Read(parentIdIn);
        state.add_SceneNode(id, parentIdIn);
      }
    }
  }

  void serializeTransformFunction(bool rw, BitStream *stream, State &state, entityId id,
                                  std::vector<std::unique_ptr<BitStream>> *compStreams, const uint8_t *transFuncId) {
    BitStream *inStream = compStreams ? (*compStreams)[2].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->Write(*transFuncId);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream->WriteCompressed((bool) true);
        stream->Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream->WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, TRANSFORMFUNCTION)) {
      if (rw) {
        TransformFunction *transformFunction;
        state.get_TransformFunction(id, &transformFunction);
        stream->Write(transformFunction->transFuncId);
      } else {
        uint8_t transFuncIdIn;
        stream->Read(transFuncIdIn);
        state.add_TransformFunction(id, transFuncIdIn);
      }
    }
  }

  void serializeMesh(bool rw, BitStream *stream, State &state, entityId id,
                     std::vector<std::unique_ptr<BitStream>> *compStreams, const std::string *meshFileName,
                     const std::string *textureFileName) {
    BitStream *inStream = compStreams ? (*compStreams)[3].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        StringCompressor::Instance()->EncodeString(meshFileName->c_str(), 256, inStream);
        StringCompressor::Instance()->EncodeString(textureFileName->c_str(), 256, inStream);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream->WriteCompressed((bool) true);
        stream->Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream->WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, MESH)) {
      if (rw) {
        Mesh *mesh;
        state.get_Mesh(id, &mesh);
        StringCompressor::Instance()->EncodeString(mesh->meshFileName.c_str(), 256, stream);
        StringCompressor::Instance()->EncodeString(mesh->textureFileName.c_str(), 256, stream);
      } else {
        char meshFileNameIn[256], textureFileNameIn[256];
        StringCompressor::Instance()->DecodeString(meshFileNameIn, 256, stream);
        StringCompressor::Instance()->DecodeString(textureFileNameIn, 256, stream);
        state.add_Mesh(id, std::string(meshFileNameIn), std::string(textureFileNameIn));
      }
    }
  }

  void serializeCamera(bool rw, BitStream *stream, State &state, entityId id,
                       std::vector<std::unique_ptr<BitStream>> *compStreams, const float *fovY,
                       const float *nearPlane, const float *farPlane) {
    BitStream *inStream = compStreams ? (*compStreams)[4].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->Write(*fovY);
        inStream->Write(*nearPlane);
        inStream->Write(*farPlane);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream->WriteCompressed((bool) true);
        stream->Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream->WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, CAMERA)) {
      if (rw) {
        Camera *camera;
        state.get_Camera(id, &camera);
        stream->Write(camera->fovY);
        stream->Write(camera->nearPlane);
        stream->Write(camera->farPlane);
      } else {
        float fovYIn, nearPlaneIn, farPlaneIn;
        stream->Read(fovYIn);
        stream->Read(nearPlaneIn);
        stream->Read(farPlaneIn);
        state.add_Camera(id, fovYIn, nearPlaneIn, farPlaneIn);
      }
    }
  }

  void serializePhysics(bool rw, BitStream *stream, State &state, entityId id,
                        std::vector<std::unique_ptr<BitStream>> *compStreams, float *mass,
                        std::shared_ptr<void> *initData, int *useCase) {
    BitStream *inStream = compStreams ? (*compStreams)[5].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
//        Physics::serialize<BitStream>(true, *inStream, *mass, *initData, *useCase);
        Physics::serialize(true, *inStream, *mass, *initData, *useCase);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream->WriteCompressed((bool) true);
        stream->Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream->WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, PHYSICS)) {
      if (rw) {
        Physics *physics;
        state.get_Physics(id, &physics);
//        physics->serialize<BitStream>(true, stream);
        physics->serialize(true, *stream);
      } else {
        Physics physics(0, nullptr, Physics::INVALID);
        physics.serialize(false, *stream);
        state.add_Physics(id, physics.mass, physics.initData, (Physics::UseCase)physics.useCase);
      }
    }
  }

  void serializeNetworkedPhysics(bool rw, BitStream *stream, State &state, entityId id,
                                 std::vector<std::unique_ptr<BitStream>> *compStreams) {
    BitStream *inStream = compStreams ? (*compStreams)[6].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->WriteCompressed((bool) false); // placeholder - takes no constructor arguments
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream->WriteCompressed((bool) true);
        stream->Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream->WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, NETWORKEDPHYSICS)) {
      if (rw) {
      } else {
        state.add_NetworkedPhysics(id);
      }
    }
  }

  void serializePyramidControls(bool rw, BitStream *stream, State &state, entityId id,
                                std::vector<std::unique_ptr<BitStream>> *compStreams, const entityId *mouseCtrlId) {
    BitStream *inStream = compStreams ? (*compStreams)[7].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->Write(*mouseCtrlId);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream->WriteCompressed((bool) true);
        stream->Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream->WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, PYRAMIDCONTROLS)) {
      if (rw) {
        PyramidControls *pyramidControls;
        state.get_PyramidControls(id, &pyramidControls);
        stream->Write(pyramidControls->mouseCtrlId);
      } else {
        entityId mouseControlIdIn;
        stream->Read(mouseControlIdIn);
        state.add_PyramidControls(id, mouseControlIdIn);
      }
    }
  }

  void serializeTrackControls(bool rw, BitStream *stream, State &state, entityId id,
                              std::vector<std::unique_ptr<BitStream>> *compStreams) {
    BitStream *inStream = compStreams ? (*compStreams)[8].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->WriteCompressed((bool) false); // placeholder - takes no constructor arguments
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream->WriteCompressed((bool) true);
        stream->Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream->WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, TRACKCONTROLS)) {
      if (rw) {
      } else {
        state.add_TrackControls(id);
      }
    }
  }

  void serializeWalkControls(bool rw, BitStream *stream, State &state, entityId id,
                               std::vector<std::unique_ptr<BitStream>> *compStreams, const entityId *mouseCtrlId) {
    BitStream *inStream = compStreams ? (*compStreams)[9].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->Write(*mouseCtrlId);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream->WriteCompressed((bool) true);
        stream->Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream->WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, WALKCONTROLS)) {
      if (rw) {
        WalkControls *walkControls;
        state.get_WalkControls(id, &walkControls);
        stream->Write(walkControls->mouseCtrlId);
      } else {
        entityId mouseControlIdIn;
        stream->Read(mouseControlIdIn);
        state.add_WalkControls(id, mouseControlIdIn);
      }
    }
  }

  void serializeFreeControls(bool rw, BitStream *stream, State &state, entityId id,
                             std::vector<std::unique_ptr<BitStream>> *compStreams, const entityId *mouseCtrlId) {
    BitStream *inStream = compStreams ? (*compStreams)[10].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->Write(*mouseCtrlId);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream->WriteCompressed((bool) true);
        stream->Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream->WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, FREECONTROLS)) {
      if (rw) {
        FreeControls *freeControls;
        state.get_FreeControls(id, &freeControls);
        stream->Write(freeControls->mouseCtrlId);
      } else {
        entityId mouseControlIdIn;
        stream->Read(mouseControlIdIn);
        state.add_FreeControls(id, mouseControlIdIn);
      }
    }
  }

  void serializeMouseControls(bool rw, BitStream *stream, State &state, entityId id,
                              std::vector<std::unique_ptr<BitStream>> *compStreams, const bool *invertedX,
                              const bool *invertedY) {
    BitStream *inStream = compStreams ? (*compStreams)[11].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->WriteCompressed(*invertedX);
        inStream->WriteCompressed(*invertedY);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream->WriteCompressed((bool) true);
        stream->Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream->WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, MOUSECONTROLS)) {
      if (rw) {
        MouseControls *mouseControls;
        state.get_MouseControls(id, &mouseControls);
        stream->WriteCompressed(mouseControls->invertedX);
        stream->WriteCompressed(mouseControls->invertedY);
      } else {
        bool invertedXIn, invertedYIn;
        stream->ReadCompressed(invertedXIn);
        stream->ReadCompressed(invertedYIn);
        state.add_MouseControls(id, invertedXIn, invertedYIn);
      }
    }
  }
}
