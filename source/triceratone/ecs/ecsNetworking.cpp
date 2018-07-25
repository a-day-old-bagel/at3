
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



  ezecs::entityId serializeEntityCreationRequest(bool rw, BitStream &stream, State &state, entityId id) {
    stream.Serialize(rw, id);
    if ( ! rw) { state.createEntity(&id); }
    return serializeComponentCreationRequest(rw, stream, state, id);
  }

  ezecs::entityId serializeComponentCreationRequest(bool rw, BitStream &stream, State &state, entityId id) {
    compMask components;
    if (rw) { components = state.getComponents(id); }
    stream.SerializeBitsFromIntegerRange(rw, components, 0u, (uint32_t)MAX_COMPONENT_ENUM, false);

    if (components & (uint32_t)PLACEMENT) {
//      state.add_Placement()
    }
    if (components & (uint32_t)TRANSFORMFUNCTION) {
//      state.add_TransformFunction()
    }
    if (components & (uint32_t)PERSPECTIVE) {
//      state.add_Perspective()
    }
    if (components & (uint32_t)PHYSICS) {
//      state.add_Physics()
    }
    if (components & (uint32_t)PYRAMIDCONTROLS) {
//      state.add_PyramidControls()
    }
    if (components & (uint32_t)TRACKCONTROLS) {
//      state.add_TrackControls()
    }
    if (components & (uint32_t)PLAYERCONTROLS) {
//      state.add_PlayerControls()
    }
    if (components & (uint32_t)FREECONTROLS) {
//      state.add_FreeControls()
    }
    if (components & (uint32_t)MOUSECONTROLS) {
//      state.add_MouseControls()
    }

    return id;
  }
}
