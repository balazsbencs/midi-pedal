#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "display/live_renderer.hpp"

namespace {

template <std::size_t Capacity>
midi::AsciiString<Capacity> ascii(std::string_view value) {
  midi::AsciiString<Capacity> output{};
  const auto length = std::min(value.size(), Capacity);
  for (std::size_t index = 0; index < length; ++index) output.data[index] = value[index];
  output.length = static_cast<std::uint8_t>(length);
  return output;
}

class FileTarget final : public midi::display::RenderTarget {
 public:
  void write(midi::display::Rect rect, std::span<const std::uint16_t> pixels) override {
    for (std::uint16_t row = 0; row < rect.height; ++row) {
      for (std::uint16_t column = 0; column < rect.width; ++column) {
        screen[(static_cast<std::size_t>(rect.y + row) * midi::display::ScreenWidth) +
               rect.x + column] = pixels[static_cast<std::size_t>(row) * rect.width + column];
      }
    }
  }

  std::vector<std::uint16_t> screen =
      std::vector<std::uint16_t>(static_cast<std::size_t>(midi::display::ScreenWidth) *
                                     midi::display::ScreenHeight,
                                 midi::display::ColorBackground);
};

midi::LiveView view_for(std::string_view name) {
  midi::LiveView view{};
  view.bank = 1;
  view.page = 1;
  view.positions = {1, 1, 1, 1};
  view.usbConnected = true;
  view.expressionAvailable = true;
  view.expressionValue = 64;
  if (name == "factory-empty" || name == "expression-unavailable") {
    view.expressionAvailable = false;
    view.usbConnected = false;
    view.bankName = {};
    view.selectedPositions = {};
    view.expressionLabel = {};
  } else if (name == "toggle-position-2") {
    view.bankName = ascii<20>("STAGE");
    view.selectedPositions = {{{ascii<12>("LEAD"), midi::display::ColorAccent},
                              {ascii<12>("RHY"), midi::display::ColorSuccess},
                              {ascii<12>("FX"), midi::display::ColorWarning},
                              {ascii<12>("TAP"), midi::display::ColorForeground}}};
    view.expressionLabel = ascii<12>("VOL");
    view.positions = {2, 2, 2, 2};
    view.expressionValue = 96;
  } else if (name == "recoverable-error") {
    view.bankName = ascii<20>("STAGE");
    view.selectedPositions = {{{ascii<12>("LEAD"), midi::display::ColorAccent},
                              {ascii<12>("RHY"), midi::display::ColorSuccess},
                              {ascii<12>("FX"), midi::display::ColorWarning},
                              {ascii<12>("TAP"), midi::display::ColorForeground}}};
    view.expressionLabel = ascii<12>("VOL");
    view.configurationError = true;
    view.queueOverflow = true;
    view.watchdogReset = true;
  } else {
    view.bankName = ascii<20>("STAGE");
    view.selectedPositions = {{{ascii<12>("LEAD"), midi::display::ColorAccent},
                              {ascii<12>("RHY"), midi::display::ColorSuccess},
                              {ascii<12>("FX"), midi::display::ColorWarning},
                              {ascii<12>("TAP"), midi::display::ColorForeground}}};
    view.expressionLabel = ascii<12>("VOL");
  }
  return view;
}

void write_golden(const std::filesystem::path& path, const midi::LiveView& view) {
  FileTarget target;
  midi::display::LiveRenderer renderer(target);
  renderer.render(view);
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  for (const auto pixel : target.screen) {
    const auto high = static_cast<char>(pixel >> 8);
    const auto low = static_cast<char>(pixel);
    output.write(&high, 1);
    output.write(&low, 1);
  }
}

}  // namespace

int main() {
  const std::filesystem::path directory = std::filesystem::path(MIDI_PEDAL_SOURCE_DIR) /
                                          "firmware/tests/golden/display";
  std::filesystem::create_directories(directory);
  for (const auto name : {"factory-empty", "toggle-position-1", "toggle-position-2",
                          "expression-unavailable", "recoverable-error"}) {
    write_golden(directory / (std::string(name) + ".rgb565"), view_for(name));
  }
  return 0;
}
