import { documentCounts } from "./selectors";
import { serializeConfigDocument, validateConfig, type ConfigDocumentV1, type ValidationError } from "@midi-pedal/protocol";

export interface ImportPreviewOk { ok: true; document: ConfigDocumentV1; counts: ReturnType<typeof documentCounts> }
export interface ImportPreviewError { ok: false; document?: undefined; errors: ValidationError[]; message: string }
export type ImportPreview = ImportPreviewOk | ImportPreviewError;

export function exportDraft(document: ConfigDocumentV1): Blob {
  return new Blob([JSON.stringify(serializeConfigDocument(document), null, 2) + "\n"], { type: "application/json" });
}

export function previewImport(text: string): ImportPreview {
  let input: unknown;
  try { input = JSON.parse(text); }
  catch { return { ok: false, document: undefined, errors: [{ path: "/", code: "json", message: "file is not valid JSON" }], message: "The selected file is not valid JSON." }; }
  const result = validateConfig(input);
  if (!result.ok) return { ok: false, document: undefined, errors: result.errors, message: "The configuration has validation errors." };
  return { ok: true, document: result.value, counts: documentCounts(result.value) };
}
