import { describe, expect, it } from "vitest";

import { makeFactoryDocument } from "./editor_state";
import { exportDraft, previewImport } from "./files";

describe("draft files", () => {
  it("exports opaque top-level metadata with the draft", async () => {
    const document = { ...makeFactoryDocument(), passthroughTopLevel: { communityRating: { stars: 5 } } };
    const blob = exportDraft(document);
    const text = await new Promise<string>((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = () => resolve(String(reader.result));
      reader.onerror = () => reject(reader.error);
      reader.readAsText(blob);
    });
    expect(JSON.parse(text).communityRating).toEqual({ stars: 5 });
  });

  it("previews valid imports and rejects invalid input without a document", () => {
    const preview = previewImport(JSON.stringify({ schemaVersion: 1, deviceModel: "MIDI_PEDAL_PICO2", banks: [] }));
    expect(preview).toMatchObject({ ok: true, counts: { banks: 128, pages: 512, presets: 2048 } });
    expect(previewImport('{"schemaVersion":1,"banks":"invalid"}')).toMatchObject({ ok: false, document: undefined });
  });
});
