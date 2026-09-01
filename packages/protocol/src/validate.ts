import { Ajv2020, type ErrorObject, type ValidateFunction } from "ajv/dist/2020.js";
import schema from "../schema/config-v1.schema.json" with { type: "json" };
import type {
  Bank,
  ConfigDocumentV1,
  ConfigV1,
  JsonValue,
  Message,
  Page,
  Preset,
  ValidationError,
  ValidationResult,
} from "./model.js";

const ROOT_KEYS = new Set(["schemaVersion", "deviceModel", "banks"]);
const DANGEROUS_KEYS = new Set(["__proto__", "prototype", "constructor"]);
const ajv = new Ajv2020({ allErrors: true, strict: true });
const validateShape: ValidateFunction = ajv.compile(schema);

function error(path: string, code: string, message: string): ValidationError {
  return { path, code, message };
}

function isJsonValue(value: unknown, path: string, errors: ValidationError[]): value is JsonValue {
  if (value === null || typeof value === "string" || typeof value === "boolean") return true;
  if (typeof value === "number") return Number.isFinite(value);
  if (Array.isArray(value)) {
    value.forEach((item, index) => isJsonValue(item, `${path}/${index}`, errors));
    return !errors.some(item => item.path.startsWith(path));
  }
  if (typeof value === "object") {
    for (const [key, item] of Object.entries(value)) {
      if (DANGEROUS_KEYS.has(key)) errors.push(error(`${path}/${key}`, "unsafe-key", "prototype-pollution key is not allowed"));
      isJsonValue(item, `${path}/${key}`, errors);
    }
    return !errors.some(item => item.path.startsWith(path));
  }
  errors.push(error(path, "not-json", "value must be JSON-compatible"));
  return false;
}

function ajvErrors(input: ErrorObject[] | null | undefined): ValidationError[] {
  return (input ?? []).map(item => error(item.instancePath || "/", item.keyword, item.message ?? "invalid value"));
}

function toDefaultBank(bankIndex: number): Bank {
  const pages = [0, 1, 2, 3].map(pageIndex => {
    const presets = [0, 1, 2, 3].map(presetIndex => ({
      id: 0x30000000 + bankIndex * 16 + pageIndex * 4 + presetIndex + 1,
      position1: { label: "EMPTY", accentRgb565: 0x7bef },
      position2: { label: "EMPTY", accentRgb565: 0x7bef },
      toggleOn: null,
      slots: [],
    } satisfies Preset)) as unknown as [Preset, Preset, Preset, Preset];
    return { id: 0x20000000 + bankIndex * 4 + pageIndex + 1, presets } satisfies Page;
  }) as [Page, Page, Page, Page];
  return {
    id: 0x10000000 + bankIndex + 1,
    name: `BANK ${bankIndex + 1}`,
    pages,
    expression: { enabled: false, label: "EXPR", channel: 1, controller: 11, destination: "BOTH", minimum: 0, maximum: 127, inverted: false },
  };
}

function normalizeBanks(input: ConfigV1["banks"]): Bank[] {
  const banks = input.map((bank, bankIndex) => {
    const fallback = toDefaultBank(bankIndex);
    return {
      ...fallback,
      ...bank,
      id: bank.id || fallback.id,
      pages: bank.pages.map((page, pageIndex) => ({
        ...page,
        id: page.id || fallback.pages[pageIndex]!.id,
        presets: page.presets.map((preset, presetIndex) => ({
          ...preset,
          id: preset.id || fallback.pages[pageIndex]!.presets[presetIndex]!.id,
          slots: preset.slots.map((slot, slotIndex) => ({
            ...slot,
            id: slot.id || (0x40000000 + bankIndex * 128 + pageIndex * 32 + presetIndex * 8 + slotIndex + 1),
          })),
        })) as [Preset, Preset, Preset, Preset],
      })) as [Page, Page, Page, Page],
    } satisfies Bank;
  });
  while (banks.length < 128) banks.push(toDefaultBank(banks.length));
  return banks;
}

