import { crc32 } from "./crc32.js";
import type { Bank, ConfigV1, Destination, ExpressionAssignment, Message, MessageSlot, Page, PositionFilter, Preset, Trigger } from "./model.js";

const HEADER_SIZE = 32;
const INDEX_SIZE = 128 * 4;
const PAYLOAD_OFFSET = HEADER_SIZE + INDEX_SIZE;
const MAX_IMAGE_SIZE = 768 * 1024;
const EMPTY_OFFSET = 0xffffffff;

export interface ImageMetadata {
  formatVersion: number;
  imageSize: number;
  sequence: number;
  bankCount: number;
  crc32: number;
}

export class ImageError extends Error {
  constructor(readonly code: string, message: string) {
    super(message);
    this.name = "ImageError";
  }
}

class Writer {
  private readonly output: number[] = [];

  get length(): number { return this.output.length; }
  u8(value: number): void { this.output.push(value & 0xff); }
  u16(value: number): void { this.u8(value); this.u8(value >>> 8); }
  u32(value: number): void { this.u8(value); this.u8(value >>> 8); this.u8(value >>> 16); this.u8(value >>> 24); }
  bytes(value: Uint8Array): void { for (const byte of value) this.u8(byte); }
  string(value: string): void {
    const encoded = new TextEncoder().encode(value);
    if (encoded.length > 255) throw new ImageError("STRING_TOO_LONG", "encoded string exceeds one-byte length");
    this.u8(encoded.length);
    this.bytes(encoded);
  }
  finish(): Uint8Array { return Uint8Array.from(this.output); }
}

class Reader {
  private offset = 0;

  constructor(readonly bytes: Uint8Array, readonly end = bytes.length) {}

  get position(): number { return this.offset; }
  get remaining(): number { return this.end - this.offset; }
  seek(position: number): void {
    if (position < 0 || position > this.end) throw new ImageError("TRUNCATED", "reader seek is outside its boundary");
    this.offset = position;
  }
  private require(length: number): void {
    if (length < 0 || this.offset + length > this.end) throw new ImageError("TRUNCATED", `read at ${this.offset} exceeds record/image boundary`);
  }
  u8(): number { this.require(1); return this.bytes[this.offset++]!; }
  u16(): number { const low = this.u8(); return low | (this.u8() << 8); }
  u32(): number { return (this.u8() | (this.u8() << 8) | (this.u8() << 16) | (this.u8() << 24)) >>> 0; }
  string(): string {
    const length = this.u8();
    this.require(length);
    let output = "";
    for (let index = 0; index < length; index += 1) {
      const byte = this.u8();
      if (byte < 0x20 || byte > 0x7e) throw new ImageError("STRING", "non-printable ASCII string byte");
      output += String.fromCharCode(byte);
    }
    return output;
  }
}

const destinationCode: Record<Destination, number> = { TRS: 0, USB: 1, BOTH: 2 };
const triggerCode: Record<Trigger, number> = { PRESS: 0, RELEASE: 1, LONG_PRESS: 2, DOUBLE_TAP: 3 };
const positionCode: Record<PositionFilter, number> = { POSITION_1: 0, POSITION_2: 1, BOTH: 2 };
const navCode: Record<Extract<Message, { type: "NAV" }> ["operation"], number> = { BANK_UP: 0, BANK_DOWN: 1, BANK_SET: 2, PAGE_UP: 3, PAGE_DOWN: 4, PAGE_SET: 5 };
const relayCode: Record<Extract<Message, { type: "RELAY" }> ["operation"], number> = { OPEN: 0, CLOSE: 1, TOGGLE: 2 };

function reverseCode<T extends string>(entries: Record<T, number>, value: number, label: string): T {
  const result = (Object.keys(entries) as T[]).find(key => entries[key] === value);
  if (!result) throw new ImageError("ENUM", `${label} value ${value} is invalid`);
  return result;
}

function writeMessage(writer: Writer, message: Message): void {
  if (message.type === "PC") {
    writer.u8(0); writer.u8(message.channel); writer.u8(message.program); writer.u8(destinationCode[message.destination]);
  } else if (message.type === "CC") {
    writer.u8(1); writer.u8(message.channel); writer.u8(message.controller); writer.u8(message.value); writer.u8(destinationCode[message.destination]);
  } else if (message.type === "RELAY") {
    writer.u8(2); writer.u8(message.contact); writer.u8(relayCode[message.operation]);
  } else {
    writer.u8(3); writer.u8(navCode[message.operation]); writer.u8(message.target ?? 0);
  }
}

