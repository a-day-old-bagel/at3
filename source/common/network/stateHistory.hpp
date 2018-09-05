#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include <SDL_timer.h>

#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakNetTypes.h"

#include "ouroboros.hpp"

namespace at3 {



  // TODO: FIXME: please make argument order consistent: put index first for all functions or something.


  /**
   * Hash function for RakNet GUIDs.
   */
  struct GuidHash {
    std::size_t operator () (const SLNet::RakNetGUID &guid) const {
      return std::hash<uint64_t>{}(guid.g);
    }
  };


  /**
   * A state snapshot is described by some state information plus some optional input information to be applied to that
   * state when it is next processed. This can be used for, for example, physics state information with additional
   * control input information that doesn't fit within the bounds of physics state descriptions. If your physics
   * state is described by positions and velocities, for example, you might not want to bake your control information
   * into that state in the snap shot, either for encapsulation reasons or because you need to do additional processing
   * on the input data before it gets applied to the state data.
   *
   * A snapshot doesn't need to be complete, either. You might just use one to hold deltas or a subset of
   * authoritative corrections.
   */
  template <typename indexType>
  class StateSnapShot {
    public:

      uint32_t time = 0;

      /*
       * If a complete state stream is written first, and then authoritative updates written after that, the original
       * incorrect state can be restored with the authoritative changes made directly after. This means there will be
       * a redundant and useless entry in the original complete state for entities that have an authoritative
       * correction later on in the stream, but that might be the cleanest way to do this for now.
       *
       * Once a client has received even just one authoritative stream, the StateSnapShot will be considered to have an
       * authoritatively-approved state. That authoritative stream doesn't need to be a complete state
       * record, since just part of the state may be sent over the network at a time as incremental corrections.
       * stateIsAuthorized just indicates that an authority has had *some* say in this current snapshot.
       */
      void addStateData(const SLNet::BitStream &data, bool authoritative = false) {
        state.Write(data);
        if (authoritative) {
          stateIsAuthorized = true;
        }
      }

      /*
       * Once inputs have been given such that the checksum is satisfied, a complete input set will be assumed to have
       * been received and the snapshot may be treated as authoritative, assuming the state is also authorized. So
       * for an authority (like a server), a complete input set is all that's needed. A dependent (like a client) may
       * need to wait for the authority to also send the corresponding authoritative state update(s) before the state
       * can be considered complete.
       */
      void addInputData(const SLNet::BitStream &data, const SLNet::RakNetGUID & guid) {
        if (inputSources) { // if no input sources defined, no checking will occur.
          if (inputSources->count(guid)) {
            if ((*inputSources)[guid]) {
              fprintf(stderr, "Discarded duplicate client input stream at state snapshot %u\n", index);
              return; // don't add more than one stream per client into the snapshot
            }
            (*inputSources)[guid] = true;
          } else {
            fprintf(stderr, "Received client input from unregistered GUID %lu!\n", SLNet::RakNetGUID::ToUint32(guid));
            return; // don't add invalid guid's to the checksum or their inputs to the stream
          }
        }
        input.Write(data);
        inputSum += SLNet::RakNetGUID::ToUint32(guid);
      }

      /*
       * Same as above, but instead of handling an individual guid, you must manage the checksum yourself by calling
       * addToInputChecksum.
       */
      void addInputData(const SLNet::BitStream &data) {
        input.Write(data);
      }

      void addToInputChecksum(uint32_t checksumPart) {
        inputSum += checksumPart;
      }

      const SLNet::BitStream & getState() {
        return state;
      }

      const SLNet::BitStream & getInput() {
        return input;
      }

      /*
       * The server should send a desiredInputSum along with its state updates so that the client can know when it's
       * achieved a complete input set.
       */
      void setInputChecksum(uint32_t sum) {
        desiredInputSum = sum;
      }
      void setAdditionalInputChecksum(uint32_t sum) {
        desiredInputSum += sum;
      }

