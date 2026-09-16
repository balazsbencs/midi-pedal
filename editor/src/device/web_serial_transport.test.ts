import { describe, expect, it } from "vitest";

import { WebSerialTransport } from "./WebSerialTransport";

describe("web serial transport", () => {
  it("maps a rejected browser read to a disconnected transport error", async () => {
    const readable = new ReadableStream<Uint8Array>({
      start(controller) { controller.error(new Error("device removed")); }
    });
    const transport = new WebSerialTransport(serialWith(readable));
    await transport.open();

    await expect(transport.read()).rejects.toMatchObject({ code: "DISCONNECTED", message: "serial read failed" });
  });

  it("cancels a pending read before releasing and closing the port", async () => {
    let cancelled = false;
    let closed = false;
    const readable = new ReadableStream<Uint8Array>({
      cancel() { cancelled = true; }
    });
    const transport = new WebSerialTransport(serialWith(readable, () => { closed = true; }));
    await transport.open();
    const pendingRead = transport.read();

    await transport.close();

    await expect(pendingRead).resolves.toBeNull();
    expect(cancelled).toBe(true);
    expect(closed).toBe(true);
  });
});

function serialWith(readable: ReadableStream<Uint8Array>, onClose: () => void = () => undefined) {
  return {
    async requestPort() {
      return {
        readable,
        writable: new WritableStream<Uint8Array>(),
        async open() {},
        async close() { onClose(); }
      };
    }
  };
}
