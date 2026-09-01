#include <array>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "core/action_engine.hpp"

namespace {
struct RecordingSink : midi::ActionSink {
  std::vector<std::string> order;
  std::vector<midi::MidiMessage> midi;
  std::vector<midi::RelayCommand> relays;
  std::vector<midi::NavigationCommand> navigation;

  bool send_midi(midi::Destination destination, midi::MidiMessage message) override {
    order.push_back(destination == midi::Destination::Trs ? "trs" : destination == midi::Destination::Usb ? "usb" : "both");
    midi.push_back(message);
    return true;
  }
  bool relay(midi::RelayCommand command) override { order.emplace_back("relay"); relays.push_back(command); return true; }
  bool navigate(midi::NavigationCommand command) override { order.emplace_back("nav"); navigation.push_back(command); return true; }
};

midi::Preset test_preset() {
  midi::Preset preset{};
  preset.id = 1;
  preset.position1.label.length = 1;
  preset.position2.label.length = 1;
  preset.toggleOn = static_cast<std::uint8_t>(midi::Trigger::Press);
  preset.slotCount = 2;
  preset.slots[0] = {11, midi::Trigger::Press, midi::PositionFilter::Position1,
                     {midi::MessageKind::Cc, midi::Destination::Both, 1, 17, 127, 0, 0, 0}};
  preset.slots[1] = {12, midi::Trigger::Press, midi::PositionFilter::Position1,
                     {midi::MessageKind::Relay, midi::Destination::Both, 0, 0, 0, 1, 1, 0}};
  return preset;
}
}  // namespace

TEST(ActionEngine, QueuesMatchingSlotsBeforeToggling) {
  RecordingSink sink;
  midi::ActionEngine engine(sink);
  midi::EngineState state;
  const auto result = engine.execute(test_preset(), state, {midi::SwitchEventKind::Press, midi::SwitchId::A, midi::Chord::None, 100});
  ASSERT_TRUE(result.accepted);
  ASSERT_EQ(sink.order, (std::vector<std::string>{"both", "relay"}));
  ASSERT_EQ(sink.midi.size(), 1U);
  EXPECT_EQ(sink.midi[0].length, 3U);
  EXPECT_EQ(sink.midi[0].bytes[0], 0xb0U);
  EXPECT_EQ(sink.midi[0].bytes[1], 17U);
  EXPECT_EQ(sink.midi[0].bytes[2], 127U);
  EXPECT_EQ(sink.relays[0].operation, midi::RelayOperation::Close);
  EXPECT_EQ(state.position(1), midi::Position::Two);
}

TEST(ActionEngine, UsesPositionBeforeToggleForSecondPress) {
  RecordingSink sink;
  midi::ActionEngine engine(sink);
  midi::EngineState state;
  const auto preset = test_preset();
  engine.execute(preset, state, {midi::SwitchEventKind::Press, midi::SwitchId::A, midi::Chord::None, 100});
  sink.order.clear(); sink.midi.clear(); sink.relays.clear();
  const auto result = engine.execute(preset, state, {midi::SwitchEventKind::Press, midi::SwitchId::A, midi::Chord::None, 700});
  EXPECT_TRUE(result.accepted);
  EXPECT_TRUE(sink.order.empty());
  EXPECT_EQ(state.position(1), midi::Position::One);
}

TEST(ActionEngine, ReportsRejectedCommandWithoutToggling) {
  struct RejectingSink final : RecordingSink {
    bool relay(midi::RelayCommand) override { return false; }
  } sink;
  midi::ActionEngine engine(sink);
  midi::EngineState state;
  const auto result = engine.execute(test_preset(), state, {midi::SwitchEventKind::Press, midi::SwitchId::A, midi::Chord::None, 100});
  EXPECT_FALSE(result.accepted);
  EXPECT_EQ(state.position(1), midi::Position::One);
}
