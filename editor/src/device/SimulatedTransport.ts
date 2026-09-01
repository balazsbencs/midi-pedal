import { Command, decodeImage, encodeFrame, inspectImage, validateConfig, type Frame } from "@midi-pedal/protocol";

import { makeFactoryDocument } from "../domain/editor_state";
import { encodeImage, serializeConfigDocument, type ConfigDocumentV1 } from "@midi-pedal/protocol";
import type { DeviceTransport } from "./DeviceTransport";

const encoder = new TextEncoder();
const decoder = new TextDecoder();

function jsonBytes(value: unknown): Uint8Array { return encoder.encode(JSON.stringify(value)); }
function parseJson<T>(bytes: Uint8Array): T { return JSON.parse(decoder.decode(bytes)) as T; }
function ok(payload: unknown = {}): Uint8Array { return new Uint8Array([0, ...jsonBytes(payload)]); }
function failure(code: string): Uint8Array { return new Uint8Array([1, ...jsonBytes({ code })]); }

export class SimulatedTransport implements DeviceTransport {
  private opened = false;
  private dropped = 0;
  private queue: Uint8Array[] = [];
  private waiters: Array<(bytes: Uint8Array | null) => void> = [];
  private capabilities: Record<string, unknown> = {
    deviceModel: "MIDI_PEDAL_PICO2", protocolVersion: 1, configSchema: 1, imageFormat: 1,
    queueCapacity: 64, destinations: ["TRS", "USB", "BOTH"], expression: true, relays: 2, display: "ST7796S"
  };
  private config: ConfigDocumentV1 = makeFactoryDocument();
  private activeImage = encodeImage(this.config.config, 0);
  private staged = new Uint8Array();
  private failingCommand?: { command: Command; code: string };
  private seenCommands: Command[] = [];
  private seenIds: number[] = [];

  async open(): Promise<void> { this.opened = true; }

  async close(): Promise<void> {
    this.opened = false;
    for (const resolve of this.waiters.splice(0)) resolve(null);
  }

  async write(bytes: Uint8Array): Promise<void> {
    if (!this.opened) throw new Error("transport is closed");
    const frame = parseFrame(bytes);
    this.seenCommands.push(frame.command);
    this.seenIds.push(frame.requestId);
    if (this.dropped > 0) { this.dropped -= 1; return; }
    const response = this.respond(frame);
    if (response) this.deliver(encodeFrame(response));
  }

  read(): Promise<Uint8Array | null> {
    if (this.queue.length) return Promise.resolve(this.queue.shift()!);
    if (!this.opened) return Promise.resolve(null);
    // The simulated device has no blocking serial driver. Returning an empty
    // chunk lets DeviceSession exercise its timeout path without leaving a
    // stale reader promise behind when a retry starts.
    return new Promise(resolve => setTimeout(() => resolve(new Uint8Array()), 1));
  }

  dropResponses(count: number): this { this.dropped = count; return this; }
  withCapabilities(values: Record<string, unknown>): this { this.capabilities = { ...this.capabilities, ...values }; return this; }
  fail(command: Command, code: string): this { this.failingCommand = { command, code }; return this; }
  requestIds(): number[] { return [...this.seenIds]; }
  commands(): Command[] { return [...this.seenCommands]; }

  private deliver(bytes: Uint8Array): void {
    const waiter = this.waiters.shift();
    if (waiter) waiter(bytes); else this.queue.push(bytes);
  }

  private respond(frame: Frame): Frame | undefined {
    if (this.failingCommand?.command === frame.command) return { requestId: frame.requestId, command: frame.command, flags: 0, payload: failure(this.failingCommand.code) };
    if (frame.command === Command.GET_CAPABILITIES) return { requestId: frame.requestId, command: frame.command, flags: 0, payload: ok(this.capabilities) };
    if (frame.command === Command.GET_CONFIG_INFO) {
      const metadata = inspectImage(this.activeImage);
      return { requestId: frame.requestId, command: frame.command, flags: 0, payload: ok({ sequence: metadata.sequence, imageSize: metadata.imageSize, imageCrc32: metadata.crc32, activeSlot: "A" }) };
    }
    if (frame.command === Command.READ_CONFIG) return { requestId: frame.requestId, command: frame.command, flags: 0, payload: ok(serializeConfigDocument(this.config)) };
    if (frame.command === Command.BEGIN_UPLOAD) {
      const size = new DataView(frame.payload.buffer, frame.payload.byteOffset, frame.payload.byteLength).getUint32(0, true);
      this.staged = new Uint8Array(size);
      return { requestId: frame.requestId, command: frame.command, flags: 0, payload: ok() };
    }
    if (frame.command === Command.WRITE_CHUNK) {
      const view = new DataView(frame.payload.buffer, frame.payload.byteOffset, frame.payload.byteLength);
      const offset = view.getUint32(0, true);
      this.staged.set(frame.payload.slice(4), offset);
      return { requestId: frame.requestId, command: frame.command, flags: 0, payload: ok() };
    }
    if (frame.command === Command.VERIFY_UPLOAD) {
      try { inspectImage(this.staged); return { requestId: frame.requestId, command: frame.command, flags: 0, payload: ok() }; }
      catch { return { requestId: frame.requestId, command: frame.command, flags: 0, payload: failure("VERIFY_FAILED") }; }
    }
    if (frame.command === Command.ACTIVATE_UPLOAD) {
      this.activeImage = this.staged.slice();
      try {
        const decoded = decodeImage(this.activeImage);
        const validated = validateConfig(serializeConfigDocument({ config: decoded, passthroughTopLevel: {} }));
        if (validated.ok) this.config = validated.value;
      } catch {
        return { requestId: frame.requestId, command: frame.command, flags: 0, payload: failure("VERIFY_FAILED") };
      }
      return { requestId: frame.requestId, command: frame.command, flags: 0, payload: ok() };
    }
    if (frame.command === Command.GET_EXPRESSION_SAMPLE) return { requestId: frame.requestId, command: frame.command, flags: 0, payload: ok({ value: 0 }) };
    if (frame.command === Command.FACTORY_EMPTY_RESET) {
      this.config = makeFactoryDocument(); this.activeImage = encodeImage(this.config.config, 0);
      return { requestId: frame.requestId, command: frame.command, flags: 0, payload: ok() };
    }
    return { requestId: frame.requestId, command: frame.command, flags: 0, payload: ok() };
  }
}

function parseFrame(bytes: Uint8Array): Frame {
  const magic = decoder.decode(bytes.slice(0, 4));
  if (magic !== "MPCF") throw new Error("simulator received non-frame bytes");
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  return { requestId: view.getUint32(6, true), command: view.getUint16(10, true) as Command, flags: view.getUint16(12, true), payload: bytes.slice(18, bytes.length - 4) };
}
