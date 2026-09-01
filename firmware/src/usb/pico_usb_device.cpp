#include "pico_usb.hpp"

#ifdef PICO_ON_DEVICE

#include <algorithm>
#include <cstddef>
#include <cstring>

#include "pico/unique_id.h"
#include "tusb.h"

#include "usb_descriptors.hpp"

namespace {
constexpr std::uint16_t UsbVendorId = 0x2e8a;
constexpr std::uint16_t UsbProductId = 0x4001;
constexpr std::uint16_t UsbDeviceVersion = 0x0100;

const tusb_desc_device_t DeviceDescriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = UsbVendorId,
    .idProduct = UsbProductId,
    .bcdDevice = UsbDeviceVersion,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

const char* const StringDescriptors[] = {"", "MIDI Pedal Project", "MIDI Pedal Pico 2"};
std::uint16_t StringBuffer[32]{};
char SerialBuffer[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1]{};
}

extern "C" {

std::uint8_t const* tud_descriptor_device_cb() { return reinterpret_cast<std::uint8_t const*>(&DeviceDescriptor); }

std::uint8_t const* tud_descriptor_configuration_cb(std::uint8_t) { return midi::usb::configuration_descriptor; }

std::uint16_t const* tud_descriptor_string_cb(std::uint8_t index, std::uint16_t) {
  std::size_t count = 0;
  if (index == 0) {
    StringBuffer[1] = 0x0409;
    count = 1;
  } else {
    const char* value = nullptr;
    if (index == 1) value = StringDescriptors[1];
    else if (index == 2) value = StringDescriptors[2];
    else if (index == 3) {
      if (SerialBuffer[0] == '\0') pico_get_unique_board_id_string(SerialBuffer, sizeof(SerialBuffer));
      value = SerialBuffer;
    }
    if (value == nullptr) return nullptr;
    count = std::min<std::size_t>(std::strlen(value), (sizeof(StringBuffer) / sizeof(StringBuffer[0])) - 1);
    for (std::size_t offset = 0; offset < count; ++offset) StringBuffer[1 + offset] = static_cast<std::uint8_t>(value[offset]);
  }
  StringBuffer[0] = static_cast<std::uint16_t>((TUSB_DESC_STRING << 8) | (2 * count + 2));
  return StringBuffer;
}

}  // extern "C"

namespace midi::usb {

bool PicoUsbDeviceApi::mounted() const { return tud_mounted(); }

std::size_t PicoUsbDeviceApi::cdc_available() const { return tud_cdc_available(); }

std::size_t PicoUsbDeviceApi::cdc_read(std::span<std::byte> output) {
  return tud_cdc_read(output.data(), static_cast<std::uint32_t>(output.size()));
}

std::size_t PicoUsbDeviceApi::cdc_write_available() const { return tud_cdc_write_available(); }

std::size_t PicoUsbDeviceApi::cdc_write(std::span<const std::byte> bytes) {
  return tud_cdc_write(bytes.data(), static_cast<std::uint32_t>(bytes.size()));
}

void PicoUsbDeviceApi::cdc_flush() { (void)tud_cdc_write_flush(); }

bool PicoUsbDeviceApi::midi_mounted() const { return tud_midi_mounted(); }

bool PicoUsbDeviceApi::midi_write(std::span<const std::uint8_t> packet) {
  return packet.size() == 4 && tud_midi_packet_write(packet.data());
}

bool PicoUsbDeviceApi::initialize() { return tud_init(0); }

void PicoUsbDeviceApi::task() { tud_task(); }

}  // namespace midi::usb

#else

namespace midi::usb {

bool PicoUsbDeviceApi::mounted() const { return false; }
std::size_t PicoUsbDeviceApi::cdc_available() const { return 0; }
std::size_t PicoUsbDeviceApi::cdc_read(std::span<std::byte>) { return 0; }
std::size_t PicoUsbDeviceApi::cdc_write_available() const { return 0; }
std::size_t PicoUsbDeviceApi::cdc_write(std::span<const std::byte>) { return 0; }
void PicoUsbDeviceApi::cdc_flush() {}
bool PicoUsbDeviceApi::midi_mounted() const { return false; }
bool PicoUsbDeviceApi::midi_write(std::span<const std::uint8_t>) { return false; }
bool PicoUsbDeviceApi::initialize() { return false; }
void PicoUsbDeviceApi::task() {}

}  // namespace midi::usb

#endif
