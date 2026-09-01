import { useEffect, useReducer, useState } from "react";

import { makeInitialState } from "./domain/editor_state";
import { editorReducer } from "./domain/editor_reducer";
import { selectedBank, selectedPreset } from "./domain/selectors";
import { AppHeader } from "./ui/layout/AppHeader";
import { StatusBar } from "./ui/layout/StatusBar";
import { BankList } from "./ui/banks/BankList";
import { ExpressionSummary } from "./ui/expression/ExpressionSummary";
import { PageMap } from "./ui/presets/PageMap";
import { PresetInspector } from "./ui/inspector/PresetInspector";
import { applyTheme, readInitialTheme, themeStorageKey, type Theme } from "./ui/theme/theme";
import "./ui/theme/tokens.css";
import "./ui/workspace.css";

export function App() {
  const [state, dispatch] = useReducer(editorReducer, undefined, makeInitialState);
  const [theme, setTheme] = useState<Theme>(() =>
    readInitialTheme(window.localStorage, window.matchMedia?.("(prefers-color-scheme: dark)") ?? { matches: false })
  );

  useEffect(() => applyTheme(theme), [theme]);

  const changeTheme = (next: Theme) => {
    setTheme(next);
    window.localStorage.setItem(themeStorageKey, next);
  };

  const exportDraft = () => {
    const payload = { ...state.draft.passthroughTopLevel, ...state.draft.config };
    const blob = new Blob([JSON.stringify(payload, null, 2) + "\n"], { type: "application/json" });
    const link = document.createElement("a");
    link.href = URL.createObjectURL(blob);
    link.download = "midi-pedal-config.json";
    link.click();
    URL.revokeObjectURL(link.href);
  };

  const connect = () => dispatch({ type: "sync.failed", message: "Connect is available from the WebSerial session controls.", previousConfigurationIntact: true });
  const importDraft = () => dispatch({ type: "sync.failed", message: "Use Import JSON from the device session once a draft file is selected.", previousConfigurationIntact: true });
  const sync = () => dispatch({ type: "sync.failed", message: "No pedal is connected.", previousConfigurationIntact: true });
  const bank = selectedBank(state);

  return (
    <div className="app-shell">
      <AppHeader state={state} theme={theme} onThemeChange={changeTheme} onConnect={connect} onExport={exportDraft} onImport={importDraft} onSync={sync} />
      <main className="workspace-grid">
        <BankList banks={state.draft.config.banks} selected={state.selection.bank} onSelect={index => dispatch({ type: "selection.bankChanged", index })} />
        <section className="map-pane">
          <PageMap bank={bank} selectedPage={state.selection.page} selectedPreset={state.selection.preset} onPageSelect={index => dispatch({ type: "selection.pageChanged", index })} onPresetSelect={index => dispatch({ type: "selection.presetChanged", index })} />
          <ExpressionSummary expression={bank.expression} />
        </section>
        <PresetInspector state={state} dispatch={dispatch} />
      </main>
      <StatusBar state={state} />
    </div>
  );
}
