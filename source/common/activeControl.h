
#pragma once

#include <string>
#include <unordered_map>
#include "topics.hpp"

namespace at3 {

  class ActiveControl {
    private:
      std::unordered_map<std::string, rtu::topics::Subscription> userInputSubs;
    public:
      ActiveControl() = default;
      void setAction(std::string topic, rtu::topics::Action action);
  };

}
