export type Destination = "TRS" | "USB" | "BOTH";
export type Trigger = "PRESS" | "RELEASE" | "LONG_PRESS" | "DOUBLE_TAP";
export type PositionFilter = "POSITION_1" | "POSITION_2" | "BOTH";
export type JsonValue = null | boolean | number | string | JsonValue[] | { [key: string]: JsonValue };

export type Message =
  | { type: "PC"; channel: number; program: number; destination: Destination }
  | { type: "CC"; channel: number; controller: number; value: number; destination: Destination }
  | { type: "RELAY"; contact: 1 | 2; operation: "OPEN" | "CLOSE" | "TOGGLE" }
  | { type: "NAV"; operation: "BANK_UP" | "BANK_DOWN" | "BANK_SET" | "PAGE_UP" | "PAGE_DOWN" | "PAGE_SET"; target?: number };

export interface MessageSlot {
  id: number;
  trigger: Trigger;
  position: PositionFilter;
  message: Message;
}

export interface PositionView {
  label: string;
  accentRgb565: number;
}

export interface Preset {
  id: number;
  position1: PositionView;
  position2: PositionView;
  toggleOn: Trigger | null;
  slots: MessageSlot[];
}

export interface Page {
  id: number;
  presets: [Preset, Preset, Preset, Preset];
}

export interface ExpressionAssignment {
  enabled: boolean;
  label: string;
  channel: number;
  controller: number;
  destination: Destination;
  minimum: number;
  maximum: number;
  inverted: boolean;
}

export interface Bank {
  id: number;
  name: string;
  pages: [Page, Page, Page, Page];
  expression: ExpressionAssignment;
}

export interface ConfigV1 {
  schemaVersion: 1;
  deviceModel: "MIDI_PEDAL_PICO2";
  banks: Bank[];
}

export interface ConfigDocumentV1 {
  config: ConfigV1;
  passthroughTopLevel: Readonly<Record<string, JsonValue>>;
}

export interface ValidationError {
  path: string;
  code: string;
  message: string;
}

export type ValidationResult<T> =
  | { ok: true; value: T }
  | { ok: false; errors: ValidationError[] };
