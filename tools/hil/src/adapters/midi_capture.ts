export interface MidiCaptureAdapter {
  start(): Promise<void>;
  stop(): Promise<void>;
  bytes(): Promise<number[]>;
}

