import { describe, expect, it } from "vitest";

import { makeInitialState } from "./editor_state";
import { editorReducer } from "./editor_reducer";

describe("editor reducer", () => {
  it("marks a local label edit dirty without changing device metadata", () => {
    const state = makeInitialState();
    const synced = editorReducer(state, {
      type: "device.loaded",
      document: state.draft,
      metadata: { imageCrc32: 0x12345678, sequence: 7, imageSize: 51252, activeSlot: "A" }
    });
    const next = editorReducer(synced, { type: "preset.labelChanged", position: 1, value: "CHORUS" });
    expect(next.dirty).toBe(true);
    expect(next.device.imageCrc32).toBe(synced.device.imageCrc32);
    expect(next.draft.config.banks[0]!.pages[0]!.presets[0]!.position1.label).toBe("CHORUS");
  });

  it("replaces only the local draft when a validated document is imported", () => {
    const state = makeInitialState();
    const imported = structuredClone(state.draft);
    imported.config.banks[2]!.name = "IMPORTED";
    const next = editorReducer(state, { type: "document.imported", document: imported });
    expect(next.draft).toEqual(imported);
    expect(next.device).toEqual(state.device);
    expect(next.dirty).toBe(true);
  });

  it("allocates stable message ids and preserves them when moving slots", () => {
    const state = makeInitialState();
    const withSlot = editorReducer(state, { type: "slot.added", messageType: "CC" });
    const slot = withSlot.draft.config.banks[0]!.pages[0]!.presets[0]!.slots[0]!;
    expect(slot.id).toBeGreaterThan(0);
    const withSecond = editorReducer(withSlot, { type: "slot.added", messageType: "PC" });
    const second = withSecond.draft.config.banks[0]!.pages[0]!.presets[0]!.slots[1]!;
    const moved = editorReducer(withSecond, { type: "slot.moved", from: 1, to: 0 });
    expect(moved.draft.config.banks[0]!.pages[0]!.presets[0]!.slots[0]!.id).toBe(second.id);
    expect(moved.draft.config.banks[0]!.pages[0]!.presets[0]!.slots[1]!.id).toBe(slot.id);
  });
});