function readMessage(reader: Reader): Message {
  const type = reader.u8();
  if (type === 0) return { type: "PC", channel: reader.u8(), program: reader.u8(), destination: reverseCode(destinationCode, reader.u8(), "destination") };
  if (type === 1) return { type: "CC", channel: reader.u8(), controller: reader.u8(), value: reader.u8(), destination: reverseCode(destinationCode, reader.u8(), "destination") };
  if (type === 2) return { type: "RELAY", contact: reader.u8() as 1 | 2, operation: reverseCode(relayCode, reader.u8(), "relay operation") };
  if (type === 3) {
    const operation = reverseCode(navCode, reader.u8(), "navigation operation");
    const target = reader.u8();
    return target === 0 ? { type: "NAV", operation } : { type: "NAV", operation, target };
  }
  throw new ImageError("ENUM", `message type ${type} is invalid`);
}

function writeSlot(writer: Writer, slot: MessageSlot): void {
  writer.u32(slot.id);
  writer.u8(triggerCode[slot.trigger]);
  writer.u8(positionCode[slot.position]);
  writeMessage(writer, slot.message);
}

function readSlot(reader: Reader): MessageSlot {
  const id = reader.u32();
  const trigger = reverseCode(triggerCode, reader.u8(), "trigger");
  const position = reverseCode(positionCode, reader.u8(), "position");
  return { id, trigger, position, message: readMessage(reader) };
}

function writePreset(writer: Writer, preset: Preset): void {
  writer.u32(preset.id);
  writer.string(preset.position1.label); writer.u16(preset.position1.accentRgb565);
  writer.string(preset.position2.label); writer.u16(preset.position2.accentRgb565);
  writer.u8(preset.toggleOn === null ? 255 : triggerCode[preset.toggleOn]);
  writer.u8(preset.slots.length);
  preset.slots.forEach(slot => writeSlot(writer, slot));
}

function readPreset(reader: Reader): Preset {
  const id = reader.u32();
  const position1 = { label: reader.string(), accentRgb565: reader.u16() };
  const position2 = { label: reader.string(), accentRgb565: reader.u16() };
  const toggle = reader.u8();
  const toggleOn = toggle === 255 ? null : reverseCode(triggerCode, toggle, "toggle trigger");
  const slotCount = reader.u8();
  if (slotCount > 8) throw new ImageError("COUNT", "preset contains more than eight slots");
  const slots = Array.from({ length: slotCount }, () => readSlot(reader));
  return { id, position1, position2, toggleOn, slots };
}

function writeExpression(writer: Writer, expression: ExpressionAssignment): void {
  writer.u8(expression.enabled ? 1 : 0);
  writer.string(expression.label);
  writer.u8(expression.channel); writer.u8(expression.controller); writer.u8(destinationCode[expression.destination]);
  writer.u8(expression.minimum); writer.u8(expression.maximum); writer.u8(expression.inverted ? 1 : 0);
}

function readExpression(reader: Reader): ExpressionAssignment {
  const enabled = reader.u8() !== 0;
  const label = reader.string();
  const channel = reader.u8(); const controller = reader.u8();
  const destination = reverseCode(destinationCode, reader.u8(), "destination");
  const minimum = reader.u8(); const maximum = reader.u8(); const inverted = reader.u8() !== 0;
  return { enabled, label, channel, controller, destination, minimum, maximum, inverted };
}

function writeBank(bank: Bank): Uint8Array {
  const body = new Writer();
  body.u32(0);
  body.u32(bank.id);
  body.string(bank.name);
  writeExpression(body, bank.expression);
  bank.pages.forEach(page => {
    body.u32(page.id);
    page.presets.forEach(preset => writePreset(body, preset));
  });
  const result = body.finish();
  new DataView(result.buffer).setUint32(0, result.length, true);
  return result;
}

function readBank(reader: Reader): Bank {
  const recordStart = reader.position;
  const recordLength = reader.u32();
  if (recordLength < 4 || recordLength - 4 > reader.remaining) throw new ImageError("RECORD_LENGTH", "bank record length is outside payload");
  const recordEnd = recordStart + recordLength;
  const bounded = new Reader(reader.bytes, recordEnd);
  bounded.seek(reader.position);
  const id = bounded.u32();
  const name = bounded.string();
  const expression = readExpression(bounded);
  const pages = Array.from({ length: 4 }, () => {
    const pageId = bounded.u32();
    const presets = Array.from({ length: 4 }, () => readPreset(bounded)) as [Preset, Preset, Preset, Preset];
    return { id: pageId, presets } satisfies Page;
  }) as [Page, Page, Page, Page];
  if (bounded.position !== recordEnd) throw new ImageError("RECORD_LENGTH", "bank record has trailing or missing bytes");
  reader.seek(recordEnd);
  return { id, name, pages, expression };
}

/** Encode one length-prefixed bank record for the device READ_CONFIG response. */
export function encodeBankRecord(bank: Bank): Uint8Array {
  return writeBank(bank);
}