function validateMessageSemantics(config: ConfigV1): ValidationError[] {
  const errors: ValidationError[] = [];
  const ids = new Set<number>();
  const addId = (id: number, path: string) => {
    if (!Number.isInteger(id) || id < 1 || id > 0xffffffff) errors.push(error(path, "id-range", "id must be a nonzero uint32"));
    else if (ids.has(id)) errors.push(error(path, "duplicate-id", "id must be unique across the document"));
    else ids.add(id);
  };
  config.banks.forEach((bank, bankIndex) => {
    addId(bank.id, `/banks/${bankIndex}/id`);
    bank.pages.forEach((page, pageIndex) => {
      addId(page.id, `/banks/${bankIndex}/pages/${pageIndex}/id`);
      page.presets.forEach((preset, presetIndex) => {
        addId(preset.id, `/banks/${bankIndex}/pages/${pageIndex}/presets/${presetIndex}/id`);
        if (preset.position1.label.length > 12) errors.push(error(`/banks/${bankIndex}/pages/${pageIndex}/presets/${presetIndex}/position1/label`, "max-length", "label is limited to 12 printable ASCII characters"));
        if (preset.position2.label.length > 12) errors.push(error(`/banks/${bankIndex}/pages/${pageIndex}/presets/${presetIndex}/position2/label`, "max-length", "label is limited to 12 printable ASCII characters"));
        preset.slots.forEach((slot, slotIndex) => {
          addId(slot.id, `/banks/${bankIndex}/pages/${pageIndex}/presets/${presetIndex}/slots/${slotIndex}/id`);
          const message = slot.message as Message;
          if (message.type === "NAV") {
            const targetRequired = message.operation.endsWith("_SET");
            if (targetRequired && message.target === undefined) errors.push(error(`/banks/${bankIndex}/pages/${pageIndex}/presets/${presetIndex}/slots/${slotIndex}/message/target`, "required", "set navigation requires a target"));
            if (!targetRequired && message.target !== undefined) errors.push(error(`/banks/${bankIndex}/pages/${pageIndex}/presets/${presetIndex}/slots/${slotIndex}/message/target`, "unexpected", "relative navigation cannot have a target"));
            if (message.operation === "PAGE_SET" && message.target !== undefined && message.target > 4) errors.push(error(`/banks/${bankIndex}/pages/${pageIndex}/presets/${presetIndex}/slots/${slotIndex}/message/target`, "range", "page target must be 1–4"));
          }
        });
      });
    });
    if (bank.expression.minimum > bank.expression.maximum) errors.push(error(`/banks/${bankIndex}/expression`, "range", "minimum must not exceed maximum"));
  });
  return errors;
}

export function validateConfig(input: unknown): ValidationResult<ConfigDocumentV1> {
  if (typeof input !== "object" || input === null || Array.isArray(input)) return { ok: false, errors: [error("/", "type", "configuration must be an object")] };
  const root = input as Record<string, unknown>;
  const passthroughTopLevel: Record<string, JsonValue> = {};
  const metadataErrors: ValidationError[] = [];
  for (const [key, value] of Object.entries(root)) {
    if (!ROOT_KEYS.has(key)) {
      if (DANGEROUS_KEYS.has(key)) metadataErrors.push(error(`/${key}`, "unsafe-key", "prototype-pollution key is not allowed"));
      else if (isJsonValue(value, `/${key}`, metadataErrors)) passthroughTopLevel[key] = value;
    }
  }
  const behavioral = { schemaVersion: root.schemaVersion, deviceModel: root.deviceModel, banks: root.banks };
  if (!validateShape(behavioral)) return { ok: false, errors: [...metadataErrors, ...ajvErrors(validateShape.errors)].sort((a, b) => a.path.localeCompare(b.path) || a.code.localeCompare(b.code)) };
  const config = { schemaVersion: 1, deviceModel: "MIDI_PEDAL_PICO2", banks: normalizeBanks(root.banks as ConfigV1["banks"]) } satisfies ConfigV1;
  const semanticErrors = validateMessageSemantics(config);
  const errors = [...metadataErrors, ...semanticErrors].sort((a, b) => a.path.localeCompare(b.path) || a.code.localeCompare(b.code));
  return errors.length ? { ok: false, errors } : { ok: true, value: { config, passthroughTopLevel } };
}

export function serializeConfigDocument(document: ConfigDocumentV1): Record<string, JsonValue> {
  return {
    ...document.passthroughTopLevel,
    schemaVersion: document.config.schemaVersion,
    deviceModel: document.config.deviceModel,
    banks: document.config.banks as unknown as JsonValue,
  };
}
