import type { EditorState } from "../../domain/editor_state";
import { ThemeSwitch } from "../theme/ThemeSwitch";
import type { Theme } from "../theme/theme";

interface AppHeaderProps {
  state: EditorState;
  theme: Theme;
  onThemeChange: (theme: Theme) => void;
  onConnect: () => void;
  onExport: () => void;
  onImport: () => void;
  onSync: () => void;
}

export function AppHeader({ state, theme, onThemeChange, onConnect, onExport, onImport, onSync }: AppHeaderProps) {
  return (
    <header className="app-header">
      <div className="brand-lockup">
        <span className="brand-mark" aria-hidden="true"><span /><span /><span /><span /></span>
        <div><strong>MIDI PEDAL</strong><h1>Editor</h1></div>
      </div>
      <div className="header-actions">
        <span className={`connection-pill ${state.device.connected ? "is-connected" : ""}`} aria-live="polite"><span className="status-dot" aria-hidden="true" />{state.device.connected ? "Connected" : "Offline"}</span>
        <ThemeSwitch theme={theme} onChange={onThemeChange} />
        <button type="button" aria-label="Import JSON" onClick={onImport}><span aria-hidden="true">↑</span> Import JSON</button>
        <button type="button" aria-label="Export JSON" onClick={onExport}><span aria-hidden="true">↓</span> Export JSON</button>
        <button type="button" className="primary-button" disabled={!state.device.connected || state.validationErrors.length > 0 || !state.dirty} onClick={onSync}><span aria-hidden="true">↻</span> Sync to pedal</button>
        {!state.device.connected && <button type="button" className="connect-button" onClick={onConnect}><span aria-hidden="true">⌁</span> Connect pedal</button>}
      </div>
    </header>
  );
}
