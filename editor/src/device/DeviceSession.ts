import {
  Command, FrameDecoder, encodeBeginUploadPayload, encodeFrame, encodeImage,
  encodeWriteChunkPayload, inspectImage, type ConfigV1, type ConfigDocumentV1
} from "@midi-pedal/protocol";

import { validateConfig } from "@midi-pedal/protocol";
import type { DeviceTransport } from "./DeviceTransport";

const decoder = new TextDecoder();
const encoder = new TextEncoder();

export class DeviceSessionError extends Error {
  constructor(readonly code: string, message: string) { super(message); this.name = "DeviceSessionError"; }
}

export interface SessionOptions { timeoutMs?: number; }
export interface SyncProgress { stage: "BEGIN" | "WRITE" | "VERIFY" | "ACTIVATE" | "READBACK"; completed: number; total: number; }
export interface DeviceCapabilities { deviceModel: string; protocolVersion: number; configSchema: number; imageFormat: number; [key: string]: unknown }
export interface ConfigInfo { sequence: number; imageSize: number; imageCrc32: number; activeSlot: "A" | "B" }
export interface SyncResult { ok: true; activeCrc32: number; metadata: ConfigInfo }

export class DeviceSession {
  private nextRequestId = 1;
  private readonly timeoutMs: number;
  private readonly frameDecoder = new FrameDecoder();
  private capabilities?: DeviceCapabilities;
  private info?: ConfigInfo;
  private connected = false;

  constructor(private readonly transport: DeviceTransport, options: SessionOptions = {}) { this.timeoutMs = options.timeoutMs ?? 1000; }

  async connect(): Promise<{ capabilities: DeviceCapabilities; info: ConfigInfo }> {
    await this.transport.open();
    try {
      const capabilities = await this.getCapabilities();
      if (capabilities.deviceModel !== "MIDI_PEDAL_PICO2" || capabilities.protocolVersion !== 1 || capabilities.configSchema !== 1 || capabilities.imageFormat !== 1) {
        throw new DeviceSessionError("INCOMPATIBLE_DEVICE", "pedal firmware or configuration format is not compatible");
      }
      const info = await this.getConfigInfo();
      this.connected = true;
      return { capabilities, info };
    } catch (error) {
      await this.transport.close();
      throw error;
    }
  }

  async disconnect(): Promise<void> { this.connected = false; await this.transport.close(); }

  async getCapabilities(): Promise<DeviceCapabilities> {
    const value = await this.requestJson<DeviceCapabilities>(Command.GET_CAPABILITIES, new Uint8Array());
    this.capabilities = value; return value;
  }

  async getConfigInfo(): Promise<ConfigInfo> {
    const value = await this.requestJson<ConfigInfo>(Command.GET_CONFIG_INFO, new Uint8Array());
    this.info = value; return value;
  }

  async readConfiguration(): Promise<ConfigDocumentV1> {
    const payload = await this.request(Command.READ_CONFIG, new Uint8Array());
    const value = this.parseJsonResponse<unknown>(payload);
    const result = validateConfig(value);
    if (!result.ok) throw new DeviceSessionError("INVALID_CONFIGURATION", "pedal returned an invalid configuration");
    return result.value;
  }

