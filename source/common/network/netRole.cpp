
#include "netRole.hpp"

namespace at3 {
  template<typename Derived>
  Derived& NetRole<Derived>::role() {
    return *static_cast<Derived*>(this);
  }

  template<typename Derived>
  void NetRole<Derived>::tick() {
    role().tick();
  }
}
