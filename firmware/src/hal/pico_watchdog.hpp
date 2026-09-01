#pragma once

#include <cstdint>

namespace midi {

class PicoWatchdog final {
 public:
  static constexpr std::uint32_t TimeoutMs = 2000;

  // Returns whether the current boot was caused by an expired watchdog.
  [[nodiscard]] bool initialize();
  void feed();

 private:
  bool initialized_{};
  bool reset_cause_{};
};

}  // namespace midi
