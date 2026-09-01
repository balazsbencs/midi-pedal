#ifdef PICO_ON_DEVICE

#include <cstdint>

#include "pico/stdlib.h"

#include "app/controller.hpp"
#include "board/pico2_pins.hpp"

namespace {
class PicoPorts final : public midi::ControllerPorts {
 public:
  PicoPorts() {
    gpio_init(midi::board::Relay1);
    gpio_init(midi::board::Relay2);
    gpio_set_dir(midi::board::Relay1, GPIO_OUT);
    gpio_set_dir(midi::board::Relay2, GPIO_OUT);
    gpio_put(midi::board::Relay1, 0);
    gpio_put(midi::board::Relay2, 0);
  }

  void relay_set(std::uint8_t contact, bool closed) override {
    if (contact == 1) gpio_put(midi::board::Relay1, closed ? 1 : 0);
    if (contact == 2) gpio_put(midi::board::Relay2, closed ? 1 : 0);
  }

  bool read_active_config() override { return false; }
  void show_boot_status() override {}
};
}  // namespace

int main() {
  PicoPorts ports;
  midi::Controller controller(ports);
  controller.initialize();
  while (true) tight_loop_contents();
}

#endif
