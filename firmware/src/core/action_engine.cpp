#include "action_engine.hpp"

namespace midi {

Position EngineState::position(std::uint32_t preset_id) const {
  const auto index = bit_index(preset_id);
  return ((positions_[index / 32] >> (index % 32)) & 1u) == 0 ? Position::One : Position::Two;
}

void EngineState::set_position(std::uint32_t preset_id, Position position_value) {
  const auto index = bit_index(preset_id);
  auto& word = positions_[index / 32];
  const auto bit = static_cast<std::uint32_t>(1u << (index % 32));
  if (position_value == Position::Two) word |= bit;
  else word &= ~bit;
}

void EngineState::toggle(std::uint32_t preset_id) {
  const auto index = bit_index(preset_id);
  positions_[index / 32] ^= static_cast<std::uint32_t>(1u << (index % 32));
}

bool ActionEngine::event_matches(Trigger trigger, SwitchEventKind event) {
  switch (event) {
    case SwitchEventKind::Press: return trigger == Trigger::Press;
    case SwitchEventKind::Release: return trigger == Trigger::Release;
    case SwitchEventKind::LongPress: return trigger == Trigger::LongPress;
    case SwitchEventKind::DoubleTap: return trigger == Trigger::DoubleTap;
    default: return false;
  }
}

bool ActionEngine::position_matches(PositionFilter filter, Position position) {
  return filter == PositionFilter::Both ||
         (filter == PositionFilter::Position1 && position == Position::One) ||
         (filter == PositionFilter::Position2 && position == Position::Two);
}

MidiMessage ActionEngine::encode_midi(const Message& message) {
  MidiMessage output{};
  if (message.kind == MessageKind::Pc) {
    output.length = 2;
    output.bytes = {static_cast<std::uint8_t>(0xc0u | (message.channel - 1u)), message.data1, 0};
  } else {
    output.length = 3;
    output.bytes = {static_cast<std::uint8_t>(0xb0u | (message.channel - 1u)), message.data1, message.data2};
  }
  return output;
}

RelayCommand ActionEngine::relay_command(const Message& message) {
  return {message.contact, static_cast<RelayOperation>(message.operation)};
}

NavigationCommand ActionEngine::navigation_command(const Message& message) {
  return {static_cast<NavigationOperation>(message.operation), message.target};
}

ActionResult ActionEngine::execute(const Preset& preset, EngineState& state, const SwitchEvent& event) const {
  ActionResult result{};
  if (event.kind == SwitchEventKind::ChordPress || event.kind == SwitchEventKind::ChordRepeat || event.kind == SwitchEventKind::ChordRelease) return result;

  const auto current = state.position(preset.id);
  for (std::size_t index = 0; index < preset.slotCount && index < preset.slots.size(); ++index) {
    const auto& slot = preset.slots[index];
    if (!event_matches(slot.trigger, event.kind) || !position_matches(slot.position, current)) continue;
    ++result.attempted;
    bool accepted = true;
    switch (slot.message.kind) {
      case MessageKind::Pc:
      case MessageKind::Cc:
        accepted = sink_.send_midi(slot.message.destination, encode_midi(slot.message));
        break;
      case MessageKind::Relay:
        accepted = sink_.relay(relay_command(slot.message));
        break;
      case MessageKind::Navigation:
        accepted = sink_.navigate(navigation_command(slot.message));
        break;
    }
    if (!accepted) result.accepted = false;
  }

  const auto toggle_trigger = preset.toggleOn;
  if (toggle_trigger != 255 && event_matches(static_cast<Trigger>(toggle_trigger), event.kind)) {
    if (result.accepted) {
      state.toggle(preset.id);
      result.toggled = true;
    } else {
      result.accepted = false;
    }
  }
  return result;
}

}  // namespace midi
