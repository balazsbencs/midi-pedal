#include <array>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <span>
#include <string>
#include <vector>

#include <gtest/gtest.h>

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

class FakeTarget final : public midi::display::RenderTarget {
 public:
  struct Transfer {
    midi::display::Rect rect{};
    std::vector<std::uint16_t> pixels;
  };

  void write(midi::display::Rect rect, std::span<const std::uint16_t> pixels) override {
    transfers.push_back(Transfer{rect, std::vector<std::uint16_t>(pixels.begin(), pixels.end())});
  }

  std::vector<Transfer> transfers;
};

midi::LiveView base_view() {
  midi::LiveView view{};
  view.bank = 1;
  view.page = 1;
  view.positions = {1, 2, 1, 2};
  view.expressionAvailable = true;
  view.expressionValue = 96;
  view.usbConnected = true;
  return view;
}

bool has_rect_in(const FakeTarget& target, std::uint16_t x, std::uint16_t y,
                std::uint16_t width, std::uint16_t height) {
  for (const auto& transfer : target.transfers) {
    if (transfer.rect.x == x && transfer.rect.y == y && transfer.rect.width == width &&
        transfer.rect.height > 0 && transfer.rect.height <= height) {
      return true;
    }
  }
  return false;
}

bool has_color_in(const FakeTarget& target, midi::display::Rect area, std::uint16_t color) {
  for (const auto& transfer : target.transfers) {
    for (std::uint16_t row = 0; row < transfer.rect.height; ++row) {
      for (std::uint16_t column = 0; column < transfer.rect.width; ++column) {
        const auto x = static_cast<std::uint16_t>(transfer.rect.x + column);
        const auto y = static_cast<std::uint16_t>(transfer.rect.y + row);
        if (x >= area.x && x < area.x + area.width && y >= area.y && y < area.y + area.height &&
            transfer.pixels[static_cast<std::size_t>(row) * transfer.rect.width + column] == color) {
          return true;
        }
      }
    }
  }
  return false;
}

std::vector<std::uint8_t> rasterize(const std::vector<FakeTarget::Transfer>& transfers) {
  std::vector<std::uint16_t> screen(static_cast<std::size_t>(midi::display::ScreenWidth) *
                                        midi::display::ScreenHeight,
                                    midi::display::ColorBackground);
  for (const auto& transfer : transfers) {
    for (std::uint16_t row = 0; row < transfer.rect.height; ++row) {
      for (std::uint16_t column = 0; column < transfer.rect.width; ++column) {
        screen[(static_cast<std::size_t>(transfer.rect.y + row) * midi::display::ScreenWidth) +
               transfer.rect.x + column] = transfer.pixels[static_cast<std::size_t>(row) * transfer.rect.width + column];
      }
    }
  }
  std::vector<std::uint8_t> bytes;
  bytes.reserve(screen.size() * 2);
  for (const auto pixel : screen) {
    bytes.push_back(static_cast<std::uint8_t>(pixel >> 8));
    bytes.push_back(static_cast<std::uint8_t>(pixel));
  }
  return bytes;
}

}  // namespace

TEST(LiveRenderer, UsesApprovedLandscapeGeometryAndDirtyRegions) {
  FakeTarget target;
  midi::display::LiveRenderer renderer(target);
  renderer.render(base_view());

  ASSERT_FALSE(target.transfers.empty());
  EXPECT_TRUE(has_rect_in(target, 0, 0, midi::display::ScreenWidth, midi::display::HeaderHeight));
  EXPECT_TRUE(has_rect_in(target, 0, midi::display::HeaderHeight, midi::display::QuadrantWidth,
                         midi::display::QuadrantHeight));
  EXPECT_TRUE(has_rect_in(target, midi::display::QuadrantWidth, midi::display::HeaderHeight,
                         midi::display::QuadrantWidth, midi::display::QuadrantHeight));
  EXPECT_TRUE(has_rect_in(target, 0, midi::display::HeaderHeight + midi::display::QuadrantHeight,
                         midi::display::QuadrantWidth, midi::display::QuadrantHeight));
  EXPECT_TRUE(has_rect_in(target, midi::display::QuadrantWidth,
                         midi::display::HeaderHeight + midi::display::QuadrantHeight,
                         midi::display::QuadrantWidth, midi::display::QuadrantHeight));
  EXPECT_TRUE(has_rect_in(target, 0, midi::display::FooterY, midi::display::ScreenWidth,
                         midi::display::FooterHeight));
  for (const auto& transfer : target.transfers) {
    EXPECT_EQ(transfer.pixels.size(),
              static_cast<std::size_t>(transfer.rect.width) * transfer.rect.height);
  }
}

TEST(LiveRenderer, DoesNotTransferUnchangedView) {
  FakeTarget target;
  midi::display::LiveRenderer renderer(target);
  const auto view = base_view();
  renderer.render(view);
  target.transfers.clear();
  renderer.render(view);
  EXPECT_TRUE(target.transfers.empty());
}

TEST(LiveRenderer, RedrawsHeaderWhenWatchdogResetDiagnosticChanges) {
  FakeTarget target;
  midi::display::LiveRenderer renderer(target);
  auto view = base_view();
  renderer.render(view);
  target.transfers.clear();

  view.watchdogReset = true;
  renderer.render(view);

  EXPECT_TRUE(has_rect_in(target, 0, 0, midi::display::ScreenWidth, midi::display::HeaderHeight));
}

