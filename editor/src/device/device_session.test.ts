import { describe, expect, it } from "vitest";

import { makeFactoryDocument } from "../domain/editor_state";
import { Command } from "@midi-pedal/protocol";
import { DeviceSession } from "./DeviceSession";
import { SimulatedTransport } from "./SimulatedTransport";

describe("device session", () => {
  it("retries the same request id three times then reports timeout", async () => {
    const transport = new SimulatedTransport().dropResponses(3);
    const session = new DeviceSession(transport, { timeoutMs: 5 });
    await transport.open();
    await expect(session.getCapabilities()).rejects.toMatchObject({ code: "TIMEOUT" });
    expect(transport.requestIds()).toEqual([1, 1, 1]);
  });

  it("reports the complete atomic sync lifecycle", async () => {
    const transport = new SimulatedTransport();
    const session = new DeviceSession(transport, { timeoutMs: 50 });
    await session.connect();
    const stages: string[] = [];
    const result = await session.syncConfiguration(makeFactoryDocument().config, event => stages.push(event.stage));
    expect(stages.filter((stage, index) => index === 0 || stage !== stages[index - 1])).toEqual(["BEGIN", "WRITE", "VERIFY", "ACTIVATE", "READBACK"]);
    expect(result.activeCrc32).toEqual(expect.any(Number));
    expect(transport.requestIds().map((id, index, ids) => index === 0 || id !== ids[index - 1]).filter(Boolean).length).toBeGreaterThan(1);
  });

  it("rejects an incompatible capability response before reading config", async () => {
    const transport = new SimulatedTransport().withCapabilities({ deviceModel: "OTHER_DEVICE" });
    const session = new DeviceSession(transport, { timeoutMs: 20 });
    await expect(session.connect()).rejects.toMatchObject({ code: "INCOMPATIBLE_DEVICE" });
    expect(transport.commands()).toEqual([Command.GET_CAPABILITIES]);
  });
});
