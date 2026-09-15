#include "pico/stdlib.h"

#include "display/st7796s.hpp"
#include "hal/ports.hpp"

int main() {
  stdio_init_all();

  midi::display::St7796sDisplay display;
  display.initialize();

  midi::LiveView view{};
  view.bank = 1;
  view.page = 1;
  view.positions = {1, 2, 1, 2};
  view.expressionAvailable = false;
  view.usbConnected = true;
  display.present(view);

  while (true) tight_loop_contents();
}
