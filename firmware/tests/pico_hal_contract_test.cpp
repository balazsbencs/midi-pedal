#include <array>
#include <cstdint>

#include <gtest/gtest.h>

#include "board/pico2_pins.hpp"
#include "hal/pico_expression_adc.hpp"
#include "hal/pico_relays.hpp"
#include "hal/pico_switches.hpp"
#include "hal/pico_uart_midi.hpp"
#include "hal/pico_watchdog.hpp"

TEST(PicoHalContract, UsesApprovedPinsAndElectricalDefaults) {
  EXPECT_EQ(midi::board::SwitchA, 2U);
  EXPECT_EQ(midi::board::SwitchD, 5U);
  EXPECT_EQ(midi::board::MidiTx, 8U);
  EXPECT_EQ(midi::board::Relay1, 10U);
  EXPECT_EQ(midi::board::Relay2, 11U);
  EXPECT_EQ(midi::board::ExpressionAdc, 26U);
  EXPECT_EQ(midi::PicoUartMidi::BaudRate, 31250U);
  EXPECT_EQ(midi::PicoExpressionAdc::AdcInput, 0U);
  EXPECT_TRUE(midi::PicoRelays::contacts_default_open());
}

TEST(PicoHalContract, SwitchMaskUsesStableBitOrder) {
  EXPECT_EQ(midi::PicoSwitches::mask_for_index(0), 0x01U);
  EXPECT_EQ(midi::PicoSwitches::mask_for_index(3), 0x08U);
}

TEST(PicoHalContract, WatchdogIsSafeToInitializeOnTheHost) {
  midi::PicoWatchdog watchdog;

  EXPECT_EQ(midi::PicoWatchdog::TimeoutMs, 2000U);
  EXPECT_FALSE(watchdog.initialize());
  watchdog.feed();
  watchdog.feed();
}