      /*
       * Alternatively to setInputChecksum, a list of each client's guid can be provided so that the StateSnapShot
       * can keep track of which clients have or have not provided input instead of just knowing when they *all* have.
       * This can be used to kick clients that are lagging too far behind.
       * To this end, you can call getUnfulfilledGuids to know which clients still haven't provided input.
       */
      void setInputSources(const DataStructures::List<SLNet::RakNetGUID> & guids) {
        desiredInputSum = 0;
        inputSources = std::make_unique<std::unordered_map<SLNet::RakNetGUID, bool, GuidHash>>();
        for (uint32_t i = 0; i < guids.Size(); ++i) {
          if (SLNet::RakNetGUID::ToUint32(guids[i]) == 0) {
            fprintf(stderr, "State snapshot received input source with GUID of 0!\n");
          }
          if (inputSources && inputSources->count(guids[i])) {
            fprintf(stderr, "Discarded duplicate input source GUID at state snapshot %u\n", index);
          }
          desiredInputSum += SLNet::RakNetGUID::ToUint32(guids[i]);
          inputSources->emplace(guids[i], false);
        }
      }

      indexType getIndex() {
        return index;
      }

      uint32_t getInputChecksum() {
        return desiredInputSum;
      }

      uint32_t getCurrentChecksum() {
        return inputSum;
      }

      bool isComplete() {
        return stateIsAuthorized && inputSum == desiredInputSum;
      }

      std::vector<SLNet::RakNetGUID> getUnfulfilledInputs() {
        std::vector<SLNet::RakNetGUID> unfulfilledGuids;
        if (inputSources) {
          for (const auto &input : *inputSources) {
            if (input.second) {
              unfulfilledGuids.emplace_back(input.first);
            }
          }
        }
        return unfulfilledGuids;
      }

      /*
       * These act as a constructor/destructor pair when this class is used in the Ouroboros data structure.
       * Unlike a real constructor/destructor, these are not called on the creation/destruction of a StateSnapShot
       * object, but rather upon validation/invalidation by the Ouroboros. An instance of this class within an
       * Ouroboros remains allocated for the duration of the Ouroboros.
       */
      static rtu::Delegate<StateSnapShot&(StateSnapShot&, const indexType&)> init;
      static rtu::Delegate<void(StateSnapShot&, const indexType&)> clear;

    private:

      SLNet::BitStream state;
      SLNet::BitStream input;
      indexType index = 0;
      uint32_t inputSum = 0;
      uint32_t desiredInputSum = 0;
      std::unique_ptr<std::unordered_map<SLNet::RakNetGUID, bool, GuidHash>> inputSources;
      bool stateIsAuthorized = false;

      /*
       * These are used as the targets of the constructor/destructor-like function delegates described by init/clear
       */
      static StateSnapShot & initImpl(StateSnapShot &stateSnapShot, const indexType &index) {
        stateSnapShot.index = index;
        return stateSnapShot;
      }
      static void clearImpl(StateSnapShot &stateSnapShot, const indexType &index) {
        stateSnapShot.state.Reset();
        stateSnapShot.input.Reset();
        if (stateSnapShot.inputSources) {
          stateSnapShot.inputSources->clear();
        }
        stateSnapShot.inputSum = 0;
        stateSnapShot.desiredInputSum = 0;
        stateSnapShot.stateIsAuthorized = false;
      }
  };
  template<typename indexType>
  rtu::Delegate<StateSnapShot<indexType>&(StateSnapShot<indexType>&, const indexType&)> StateSnapShot<indexType>::init =
      RTU_FUNC_DLGT(StateSnapShot::initImpl);
  template<typename indexType>
  rtu::Delegate<void(StateSnapShot<indexType>&, const indexType&)> StateSnapShot<indexType>::clear =
      RTU_FUNC_DLGT(StateSnapShot::clearImpl);






  template <typename indexType, indexType maxStoredStates>
  class StateHistory {
    public:

      StateHistory() : states(StateSnapShot<indexType>::init, StateSnapShot<indexType>::clear) {
        RTU_STATIC_SUB(debugSub, "key_down_f8", StateHistory::debugOuroboros, this);
      }

      bool addToInput(const SLNet::BitStream &input, indexType index, const SLNet::RakNetGUID &id) {
        if (states[index].isComplete()) {
          printf("Adding input to an already complete state %u!\n", index);
        }
        if (states.isValid(index)) {
          if (states.isFull()) {
            states.decaudate();
          }
          states[index].addInputData(input, id);
          // return handlePossibleCollapseToTruth(index);
          checkForCompleteness(index);
          return true;
        } else {
          printf("Attempted to add input to invalid state history index %u!\n", index);
          return false;
        }
      }

