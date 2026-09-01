#ifdef PICO_ON_DEVICE

#include <cstddef>
#include <cstdint>
#include <span>

#include "pico/stdlib.h"

#include "app/controller.hpp"
#include "app/controller_protocol_service.hpp"
#include "board/pico2_pins.hpp"
#include "core/config_store.hpp"
#include "hal/pico_expression_adc.hpp"
#include "hal/pico_flash_port.hpp"
#include "hal/pico_relays.hpp"
#include "hal/pico_switches.hpp"
#include "hal/pico_uart_midi.hpp"
#include "display/st7796s.hpp"
#include "usb/pico_usb.hpp"
#include "usb/protocol_dispatcher.hpp"

namespace {
extern "C" {
extern const std::uint8_t _binary_firmware_defaults_factory_empty_bin_start[];
extern const std::uint8_t _binary_firmware_defaults_factory_empty_bin_end[];
}

class PicoPorts final : public midi::ControllerPorts {
 public:
  PicoPorts() { relays_.initialize(); }

  void attach_config(midi::ConfigStore& config) { config_ = &config; }

  void relay_set(std::uint8_t contact, bool closed) override {
    relays_.set(contact, closed);
  }

  bool read_active_config() override { return config_ != nullptr && config_->active_info().has_value(); }
  void show_boot_status() override {}

  midi::PicoSwitches& switches() { return switches_; }
  midi::PicoUartMidi& midi() { return midi_; }
  midi::PicoExpressionAdc& expression() { return expression_; }

 private:
  midi::ConfigStore* config_{};
  midi::PicoRelays relays_;
  midi::PicoSwitches switches_;
  midi::PicoUartMidi midi_;
  midi::PicoExpressionAdc expression_;
};

class PicoExpressionInput final : public midi::ExpressionSampleInput {
 public:
  explicit PicoExpressionInput(const midi::PicoExpressionAdc& adc) : adc_(adc) {}

  [[nodiscard]] std::uint16_t read_adc() const override { return adc_.read(); }

 private:
  const midi::PicoExpressionAdc& adc_;
};
}  // namespace

int main() {
  stdio_init_all();
  midi::PicoFlashPort flash;
  PicoPorts ports;
  midi::ConfigStore config(flash);
  ports.attach_config(config);
  ports.switches().initialize();
  ports.midi().initialize();
  ports.expression().initialize();
  midi::display::St7796sDisplay display;
  display.initialize();
  midi::Controller controller(ports);
  controller.initialize();

  PicoExpressionInput expression_input(ports.expression());
  midi::ExpressionProcessor expression({0, 4095});
  const auto factory_start = reinterpret_cast<std::uintptr_t>(_binary_firmware_defaults_factory_empty_bin_start);
  const auto factory_end = reinterpret_cast<std::uintptr_t>(_binary_firmware_defaults_factory_empty_bin_end);
  const auto factory_size = factory_end >= factory_start ? static_cast<std::size_t>(factory_end - factory_start) : 0;
  const auto factory_image = std::span<const std::byte>(
      reinterpret_cast<const std::byte*>(factory_start), factory_size);
  midi::ControllerProtocolService protocol(config, expression_input, expression, factory_image);
  midi::usb::PicoUsbDeviceApi usb_api;
  midi::usb::PicoUsbPort usb(usb_api);
  (void)usb.initialize();
  midi::usb::ProtocolDispatcher dispatcher(protocol, usb);

  midi::LiveView boot_view{};
  boot_view.bank = 1;
  boot_view.page = 1;
  boot_view.positions = {1, 1, 1, 1};
  boot_view.usbConnected = true;
  display.present(boot_view);
  while (true) {
    usb.service(dispatcher);
    ports.midi().service();
    tight_loop_contents();
  }
}

#endif
