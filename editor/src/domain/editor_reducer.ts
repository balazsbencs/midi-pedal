import type { Bank, Message, MessageSlot, Preset, Trigger } from "@midi-pedal/protocol";

import type { EditorAction } from "./editor_actions";
import type { EditorState } from "./editor_state";

function updateSelectedPreset(state: EditorState, update: (preset: Preset) => Preset): EditorState {
  const banks = [...state.draft.config.banks];
  const bank = banks[state.selection.bank]!;
  const pages = [...bank.pages] as typeof bank.pages;
  const page = pages[state.selection.page]!;
  const presets = [...page.presets] as typeof page.presets;
  presets[state.selection.preset] = update(page.presets[state.selection.preset]!);
  pages[state.selection.page] = { ...page, presets };
  banks[state.selection.bank] = { ...bank, pages };
  return { ...state, draft: { ...state.draft, config: { ...state.draft.config, banks } }, dirty: true };
}

function updateSelectedBank(state: EditorState, update: (bank: Bank) => Bank): EditorState {
  const banks = [...state.draft.config.banks];
  banks[state.selection.bank] = update(banks[state.selection.bank]!);
  return { ...state, draft: { ...state.draft, config: { ...state.draft.config, banks } }, dirty: true };
}

function nextId(state: EditorState): number {
  let maximum = 0;
  for (const bank of state.draft.config.banks) {
    maximum = Math.max(maximum, bank.id);
    for (const page of bank.pages) {
      maximum = Math.max(maximum, page.id);
      for (const preset of page.presets) {
        maximum = Math.max(maximum, preset.id);
        for (const slot of preset.slots) maximum = Math.max(maximum, slot.id);
      }
    }
  }
  if (maximum === 0xffffffff) throw new Error("no stable message id remains");
  return maximum + 1;
}

function defaultMessage(type: Message["type"] = "CC"): Message {
  if (type === "PC") return { type, channel: 1, program: 0, destination: "BOTH" };
  if (type === "RELAY") return { type, contact: 1, operation: "TOGGLE" };
  if (type === "NAV") return { type, operation: "PAGE_UP" };
  return { type: "CC", channel: 1, controller: 1, value: 127, destination: "BOTH" };
}

function clampIndex(index: number, maximum: number): number {
  return Math.min(Math.max(index, 0), maximum);
}

export function editorReducer(state: EditorState, action: EditorAction): EditorState {
  switch (action.type) {
    case "device.loaded":
      return { ...state, draft: action.document, device: { connected: true, ...action.metadata }, dirty: false, validationErrors: [], sync: { stage: "idle" } };
    case "device.disconnected":
      return { ...state, device: { ...state.device, connected: false }, sync: { stage: "idle" } };
    case "selection.bankChanged":
      return { ...state, selection: { ...state.selection, bank: clampIndex(action.index, state.draft.config.banks.length - 1), page: 0, preset: 0 } };
    case "selection.pageChanged":
      return { ...state, selection: { ...state.selection, page: clampIndex(action.index, 3), preset: 0 } };
    case "selection.presetChanged":
      return { ...state, selection: { ...state.selection, preset: clampIndex(action.index, 3) } };
    case "selection.positionChanged":
      return { ...state, selection: { ...state.selection, position: action.position } };
    case "bank.nameChanged":
      return updateSelectedBank(state, bank => ({ ...bank, name: action.value }));
    case "preset.labelChanged":
      return updateSelectedPreset(state, preset => ({ ...preset, [action.position === 1 ? "position1" : "position2"]: { ...preset[action.position === 1 ? "position1" : "position2"], label: action.value } }));
    case "preset.accentChanged":
      return updateSelectedPreset(state, preset => ({ ...preset, [action.position === 1 ? "position1" : "position2"]: { ...preset[action.position === 1 ? "position1" : "position2"], accentRgb565: action.value } }));
    case "preset.toggleChanged":
      return updateSelectedPreset(state, preset => ({ ...preset, toggleOn: action.value }));
    case "slot.added": {
      const slot = action.slot ?? { id: nextId(state), trigger: "PRESS" as Trigger, position: "BOTH", message: defaultMessage(action.messageType) };
      return updateSelectedPreset(state, preset => preset.slots.length >= 8 ? preset : { ...preset, slots: [...preset.slots, slot] });
    }
    case "slot.updated":
      return updateSelectedPreset(state, preset => ({ ...preset, slots: preset.slots.map((slot, index) => index !== action.index ? slot : {
        ...slot,
        ...action.patch,
        message: action.patch.message ? { ...slot.message, ...action.patch.message } as Message : slot.message
      }) }));
    case "slot.removed":
      return updateSelectedPreset(state, preset => ({ ...preset, slots: preset.slots.filter((_, index) => index !== action.index) }));
    case "slot.moved":
      return updateSelectedPreset(state, preset => {
        if (action.from < 0 || action.from >= preset.slots.length || action.to < 0 || action.to >= preset.slots.length) return preset;
        const slots = [...preset.slots];
        const [moved] = slots.splice(action.from, 1);
        slots.splice(action.to, 0, moved!);
        return { ...preset, slots };
      });
    case "expression.changed":
      return updateSelectedBank(state, bank => ({ ...bank, expression: { ...bank.expression, ...action.patch } }));
    case "document.imported":
      return { ...state, draft: action.document, dirty: true, validationErrors: [] };
    case "sync.started":
      return { ...state, sync: { stage: action.stage, completed: action.completed, total: action.total } };
    case "sync.succeeded":
      return { ...state, device: { connected: true, ...action.metadata }, dirty: false, sync: { stage: "success", message: action.message ?? "Configuration synchronized" }, validationErrors: [] };
    case "sync.failed":
      return { ...state, sync: { stage: "error", message: action.message, previousConfigurationIntact: action.previousConfigurationIntact } };
    case "draft.resetToDevice":
      return { ...state, draft: action.document, dirty: false, validationErrors: [] };
  }
}

export function selectedBank(state: EditorState): Bank { return state.draft.config.banks[state.selection.bank]!; }
export function selectedPage(state: EditorState) { return selectedBank(state).pages[state.selection.page]!; }
export function selectedPreset(state: EditorState): Preset { return selectedPage(state).presets[state.selection.preset]!; }
