import { describe, expect, it } from "vitest";
import { Command, FrameDecoder, StatusCode, encodeFrame } from "../src/frame.js";

describe("USB configuration frames", () => {
  it("reassembles arbitrary serial chunks", () => {
    const bytes = encodeFrame({ requestId: 42, command: Command.GET_CAPABILITIES, flags: 0, payload: new Uint8Array() });
    const decoder = new FrameDecoder();
    expect(decoder.push(bytes.slice(0, 5))).toEqual([]);
    expect(decoder.push(bytes.slice(5))).toEqual([{ type: "frame", frame: {
      requestId: 42, command: Command.GET_CAPABILITIES, flags: 0, payload: new Uint8Array()
    } }]);
  });

  it("rejects a corrupt payload without losing the following frame", () => {
    const corrupt = encodeFrame({ requestId: 1, command: Command.GET_CONFIG_INFO, flags: 0, payload: new Uint8Array([1]) });
    corrupt[corrupt.length - 1]! ^= 0xff;
    const valid = encodeFrame({ requestId: 2, command: Command.GET_CAPABILITIES, flags: 0, payload: new Uint8Array() });
    const events = new FrameDecoder().push(new Uint8Array([...corrupt, ...valid]));
    expect(events[0]).toEqual({ type: "error", code: StatusCode.CRC_MISMATCH });
    expect(events[1]).toMatchObject({ type: "frame", frame: { requestId: 2 } });
  });

  it("resynchronizes after serial noise and reports unsupported versions", () => {
    const valid = encodeFrame({ requestId: 9, command: Command.READ_CONFIG, flags: 0, payload: new Uint8Array([7]) });
    const unsupported = valid.slice();
    new DataView(unsupported.buffer).setUint16(4, 99, true);
    expect(new FrameDecoder().push(new Uint8Array([0x01, 0x02, ...unsupported, ...valid]))).toEqual([
      { type: "error", code: StatusCode.UNSUPPORTED_VERSION },
      { type: "frame", frame: { requestId: 9, command: Command.READ_CONFIG, flags: 0, payload: new Uint8Array([7]) } },
    ]);
  });

  it("rejects a payload larger than the protocol cap", () => {
    const oversized = new Uint8Array(18);
    oversized.set([0x4d, 0x50, 0x43, 0x46]);
    new DataView(oversized.buffer).setUint16(4, 1, true);
    new DataView(oversized.buffer).setUint32(14, 4097, true);
    expect(new FrameDecoder().push(oversized)).toEqual([{ type: "error", code: StatusCode.PAYLOAD_TOO_LARGE }]);
  });
});
