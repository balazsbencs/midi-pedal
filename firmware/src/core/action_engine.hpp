#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "config_types.hpp"
#include "switch_engine.hpp"

namespace midi {

enum class Position : std::uint8_t { One = 0, Two = 1 };

class EngineState {
 public:
  static constexpr std::size_t PresetCount = 2048;

  [[nodiscard]] Position position(std::uint32_t preset_id) const;
  void set_position(std::uint32_t preset_id, Position position);
  void toggle(std::uint32_t preset_id);
  void reset() { positions_.fill(0); }

 private:
  static std::size_t bit_index(std::uint32_t preset_id) { return static_cast<std::size_t>(preset_id & (PresetCount - 1)); }
  std::array<std::uint32_t, PresetCount / 32> positions_{};
};

enum class RelayOperation : std::uint8_t { Open = 0, Close = 1, Toggle = 2 };
enum class NavigationOperation : std::uint8_t {
  BankUp = 0,
  BankDown = 1,
  BankSet = 2,
  PageUp = 3,
  PageDown = 4,
  PageSet = 5,
};

struct RelayCommand {
  std::uint8_t contact{};
  RelayOperation operation{RelayOperation::Open};
};

struct NavigationCommand {
  NavigationOperation operation{NavigationOperation::BankUp};
  std::uint8_t target{};
};

class ActionSink {
 public:
  virtual ~ActionSink() = default;
  virtual bool send_midi(Destination destination, MidiMessage message) = 0;
  virtual bool relay(RelayCommand command) = 0;
  virtual bool navigate(NavigationCommand command) = 0;
};

struct ActionResult {
  bool accepted{true};
  bool toggled{};
  std::size_t attempted{};
};

class ActionEngine {
 public:
  explicit ActionEngine(ActionSink& sink) : sink_(sink) {}

  ActionResult execute(const Preset& preset, EngineState& state, const SwitchEvent& event) const;

 private:
  static bool event_matches(Trigger trigger, SwitchEventKind event);
  static bool position_matches(PositionFilter filter, Position position);
  static MidiMessage encode_midi(const Message& message);
  static RelayCommand relay_command(const Message& message);
  static NavigationCommand navigation_command(const Message& message);

  ActionSink& sink_;
};

}  // namespace midi
