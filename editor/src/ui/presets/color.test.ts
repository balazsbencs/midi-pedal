import { describe, expect, it } from "vitest";
import { accentChoices, hexToRgb565, rgb565ToHex } from "./color";

describe("RGB565 accent previews", () => {
  it("renders palette swatches from the exact stored device values", () => {
    expect(accentChoices.map(choice => rgb565ToHex(choice.value))).toEqual([
      "#29e7d6",
      "#f75d5a",
      "#ce9aff",
      "#ffca42",
      "#4af7a5"
    ]);
  });

  it("quantizes arbitrary web colors to the same value the device stores", () => {
    expect(hexToRgb565("#2BE5D2")).toBe(0x2f3a);
    expect(rgb565ToHex(hexToRgb565("#2BE5D2"))).toBe("#29e7d6");
  });
});
