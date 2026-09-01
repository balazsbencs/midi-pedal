#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "protocol_dispatcher.hpp"
#include "usb_transport.hpp"

namespace midi::usb {

class UsbDeviceApi {
 public:
  virtual ~UsbDeviceApi() = default;
  [[nodiscard]] virtual bool mounted() const = 0;
  [[nodiscard]] virtual std::size_t cdc_available() const = 0;
  virtual std::size_t cdc_read(std::span<std::byte> output) = 0;
  [[nodiscard]] virtual std::size_t cdc_write_available() const = 0;
  virtual std::size_t cdc_write(std::span<const std::byte> bytes) = 0;
  virtual void cdc_flush() = 0;
  [[nodiscard]] virtual bool midi_mounted() const = 0;
  [[nodiscard]] virtual bool midi_write(std::span<const std::uint8_t> packet) = 0;
  [[nodiscard]] virtual bool initialize() = 0;
  virtual void task() = 0;
};

class PicoUsbPort final : public UsbPort, public ProtocolResponseSink {
 public:
  static constexpr std::size_t CdcQueueCapacity = 4;
  static constexpr std::size_t CdcFrameCapacity = ProtocolDispatcher::ResponseFrameCapacity;

  explicit PicoUsbPort(UsbDeviceApi& api) : api_(api) {}

  [[nodiscard]] bool initialize() { return api_.initialize(); }
  void service(ProtocolDispatcher& dispatcher);

  [[nodiscard]] bool mounted() const override { return api_.mounted(); }
  [[nodiscard]] bool write_cdc(std::span<const std::byte> bytes) override { return enqueue_cdc(bytes); }
  [[nodiscard]] bool write_midi(std::span<const std::uint8_t> packet) override;
  [[nodiscard]] bool write_frame(std::span<const std::byte> bytes) override { return enqueue_cdc(bytes); }
  [[nodiscard]] std::size_t queued_cdc_frames() const { return queue_size_; }

 private:
  struct PendingFrame {
    std::array<std::byte, CdcFrameCapacity> bytes{};
    std::size_t length{};
    std::size_t offset{};
  };

  [[nodiscard]] bool enqueue_cdc(std::span<const std::byte> bytes);
  void flush_cdc();

  UsbDeviceApi& api_;
  std::array<PendingFrame, CdcQueueCapacity> queue_{};
  std::size_t queue_read_{};
  std::size_t queue_write_{};
  std::size_t queue_size_{};
};

class PicoUsbDeviceApi final : public UsbDeviceApi {
 public:
  [[nodiscard]] bool mounted() const override;
  [[nodiscard]] std::size_t cdc_available() const override;
  std::size_t cdc_read(std::span<std::byte> output) override;
  [[nodiscard]] std::size_t cdc_write_available() const override;
  std::size_t cdc_write(std::span<const std::byte> bytes) override;
  void cdc_flush() override;
  [[nodiscard]] bool midi_mounted() const override;
  [[nodiscard]] bool midi_write(std::span<const std::uint8_t> packet) override;
  [[nodiscard]] bool initialize() override;
  void task() override;
};

}  // namespace midi::usb
