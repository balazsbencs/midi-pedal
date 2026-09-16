import { describe, expect, it } from "vitest";

import { makeFactoryDocument } from "../domain/editor_state";
import { Command, encodeFrame } from "@midi-pedal/protocol";
import { DeviceSession } from "./DeviceSession";
import { TransportError, type DeviceTransport } from "./DeviceTransport";
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

  it("reads each bank as a bounded binary record", async () => {
    const transport = new SimulatedTransport();
    const session = new DeviceSession(transport, { timeoutMs: 50 });
    await session.connect();

    const document = await session.readConfiguration();

    expect(document.config.banks).toHaveLength(128);
    expect(transport.commands().filter(command => command === Command.READ_CONFIG)).toHaveLength(128);
  });

  it("rejects an incompatible capability response before reading config", async () => {
    const transport = new SimulatedTransport().withCapabilities({ deviceModel: "OTHER_DEVICE" });
    const session = new DeviceSession(transport, { timeoutMs: 20 });
    await expect(session.connect()).rejects.toMatchObject({ code: "INCOMPATIBLE_DEVICE" });
    expect(transport.commands()).toEqual([Command.GET_CAPABILITIES]);
  });

  it("reuses the pending serial read when a response arrives after the first timeout", async () => {
    let resolveRead: ((bytes: Uint8Array | null) => void) | undefined;
    let writes = 0;
    const responseTransport: DeviceTransport = {
      async open() {},
      async close() { resolveRead?.(null); },
      async write(bytes) {
        writes += 1;
        if (writes === 2) resolveRead?.(responseFor(bytes));
      },
      read() { return new Promise(resolve => { resolveRead = resolve; }); }
    };
    const session = new DeviceSession(responseTransport, { timeoutMs: 5 });

    await expect(session.getCapabilities()).resolves.toMatchObject({ deviceModel: "MIDI_PEDAL_PICO2" });
    expect(writes).toBe(2);
  });

  it("reports a serial read failure as a disconnect without retrying a dead port", async () => {
    let writes = 0;
    const disconnectedTransport: DeviceTransport = {
      async open() {}, async close() {},
      async write() { writes += 1; },
      async read() { throw new TransportError("DISCONNECTED", "serial read failed"); }
    };
    const session = new DeviceSession(disconnectedTransport, { timeoutMs: 5 });

    await expect(session.getCapabilities()).rejects.toMatchObject({ code: "DISCONNECTED", message: "serial read failed" });
    expect(writes).toBe(1);
  });
});

function responseFor(request: Uint8Array): Uint8Array {
  const view = new DataView(request.buffer, request.byteOffset, request.byteLength);
  const requestId = view.getUint32(6, true);
  const command = view.getUint16(10, true) as Command;
  const payload = new TextEncoder().encode(JSON.stringify({
    deviceModel: "MIDI_PEDAL_PICO2", protocolVersion: 1, configSchema: 1, imageFormat: 1
  }));
  return encodeFrame({ requestId, command, flags: 0, payload: new Uint8Array([0, ...payload]) });
}
