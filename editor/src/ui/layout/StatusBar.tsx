import type { EditorState } from "../../domain/editor_state";

export function StatusBar({ state }: { state: EditorState }) {
  const validation = state.validationErrors.length === 0 ? "Draft valid" : `${state.validationErrors.length} validation error${state.validationErrors.length === 1 ? "" : "s"}`;
  const sync = state.dirty ? "Unsynced changes" : state.device.connected ? "Matches pedal" : "Not connected";
  return (
    <footer className="status-bar" aria-live="polite">
      <span className={state.validationErrors.length ? "status-error" : "status-success"}>{validation}</span>
      <span>{sync}</span>
      {state.sync.stage === "error" && <span className="status-error">{state.sync.message}</span>}
      {state.sync.stage === "success" && <span className="status-success">{state.sync.message}</span>}
    </footer>
  );
}

