#ifdef PICO_ON_DEVICE

#include <cstdint>

#include "pico/stdlib.h"

#include "app/controller.hpp"
#include "board/pico2_pins.hpp"
#include "hal/pico_expression_adc.hpp"
#include "hal/pico_relays.hpp"
#include "hal/pico_switches.hpp"
#include "hal/pico_uart_midi.hpp"
#include "display/st7796s.hpp"

namespace {
class PicoPorts final : public midi::ControllerPorts {
 public:
  PicoPorts() { relays_.initialize(); }

  void relay_set(std::uint8_t contact, bool closed) override {
    relays_.set(contact, closed);
  }

  bool read_active_config() override { return false; }
  void show_boot_status() override {}

  midi::PicoSwitches& switches() { return switches_; }
  midi::PicoUartMidi& midi() { return midi_; }
  midi::PicoExpressionAdc& expression() { return expression_; }

 private:
  midi::PicoRelays relays_;
  midi::PicoSwitches switches_;
  midi::PicoUartMidi midi_;
  midi::PicoExpressionAdc expression_;
};
}  // namespace

int main() {
  stdio_init_all();
  PicoPorts ports;
  ports.switches().initialize();
  ports.midi().initialize();
  ports.expression().initialize();
  midi::display::St7796sDisplay display;
  display.initialize();
  midi::Controller controller(ports);
  controller.initialize();
  midi::LiveView boot_view{};
  boot_view.bank = 1;
  boot_view.page = 1;
  boot_view.positions = {1, 1, 1, 1};
  boot_view.usbConnected = true;
  display.present(boot_view);
  while (true) {
    ports.midi().service();
    tight_loop_contents();
  }
}

#endif
