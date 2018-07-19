#pragma once

namespace at3 {
  template <typename Derived>
  class NetRole {
      Derived& role();
    public:
      NetRole() = default;
      virtual ~NetRole() = default;
      void tick();
  };
}
