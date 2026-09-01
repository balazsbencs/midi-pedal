#include "pico_relays.hpp"

#include "../board/pico2_pins.hpp"

#ifdef PICO_ON_DEVICE
#include "hardware/gpio.h"
#endif

namespace midi {

void PicoRelays::initialize() {
#ifdef PICO_ON_DEVICE
  constexpr std::uint8_t pins[] = {board::Relay1, board::Relay2};
  for (const auto pin : pins) {
    gpio_init(pin);
    gpio_put(pin, 0);
    gpio_set_dir(pin, GPIO_OUT);
  }
#endif
}

void PicoRelays::set(std::uint8_t contact, bool closed) {
#ifdef PICO_ON_DEVICE
  if (contact == 1) gpio_put(board::Relay1, closed ? 1 : 0);
  else if (contact == 2) gpio_put(board::Relay2, closed ? 1 : 0);
#else
  (void)contact;
  (void)closed;
#endif
}

}  // namespace midi
