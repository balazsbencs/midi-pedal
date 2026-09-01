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
      <div className="brand-lockup"><span className="brand-mark" aria-hidden="true">MP</span><div><p className="eyebrow">MIDI PEDAL</p><h1>Editor <span className="version-chip">v1</span></h1></div></div>
      <div className="header-actions">
        <span className={`connection-pill ${state.device.connected ? "is-connected" : ""}`} aria-live="polite"><span className="status-dot" aria-hidden="true" />{state.device.connected ? "Connected" : "Offline"}</span>
        <ThemeSwitch theme={theme} onChange={onThemeChange} />
        <button type="button" onClick={onExport}>Export JSON</button>
        <button type="button" onClick={onImport}>Import JSON</button>
        <button type="button" className="primary-button" disabled={!state.device.connected || state.validationErrors.length > 0 || !state.dirty} onClick={onSync}>Sync to pedal</button>
        {!state.device.connected && <button type="button" className="connect-button" onClick={onConnect}>Connect pedal</button>}
      </div>
    </header>
  );
}
