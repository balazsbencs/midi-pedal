#include "pico_expression_adc.hpp"

#include "../board/pico2_pins.hpp"

#ifdef PICO_ON_DEVICE
#include "hardware/adc.h"
#endif

namespace midi {

void PicoExpressionAdc::initialize() {
#ifdef PICO_ON_DEVICE
  adc_init();
  adc_gpio_init(board::ExpressionAdc);
  adc_select_input(AdcInput);
#endif
}

std::uint16_t PicoExpressionAdc::read() const {
#ifdef PICO_ON_DEVICE
  return adc_read();
#else
  return 0;
#endif
}

}  // namespace midi
