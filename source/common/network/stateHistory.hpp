#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include <SDL_timer.h>

#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakNetTypes.h"

#include "math.hpp"
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
      void addStateData(SLNet::BitStream &data, bool authoritative = false) {
        if (authoritative) {
          authState.Write(data);
          stateIsAuthorized = true;
        } else {
          state.Write(data);
        }
      }

      /*
       * Once inputs have been given such that the checksum is satisfied, a complete input set will be assumed to have
       * been received and the snapshot may be treated as authoritative, assuming the state is also authorized. So
       * for an authority (like a server), a complete input set is all that's needed. A dependent (like a client) may
       * need to wait for the authority to also send the corresponding authoritative state update(s) before the state
       * can be considered complete.
       */
      void addInputData(SLNet::BitStream &data, const SLNet::RakNetGUID & guid, SLNet::BitSize_t length = 0) {
        if (inputSources) { // if no input sources defined, no checking will occur.
          if (inputSources->count(guid)) {
            if ((*inputSources)[guid]) {
              printf("Duplicate input from %lu at %u!\n", SLNet::RakNetGUID::ToUint32(guid), index);
              inputSum -= SLNet::RakNetGUID::ToUint32(guid);
            }
          } else {
            printf("Unexpected input from %lu at %u!\n", SLNet::RakNetGUID::ToUint32(guid), index);
          }
          (*inputSources)[guid] = true;
        }

        if (length) {
          input.Write(data, length);
        } else {
          input.Write(data);
        }
        inputSum += SLNet::RakNetGUID::ToUint32(guid);
      }
      // void addInputData(SLNet::BitStream &data, const SLNet::RakNetGUID & guid) {
      //   // if (inputSources) { // if no input sources defined, no checking will occur.
      //   //   if (inputSources->count(guid)) {
      //   //     if ((*inputSources)[guid]) {
      //   //       fprintf(stderr, "Discarded duplicate client input stream at state snapshot %u\n", index);
      //   //       return; // don't add more than one stream per client into the snapshot
      //   //     }
      //   //     (*inputSources)[guid] = true;
      //   //   } else {
      //   //     fprintf(stderr, "Received input %u from unexpected GUID %lu!\n", index, SLNet::RakNetGUID::ToUint32(guid));
      //   //     return; // don't add invalid guid's to the checksum or their inputs to the stream
      //   //   }
      //   // }
      //
      //   if (inputSources) { // if no input sources defined, no checking will occur.
      //     if (inputSources->count(guid)) {
      //       if ((*inputSources)[guid]) {
      //         printf("Duplicate input from %lu at %u!\n", SLNet::RakNetGUID::ToUint32(guid), index);
      //         inputSum -= SLNet::RakNetGUID::ToUint32(guid);
      //       }
      //     } else {
      //       printf("Unexpected input from %lu at %u!\n", SLNet::RakNetGUID::ToUint32(guid), index);
      //     }
      //     (*inputSources)[guid] = true;
      //   }
      //
      //   input.Write(data);
      //   inputSum += SLNet::RakNetGUID::ToUint32(guid);
      // }
      // void addInputData(SLNet::BitStream &data, const SLNet::RakNetGUID & guid, SLNet::BitSize_t length) {
      //   if (inputSources) { // if no input sources defined, no checking will occur.
      //     if (inputSources->count(guid)) {
      //       if ((*inputSources)[guid]) {
      //         printf("Duplicate input from %lu at %u!\n", SLNet::RakNetGUID::ToUint32(guid), index);
      //         inputSum -= SLNet::RakNetGUID::ToUint32(guid);
      //       }
      //     } else {
      //       printf("Unexpected input from %lu at %u!\n", SLNet::RakNetGUID::ToUint32(guid), index);
      //     }
      //     (*inputSources)[guid] = true;
      //   }
      //
      //   input.Write(data, length);
      //   inputSum += SLNet::RakNetGUID::ToUint32(guid);
      // }

      /*
       * Same as above, but instead of handling an individual guid, you must manage the checksum yourself by calling
       * addToInputChecksum.
       */
      void addInputData(SLNet::BitStream &data) {
        input.Write(data);
      }
      void addInputData(SLNet::BitStream &data, SLNet::BitSize_t length) {
        input.Write(data, length);
      }

      void addToInputChecksum(const SLNet::RakNetGUID & checksumPart) {
        inputSum += SLNet::RakNetGUID::ToUint32(checksumPart);
      }

      void acknowledgeCompletion() {
        authorizationAcknowledged = true;
      }

      const SLNet::BitStream & getState() const {
        return state;
      }
      const SLNet::BitStream & getAuthState() const {
        return authState;
      }
      const SLNet::BitStream & getInput() const {
        return input;
      }

      SLNet::BitStream & getState() {
        return state;
      }
      SLNet::BitStream & getAuthState() {
        return authState;
      }
      SLNet::BitStream & getInput() {
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
        // desiredInputSum = 0;
        // if (inputSources) {
        //   inputSources->clear();
        // } else {
        //   inputSources = std::make_unique<std::unordered_map<SLNet::RakNetGUID, bool, GuidHash>>();
        // }

        if ( ! inputSources) {
          inputSources = std::make_unique<std::unordered_map<SLNet::RakNetGUID, bool, GuidHash>>();
        }

        for (uint32_t i = 0; i < guids.Size(); ++i) {
          if (inputSources->count(guids[i])) {
            printf("Duplicate input source registration %lu at %u!\n", SLNet::RakNetGUID::ToUint32(guids[i]), index);
          }
          desiredInputSum += SLNet::RakNetGUID::ToUint32(guids[i]);
          inputSources->emplace(guids[i], false);
        }
      }

      void excludeInputSources(const DataStructures::List<SLNet::RakNetGUID> & guids) {
        for (uint32_t i = 0; i < guids.Size(); ++i) {
          if (inputSources && ! inputSources->erase(guids[i])) {
            printf("Attempted to exclude unregistered input source %u at %u!\n", guids[i], index);
          } else {
            desiredInputSum -= SLNet::RakNetGUID::ToUint32(guids[i]);
          }
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
      bool completionAcked() {
        return authorizationAcknowledged;
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

      SLNet::BitStream input, state, authState;
      indexType index = 0;
      uint32_t inputSum = 0;
      uint32_t desiredInputSum = 0;
      std::unique_ptr<std::unordered_map<SLNet::RakNetGUID, bool, GuidHash>> inputSources;
      bool stateIsAuthorized = false;
      bool authorizationAcknowledged = false;

      /*
       * These are used as the targets of the constructor/destructor-like function delegates described by init/clear
       */
      static StateSnapShot & initImpl(StateSnapShot &stateSnapShot, const indexType &index) {
        stateSnapShot.index = index;
        return stateSnapShot;
      }
      static void clearImpl(StateSnapShot &stateSnapShot, const indexType &index) {
        stateSnapShot.input.Reset();
        stateSnapShot.state.Reset();
        stateSnapShot.authState.Reset();

        if (stateSnapShot.inputSources) {
          stateSnapShot.inputSources->clear();
        }

        stateSnapShot.inputSum = 0;
        stateSnapShot.desiredInputSum = 0;
        stateSnapShot.stateIsAuthorized = false;
        stateSnapShot.authorizationAcknowledged = false;
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

      typedef rtu::Ouroboros<StateSnapShot<indexType>, indexType, maxStoredStates> Otype;

      StateHistory() : states(StateSnapShot<indexType>::init, StateSnapShot<indexType>::clear) {
        RTU_STATIC_SUB(debugSub, "key_down_f8", StateHistory::debugOuroboros, this);
      }

      bool addToInput(SLNet::BitStream &input, indexType index, const SLNet::RakNetGUID &id,
          SLNet::BitSize_t length = 0) {
        if (states[index].isComplete()) {
          printf("Adding input to an already complete state %u!\n", index);
        }
        states[index].addInputData(input, id, length);
        checkForCompleteness(index);
        return states.isValid(index);
      }
      // bool addToInput(SLNet::BitStream &input, indexType index, const SLNet::RakNetGUID &id) {
      //   if (states[index].isComplete()) {
      //     printf("Adding input to an already complete state %u!\n", index);
      //   }
      //
      //   // if (states.isValid(index)) {
      //   //   states[index].addInputData(input, id);
      //   //   checkForCompleteness(index);
      //   //   return true;
      //   // } else {
      //   //   printf("Attempted to add input to invalid state history index %u!\n", index);
      //   //   return false;
      //   // }
      //
      //   states[index].addInputData(input, id);
      //   checkForCompleteness(index);
      //   return states.isValid(index);
      // }
      // bool addToInput(SLNet::BitStream &input, indexType index, const SLNet::RakNetGUID &id, SLNet::BitSize_t length) {
      //   if (states[index].isComplete()) {
      //     printf("Adding input to an already complete state %u!\n", index);
      //   }
      //   states[index].addInputData(input, id, length);
      //   checkForCompleteness(index);
      //   return states.isValid(index);
      // }

      /*
       * Must also call addToInputChecksum if you use this version since it doesnt take the GUID.
       */
      bool addToInput(SLNet::BitStream &input, indexType index, SLNet::BitSize_t length = 0) {
        if (states[index].isComplete()) {
          printf("Adding input to an already complete state %u!\n", index);
        }
        states[index].addInputData(input, length);
        checkForCompleteness(index);
        return states.isValid(index);
      }
      // bool addToInput(SLNet::BitStream &input, indexType index) {
      //   if (states[index].isComplete()) {
      //     printf("Adding input to an already complete state %u!\n", index);
      //   }
      //   // if (states.isValid(index)) {
      //   //   states[index].addInputData(input);
      //   //   checkForCompleteness(index);
      //   //   return true;
      //   // } else {
      //   //   printf("\n");
      //   //   printf("Attempted to add input to invalid state history index %u!\n", index);
      //   //   debugOuroboros();
      //   //   printf("\n");
      //   //   return false;
      //   // }
      //
      //   states[index].addInputData(input);
      //   checkForCompleteness(index);
      //   return states.isValid(index);
      // }
      // bool addToInput(SLNet::BitStream &input, indexType index, SLNet::BitSize_t length) {
      //   if (states[index].isComplete()) {
      //     printf("Adding input to an already complete state %u!\n", index);
      //   }
      //   states[index].addInputData(input, length);
      //   checkForCompleteness(index);
      //   return states.isValid(index);
      // }

      bool addToInputChecksum(indexType index, const SLNet::RakNetGUID & checksumPart) {
        if (states[index].isComplete()) {
          printf("Adding to the input checksum of an already complete state %u!\n", index);
        }

        // if (states.isValid(index)) {
        //   states[index].addToInputChecksum(checksumPart);
        //   checkForCompleteness(index);
        //   return true;
        // } else {
        //   printf("\n");
        //   printf("Attempted to add to the input checksum of invalid state history index %u!\n", index);
        //   debugOuroboros();
        //   printf("\n");
        //   return false;
        // }

        states[index].addToInputChecksum(checksumPart);
        checkForCompleteness(index);
        return states.isValid(index);
      }

//#     define AT3_STATE_HISTORY_DEBUG
      bool addToState(SLNet::BitStream &state, indexType index, bool authoritative = false) {
        if (states[index].isComplete()) {
          printf("Adding state to an already complete state %u!\n", index);
        }

        // if (states.isValid(index)) {
        //   states[index].addStateData(state, authoritative);
        //   checkForCompleteness(index);
        //   return true;
        // } else {
        //   printf("Attempted to add state to invalid state history index %u!\n", index);
        //   return false;
        // }

        states[index].addStateData(state, authoritative);
        checkForCompleteness(index);
        return states.isValid(index);
      }
#     undef AT3_STATE_HISTORY_DEBUG

      bool setInputChecksum(indexType index, uint32_t sum) {
        // if (states.isValid(index)) {
        //   states[index].setInputChecksum(sum);
        //   checkForCompleteness(index);
        //   return true;
        // } else {
        //   return false;
        // }

        states[index].setInputChecksum(sum);
        checkForCompleteness(index);
        return states.isValid(index);
      }

      bool setAdditionalInputChecksum(indexType index, uint32_t sum) {
        // if (states.isValid(index)) {
        //   states[index].setAdditionalInputChecksum(sum);
        //   checkForCompleteness(index);
        //   return true;
        // } else {
        //   return false;
        // }

        states[index].setAdditionalInputChecksum(sum);
        checkForCompleteness(index);
        return states.isValid(index);
      }

      void setTime(indexType index, uint32_t time) {
        // if (states.isValid(index)) {
        //   states[index].time = time;
        // } else {
        //   printf("Attempted to set the time for invalid state history index %u!\n", index);
        // }

        states[index].time = time;
      }

      bool setInputSources(const DataStructures::List<SLNet::RakNetGUID> &guids, indexType index) {
        // if (states.isValid(index)) {
        //   states[index].setInputSources(guids);
        //   checkForCompleteness(index);
        //   return true;
        // } else {
        //   return false;
        // }

        states[index].setInputSources(guids);
        return states.isValid(index);
      }

      bool excludeInputSources(const DataStructures::List<SLNet::RakNetGUID> & guids, indexType index) {
        // if (states.isValid(index)) {
        //   states[index].excludeInputSources(guids);
        //   return true;
        // } else {
        //   return false;
        // }

        states[index].excludeInputSources(guids);
        return states.isValid(index);
      }

      bool newTruthIsAvailable() {
        return newCompletionReady;
      }

      uint32_t getInputChecksum(indexType index) {
        // if (states.isValid(index)) {
        //   return states[index].getInputChecksum();
        // } else {
        //   return 0;
        // }

        return states[index].getInputChecksum();
      }

      bool contiguousTruthIsAvailable() {
        // if (completeSnapShotReady) {
        //
        //   // printf("\nTRUTH @ %u with %lu of %lu\n", latestCompletion,
        //   //        (unsigned long)states[latestCompletion].getCurrentChecksum(),
        //   //        (unsigned long)states[latestCompletion].getInputChecksum());
        //   // debugOuroboros();
        //   //
        //   // states.decaudate(latestCompletion);
        //   // currentReplayHead = latestCompletion;
        //   // completeSnapShotReady = false;
        //   // replayRunning = true;
        //   //
        //   // debugOuroboros();
        //
        //   return true;
        // } else {
        //   // printf("\nNO TRUTH\n");
        //   // debugOuroboros();
        //   return false;
        // }

        return contiguousCompletionReady;
      }

      void beginReplayFromNewTruth() {
        printf("\nNEW TRUTH @ %u with %lu of %lu\n", newestCompletion,
               (unsigned long)states[newestCompletion].getCurrentChecksum(),
               (unsigned long)states[newestCompletion].getInputChecksum());
        debugOuroboros();

        states.decaudate(newestCompletion);
        currentReplayHead = newestCompletion;
        // addExtraReplayTime(currentReplayHead);
        addExtraReplayFrames(currentReplayHead);
        newCompletionReady = false;
        replayRunning = true;

        debugOuroboros();
      }

      void beginReplayFromContiguousTruth() {
        printf("\nCONTIGUOUS TRUTH @ %u with %lu of %lu\n", contiguousCompletion,
               (unsigned long)states[contiguousCompletion].getCurrentChecksum(),
               (unsigned long)states[contiguousCompletion].getInputChecksum());
        debugOuroboros();

        states.decaudate(contiguousCompletion);
        currentReplayHead = contiguousCompletion;
        // addExtraReplayTime(currentReplayHead);
        addExtraReplayFrames(currentReplayHead);
        contiguousCompletionReady = false;
        replayRunning = true;

        debugOuroboros();
      }

      bool setEmptyIndex(indexType index) {
        return states.setEmptyIndex(index);
      }

#     define AT3_STATE_HISTORY_DEBUG
      indexType appendIndex() {
        // printf("\nAPPEND INDEX\n");
        // debugOuroboros();
        if (states.isFull()) {
#         ifdef AT3_STATE_HISTORY_DEBUG
            printf("\nHISTORY FULL AT %u:\n", getNewestCompleteIndex());
            debugOuroboros();
            states.decaudate();
            debugOuroboros();
#         else //AT3_STATE_HISTORY_DEBUG
            states.decaudate();
#         endif //AT3_STATE_HISTORY_DEBUG
        }
        indexType index = states.capitate().getIndex();
        checkForCompleteness(index);
        // debugOuroboros();
        return index;
        // return states.capitate().getIndex();
      }

      indexType getNewestIndex() {
        return states[states.getHeadSlot()].getIndex();
      }

      bool isFull() {
        return
        states.isFull();
      }

      bool isReplaying() {
        return replayRunning;
      }

      float getExtraReplayTime() {
        float extraTime = extraReplayTime;
        extraReplayTime = 0.f;
        return extraTime;
      }

      indexType getReplayFrameCount() {
        indexType frames = extraFrames;
        extraFrames = 0;
        return frames;
      }

      indexType getNewestCompleteIndex() {
        return newestCompletion;
      }
      indexType getNewestContiguousCompleteIndex() {
        return contiguousCompletion;
      }

      SLNet::BitStream & getNewestCompleteState() {
        return states[getNewestCompleteIndex()].getState();
      }
      SLNet::BitStream & getNewestCompleteAuthState() {
        return states[getNewestCompleteIndex()].getAuthState();
      }
      SLNet::BitStream & getNewestCompleteInput() {
        return states[getNewestCompleteIndex()].getInput();
      }

      SLNet::BitStream & getNewestContiguousCompleteState() {
        return states[getNewestContiguousCompleteIndex()].getState();
      }
      SLNet::BitStream & getNewestContiguousCompleteAuthState() {
        return states[getNewestContiguousCompleteIndex()].getAuthState();
      }
      SLNet::BitStream & getNewestContiguousCompleteInput() {
        return states[getNewestContiguousCompleteIndex()].getInput();
      }

      bool getNextReplayStreams(SLNet::BitStream & stateOut, SLNet::BitStream & authOut, SLNet::BitStream & inputOut) {
        if (replayRunning) {
          stateOut.Write(states[currentReplayHead].getState());
          authOut.Write(states[currentReplayHead].getAuthState());
          inputOut.Write(states[currentReplayHead].getInput());
          if (states.isValid(Otype::getNext(currentReplayHead))) {
            currentReplayHead = Otype::getNext(currentReplayHead);
          } else {
            replayRunning = false;
          }
        }
        return replayRunning;
      }

      std::vector<SLNet::RakNetGUID> getUnfulfilledInputs(indexType index) {
        // if (states.isValid(index)) {
        //   return states[index].getUnfulfilledInputs();
        // } else {
        //   return std::vector<SLNet::RakNetGUID>();
        // }
        return states[index].getUnfulfilledInputs();
      }

      uint32_t getNewestReplayBeginTime() {
        // TODO: once the replay has finished, will there be extra time lost (the time it takes to replay) that won't be considered by Game's dt? Or will that timer take care of it?
        return isReplaying() ? states[newestCompletion].time : 0;
      }
      uint32_t getContiguousReplayBeginTime() {
        // TODO: once the replay has finished, will there be extra time lost (the time it takes to replay) that won't be considered by Game's dt? Or will that timer take care of it?
        return isReplaying() ? states[contiguousCompletion].time : 0;
      }




      void _turnOffReplay() {
        replayRunning = false;
      }



    private:

      Otype states;
      indexType newestCompletion = 0;
      indexType contiguousCompletion = 0;
      indexType currentReplayHead = 0;
      float extraReplayTime = 0.f;
      uint8_t extraFrames = 0;
      bool newCompletionReady = false;
      bool contiguousCompletionReady = false;
      bool replayRunning = false;


      void addExtraReplayTime(indexType index) {

        fprintf(stderr, "addExtraReplayTime not implemented!\n");
        // indexType diff = index - getNewestIndex();

        // TODO: Use SDL_GetPerformanceTimer() instead if this code is re-enabled

        // float extraTime = (states[getNewestIndex()].time - states[index].time) * msToS;
        // float gapTime = (SDL_GetTicks() - states[getNewestIndex()].time) * msToS;
        // // float gapTime = 0.f;
        // printf("Extra Replay Time (%u, %u): %u - %u = %f\n", getNewestIndex(), index, states[getNewestIndex()].time, states[index].time, extraTime + gapTime);
        // extraReplayTime += extraTime + gapTime;
      }

      void addExtraReplayFrames(indexType startingIndex) {
        extraFrames = getNewestIndex() - startingIndex;
      }

      void checkForCompleteness(indexType index) {

        if ( ! states[index].isComplete() || states[index].completionAcked()) {
          return; // the following checks only occur when a state becomes complete
        }
        states[index].acknowledgeCompletion();

        for (indexType i = Otype::getNext(contiguousCompletion);
             states.isValid(i) && states[i].isComplete() && ! states.firstIsFresherThanSecond(i, index);
             i = Otype::getNext(i))
        {
          contiguousCompletion = i;
          contiguousCompletionReady = true;
        }

        if (states.isValid(index)) {
          if (newCompletionReady && ! states.firstIsFresherThanSecond(index, newestCompletion)) {
            return;
          }
          newestCompletion = index;
          newCompletionReady = true;
        }
      }

      void debugOuroboros() {

        // Test 0
//    bool fill = (bool)(rand() % 2);
//    if (fill && ! states.isFull()) {
//      printf("NEW HEAD: %u\n", states.capitate().getIndex());
//    } else if (fill && states.isFull()) {
//      states.decaudate();
//      printf("NEW TAIL: %u\n", getNewestCompleteIndex());
//    } else if ( ! fill && ! states.isEmpty()) {
//      states.decaudate();
//      printf("NEW TAIL: %u\n", getNewestCompleteIndex());
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

        printf("%s\n", states.toDebugString().c_str());
      }

      // TODO: store tail-time whenever a collapse happens, then set the bullet pre-step to walk through the control
      // states as it steps through the (currentTime(ms) - tailTime(ms)) / 16.67(ms) steps
      // should verify that correct number of steps are taken?

  };
}
