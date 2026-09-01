import type { ConfigV1 } from "@midi-pedal/protocol";

import { DeviceSession, DeviceSessionError } from "../device/DeviceSession";

export type SyncStage = "BEGIN" | "WRITE" | "VERIFY" | "ACTIVATE" | "READBACK";
export interface SyncEvent { stage: SyncStage; completed: number; total: number }
export type SyncResult =
  | { ok: true; activeCrc32: number; metadata: unknown }
  | { ok: false; stage: SyncStage; message: string; previousConfigurationIntact: true; code: string };

export async function synchronizeDraft(session: DeviceSession, config: ConfigV1, onProgress?: (event: SyncEvent) => void): Promise<SyncResult> {
  let stage: SyncStage = "BEGIN";
  try {
    const result = await session.syncConfiguration(config, event => {
      stage = event.stage;
      onProgress?.(event);
    });
    return result;
  } catch (error) {
    const sessionError = error instanceof DeviceSessionError ? error : new Error(String(error));
    return { ok: false, stage, message: sessionError.message, previousConfigurationIntact: true, code: error instanceof DeviceSessionError ? error.code : "UNKNOWN" };
  }
}

