
#include "ecsNetworking.hpp"

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
          serializeComponentCreationRequest(true, stream, state, 0, compStreams);
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
    serializePlacement(rw, stream, state, id, compStreams);
    serializeSceneNode(rw, stream, state, id, compStreams);
    serializeTransformFunction(rw, stream, state, id, compStreams);
    serializeMesh(rw, stream, state, id, compStreams);
    serializeCamera(rw, stream, state, id, compStreams);
    serializePhysics(rw, stream, state, id, compStreams);
    serializeNetworkedPhysics(rw, stream, state, id, compStreams);
    serializePyramidControls(rw, stream, state, id, compStreams);
    serializeTrackControls(rw, stream, state, id, compStreams);
    serializePlayerControls(rw, stream, state, id, compStreams);
    serializeFreeControls(rw, stream, state, id, compStreams);
    serializeMouseControls(rw, stream, state, id, compStreams);
  }

  bool hasComponent(bool rw, BitStream &stream, State &state, entityId id,
                    const compMask &type) {
    bool hasComp;
    if (rw) { hasComp = (bool) (state.getComponents(id) & type); }
    stream.SerializeCompressed(rw, hasComp);
    return hasComp;
  }

  void serializePlacement(bool rw, BitStream &stream, State &state, entityId id,
                          std::vector<std::unique_ptr<BitStream>> *compStreams, const glm::mat4 *mat) {
    BitStream *inStream = compStreams ? (*compStreams)[PLACEMENT].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->Write(*mat);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream.WriteCompressed((bool) true);
        stream.Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream.WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, PLACEMENT)) {

    }
  }

  void serializeSceneNode(bool rw, BitStream &stream, State &state, entityId id,
                          std::vector<std::unique_ptr<BitStream>> *compStreams, const entityId *parentId) {
    BitStream *inStream = compStreams ? (*compStreams)[SCENENODE].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->Write(*parentId);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream.WriteCompressed((bool) true);
        stream.Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream.WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, SCENENODE)) {

    }
  }

  void serializeTransformFunction(bool rw, BitStream &stream, State &state, entityId id,
                                  std::vector<std::unique_ptr<BitStream>> *compStreams) {
    BitStream *inStream = compStreams ? (*compStreams)[TRANSFORMFUNCTION].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
//        inStream->  // FIXME: use numbers or strings to identify transform functions (probably numbers)
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream.WriteCompressed((bool) true);
        stream.Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream.WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, TRANSFORMFUNCTION)) {

    }
  }

  void serializeMesh(bool rw, BitStream &stream, State &state, entityId id,
                     std::vector<std::unique_ptr<BitStream>> *compStreams, const std::string *meshFileName,
                     const std::string *textureFileName) {
    BitStream *inStream = compStreams ? (*compStreams)[MESH].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->Write(*meshFileName);
        inStream->Write(*textureFileName);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream.WriteCompressed((bool) true);
        stream.Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream.WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, MESH)) {

    }
  }

  void serializeCamera(bool rw, BitStream &stream, State &state, entityId id,
                       std::vector<std::unique_ptr<BitStream>> *compStreams, const float *fovY,
                       const float *nearPlane, const float *farPlane) {
    BitStream *inStream = compStreams ? (*compStreams)[CAMERA].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->Write(*fovY);
        inStream->Write(*nearPlane);
        inStream->Write(*farPlane);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream.WriteCompressed((bool) true);
        stream.Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream.WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, CAMERA)) {

    }
  }

  void serializePhysics(bool rw, BitStream &stream, State &state, entityId id,
                        std::vector<std::unique_ptr<BitStream>> *compStreams) {
    BitStream *inStream = compStreams ? (*compStreams)[PHYSICS].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
//        inStream->  // FIXME
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream.WriteCompressed((bool) true);
        stream.Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream.WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, PHYSICS)) {

    }
  }

  void serializeNetworkedPhysics(bool rw, BitStream &stream, State &state, entityId id,
                                 std::vector<std::unique_ptr<BitStream>> *compStreams) {
    BitStream *inStream = compStreams ? (*compStreams)[NETWORKEDPHYSICS].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->WriteCompressed((bool) false); // placeholder - takes no constructor arguments
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream.WriteCompressed((bool) true);
        stream.Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream.WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, NETWORKEDPHYSICS)) {

    }
  }

  void serializePyramidControls(bool rw, BitStream &stream, State &state, entityId id,
                                std::vector<std::unique_ptr<BitStream>> *compStreams, const entityId *mouseCtrlId) {
    BitStream *inStream = compStreams ? (*compStreams)[PYRAMIDCONTROLS].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->Write(*mouseCtrlId);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream.WriteCompressed((bool) true);
        stream.Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream.WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, PYRAMIDCONTROLS)) {

    }
  }

  void serializeTrackControls(bool rw, BitStream &stream, State &state, entityId id,
                              std::vector<std::unique_ptr<BitStream>> *compStreams) {
    BitStream *inStream = compStreams ? (*compStreams)[TRACKCONTROLS].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->WriteCompressed((bool) false); // placeholder - takes no constructor arguments
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream.WriteCompressed((bool) true);
        stream.Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream.WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, TRACKCONTROLS)) {

    }
  }

  void serializePlayerControls(bool rw, BitStream &stream, State &state, entityId id,
                               std::vector<std::unique_ptr<BitStream>> *compStreams, const entityId *mouseCtrlId) {
    BitStream *inStream = compStreams ? (*compStreams)[PLAYERCONTROLS].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->Write(*mouseCtrlId);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream.WriteCompressed((bool) true);
        stream.Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream.WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, PLAYERCONTROLS)) {

    }
  }

  void serializeFreeControls(bool rw, BitStream &stream, State &state, entityId id,
                             std::vector<std::unique_ptr<BitStream>> *compStreams, const entityId *mouseCtrlId) {
    BitStream *inStream = compStreams ? (*compStreams)[FREECONTROLS].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->Write(*mouseCtrlId);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream.WriteCompressed((bool) true);
        stream.Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream.WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, FREECONTROLS)) {

    }
  }

  void serializeMouseControls(bool rw, BitStream &stream, State &state, entityId id,
                              std::vector<std::unique_ptr<BitStream>> *compStreams, const bool *invertedX,
                              const bool *invertedY) {
    BitStream *inStream = compStreams ? (*compStreams)[MOUSECONTROLS].get() : nullptr;
    if (inStream) { // If component constructor data is present
      if (rw) { // Write mode in this case means writing component constructor arguments to inStream
        inStream->WriteCompressed(*invertedX);
        inStream->WriteCompressed(*invertedY);
      } else if (inStream->GetNumberOfBitsUsed()) { // Read mode in this case means copying from inStream to stream.
        stream.WriteCompressed((bool) true);
        stream.Write(*inStream);
      } else { // This happens when instream has no entry for a given component's constructor arguments.
        stream.WriteCompressed((bool) false);
      }
    } else if (hasComponent(rw, stream, state, id, MOUSECONTROLS)) {

    }
  }
}
