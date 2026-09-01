import type { DeviceMetadata } from "../../domain/editor_state";

export function ConnectionStatus({ device }: { device: DeviceMetadata }) {
  return (
    <div className={`connection-status ${device.connected ? "is-connected" : ""}`} aria-live="polite">
      <span className="status-dot" aria-hidden="true" />
      <span>{device.connected ? `Pico 2 · ${device.activeSlot ? `slot ${device.activeSlot}` : "ready"}` : "No pedal connected"}</span>
    </div>
  );
}

