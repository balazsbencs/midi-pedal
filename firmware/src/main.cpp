#ifdef PICO_ON_DEVICE

#include <cstddef>
#include <cstdint>
#include <span>

#include "pico/stdlib.h"

#include "app/controller.hpp"
#include "app/controller_protocol_service.hpp"
#include "app/pedal_runtime.hpp"
#include "board/pico2_pins.hpp"
#include "core/config_store.hpp"
#include "hal/pico_expression_adc.hpp"
#include "hal/pico_flash_port.hpp"
#include "hal/pico_relays.hpp"
#include "hal/pico_switches.hpp"
#include "hal/pico_uart_midi.hpp"
#include "hal/pico_watchdog.hpp"
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
  midi::PicoRelays& relays() { return relays_; }

 private:
  midi::ConfigStore* config_{};
  midi::PicoRelays relays_;
  midi::PicoSwitches switches_;
  midi::PicoUartMidi midi_;
  midi::PicoExpressionAdc expression_;
};

class PicoConfigSource final : public midi::RuntimeConfigSource {
 public:
  explicit PicoConfigSource(midi::ConfigStore& store) : store_(store) {}

  [[nodiscard]] midi::RuntimeConfigSnapshot status() const override {
    const auto info = store_.active_info();
    return info.has_value() ? midi::RuntimeConfigSnapshot{true, info->sequence, info->bank_count}
                             : midi::RuntimeConfigSnapshot{};
  }

  [[nodiscard]] bool load_bank(std::uint8_t index, midi::BankConfig& output) const override {
    return store_.load_bank(index, output);
  }

 private:
  midi::ConfigStore& store_;
};

class PicoSwitchInput final : public midi::RuntimeSwitchSource {
 public:
  explicit PicoSwitchInput(const midi::PicoSwitches& switches) : switches_(switches) {}

  [[nodiscard]] std::uint8_t read_mask() const override { return switches_.read_mask(); }

 private:
  const midi::PicoSwitches& switches_;
};

class PicoExpressionInput final : public midi::ExpressionSampleInput, public midi::RuntimeExpressionSource {
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
  midi::PicoWatchdog watchdog;
  const auto watchdog_reset = watchdog.initialize();
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
  midi::ExpressionProcessor expression(config.expression_calibration());
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
  PicoConfigSource config_source(config);
  PicoSwitchInput switch_input(ports.switches());
  midi::usb::UsbTransport usb_transport(usb);
  midi::PedalRuntime runtime(config_source, switch_input, expression_input, ports.midi(), ports.relays(),
                             usb_transport, usb, display, expression, watchdog_reset);
  protocol.set_live_action_state(&runtime);
  (void)runtime.initialize();
  while (true) {
    runtime.tick(to_ms_since_boot(get_absolute_time()));
    usb.service(dispatcher);
    runtime.finish_control_phase();
    ports.midi().service();
    watchdog.feed();
    tight_loop_contents();
  }
}

#endif
