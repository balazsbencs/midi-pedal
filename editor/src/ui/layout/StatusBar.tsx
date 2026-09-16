import type { EditorState } from "../../domain/editor_state";
import { ErrorSummary } from "../states/ErrorSummary";

export function StatusBar({ state }: { state: EditorState }) {
  const validation = state.validationErrors.length === 0 ? "Draft valid" : `${state.validationErrors.length} validation error${state.validationErrors.length === 1 ? "" : "s"}`;
  const sync = state.dirty ? "Unsynced changes" : state.device.connected ? "Matches pedal" : "Not connected";
  return (
    <footer className="status-bar" aria-live="polite">
      <div className="status-group">
        <span className={state.validationErrors.length ? "status-error" : "status-success"}><span className="status-dot" aria-hidden="true" />{validation}</span>
        <span>{sync}</span>
        {state.sync.stage === "error" && <span className="status-error">{state.sync.message}</span>}
        {state.sync.stage === "success" && <span className="status-success">{state.sync.message}</span>}
      </div>
      <div className="status-breadcrumb" aria-label="Current selection">
        <span>Bank {state.selection.bank + 1}</span><b aria-hidden="true">/</b><span>Page {state.selection.page + 1}</span><b aria-hidden="true">/</b><span>Preset {String.fromCharCode(65 + state.selection.preset)}</span>
      </div>
      <ErrorSummary errors={state.validationErrors} />
    </footer>
  );
}
