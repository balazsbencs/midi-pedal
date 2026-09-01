import { crc32 } from "./crc32.js";

export enum Command {
  GET_CAPABILITIES = 1,
  GET_CONFIG_INFO = 2,
  READ_CONFIG = 3,
  BEGIN_UPLOAD = 4,
  WRITE_CHUNK = 5,
  VERIFY_UPLOAD = 6,
  ACTIVATE_UPLOAD = 7,
  GET_EXPRESSION_SAMPLE = 8,
  SET_EXPRESSION_CALIBRATION = 9,
  FACTORY_EMPTY_RESET = 10,
}

export enum StatusCode {
  CRC_MISMATCH = "CRC_MISMATCH",
  UNSUPPORTED_VERSION = "UNSUPPORTED_VERSION",
  PAYLOAD_TOO_LARGE = "PAYLOAD_TOO_LARGE",
  UNKNOWN_COMMAND = "UNKNOWN_COMMAND",
  INVALID_FRAME = "INVALID_FRAME",
  INCOMPATIBLE_DEVICE = "INCOMPATIBLE_DEVICE",
  INVALID_CONFIGURATION = "INVALID_CONFIGURATION",
  INVALID_STATE = "INVALID_STATE",
  VERIFY_FAILED = "VERIFY_FAILED",
  BUSY = "BUSY",
}

export interface Frame {
  requestId: number;
  command: Command;
  flags: number;
  payload: Uint8Array;
}

export type DecodeEvent =
  | { type: "frame"; frame: Frame }
  | { type: "error"; code: StatusCode };

const MAGIC = new Uint8Array([0x4d, 0x50, 0x43, 0x46]);
const VERSION = 1;
const HEADER_SIZE = 18;
const CRC_SIZE = 4;
export const MAX_FRAME_PAYLOAD = 4096;

function u32(value: number, label: string): number {
  if (!Number.isInteger(value) || value < 0 || value > 0xffffffff) throw new RangeError(`${label} must be a uint32`);
  return value >>> 0;
}

function u16(value: number, label: string): number {
  if (!Number.isInteger(value) || value < 0 || value > 0xffff) throw new RangeError(`${label} must be a uint16`);
  return value;
}

function isCommand(value: number): value is Command {
  return Object.values(Command).filter(item => typeof item === "number").includes(value);
}

export function encodeFrame(frame: Frame): Uint8Array {
  u32(frame.requestId, "requestId");
  u16(frame.command, "command");
  u16(frame.flags, "flags");
  if (frame.payload.length > MAX_FRAME_PAYLOAD) throw new RangeError(`payload exceeds ${MAX_FRAME_PAYLOAD} bytes`);
  const output = new Uint8Array(HEADER_SIZE + frame.payload.length + CRC_SIZE);
  output.set(MAGIC, 0);
  const view = new DataView(output.buffer);
  view.setUint16(4, VERSION, true);
  view.setUint32(6, frame.requestId >>> 0, true);
  view.setUint16(10, frame.command, true);
  view.setUint16(12, frame.flags, true);
  view.setUint32(14, frame.payload.length, true);
  output.set(frame.payload, HEADER_SIZE);
  view.setUint32(HEADER_SIZE + frame.payload.length, crc32(output.slice(0, HEADER_SIZE + frame.payload.length)), true);
  return output;
}

function concat(left: Uint8Array, right: Uint8Array): Uint8Array {
  const output = new Uint8Array(left.length + right.length);
  output.set(left); output.set(right, left.length);
  return output;
}

function magicIndex(bytes: Uint8Array): number {
  outer: for (let start = 0; start <= bytes.length - MAGIC.length; start += 1) {
    for (let index = 0; index < MAGIC.length; index += 1) if (bytes[start + index] !== MAGIC[index]) continue outer;
    return start;
  }
  return -1;
}

export class FrameDecoder {
  private buffer: Uint8Array<ArrayBufferLike> = new Uint8Array();

