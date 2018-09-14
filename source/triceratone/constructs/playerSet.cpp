
#include "playerSet.hpp"
#include "serialization.hpp"

#include "freeCam.hpp"
#include "walker.hpp"
#include "pyramid.hpp"
#include "duneBuggy.hpp"

using namespace ezecs;
using namespace rtu::topics;

namespace at3 {
  namespace PlayerSet {
    entityId create(State &state, EntityComponentSystemInterface &ecs) {
      entityId playerId = 0;
      // This component will be sent to the network once the Scene System picks it up in SceneSystem::onDiscoverPlayer.
      // This is done instead of using the ecs functions because onDiscoverPlayer creates the avatars and stores their
      // ids inside the player component before the player component is sent out.
      state.createEntity(&playerId);
      state.addNetworking(playerId);
      state.addPlayer(playerId, 0, 0, 0, 0); // These will be filled in SceneSystem::onDiscoverPlayer.
      if (settings::network::role == settings::network::SERVER) {
        // TODO: make the player component just sit in the freeCam
        ecs.broadcastManualEntity(playerId);
        Player * player;
        state.getPlayer(playerId, &player);
        FreeCam::broadcastLatest(state, ecs);
        Walker::broadcastLatest(state, ecs);
        Pyramid::broadcastLatest(state, ecs);
        DuneBuggy::broadcastLatest(state, ecs);
      }
      return playerId;
    }
    void fill(State &state, const entityId & id, EntityComponentSystemInterface &ecs, const glm::mat4 &mat) {
      Player *player;
      state.getPlayer(id, &player);

      // FIXME: until the local-end temporary IDS are set up (if ever), clients MUST NOT create any entities here.
      // currently, that won't happen under normal circumstances since a server creates all the control entities before
      // a client can reach this code. But I want to make sure for debugging purposes (even in release builds)
      bool hasCreatedEntity = false;

      // TODO: if player->free is nonzero, should also check if it has the correct components? Or just let it crash?
      if ( ! player->free) {
        // Create the player component at the same id as the freeCam, because why not?
        player->free = FreeCam::create(state, mat);
        hasCreatedEntity = true;
      }
      if ( ! player->walk) {
        player->walk = Walker::create(state, mat);
        hasCreatedEntity = true;
      }
      if ( ! player->pyramid) {
        player->pyramid = Pyramid::create(state, mat);
        hasCreatedEntity = true;
      }
      if ( ! player->track) {
        player->track = DuneBuggy::create(state, mat);
        hasCreatedEntity = true;
      }
      if (hasCreatedEntity && settings::network::role == settings::network::CLIENT) {
        fprintf(stderr, "Client accidentally made some stuff. Your game is now horribly broken.\n");
      }
    }
  }
}
