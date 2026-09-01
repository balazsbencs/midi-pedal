#include "pedal_runtime.hpp"

#include <algorithm>

namespace midi {

PedalRuntime::PedalRuntime(RuntimeConfigSource& config, RuntimeSwitchSource& switches,
                           RuntimeExpressionSource& expression_input, MidiPort& trs_midi, RelayPort& relays,
                           usb::UsbTransport& usb_midi, usb::UsbPort& usb_port, DisplayPort& display,
                           ExpressionProcessor& expression, bool watchdog_reset)
    : config_(config),
      switches_(switches),
      expression_input_(expression_input),
      trs_midi_(trs_midi),
      relays_(relays),
      usb_midi_(usb_midi),
      usb_port_(usb_port),
      display_(display),
      expression_(expression),
      watchdog_reset_(watchdog_reset),
      actions_(*this) {}

bool PedalRuntime::initialize() {
  relays_.set(1, false);
  relays_.set(2, false);
  relay_closed_ = {};
  engine_state_.reset();
  bank_index_ = 0;
  page_index_ = 0;
  have_view_ = false;
  have_expression_schedule_ = false;
  usb_dropped_midi_ = usb_midi_.dropped_midi();
  config_snapshot_ = config_.status();
  const auto loaded = reload_bank();
  render();
  return loaded;
}

bool PedalRuntime::reload_bank() {
  config_snapshot_ = config_.status();
  const auto valid_count = config_snapshot_.available && config_snapshot_.bank_count > 0 &&
                           config_snapshot_.bank_count <= 128;
  if (!valid_count) {
    bank_ = {};
    bank_loaded_ = false;
    // No active image is a supported first-boot state: the device remains
    // usable over CDC and renders the safe, empty factory view.
    configuration_error_ = config_snapshot_.available;
    expression_.set_assignment(bank_.expression);
    return false;
  }
  bank_index_ = static_cast<std::uint16_t>(bank_index_ % config_snapshot_.bank_count);
  BankConfig loaded{};
  if (!config_.load_bank(static_cast<std::uint8_t>(bank_index_), loaded)) {
    bank_ = {};
    bank_loaded_ = false;
    configuration_error_ = true;
    expression_.set_assignment(bank_.expression);
    return false;
  }
  bank_ = loaded;
  bank_loaded_ = true;
  configuration_error_ = false;
  expression_.set_assignment(bank_.expression);
  return true;
}

void PedalRuntime::refresh_configuration() {
  const auto latest = config_.status();
  if (latest.available == config_snapshot_.available && latest.sequence == config_snapshot_.sequence &&
      latest.bank_count == config_snapshot_.bank_count) return;
  config_snapshot_ = latest;
  if (latest.available && latest.bank_count != 0) {
    bank_index_ = static_cast<std::uint16_t>(bank_index_ % latest.bank_count);
  } else {
    bank_index_ = 0;
  }
  reload_bank();
}

const Preset& PedalRuntime::current_preset(SwitchId id) const {
  return bank_.pages[page_index_].presets[static_cast<std::size_t>(id)];
}

void PedalRuntime::handle_chord(Chord chord) {
  NavigationCommand command{};
  switch (chord) {
    case Chord::BankDown: command.operation = NavigationOperation::BankDown; break;
    case Chord::BankUp: command.operation = NavigationOperation::BankUp; break;
    case Chord::PageDown: command.operation = NavigationOperation::PageDown; break;
    case Chord::PageUp: command.operation = NavigationOperation::PageUp; break;
    case Chord::None: return;
  }
  (void)navigate(command);
}

void PedalRuntime::handle_event(const SwitchEvent& event) {
  if (event.kind == SwitchEventKind::ChordRelease) return;
  struct ResetLiveAction final {
    bool& active;
    ~ResetLiveAction() { active = false; }
  };
  if (event.kind == SwitchEventKind::ChordPress || event.kind == SwitchEventKind::ChordRepeat) {
    live_action_active_ = true;
    const ResetLiveAction reset{live_action_active_};
    handle_chord(event.chord);
    return;
  }
  if (!bank_loaded_) return;
  live_action_active_ = true;
  const ResetLiveAction reset{live_action_active_};
  const auto result = actions_.execute(current_preset(event.switch_id), engine_state_, event);
  if (!result.accepted) queue_overflow_ = true;
}

void PedalRuntime::tick(std::uint32_t now_ms) {
  refresh_configuration();
  const auto events = switch_engine_.update(switches_.read_mask(), now_ms);
  for (const auto& event : events) handle_event(event);

  if (expression_sample_due(now_ms)) {
    if (const auto message = expression_.sample(expression_input_.read_adc(), now_ms); message.has_value()) {
      if (!send_midi(expression_.assignment().destination, *message)) queue_overflow_ = true;
    }
  }
  const auto dropped = usb_midi_.dropped_midi();
  if (dropped != usb_dropped_midi_) {
    usb_dropped_midi_ = dropped;
    queue_overflow_ = true;
  }
  render();
}

bool PedalRuntime::expression_sample_due(std::uint32_t now_ms) {
  if (have_expression_schedule_ &&
      static_cast<std::uint32_t>(now_ms - next_expression_sample_at_) >= 0x80000000u) {
    return false;
  }
  have_expression_schedule_ = true;
  // Rescheduling from the current sample intentionally skips missed periods:
  // a late main loop never issues a burst of ADC reads.
  next_expression_sample_at_ = now_ms + 1u;
  return true;
}

bool PedalRuntime::send_midi(Destination destination, MidiMessage message) {
  const auto trs_ok = trs_midi_.enqueue(destination, message);
  const auto usb_ok = usb_midi_.send_midi(destination, message);
  if (!trs_ok || !usb_ok) queue_overflow_ = true;
  return trs_ok && usb_ok;
}

bool PedalRuntime::relay(RelayCommand command) {
  if (command.contact < 1 || command.contact > 2) return false;
  const auto index = static_cast<std::size_t>(command.contact - 1);
  bool closed = relay_closed_[index];
  switch (command.operation) {
    case RelayOperation::Open: closed = false; break;
    case RelayOperation::Close: closed = true; break;
    case RelayOperation::Toggle: closed = !closed; break;
  }
  relays_.set(command.contact, closed);
  relay_closed_[index] = closed;
  return true;
}

bool PedalRuntime::navigate(NavigationCommand command) {
  if (!config_snapshot_.available || config_snapshot_.bank_count == 0 || config_snapshot_.bank_count > 128) return false;
  auto next_bank = bank_index_;
  auto next_page = page_index_;
  switch (command.operation) {
    case NavigationOperation::BankUp:
      next_bank = static_cast<std::uint16_t>((bank_index_ + 1u) % config_snapshot_.bank_count);
      break;
    case NavigationOperation::BankDown:
      next_bank = static_cast<std::uint16_t>((bank_index_ + config_snapshot_.bank_count - 1u) % config_snapshot_.bank_count);
      break;
    case NavigationOperation::BankSet:
      if (command.target < 1 || command.target > config_snapshot_.bank_count) return false;
      next_bank = static_cast<std::uint16_t>(command.target - 1u);
      break;
    case NavigationOperation::PageUp: next_page = static_cast<std::uint8_t>((page_index_ + 1u) % 4u); break;
    case NavigationOperation::PageDown: next_page = static_cast<std::uint8_t>((page_index_ + 3u) % 4u); break;
    case NavigationOperation::PageSet:
      if (command.target < 1 || command.target > 4) return false;
      next_page = static_cast<std::uint8_t>(command.target - 1u);
      break;
  }
  const auto bank_changed = next_bank != bank_index_;
  bank_index_ = next_bank;
  page_index_ = next_page;
  if (bank_changed && !reload_bank()) return false;
  return true;
}

bool PedalRuntime::same_view(const LiveView& left, const LiveView& right) {
  return left.bank == right.bank && left.page == right.page && left.positions == right.positions &&
         left.expressionAvailable == right.expressionAvailable && left.expressionValue == right.expressionValue &&
         left.usbConnected == right.usbConnected && left.configurationError == right.configurationError &&
         left.queueOverflow == right.queueOverflow && left.watchdogReset == right.watchdogReset;
}

void PedalRuntime::render() {
  LiveView view{};
  view.bank = static_cast<std::uint8_t>(bank_index_ + 1u);
  view.page = static_cast<std::uint8_t>(page_index_ + 1u);
  for (std::size_t index = 0; index < view.positions.size(); ++index) view.positions[index] = 1;
  if (bank_loaded_) {
    for (std::size_t index = 0; index < view.positions.size(); ++index) {
      const auto& preset = bank_.pages[page_index_].presets[index];
      view.positions[index] = engine_state_.position(preset.id) == Position::Two ? 2 : 1;
    }
  }
  view.expressionAvailable = expression_.view().available;
  view.expressionValue = expression_.view().value;
  view.usbConnected = usb_port_.mounted();
  view.configurationError = configuration_error_;
  view.queueOverflow = queue_overflow_;
  view.watchdogReset = watchdog_reset_;
  if (!have_view_ || !same_view(last_view_, view)) display_.present(view);
  last_view_ = view;
  have_view_ = true;
}

}  // namespace midi
