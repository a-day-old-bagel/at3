
#pragma once

#include <string>
#include <unordered_map>
#include "topics.hpp"

namespace at3 {

  class OneToOneEventMap {
    private:
      std::unordered_map<std::string, rtu::topics::Subscription> userInputSubs;
    public:
      OneToOneEventMap() = default;
      void setAction(std::string topic, rtu::topics::Action action);
  };

}
