#include "pico_watchdog.hpp"

#ifdef PICO_ON_DEVICE
#include "hardware/watchdog.h"
#endif

namespace midi {

bool PicoWatchdog::initialize() {
  if (initialized_) return reset_cause_;

#ifdef PICO_ON_DEVICE
  reset_cause_ = watchdog_caused_reboot();
  watchdog_enable(TimeoutMs, true);
#endif

  initialized_ = true;
  return reset_cause_;
}

void PicoWatchdog::feed() {
#ifdef PICO_ON_DEVICE
  if (initialized_) watchdog_update();
#endif
}

}  // namespace midi