      /*
       * Must also call addToInputChecksum if you use this version since it doesnt take the GUID.
       */
      bool addToInput(const SLNet::BitStream &input, indexType index) {
        if (states[index].isComplete()) {
          printf("Adding input to an already complete state %u!\n", index);
        }
        if (states.isValid(index)) {
          if (states.isFull()) {
            states.decaudate();
          }
          states[index].addInputData(input);
          // return handlePossibleCollapseToTruth(index);
          checkForCompleteness(index);
          return true;
        } else {
          printf("\n");
          printf("Attempted to add input to invalid state history index %u!\n", index);
          debugOuroboros();
          printf("\n");
          return false;
        }
      }

      bool addToInputChecksum(indexType index, uint32_t checksumPart) {
        if (states[index].isComplete()) {
          printf("Adding to the input checksum of an already complete state %u!\n", index);
        }
        if (states.isValid(index)) {
          states[index].addToInputChecksum(checksumPart);
          // return handlePossibleCollapseToTruth(index);
          checkForCompleteness(index);
          return true;
        } else {
          printf("\n");
          printf("Attempted to add to the input checksum of invalid state history index %u!\n", index);
          debugOuroboros();
          printf("\n");
          return false;
        }
      }

//#     define AT3_STATE_HISTORY_DEBUG
      bool addToState(const SLNet::BitStream &state, indexType index, bool authoritative = false) {
        if (states[index].isComplete()) {
          printf("Adding state to an already complete state %u!\n", index);
        }
        if (states.isValid(index)) {
          if (states.isFull()) {
#           ifdef AT3_STATE_HISTORY_DEBUG
            printf("\nIS FULL: @@@@\n");
            debugOuroboros();
            states.decaudate();
            debugOuroboros();
#           else //AT3_STATE_HISTORY_DEBUG
            states.decaudate();
#           endif //AT3_STATE_HISTORY_DEBUG
          }
          states[index].addStateData(state, authoritative);
          // return handlePossibleCollapseToTruth(index);
          checkForCompleteness(index);
          return true;
        } else {
          printf("Attempted to add state to invalid state history index %u!\n", index);
          return false;
        }
      }
#     undef AT3_STATE_HISTORY_DEBUG

      bool setInputChecksum(indexType index, uint32_t sum) {
        if (states.isValid(index)) {
          states[index].setInputChecksum(sum);
          // return handlePossibleCollapseToTruth(index);
          checkForCompleteness(index);
          return true;
        } else {
          return false;
        }
      }

      bool setAdditionalInputChecksum(indexType index, uint32_t sum) {
        if (states.isValid(index)) {
          states[index].setAdditionalInputChecksum(sum);
          // return handlePossibleCollapseToTruth(index);
          checkForCompleteness(index);
          return true;
        } else {
          return false;
        }
      }

      void setTime(indexType index, uint32_t time) {
        if (states.isValid(index)) {
          states[index].time = time;
        } else {
          printf("Attempted to set the time for invalid state history index %u!\n", index);
        }
      }

      uint32_t getInputChecksum(indexType index) {
        if (states.isValid(index)) {
          return states[index].getInputChecksum();
        } else {
          return 0;
        }
      }

      bool setInputSources(const DataStructures::List<SLNet::RakNetGUID> &guids, indexType index) {
        if (states.isValid(index)) {
          states[index].setInputSources(guids);
          // return handlePossibleCollapseToTruth(index);
          checkForCompleteness(index);
          return true;
        } else {
          return false;
        }
      }

      bool findTheTruth() {\
        if (completeSnapShotReady) {
          states.decaudate(latestCompletion);
          currentReplayHead = latestCompletion;
          completeSnapShotReady = false;
          replayRunning = true;
          printf("\n");
          printf("truth @ %u with %lu of %lu\n", latestCompletion, (unsigned long)states[latestCompletion].getCurrentChecksum(), (unsigned long)states[latestCompletion].getInputChecksum());
          debugOuroboros();
          printf("\n");
          return true;
        } else {
          return false;
        }
      }