  async syncConfiguration(config: ConfigV1, onProgress?: (event: SyncProgress) => void): Promise<SyncResult> {
    if (!this.connected) throw new DeviceSessionError("DISCONNECTED", "connect to a pedal before syncing");
    const sequence = ((this.info?.sequence ?? 0) + 1) >>> 0;
    const image = encodeImage(config, sequence);
    const imageInfo = inspectImage(image);
    onProgress?.({ stage: "BEGIN", completed: 0, total: 1 });
    await this.request(Command.BEGIN_UPLOAD, encodeBeginUploadPayload({ imageSize: image.length, sequence, crc32: imageInfo.crc32 }));
    const total = Math.ceil(image.length / 1024);
    for (let offset = 0, chunk = 0; offset < image.length; offset += 1024, chunk += 1) {
      const data = image.slice(offset, Math.min(offset + 1024, image.length));
      onProgress?.({ stage: "WRITE", completed: chunk + 1, total });
      await this.request(Command.WRITE_CHUNK, encodeWriteChunkPayload({ offset, data }));
    }
    onProgress?.({ stage: "VERIFY", completed: 1, total: 1 });
    await this.request(Command.VERIFY_UPLOAD, new Uint8Array());
    onProgress?.({ stage: "ACTIVATE", completed: 1, total: 1 });
    await this.request(Command.ACTIVATE_UPLOAD, new Uint8Array());
    onProgress?.({ stage: "READBACK", completed: 0, total: 1 });
    const metadata = await this.getConfigInfo();
    if (metadata.imageCrc32 !== imageInfo.crc32 || metadata.imageSize !== image.length) throw new DeviceSessionError("VERIFY_FAILED", "pedal did not report the activated image checksum");
    onProgress?.({ stage: "READBACK", completed: 1, total: 1 });
    this.info = metadata;
    return { ok: true, activeCrc32: metadata.imageCrc32, metadata };
  }

  async readExpressionSample(): Promise<number> { return (await this.requestJson<{ value: number }>(Command.GET_EXPRESSION_SAMPLE, new Uint8Array())).value; }
  async setCalibration(heel: number, toe: number): Promise<void> { await this.request(Command.SET_EXPRESSION_CALIBRATION, new Uint8Array([heel & 0xff, heel >>> 8, toe & 0xff, toe >>> 8])); }
  async factoryEmptyReset(): Promise<void> { await this.request(Command.FACTORY_EMPTY_RESET, new Uint8Array()); }

  private async requestJson<T>(command: Command, payload: Uint8Array): Promise<T> { return this.parseJsonResponse<T>(await this.request(command, payload)); }

  private parseJsonResponse<T>(payload: Uint8Array): T {
    try { return JSON.parse(decoder.decode(payload)) as T; }
    catch { throw new DeviceSessionError("INVALID_FRAME", "device response payload is not valid JSON"); }
  }

  private async request(command: Command, payload: Uint8Array): Promise<Uint8Array> {
    const requestId = this.nextRequestId++ >>> 0;
    for (let attempt = 0; attempt < 3; attempt += 1) {
      await this.transport.write(encodeFrame({ requestId, command, flags: 0, payload }));
      try {
        return await this.waitForResponse(requestId, command);
      } catch (error) {
        if (!(error instanceof DeviceSessionError) || (error.code !== "TIMEOUT" && error.code !== "DISCONNECTED")) throw error;
        if (attempt === 2) throw error;
      }
    }
    throw new DeviceSessionError("TIMEOUT", "device did not respond");
  }

  private async waitForResponse(requestId: number, command: Command): Promise<Uint8Array> {
    const deadline = Date.now() + this.timeoutMs;
    while (Date.now() < deadline) {
      const remaining = Math.max(1, deadline - Date.now());
      const bytes = await Promise.race([
        this.transport.read(),
        new Promise<null>(resolve => setTimeout(() => resolve(null), remaining))
      ]);
      if (bytes === null) throw new DeviceSessionError("TIMEOUT", "device response timed out");
      if (bytes === null) throw new DeviceSessionError("DISCONNECTED", "device disconnected");
      for (const event of this.frameDecoder.push(bytes)) {
        if (event.type === "error") continue;
        if (event.frame.requestId !== requestId || event.frame.command !== command) continue;
        if (event.frame.payload[0] !== 0) {
          let detail = "device rejected the command";
          try { detail = (JSON.parse(decoder.decode(event.frame.payload.slice(1))) as { code?: string }).code ?? detail; } catch { /* keep generic detail */ }
          throw new DeviceSessionError(detail, `device rejected ${Command[command]}`);
        }
        return event.frame.payload.slice(1);
      }
    }
    throw new DeviceSessionError("TIMEOUT", "device response timed out");
  }
}
