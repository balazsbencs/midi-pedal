import type { HilCase } from "../types";

export const requiredReleaseCaseIds = [
  "switch.latency-under-50ms", "switch.chord-bank-down", "switch.chord-bank-up", "switch.chord-page-down", "switch.chord-page-up",
  "midi.trs-bytes", "midi.usb-packets", "midi.per-message-routing", "midi.queue-order",
  "relay.boot-open", "relay.reset-open", "relay.watchdog-open", "relay.brownout-open", "relay.unpowered-open",
  "expression.calibration-sweep", "display.response-under-100ms", "power.usb-only", "power.9v-only", "power.source-handover",
  "config.interrupted-upload", "editor.keyboard", "editor.themes", "editor.zoom-200"
] as const;

export const releaseSuite: HilCase[] = requiredReleaseCaseIds.map(id => ({
  id,
  releaseBlocking: true,
  run: async () => ({ status: "SKIP", evidence: [], notes: [`SKIP: physical evidence required for ${id}`], skipReason: "physical evidence required" })
}));