  push(chunk: Uint8Array): DecodeEvent[] {
    this.buffer = concat(this.buffer, chunk);
    const events: DecodeEvent[] = [];
    while (this.buffer.length >= MAGIC.length) {
      const start = magicIndex(this.buffer);
      if (start < 0) {
        this.buffer = this.buffer.slice(Math.max(0, this.buffer.length - (MAGIC.length - 1)));
        break;
      }
      if (start > 0) this.buffer = this.buffer.slice(start);
      if (this.buffer.length < HEADER_SIZE) break;
      const view = new DataView(this.buffer.buffer, this.buffer.byteOffset, this.buffer.byteLength);
      const version = view.getUint16(4, true);
      const requestId = view.getUint32(6, true);
      const commandValue = view.getUint16(10, true);
      const flags = view.getUint16(12, true);
      const payloadLength = view.getUint32(14, true);
      if (payloadLength > MAX_FRAME_PAYLOAD) {
        events.push({ type: "error", code: StatusCode.PAYLOAD_TOO_LARGE });
        this.buffer = this.buffer.slice(MAGIC.length);
        continue;
      }
      const frameLength = HEADER_SIZE + payloadLength + CRC_SIZE;
      if (this.buffer.length < frameLength) break;
      const encoded = this.buffer.slice(0, frameLength);
      this.buffer = this.buffer.slice(frameLength);
      if (version !== VERSION) {
        events.push({ type: "error", code: StatusCode.UNSUPPORTED_VERSION });
        continue;
      }
      if (!isCommand(commandValue)) {
        events.push({ type: "error", code: StatusCode.UNKNOWN_COMMAND });
        continue;
      }
      const encodedView = new DataView(encoded.buffer, encoded.byteOffset, encoded.byteLength);
      const expected = encodedView.getUint32(HEADER_SIZE + payloadLength, true);
      const actual = crc32(encoded.slice(0, HEADER_SIZE + payloadLength));
      if (actual !== expected) {
        events.push({ type: "error", code: StatusCode.CRC_MISMATCH });
        continue;
      }
      events.push({ type: "frame", frame: { requestId, command: commandValue, flags, payload: encoded.slice(HEADER_SIZE, HEADER_SIZE + payloadLength) } });
    }
    return events;
  }
}

export interface BeginUploadPayload { imageSize: number; sequence: number; crc32: number }
export interface WriteChunkPayload { offset: number; data: Uint8Array }

export function encodeReadConfigPayload(bankIndex: number): Uint8Array {
  if (!Number.isInteger(bankIndex) || bankIndex < 0 || bankIndex > 127) throw new RangeError("bankIndex must be 0–127");
  return new Uint8Array([bankIndex]);
}

export function encodeBeginUploadPayload(payload: BeginUploadPayload): Uint8Array {
  const output = new Uint8Array(12); const view = new DataView(output.buffer);
  view.setUint32(0, u32(payload.imageSize, "imageSize"), true);
  view.setUint32(4, u32(payload.sequence, "sequence"), true);
  view.setUint32(8, u32(payload.crc32, "crc32"), true);
  return output;
}

export function decodeBeginUploadPayload(payload: Uint8Array): BeginUploadPayload {
  if (payload.length !== 12) throw new RangeError("BEGIN_UPLOAD payload must be 12 bytes");
  const view = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  return { imageSize: view.getUint32(0, true), sequence: view.getUint32(4, true), crc32: view.getUint32(8, true) };
}

export function encodeWriteChunkPayload(payload: WriteChunkPayload): Uint8Array {
  if (payload.data.length > 1024) throw new RangeError("WRITE_CHUNK data exceeds 1024 bytes");
  const output = new Uint8Array(4 + payload.data.length); const view = new DataView(output.buffer);
  view.setUint32(0, u32(payload.offset, "offset"), true); output.set(payload.data, 4); return output;
}

export function decodeWriteChunkPayload(payload: Uint8Array): WriteChunkPayload {
  if (payload.length < 4 || payload.length > 1028) throw new RangeError("WRITE_CHUNK payload length is invalid");
  const view = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  return { offset: view.getUint32(0, true), data: payload.slice(4) };
}
