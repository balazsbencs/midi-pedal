#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace midi {

struct LiveView {
  std::uint8_t bank{};
  std::uint8_t page{};
  std::array<std::uint8_t, 4> positions{};
  bool expressionAvailable{};
  std::uint8_t expressionValue{};
};

struct MidiMessage {
  std::array<std::uint8_t, 3> bytes{};
  std::uint8_t length{};
};

// Defined by the shared configuration model. A forward declaration keeps the
// HAL contracts lightweight while allowing ports and configuration to use the
// same stable enum.
enum class Destination : std::uint8_t;

struct Clock {
  virtual ~Clock() = default;
  virtual std::uint32_t now_ms() const = 0;
};

struct RelayPort {
  virtual ~RelayPort() = default;
  virtual void set(std::uint8_t contact, bool closed) = 0;
};

struct MidiPort {
  virtual ~MidiPort() = default;
  virtual bool enqueue(Destination destination, MidiMessage message) = 0;
};

struct DisplayPort {
  virtual ~DisplayPort() = default;
  virtual void present(const LiveView& view) = 0;
};

struct ConfigStore {
  virtual ~ConfigStore() = default;
  virtual bool read_active_config() = 0;
};

struct DiagnosticPort {
  virtual ~DiagnosticPort() = default;
  virtual void record(std::uint16_t code) = 0;
};

struct ControllerPorts {
  virtual ~ControllerPorts() = default;
  virtual void relay_set(std::uint8_t contact, bool closed) = 0;
  virtual bool read_active_config() = 0;
  virtual void show_boot_status() = 0;
};

}  // namespace midi
