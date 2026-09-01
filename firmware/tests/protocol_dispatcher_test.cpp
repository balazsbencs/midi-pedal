#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "core/crc32.hpp"
#include "usb/protocol_dispatcher.hpp"

namespace {
std::vector<std::byte> request(std::uint32_t request_id, std::uint16_t command, std::span<const std::byte> payload = {}) {
  std::vector<std::byte> bytes(18 + payload.size() + 4);
  bytes[0] = std::byte{'M'};
  bytes[1] = std::byte{'P'};
  bytes[2] = std::byte{'C'};
  bytes[3] = std::byte{'F'};
  auto put16 = [&](std::size_t at, std::uint16_t value) {
    bytes[at] = static_cast<std::byte>(value & 0xffu);
    bytes[at + 1] = static_cast<std::byte>(value >> 8u);
  };
  auto put32 = [&](std::size_t at, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) bytes[at + shift / 8] = static_cast<std::byte>(value >> shift);
  };
  put16(4, 1);
  put32(6, request_id);
  put16(10, command);
  put16(12, 0);
  put32(14, static_cast<std::uint32_t>(payload.size()));
  std::copy(payload.begin(), payload.end(), bytes.begin() + 18);
  put32(18 + payload.size(), midi::crc32(std::span<const std::byte>(bytes.data(), 18 + payload.size())));
  return bytes;
}

struct RecordingSink final : midi::usb::ProtocolResponseSink {
  std::vector<std::vector<std::byte>> frames;
  bool accept{true};

  bool write_frame(std::span<const std::byte> bytes) override {
    frames.emplace_back(bytes.begin(), bytes.end());
    return accept;
  }
};

struct FakeService final : midi::usb::ProtocolService {
  std::size_t capabilities_calls{};
  std::size_t config_info_calls{};
  std::size_t read_config_calls{};
  std::size_t begin_upload_calls{};
  std::size_t verify_upload_calls{};
  std::array<std::byte, 3> bank_record{std::byte{0x42}, std::byte{0x43}, std::byte{0x44}};

  midi::usb::ServiceError capabilities(std::span<std::byte> output, std::size_t& size) override {
    ++capabilities_calls;
    const std::array bytes{std::byte{'{'}, std::byte{'}'}};
    std::copy(bytes.begin(), bytes.end(), output.begin());
    size = bytes.size();
    return midi::usb::ServiceError::None;
  }

  midi::usb::ServiceError config_info(std::span<std::byte>, std::size_t& size) override {
    ++config_info_calls;
    size = 0;
    return midi::usb::ServiceError::None;
  }

  midi::usb::ServiceError read_config(std::uint8_t, std::span<std::byte> output, std::size_t& size) override {
    ++read_config_calls;
    std::copy(bank_record.begin(), bank_record.end(), output.begin());
    size = bank_record.size();
    return midi::usb::ServiceError::None;
  }

  midi::usb::ServiceError begin_upload(std::uint32_t, std::uint32_t, std::uint32_t) override {
    ++begin_upload_calls;
    return midi::usb::ServiceError::None;
  }

  midi::usb::ServiceError write_chunk(std::uint32_t, std::span<const std::byte>) override { return midi::usb::ServiceError::None; }
  midi::usb::ServiceError verify_upload() override { ++verify_upload_calls; return midi::usb::ServiceError::None; }
  midi::usb::ServiceError activate_upload() override { return midi::usb::ServiceError::None; }
  midi::usb::ServiceError expression_sample(std::uint16_t& value) override { value = 2048; return midi::usb::ServiceError::None; }
  midi::usb::ServiceError set_expression_calibration(std::uint16_t, std::uint16_t) override { return midi::usb::ServiceError::None; }
  midi::usb::ServiceError factory_empty_reset() override { return midi::usb::ServiceError::None; }
};

std::uint32_t u32(std::span<const std::byte> bytes, std::size_t at) {
  return static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[at]) |
    (std::to_integer<std::uint8_t>(bytes[at + 1]) << 8u) |
    (std::to_integer<std::uint8_t>(bytes[at + 2]) << 16u) |
    (std::to_integer<std::uint8_t>(bytes[at + 3]) << 24u));
}
}  // namespace

