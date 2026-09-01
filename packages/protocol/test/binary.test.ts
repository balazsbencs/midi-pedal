import { describe, expect, it } from "vitest";
import fullBoundary from "../../../protocol/fixtures/json/full-boundary-valid.json" with { type: "json" };
import minimal from "../../../protocol/fixtures/json/minimal-valid.json" with { type: "json" };
import { decodeBankRecord, decodeImage, encodeBankRecord, encodeImage, inspectImage } from "../src/binary.js";
import { validateConfig } from "../src/validate.js";

const minimalResult = validateConfig(minimal);
const fullResult = validateConfig(fullBoundary);
if (!minimalResult.ok || !fullResult.ok) throw new Error("test fixtures must validate");
const minimalConfig = minimalResult.value.config;
const fullConfig = fullResult.value.config;

describe("binary configuration image v1", () => {
  it("round-trips an individual bank record for device readback", () => {
    const bank = structuredClone(minimalConfig.banks[0]!);
    bank.name = "READBACK";
    bank.pages[0]!.presets[0]!.slots.push({
      id: 0x777,
      trigger: "PRESS",
      position: "BOTH",
      message: { type: "CC", channel: 1, controller: 12, value: 99, destination: "USB" },
    });

    const encoded = encodeBankRecord(bank);
    expect(decodeBankRecord(encoded)).toEqual(bank);
  });

  it("writes the fixed 32-byte v1 header", () => {
    const image = encodeImage(minimalConfig, 7);
    expect(new TextDecoder().decode(image.slice(0, 4))).toBe("MPDL");
    expect(new DataView(image.buffer, image.byteOffset, image.byteLength).getUint16(4, true)).toBe(1);
    expect(new DataView(image.buffer, image.byteOffset, image.byteLength).getUint16(6, true)).toBe(32);
    expect(new DataView(image.buffer, image.byteOffset, image.byteLength).getUint32(12, true)).toBe(7);
    expect(inspectImage(image)).toMatchObject({ formatVersion: 1, sequence: 7, bankCount: 128, crc32: expect.any(Number) });
  });

  it("encodes identical input byte-for-byte", () => {
    expect(encodeImage(minimalConfig, 1)).toEqual(encodeImage(minimalConfig, 1));
  });

  it("round-trips all boundary message and presentation values", () => {
    const decoded = decodeImage(encodeImage(fullConfig, 42));
    expect(decoded).toEqual(fullConfig);
  });

  it("rejects a changed payload with a CRC error", () => {
    const image = encodeImage(minimalConfig, 1);
    image[image.length - 1]! ^= 0x01;
    expect(() => inspectImage(image)).toThrow(/CRC/i);
  });
});
