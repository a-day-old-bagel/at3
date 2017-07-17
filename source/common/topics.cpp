
#include <unordered_map>
#include "topics.h"

namespace at3 {
  namespace topics {

    namespace {
      struct Topic {
        std::unordered_map<uint32_t, Action> subscriptions;
        void publish(void* data) {
          for (auto sub : subscriptions) {
            sub.second(data);
          }
        }
      };
      std::unordered_map<std::string, Topic> activeTopics;
    }

    Subscription::Subscription(const std::string && topic, const Action && action) : topic(topic), id(++nextId) {
      activeTopics[topic].subscriptions[id] = action;
    }

    Subscription::~Subscription() {
      activeTopics[topic].subscriptions.erase(id);
    }

    std::atomic<uint32_t> Subscription::nextId = 0;

    void publish(std::string topic, void* data) {
      activeTopics[topic].publish(data);
    }

  }
}