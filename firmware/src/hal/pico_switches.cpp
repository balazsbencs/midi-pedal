#include "pico_switches.hpp"

#include "../board/pico2_pins.hpp"

#ifdef PICO_ON_DEVICE
#include "pico/stdlib.h"
#endif

namespace midi {

void PicoSwitches::initialize() {
#ifdef PICO_ON_DEVICE
  constexpr std::uint8_t pins[] = {board::SwitchA, board::SwitchB, board::SwitchC, board::SwitchD};
  for (const auto pin : pins) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
  }
#endif
}

std::uint8_t PicoSwitches::read_mask() const {
#ifdef PICO_ON_DEVICE
  constexpr std::uint8_t pins[] = {board::SwitchA, board::SwitchB, board::SwitchC, board::SwitchD};
  std::uint8_t mask = 0;
  for (unsigned index = 0; index < 4; ++index) if (!gpio_get(pins[index])) mask |= mask_for_index(index);
  return mask;
#else
  return 0;
#endif
}

}  // namespace midi
