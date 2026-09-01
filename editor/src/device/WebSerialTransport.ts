import { TransportError, type DeviceTransport } from "./DeviceTransport";

interface SerialReader { read(): Promise<{ value?: Uint8Array; done: boolean }>; releaseLock(): void; }
interface SerialWriter { write(bytes: Uint8Array): Promise<void>; releaseLock(): void; }
interface SerialPortLike { readable?: ReadableStream<Uint8Array>; writable?: WritableStream<Uint8Array>; open(options: { baudRate: number }): Promise<void>; close(): Promise<void>; }
interface SerialLike { requestPort(): Promise<SerialPortLike>; }

export class WebSerialTransport implements DeviceTransport {
  private readonly serial: SerialLike;
  private port?: SerialPortLike;
  private reader?: SerialReader;
  private writer?: SerialWriter;

  constructor(serial: SerialLike = (navigator as Navigator & { serial?: SerialLike }).serial!) {
    if (!serial) throw new TransportError("UNSUPPORTED_BROWSER", "WebSerial is not available in this browser");
    this.serial = serial;
  }

  async open(): Promise<void> {
    try {
      this.port = await this.serial.requestPort();
      await this.port.open({ baudRate: 115200 });
      if (!this.port.readable || !this.port.writable) throw new TransportError("DISCONNECTED", "selected serial port has no readable/writable streams");
      this.reader = this.port.readable.getReader() as unknown as SerialReader;
      this.writer = this.port.writable.getWriter() as unknown as SerialWriter;
    } catch (error) {
      if (error instanceof TransportError) throw error;
      const name = (error as { name?: string }).name;
      if (name === "NotAllowedError") throw new TransportError("PERMISSION_DENIED", "serial permission was denied");
      if (name === "InvalidStateError") throw new TransportError("PORT_BUSY", "serial port is already in use");
      throw new TransportError("DISCONNECTED", "serial port could not be opened");
    }
  }

  async write(bytes: Uint8Array): Promise<void> {
    if (!this.writer) throw new TransportError("DISCONNECTED", "serial port is not open");
    try { await this.writer.write(bytes); } catch { throw new TransportError("DISCONNECTED", "serial write failed"); }
  }

  async read(): Promise<Uint8Array | null> {
    if (!this.reader) throw new TransportError("DISCONNECTED", "serial port is not open");
    try { const result = await this.reader.read(); return result.done ? null : result.value ?? new Uint8Array(); }
    catch { throw new TransportError("DISCONNECTED", "serial read failed"); }
  }

  async close(): Promise<void> {
    this.reader?.releaseLock(); this.writer?.releaseLock(); this.reader = undefined; this.writer = undefined;
    await this.port?.close(); this.port = undefined;
  }
}

