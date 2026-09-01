export interface DeviceTransport {
  open(): Promise<void>;
  write(bytes: Uint8Array): Promise<void>;
  read(): Promise<Uint8Array | null>;
  close(): Promise<void>;
}

export class TransportError extends Error {
  constructor(readonly code: "PERMISSION_DENIED" | "PORT_BUSY" | "DISCONNECTED" | "UNSUPPORTED_BROWSER", message: string) {
    super(message);
    this.name = "TransportError";
  }
}

