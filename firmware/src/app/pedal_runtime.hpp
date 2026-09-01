#pragma once

#include <array>
#include <cstdint>

#include "../core/action_engine.hpp"
#include "../core/expression.hpp"
#include "../core/switch_engine.hpp"
#include "../hal/ports.hpp"
#include "../usb/usb_transport.hpp"

namespace midi {

struct RuntimeConfigSnapshot {
  bool available{};
  std::uint32_t sequence{};
  std::uint16_t bank_count{};
};

class RuntimeConfigSource {
 public:
  virtual ~RuntimeConfigSource() = default;
  [[nodiscard]] virtual RuntimeConfigSnapshot status() const = 0;
  [[nodiscard]] virtual bool load_bank(std::uint8_t index, BankConfig& output) const = 0;
};

class RuntimeSwitchSource {
 public:
  virtual ~RuntimeSwitchSource() = default;
  [[nodiscard]] virtual std::uint8_t read_mask() const = 0;
};

class RuntimeExpressionSource {
 public:
  virtual ~RuntimeExpressionSource() = default;
  [[nodiscard]] virtual std::uint16_t read_adc() const = 0;
};

class LiveActionState {
 public:
  virtual ~LiveActionState() = default;
  [[nodiscard]] virtual bool live_action_active() const = 0;
};

class PedalRuntime final : public ActionSink, public LiveActionState {
 public:
  PedalRuntime(RuntimeConfigSource& config, RuntimeSwitchSource& switches, RuntimeExpressionSource& expression_input,
               MidiPort& trs_midi, RelayPort& relays, usb::UsbTransport& usb_midi, usb::UsbPort& usb_port,
               DisplayPort& display, ExpressionProcessor& expression, bool watchdog_reset = false);

  [[nodiscard]] bool initialize();
  void tick(std::uint32_t now_ms);
  [[nodiscard]] bool live_action_active() const override { return live_action_active_; }

  bool send_midi(Destination destination, MidiMessage message) override;
  bool relay(RelayCommand command) override;
  bool navigate(NavigationCommand command) override;

 private:
  bool reload_bank();
  void refresh_configuration();
  void handle_event(const SwitchEvent& event);
  void handle_chord(Chord chord);
  void render();
  [[nodiscard]] const Preset& current_preset(SwitchId id) const;
  [[nodiscard]] bool expression_sample_due(std::uint32_t now_ms);
  static bool same_view(const LiveView& left, const LiveView& right);

  RuntimeConfigSource& config_;
  RuntimeSwitchSource& switches_;
  RuntimeExpressionSource& expression_input_;
  MidiPort& trs_midi_;
  RelayPort& relays_;
  usb::UsbTransport& usb_midi_;
  usb::UsbPort& usb_port_;
  DisplayPort& display_;
  ExpressionProcessor& expression_;
  ActionEngine actions_;
  SwitchEngine switch_engine_;
  EngineState engine_state_;
  RuntimeConfigSnapshot config_snapshot_{};
  BankConfig bank_{};
  std::uint16_t bank_index_{};
  std::uint8_t page_index_{};
  bool bank_loaded_{};
  std::array<bool, 2> relay_closed_{};
  bool configuration_error_{};
  bool queue_overflow_{};
  bool watchdog_reset_{};
  bool have_expression_schedule_{};
  std::uint32_t next_expression_sample_at_{};
  std::uint32_t usb_dropped_midi_{};
  LiveView last_view_{};
  bool have_view_{};
  bool live_action_active_{};
};

}  // namespace midi