/** Decode one length-prefixed bank record returned by the device. */
export function decodeBankRecord(bytes: Uint8Array): Bank {
  const reader = new Reader(bytes);
  const bank = readBank(reader);
  if (reader.position !== bytes.length) throw new ImageError("RECORD_LENGTH", "bank response has trailing bytes");
  return bank;
}

function checkedU32(value: number, label: string): number {
  if (!Number.isInteger(value) || value < 0 || value > 0xffffffff) throw new ImageError("RANGE", `${label} must be uint32`);
  return value >>> 0;
}

export function encodeImage(config: ConfigV1, sequence: number): Uint8Array {
  checkedU32(sequence, "sequence");
  if (config.banks.length > 128) throw new ImageError("COUNT", "configuration contains more than 128 banks");
  const records = config.banks.map(writeBank);
  const offsets = new Uint32Array(128); offsets.fill(EMPTY_OFFSET);
  const payload = new Writer();
  records.forEach((record, index) => {
    offsets[index] = payload.length;
    payload.bytes(record);
  });
  const payloadBytes = payload.finish();
  const imageSize = PAYLOAD_OFFSET + payloadBytes.length;
  if (imageSize > MAX_IMAGE_SIZE) throw new ImageError("SIZE", `image exceeds ${MAX_IMAGE_SIZE} bytes`);
  const image = new Uint8Array(imageSize);
  const view = new DataView(image.buffer);
  image.set(new TextEncoder().encode("MPDL"), 0);
  view.setUint16(4, 1, true); view.setUint16(6, HEADER_SIZE, true); view.setUint32(8, imageSize, true);
  view.setUint32(12, sequence >>> 0, true); view.setUint16(16, config.banks.length, true); view.setUint16(18, 0, true);
  view.setUint32(20, HEADER_SIZE, true); view.setUint32(24, PAYLOAD_OFFSET, true); view.setUint32(28, 0, true);
  for (let index = 0; index < offsets.length; index += 1) view.setUint32(HEADER_SIZE + index * 4, offsets[index]!, true);
  image.set(payloadBytes, PAYLOAD_OFFSET);
  view.setUint32(28, crc32(image), true);
  return image;
}

function validateHeader(bytes: Uint8Array): ImageMetadata {
  if (bytes.length < PAYLOAD_OFFSET) throw new ImageError("TRUNCATED", "image is shorter than header and bank index");
  if (new TextDecoder().decode(bytes.slice(0, 4)) !== "MPDL") throw new ImageError("MAGIC", "image magic is invalid");
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const formatVersion = view.getUint16(4, true);
  const headerSize = view.getUint16(6, true);
  const imageSize = view.getUint32(8, true);
  const sequence = view.getUint32(12, true);
  const bankCount = view.getUint16(16, true);
  const indexOffset = view.getUint32(20, true);
  const payloadOffset = view.getUint32(24, true);
  const storedCrc = view.getUint32(28, true);
  if (formatVersion !== 1 || headerSize !== HEADER_SIZE) throw new ImageError("VERSION", "unsupported image header");
  if (imageSize !== bytes.length || imageSize > MAX_IMAGE_SIZE) throw new ImageError("SIZE", "image size is inconsistent or too large");
  if (bankCount > 128 || indexOffset !== HEADER_SIZE || payloadOffset !== PAYLOAD_OFFSET || payloadOffset > imageSize) throw new ImageError("LAYOUT", "image layout is invalid");
  const checksumBytes = bytes.slice(); new DataView(checksumBytes.buffer).setUint32(28, 0, true);
  if (crc32(checksumBytes) !== storedCrc) throw new ImageError("CRC", "image CRC-32 does not match");
  return { formatVersion, imageSize, sequence, bankCount, crc32: storedCrc };
}

export function inspectImage(bytes: Uint8Array): ImageMetadata {
  return validateHeader(bytes);
}

export function decodeImage(bytes: Uint8Array): ConfigV1 {
  const metadata = validateHeader(bytes);
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const banks: Bank[] = [];
  for (let index = 0; index < metadata.bankCount; index += 1) {
    const relativeOffset = view.getUint32(HEADER_SIZE + index * 4, true);
    if (relativeOffset === EMPTY_OFFSET) continue;
    if (relativeOffset >= bytes.length - PAYLOAD_OFFSET) throw new ImageError("OFFSET", "bank offset lies outside payload");
    const recordStart = PAYLOAD_OFFSET + relativeOffset;
    const reader = new Reader(bytes, bytes.length);
    reader.seek(recordStart);
    banks.push(readBank(reader));
  }
  return { schemaVersion: 1, deviceModel: "MIDI_PEDAL_PICO2", banks };
}