TEST(ProtocolDispatcher, EncodesSuccessfulCapabilityResponse) {
  FakeService service;
  RecordingSink sink;
  midi::usb::ProtocolDispatcher dispatcher(service, sink);
  const auto encoded = request(7, static_cast<std::uint16_t>(midi::usb::Command::GetCapabilities));
  dispatcher.receive(encoded);

  ASSERT_EQ(sink.frames.size(), 1U);
  const auto& response = sink.frames.front();
  ASSERT_GE(response.size(), 23U);
  EXPECT_EQ(response[0], std::byte{'M'});
  EXPECT_EQ(u32(response, 6), 7U);
  EXPECT_EQ(response[18], std::byte{0});
  EXPECT_EQ(response[19], std::byte{'{'});
  EXPECT_EQ(response[20], std::byte{'}'});
  EXPECT_EQ(service.capabilities_calls, 1U);
}

TEST(ProtocolDispatcher, ReplaysIdenticalResponseWithoutRepeatingMutation) {
  FakeService service;
  RecordingSink sink;
  midi::usb::ProtocolDispatcher dispatcher(service, sink);
  const std::array payload{std::byte{12}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{1}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0xaa}, std::byte{0xbb}, std::byte{0xcc}, std::byte{0xdd}};
  const auto encoded = request(9, static_cast<std::uint16_t>(midi::usb::Command::BeginUpload), payload);
  dispatcher.receive(encoded);
  dispatcher.receive(encoded);

  ASSERT_EQ(sink.frames.size(), 2U);
  EXPECT_EQ(sink.frames[0], sink.frames[1]);
  EXPECT_EQ(service.begin_upload_calls, 1U);
}

TEST(ProtocolDispatcher, DoesNotRepeatMutationWhenReplayWriteIsTemporarilyUnavailable) {
  FakeService service;
  RecordingSink sink;
  midi::usb::ProtocolDispatcher dispatcher(service, sink);
  const auto encoded = request(10, static_cast<std::uint16_t>(midi::usb::Command::VerifyUpload));
  dispatcher.receive(encoded);
  sink.accept = false;
  dispatcher.receive(encoded);

  EXPECT_EQ(service.verify_upload_calls, 1U);
  EXPECT_EQ(sink.frames.size(), 2U);
}

TEST(ProtocolDispatcher, RejectsMalformedCommandPayloadBeforeService) {
  FakeService service;
  RecordingSink sink;
  midi::usb::ProtocolDispatcher dispatcher(service, sink);
  const std::array malformed{std::byte{0x01}};
  dispatcher.receive(request(11, static_cast<std::uint16_t>(midi::usb::Command::BeginUpload), malformed));

  ASSERT_EQ(sink.frames.size(), 1U);
  EXPECT_EQ(sink.frames.front()[18], std::byte{1});
  const auto responseText = std::string(reinterpret_cast<const char*>(sink.frames.front().data() + 19), sink.frames.front().size() - 23);
  EXPECT_NE(responseText.find("INVALID_FRAME"), std::string::npos);
  EXPECT_EQ(service.begin_upload_calls, 0U);
}

TEST(ProtocolDispatcher, ReturnsBinaryBankRecordForRequestedIndex) {
  FakeService service;
  RecordingSink sink;
  midi::usb::ProtocolDispatcher dispatcher(service, sink);
  const std::array bank{std::byte{3}};
  dispatcher.receive(request(12, static_cast<std::uint16_t>(midi::usb::Command::ReadConfig), bank));

  ASSERT_EQ(sink.frames.size(), 1U);
  const auto& response = sink.frames.front();
  ASSERT_EQ(response.size(), 18U + 1U + service.bank_record.size() + 4U);
  EXPECT_EQ(response[18], std::byte{0});
  EXPECT_TRUE(std::equal(service.bank_record.begin(), service.bank_record.end(), response.begin() + 19));
  EXPECT_EQ(service.read_config_calls, 1U);
}
