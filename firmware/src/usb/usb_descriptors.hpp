#pragma once

#include <cstddef>
#include <cstdint>

namespace midi::usb {

extern const std::uint8_t configuration_descriptor[];
extern const std::size_t configuration_descriptor_length;
extern const std::uint8_t product_string[];
extern const std::size_t product_string_length;

}  // namespace midi::usb
