#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "app/controller.hpp"

namespace {
struct FakeHal : midi::ControllerPorts {
  std::vector<std::string> calls;

  void relay_set(std::uint8_t contact, bool closed) override { calls.push_back("relay" + std::to_string(contact) + (closed ? ":close" : ":open")); }
  bool read_active_config() override { calls.emplace_back("config:read"); return false; }
  void show_boot_status() override { calls.emplace_back("display:boot"); }
  std::size_t index_of(const std::string& value) const {
    for (std::size_t index = 0; index < calls.size(); ++index) if (calls[index] == value) return index;
    return calls.size();
  }
};
}  // namespace

TEST(ControllerBoot, OpensRelaysBeforeReadingConfiguration) {
  FakeHal hal;
  midi::Controller controller(hal);
  controller.initialize();
  ASSERT_GE(hal.calls.size(), 3U);
  EXPECT_EQ(hal.calls[0], "relay1:open");
  EXPECT_EQ(hal.calls[1], "relay2:open");
  EXPECT_LT(hal.index_of("relay2:open"), hal.index_of("config:read"));
}
