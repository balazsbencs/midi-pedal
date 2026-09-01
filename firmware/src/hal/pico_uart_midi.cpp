#include "pico_uart_midi.hpp"

#include "../board/pico2_pins.hpp"

#ifdef PICO_ON_DEVICE
#include "hardware/gpio.h"
#include "hardware/uart.h"
#endif

namespace midi {

void PicoUartMidi::initialize() {
#ifdef PICO_ON_DEVICE
  uart_init(uart1, BaudRate);
  gpio_set_function(board::MidiTx, GPIO_FUNC_UART);
  uart_set_format(uart1, 8, 1, UART_PARITY_NONE);
  uart_set_fifo_enabled(uart1, true);
#endif
}

bool PicoUartMidi::enqueue(Destination destination, MidiMessage message) {
  if (destination == Destination::Usb) return true;
  return queue_.push(message);
}

void PicoUartMidi::service() {
#ifdef PICO_ON_DEVICE
  if (!uart_is_writable(uart1)) return;
  MidiMessage message{};
  if (!queue_.pop(message)) return;
  for (std::uint8_t index = 0; index < message.length && index < message.bytes.size(); ++index) uart_putc_raw(uart1, message.bytes[index]);
#endif
}

}  // namespace midi
