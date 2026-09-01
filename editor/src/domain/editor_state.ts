import { validateConfig, type ConfigDocumentV1, type ValidationError } from "@midi-pedal/protocol";

export type Position = 1 | 2;

export interface Selection {
  bank: number;
  page: number;
  preset: number;
  position: Position;
}

export interface DeviceMetadata {
  connected: boolean;
  model?: string;
  protocolVersion?: number;
  imageCrc32?: number;
  imageSize?: number;
  sequence?: number;
  activeSlot?: "A" | "B";
  capabilities?: Record<string, unknown>;
}

export type SyncState =
  | { stage: "idle" }
  | { stage: "connecting" | "reading" | "begin" | "write" | "verify" | "activate" | "readback"; completed?: number; total?: number }
  | { stage: "success"; message: string }
  | { stage: "error"; message: string; previousConfigurationIntact: boolean };

export interface EditorState {
  draft: ConfigDocumentV1;
  device: DeviceMetadata;
  selection: Selection;
  dirty: boolean;
  validationErrors: ValidationError[];
  sync: SyncState;
}

export function makeFactoryDocument(): ConfigDocumentV1 {
  const result = validateConfig({ schemaVersion: 1, deviceModel: "MIDI_PEDAL_PICO2", banks: [] });
  if (!result.ok) throw new Error("factory-empty document must validate");
  return result.value;
}

export function makeInitialState(): EditorState {
  return {
    draft: makeFactoryDocument(),
    device: { connected: false },
    selection: { bank: 0, page: 0, preset: 0, position: 1 },
    dirty: false,
    validationErrors: [],
    sync: { stage: "idle" }
  };
}

