import { describe, expect, it } from "vitest";
import { serializeConfigDocument, validateConfig } from "../src/validate.js";
import fullBoundary from "../../../protocol/fixtures/json/full-boundary-valid.json" with { type: "json" };

const minimal = { schemaVersion: 1, deviceModel: "MIDI_PEDAL_PICO2", banks: [] } as const;

const validBank = {
  id: 1,
  name: "Bank 1",
  pages: Array.from({ length: 4 }, (_, pageIndex) => ({
    id: pageIndex + 2,
    presets: Array.from({ length: 4 }, (_, presetIndex) => ({
      id: pageIndex * 4 + presetIndex + 6,
      position1: { label: "ONE", accentRgb565: 0x7bef },
      position2: { label: "TWO", accentRgb565: 0x7bef },
      toggleOn: null,
      slots: [],
    })),
  })),
  expression: { enabled: false, label: "EXPR", channel: 1, controller: 1, destination: "BOTH", minimum: 0, maximum: 127, inverted: false },
};

describe("config v1", () => {
  it("accepts the safe empty 128-bank document", () => {
    const result = validateConfig(minimal);
    expect(result.ok).toBe(true);
    if (result.ok) expect(result.value.config.banks).toHaveLength(128);
  });

  it("accepts the full boundary fixture and normalizes its remaining banks", () => {
    const result = validateConfig(fullBoundary);
    expect(result.ok).toBe(true);
    if (result.ok) {
      expect(result.value.config.banks).toHaveLength(128);
      expect(result.value.config.banks[0]?.pages[0]?.presets[0]?.slots).toHaveLength(8);
    }
  });

  it("rejects CC values above 127 with a field path", () => {
    const result = validateConfig({
      schemaVersion: 1,
      deviceModel: "MIDI_PEDAL_PICO2",
      banks: [{ ...validBank, pages: validBank.pages.map((page, pageIndex) => pageIndex === 0 ? {
        ...page,
        presets: page.presets.map((preset, presetIndex) => presetIndex === 0 ? {
          ...preset,
          slots: [{ id: 100, trigger: "PRESS", position: "BOTH", message: { type: "CC", channel: 1, controller: 1, value: 128, destination: "BOTH" } }],
        } : preset),
      } : page) }],
    });
    expect(result).toMatchObject({ ok: false });
    if (!result.ok) expect(result.errors.some((validationError) => validationError.path.endsWith("/value"))).toBe(true);
  });

  it("round-trips opaque top-level metadata without compiling it into behavior", () => {
    const result = validateConfig({ ...minimal, communityRating: { stars: 5 } });
    expect(result.ok).toBe(true);
    if (result.ok) expect(serializeConfigDocument(result.value).communityRating).toEqual({ stars: 5 });
  });

  it("rejects duplicate stable ids across authored banks", () => {
    const result = validateConfig({ ...minimal, banks: [validBank, validBank] });
    expect(result).toMatchObject({ ok: false });
    if (!result.ok) expect(result.errors.some(error => error.code === "duplicate-id")).toBe(true);
  });

  it("rejects non-printable names and an expression range that runs backwards", () => {
    const invalidName = validateConfig({ ...minimal, banks: [{ ...validBank, name: "Bad\nName" }] });
    expect(invalidName).toMatchObject({ ok: false });
    if (!invalidName.ok) expect(invalidName.errors.some(error => error.path.endsWith("/name"))).toBe(true);
    const invalidRange = validateConfig({ ...minimal, banks: [{ ...validBank, expression: { ...validBank.expression, minimum: 100, maximum: 10 } }] });
    expect(invalidRange).toMatchObject({ ok: false });
    if (!invalidRange.ok) expect(invalidRange.errors.some(error => error.code === "range")).toBe(true);
  });

  it("requires targets only for absolute navigation and limits page targets to 1–4", () => {
    const missingTarget = validateConfig({ ...minimal, banks: [{ ...validBank, pages: validBank.pages.map((page, pageIndex) => pageIndex === 0 ? { ...page, presets: page.presets.map((preset, presetIndex) => presetIndex === 0 ? { ...preset, slots: [{ id: 100, trigger: "PRESS", position: "BOTH", message: { type: "NAV", operation: "BANK_SET" } }] } : preset) } : page) }] });
    expect(missingTarget).toMatchObject({ ok: false });
    const invalidPage = validateConfig({ ...minimal, banks: [{ ...validBank, pages: validBank.pages.map((page, pageIndex) => pageIndex === 0 ? { ...page, presets: page.presets.map((preset, presetIndex) => presetIndex === 0 ? { ...preset, slots: [{ id: 100, trigger: "PRESS", position: "BOTH", message: { type: "NAV", operation: "PAGE_SET", target: 5 } }] } : preset) } : page) }] });
    expect(invalidPage).toMatchObject({ ok: false });
  });

  it("rejects unsafe top-level metadata keys", () => {
    const result = validateConfig(JSON.parse('{"schemaVersion":1,"deviceModel":"MIDI_PEDAL_PICO2","banks":[],"__proto__":{"polluted":true}}'));
    expect(result).toMatchObject({ ok: false });
    if (!result.ok) expect(result.errors[0]?.code).toBe("unsafe-key");
  });
});
