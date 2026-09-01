#include "usb_descriptors.hpp"

namespace midi::usb {

const std::uint8_t configuration_descriptor[] = {
  9, 2, 167, 0, 6, 1, 0, 0x80, 50,
  8, 11, 0, 2, 2, 2, 0, 0,
  9, 4, 0, 0, 1, 2, 2, 0, 0,
  5, 0x24, 0, 0x20, 0x01,
  5, 0x24, 1, 0, 1,
  4, 0x24, 2, 6,
  5, 0x24, 6, 0, 1,
  7, 5, 0x81, 3, 8, 0, 16,
  9, 4, 1, 0, 2, 10, 0, 0, 0,
  7, 5, 0x02, 2, 64, 0, 0,
  7, 5, 0x82, 2, 64, 0, 0,
  9, 4, 2, 0, 0, 1, 1, 0, 0,
  9, 0x24, 1, 0, 1, 9, 0, 1, 3,
  9, 4, 3, 0, 2, 1, 3, 0, 0,
  7, 0x24, 1, 0, 1, 65, 0,
  6, 0x24, 2, 1, 1, 0,
  6, 0x24, 2, 2, 2, 0,
  9, 0x24, 3, 1, 3, 1, 2, 1, 0,
  9, 0x24, 3, 2, 4, 1, 1, 1, 0,
  9, 5, 0x03, 2, 64, 0, 0, 0, 0,
  5, 0x25, 1, 1, 1,
  9, 5, 0x83, 2, 64, 0, 0, 0, 0,
  5, 0x25, 1, 1, 3,
};

const std::size_t configuration_descriptor_length = sizeof(configuration_descriptor);

const std::uint8_t product_string[] = {
  2 + 2 * 17, 3,
  'M', 0, 'I', 0, 'D', 0, 'I', 0, ' ', 0, 'P', 0, 'e', 0, 'd', 0, 'a', 0, 'l', 0, ' ', 0,
  'P', 0, 'i', 0, 'c', 0, 'o', 0, ' ', 0, '2', 0,
};
const std::size_t product_string_length = sizeof(product_string);

}  // namespace midi::usb
