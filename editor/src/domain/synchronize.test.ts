import { describe, expect, it } from "vitest";

import { Command } from "@midi-pedal/protocol";
import { makeFactoryDocument } from "./editor_state";
import { SimulatedTransport } from "../device/SimulatedTransport";
import { DeviceSession } from "../device/DeviceSession";
import { synchronizeDraft } from "./synchronize";

describe("atomic draft synchronization", () => {
  it("reports the previous pedal configuration intact after verification failure", async () => {
    const transport = new SimulatedTransport().fail(Command.VERIFY_UPLOAD, "VERIFY_FAILED");
    const session = new DeviceSession(transport, { timeoutMs: 50 });
    await session.connect();
    const result = await synchronizeDraft(session, makeFactoryDocument().config);
    expect(result).toMatchObject({ ok: false, stage: "VERIFY", previousConfigurationIntact: true });
  });

  it("reports every lifecycle stage and verifies active checksum", async () => {
    const session = new DeviceSession(new SimulatedTransport(), { timeoutMs: 50 });
    await session.connect();
    const stages: string[] = [];
    const result = await synchronizeDraft(session, makeFactoryDocument().config, event => stages.push(event.stage));
    expect(stages.filter((stage, index) => index === 0 || stage !== stages[index - 1])).toEqual(["BEGIN", "WRITE", "VERIFY", "ACTIVATE", "READBACK"]);
    expect(result).toMatchObject({ ok: true, activeCrc32: expect.any(Number) });
  });
});