TEST(LiveRenderer, RedrawsTheNamedHeaderAndAffectedQuadrantOnly) {
  FakeTarget target;
  midi::display::LiveRenderer renderer(target);
  auto view = base_view();
  view.bankName = ascii<20>("STAGE");
  view.selectedPositions[1].label = ascii<12>("RHY");
  view.selectedPositions[1].accentRgb565 = midi::display::ColorSuccess;
  renderer.render(view);
  target.transfers.clear();

  view.bankName = ascii<20>("AMP");
  view.selectedPositions[1].label = ascii<12>("CRN");
  renderer.render(view);

  EXPECT_TRUE(has_rect_in(target, 0, 0, midi::display::ScreenWidth, midi::display::HeaderHeight));
  EXPECT_TRUE(has_rect_in(target, midi::display::QuadrantWidth, midi::display::HeaderHeight,
                          midi::display::QuadrantWidth, midi::display::QuadrantHeight));
  for (const auto& transfer : target.transfers) {
    const bool header = transfer.rect.y < midi::display::HeaderHeight;
    const bool quadrant_b = transfer.rect.x == midi::display::QuadrantWidth &&
                            transfer.rect.y >= midi::display::HeaderHeight &&
                            transfer.rect.y < midi::display::FooterY;
    EXPECT_TRUE(header || quadrant_b);
  }
}

TEST(LiveRenderer, RendersConfiguredSelectedLabelAccentAndWatchdogDiagnostic) {
  FakeTarget target;
  midi::display::LiveRenderer renderer(target);
  auto view = base_view();
  view.selectedPositions[0].label = ascii<12>("LEAD");
  view.selectedPositions[0].accentRgb565 = midi::display::ColorSuccess;
  view.watchdogReset = true;
  renderer.render(view);

  EXPECT_TRUE(has_color_in(target, {64, static_cast<std::uint16_t>(midi::display::HeaderHeight + 18),
                                    48, 14},
                           midi::display::ColorSuccess));
  EXPECT_TRUE(has_color_in(target, {0, 0, midi::display::ScreenWidth, midi::display::HeaderHeight},
                           midi::display::ColorWarning));
}

TEST(LiveRenderer, RedrawsFooterWhenExpressionAssignmentLabelChanges) {
  FakeTarget target;
  midi::display::LiveRenderer renderer(target);
  auto view = base_view();
  view.expressionLabel = ascii<12>("VOL");
  renderer.render(view);
  target.transfers.clear();

  view.expressionLabel = ascii<12>("WAH");
  renderer.render(view);

  EXPECT_TRUE(has_rect_in(target, 0, midi::display::FooterY,
                          midi::display::ScreenWidth, midi::display::FooterHeight));
}

TEST(LiveRenderer, OnlyUpdatesChangedQuadrantAndExpressionFooter) {
  FakeTarget target;
  midi::display::LiveRenderer renderer(target);
  auto view = base_view();
  renderer.render(view);
  target.transfers.clear();

  view.positions[2] = 2;
  view.expressionAvailable = false;
  renderer.render(view);

  ASSERT_FALSE(target.transfers.empty());
  for (const auto& transfer : target.transfers) {
    const bool quadrant_c = transfer.rect.x == 0 &&
        transfer.rect.y >= midi::display::HeaderHeight + midi::display::QuadrantHeight &&
        transfer.rect.y < midi::display::FooterY;
    const bool footer = transfer.rect.y >= midi::display::FooterY;
    EXPECT_TRUE(quadrant_c || footer);
  }
}

TEST(LiveRenderer, ShowsUnavailableExpressionAndPositionStateWithStableColors) {
  FakeTarget target;
  midi::display::LiveRenderer renderer(target);
  auto view = base_view();
  view.expressionAvailable = false;
  view.configurationError = true;
  view.queueOverflow = true;
  renderer.render(view);

  bool saw_error = false;
  bool saw_background = false;
  for (const auto& transfer : target.transfers) {
    for (const auto pixel : transfer.pixels) {
      saw_error = saw_error || pixel == midi::display::ColorError;
      saw_background = saw_background || pixel == midi::display::ColorBackground;
    }
  }
  EXPECT_TRUE(saw_error);
  EXPECT_TRUE(saw_background);
}

TEST(LiveRenderer, MatchesFactoryEmptyGoldenRaster) {
  FakeTarget target;
  midi::display::LiveRenderer renderer(target);
  auto view = base_view();
  view.expressionAvailable = false;
  view.usbConnected = false;
  view.positions = {1, 1, 1, 1};
  renderer.render(view);

  std::ifstream golden(std::string(MIDI_PEDAL_SOURCE_DIR) +
                       "/firmware/tests/golden/display/factory-empty.rgb565",
                       std::ios::binary);
  ASSERT_TRUE(golden.good());
  const std::vector<std::uint8_t> expected((std::istreambuf_iterator<char>(golden)),
                                           std::istreambuf_iterator<char>());
  EXPECT_EQ(rasterize(target.transfers), expected);
}
