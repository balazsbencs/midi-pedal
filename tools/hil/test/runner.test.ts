import { describe, expect, it } from "vitest";

import { fakeRig, runSuite } from "../src/runner";

describe("HIL runner", () => {
  it("fails a run when observed MIDI bytes differ without discarding evidence", async () => {
    const result = await runSuite(fakeRig({ midi: [0xb0, 17, 126] }), {
      id: "midi.cc.value",
      run: async rig => {
        const observed = await rig.captureMidi();
        const expected = [0xb0, 17, 127];
        return { expected, observed, status: JSON.stringify(expected) === JSON.stringify(observed) ? "PASS" : "FAIL", evidence: ["simulated-midi"], notes: [] };
      }
    });
    expect(result.status).toBe("FAIL");
    expect(result.cases[0]).toMatchObject({ expected: [0xb0, 17, 127], observed: [0xb0, 17, 126] });
  });

  it("requires a reason for skipped cases", async () => {
    await expect(runSuite(fakeRig(), { id: "skipped", run: async () => ({ status: "SKIP", evidence: [], notes: [] }) })).rejects.toThrow(/skip/i);
  });
});