      bool setEmptyIndex(indexType index) {
        return states.setEmptyIndex(index);
      }

      indexType appendIndex() {
        // debugOuroboros();
        return states.capitate().getIndex();
      }

      indexType getLatestIndex() {
        return states[states.getHeadSlot()].getIndex();
      }

      bool isFull() {
        return
        states.isFull();
      }

      bool isReplaying() {
        return replayRunning;
      };

      const SLNet::BitStream & getLatestCompleteState() {
        return states[states.getTailSlot()].getState();
      }

      const SLNet::BitStream & getLatestCompleteInput() {
        return states[states.getTailSlot()].getInput();
      }

      bool getNextReplayStreams(SLNet::BitStream & stateOut, SLNet::BitStream & inputOut) {
        if (replayRunning) {
          stateOut.Write(states[currentReplayHead].getState());
          inputOut.Write(states[currentReplayHead].getInput());
          if (states.isValid(rtu::Ouroboros<StateSnapShot<indexType>, indexType, maxStoredStates>::
              getNext(currentReplayHead)))
          {
            currentReplayHead = rtu::Ouroboros<StateSnapShot<indexType>, indexType, maxStoredStates>::
                getNext(currentReplayHead);
          } else {
            replayRunning = false;
          }
        }
        return replayRunning;
      }

      std::vector<SLNet::RakNetGUID> getUnfulfilledInputs(indexType index) {
        if (states.isValid(index)) {
          return states[index].getUnfulfilledInputs();
        } else {
          return std::vector<SLNet::RakNetGUID>();
        }
      }

      uint32_t getReplayBeginTime() {
        // TODO: once the replay has finished, will there be extra time lost (the time it takes to replay) that won't be considered by Game's dt? Or will that timer take care of it?
        return isReplaying() ? states[latestCompletion].time : 0;
      }



    private:

      rtu::Ouroboros<StateSnapShot<indexType>, indexType, maxStoredStates> states;
      bool completeSnapShotReady = false;
      indexType latestCompletion = 0;
      bool replayRunning = false;
      indexType currentReplayHead = 0;

      void checkForCompleteness(indexType index) {
        if (states[index].isComplete()) {
          if (completeSnapShotReady) {
            if (states.firstIsFresherThanSecond(index, latestCompletion)) {
              latestCompletion = index;
            }
          } else {
            latestCompletion = index;
            completeSnapShotReady = true;
          }
        } else {
          // printf("\n");
          // printf("NO TRUTH @ %u with %u of %u\n", index, states[index].getCurrentChecksum(), states[index].getInputChecksum());
          // debugOuroboros();
          // printf("\n");
        }
      }

      void debugOuroboros() {

        // Test 0
//    bool fill = (bool)(rand() % 2);
//    if (fill && ! states.isFull()) {
//      printf("NEW HEAD: %u\n", states.capitate().getIndex());
//    } else if (fill && states.isFull()) {
//      states.decaudate();
//      printf("NEW TAIL: %u\n", states.getTailSlot());
//    } else if ( ! fill && ! states.isEmpty()) {
//      states.decaudate();
//      printf("NEW TAIL: %u\n", states.getTailSlot());
//    } else if ( ! fill && states.isEmpty()) {
//      printf("NEW HEAD: %u\n", states.capitate().getIndex());
//    }

        // Test 1
//    static bool eat, poop;
//    if (states.isEmpty()) { eat = true; poop = false; }
//    if (states.isFull()) {
//      poop = true;
//      eat = false;
//      states.decaudate();
//      states.capitate();
//    }
//    if (eat) { states.capitate(); }
//    if (poop) { states.decaudate(); }

        // Test 2
//    states.capitate();
//    if (states.isValid(7) || states.isValid(23)) {
//      states.decaudate();
//    }
//    if (states.isValid(30) && states.isValid(31)) {
//      states.decaudate(31);
//    }

        fprintf(states.isValid() ? stdout : stderr, "%s\n", states.toDebugString().c_str());
      }

      // TODO: store tail-time whenever a collapse happens, then set the bullet pre-step to walk through the control
      // states as it steps through the (currentTime(ms) - tailTime(ms)) / 16.67(ms) steps
      // should verify that correct number of steps are taken?

  };
}
