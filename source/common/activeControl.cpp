
#include "activeControl.h"

using namespace rtu::topics;

namespace at3 {

  void ActiveControl::setAction(std::string topic, Action action) {
    if (userInputSubs.count(topic) != 0) {
      userInputSubs.erase(topic);
    }
//    userInputSubs.try_emplace(topic, Subscription(topic, action));
    userInputSubs.emplace(std::piecewise_construct, std::make_tuple(topic), std::make_tuple(topic, action));
  }
}