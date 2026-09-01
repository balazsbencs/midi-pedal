#pragma once

#include <cstdint>

#include "ports.hpp"

namespace midi {

class PicoRelays final : public RelayPort {
 public:
  static constexpr bool contacts_default_open() { return true; }

  void initialize();
  void set(std::uint8_t contact, bool closed) override;
};

}  // namespace midi
