#include <algorithm>
#include <cstdint>

#include <gtest/gtest.h>

#include "usb/usb_descriptors.hpp"

TEST(UsbDescriptor, ExposesCdcAndMidiInterfacesWithBoundedEndpoints) {
  const auto* bytes = midi::usb::configuration_descriptor;
  const auto* end = bytes + midi::usb::configuration_descriptor_length;
  bool cdcControl = false;
  bool cdcData = false;
  bool midiStreaming = false;
  for (auto* cursor = bytes; cursor + 2 <= end;) {
    const auto length = cursor[0];
    if (length < 2 || cursor + length > end) break;
    if (cursor[1] == 4 && length >= 9) {
      const auto interfaceClass = cursor[5];
      const auto interfaceSubclass = cursor[6];
      if (interfaceClass == 2 && interfaceSubclass == 2) cdcControl = true;
      if (interfaceClass == 10) cdcData = true;
      if (interfaceClass == 1 && interfaceSubclass == 3) midiStreaming = true;
    }
    if (cursor[1] == 5 && length >= 7) EXPECT_LT(cursor[2] & 0x7f, 0x80);
    cursor += length;
  }
  EXPECT_TRUE(cdcControl);
  EXPECT_TRUE(cdcData);
  EXPECT_TRUE(midiStreaming);
  ASSERT_GE(midi::usb::product_string_length, 4U);
  EXPECT_EQ(midi::usb::product_string[1], 3U);
  EXPECT_EQ(midi::usb::product_string[2], 'M');
  EXPECT_EQ(midi::usb::product_string[4], 'I');
}
