import { encodeImage, inspectImage, validateConfig, type ConfigDocumentV1 } from "@midi-pedal/protocol";

import type { EditorState } from "./editor_state";
import { selectedBank, selectedPage, selectedPreset } from "./editor_reducer";

export { selectedBank, selectedPage, selectedPreset };

export function validateDraft(document: ConfigDocumentV1) {
  const result = validateConfig({ ...document.passthroughTopLevel, ...document.config });
  return result.ok ? [] : result.errors;
}

export function draftImageCrc32(state: EditorState): number | undefined {
  try { return inspectImage(encodeImage(state.draft.config, state.device.sequence ?? 0)).crc32; }
  catch { return undefined; }
}

export function selectionPaths(state: EditorState): string[] {
  return state.validationErrors.filter(error => error.path.startsWith(`/banks/${state.selection.bank}`)).map(error => error.path);
}

export function documentCounts(document: ConfigDocumentV1) {
  const banks = document.config.banks.length;
  const pages = document.config.banks.reduce((sum, bank) => sum + bank.pages.length, 0);
  const presets = document.config.banks.reduce((sum, bank) => sum + bank.pages.reduce((pageSum, page) => pageSum + page.presets.length, 0), 0);
  return { banks, pages, presets };
}

