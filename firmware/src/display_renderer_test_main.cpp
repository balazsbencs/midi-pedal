#include <algorithm>
#include <string_view>

#include "pico/stdlib.h"

#include "display/st7796s.hpp"
#include "hal/ports.hpp"

namespace {

template <std::size_t Capacity>
midi::AsciiString<Capacity> ascii(std::string_view value) {
  midi::AsciiString<Capacity> result{};
  result.length = static_cast<std::uint8_t>(
      std::min<std::size_t>(value.size(), Capacity));
  std::copy_n(value.begin(), result.length, result.data.begin());
  return result;
}

}  // namespace

int main() {
  stdio_init_all();

  midi::display::St7796sDisplay display;
  display.initialize();

  midi::LiveView view{};
  view.bank = 7;
  view.page = 2;
  view.bankName = ascii<20>("MIDNIGHT SET");
  view.positions = {1, 2, 1, 2};
  view.selectedPositions = {{{ascii<12>("CLEAN"), 0x2F3A},
                             {ascii<12>("DRIVE"), 0xF2EB},
                             {ascii<12>("CHORUS"), 0xCCDF},
                             {ascii<12>("DELAY"), 0xFE48}}};
  view.expressionAvailable = true;
  view.expressionValue = 96;
  view.expressionLabel = ascii<12>("VOLUME");
  view.usbConnected = true;
  display.present(view);

  auto last_toggle_ms = to_ms_since_boot(get_absolute_time());
  while (true) {
    display.service();
    const auto now_ms = to_ms_since_boot(get_absolute_time());
    if (static_cast<std::uint32_t>(now_ms - last_toggle_ms) >= 1500U) {
      view.positions[1] = view.positions[1] == 2 ? 1 : 2;
      display.present(view);
      last_toggle_ms = now_ms;
    }
    tight_loop_contents();
  }
}
