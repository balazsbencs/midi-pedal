import type { ConfigDocumentV1, Destination, Message, MessageSlot, Trigger } from "@midi-pedal/protocol";

import type { DeviceMetadata, Position } from "./editor_state";

export type SlotPatch = Omit<Partial<MessageSlot>, "message"> & { message?: Record<string, unknown> };

export type EditorAction =
  | { type: "device.loaded"; document: ConfigDocumentV1; metadata: Omit<DeviceMetadata, "connected"> }
  | { type: "device.disconnected" }
  | { type: "selection.bankChanged"; index: number }
  | { type: "selection.pageChanged"; index: number }
  | { type: "selection.presetChanged"; index: number }
  | { type: "selection.positionChanged"; position: Position }
  | { type: "bank.nameChanged"; value: string }
  | { type: "preset.labelChanged"; position: Position; value: string }
  | { type: "preset.accentChanged"; position: Position; value: number }
  | { type: "preset.toggleChanged"; value: Trigger | null }
  | { type: "slot.added"; messageType?: Message["type"]; slot?: MessageSlot }
  | { type: "slot.updated"; index: number; patch: SlotPatch }
  | { type: "slot.removed"; index: number }
  | { type: "slot.moved"; from: number; to: number }
  | { type: "expression.changed"; patch: Partial<ConfigDocumentV1["config"]["banks"][number]["expression"]> }
  | { type: "document.imported"; document: ConfigDocumentV1 }
  | { type: "sync.started"; stage: "begin" | "write" | "verify" | "activate" | "readback"; completed?: number; total?: number }
  | { type: "sync.succeeded"; metadata: Omit<DeviceMetadata, "connected">; message?: string }
  | { type: "sync.failed"; message: string; previousConfigurationIntact: boolean }
  | { type: "draft.resetToDevice"; document: ConfigDocumentV1 };

export type { Destination };
