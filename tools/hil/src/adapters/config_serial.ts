export interface ConfigSerialAdapter {
  writeFrame(bytes: Uint8Array): Promise<void>;
  readFrame(): Promise<Uint8Array | null>;
}

