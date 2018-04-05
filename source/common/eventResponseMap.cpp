
#include "eventResponseMap.h"

using namespace rtu::topics;

namespace at3 {

  void EventResponseMap::setAction(std::string topic, Action action) {
    if (userInputSubs.count(topic) != 0) {
      userInputSubs.erase(topic);
    }
    userInputSubs.emplace(std::piecewise_construct, std::make_tuple(topic), std::make_tuple(topic, action));
  }

  void EventResponseMap::setAction(std::string topic, SimpleAction simpleAction) {
    if (userInputSubs.count(topic) != 0) {
      userInputSubs.erase(topic);
    }
    userInputSubs.emplace(std::piecewise_construct, std::make_tuple(topic), std::make_tuple(topic, simpleAction));
  }

}