import { useEffect, useReducer, useRef, useState } from "react";

import { DeviceSession, DeviceSessionError } from "./device/DeviceSession";
import { WebSerialTransport } from "./device/WebSerialTransport";
import { makeInitialState } from "./domain/editor_state";
import { editorReducer } from "./domain/editor_reducer";
import { exportDraft as exportDraftFile, previewImport } from "./domain/files";
import { synchronizeDraft } from "./domain/synchronize";
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
  const sessionRef = useRef<DeviceSession | null>(null);
  const fileInputRef = useRef<HTMLInputElement>(null);
  const [theme, setTheme] = useState<Theme>(() =>
    readInitialTheme(window.localStorage, window.matchMedia?.("(prefers-color-scheme: dark)") ?? { matches: false })
  );

  useEffect(() => applyTheme(theme), [theme]);

  const changeTheme = (next: Theme) => {
    setTheme(next);
    window.localStorage.setItem(themeStorageKey, next);
  };

  const exportDraft = () => {
    const blob = exportDraftFile(state.draft);
    const link = document.createElement("a");
    link.href = URL.createObjectURL(blob);
    link.download = "midi-pedal-config.json";
    link.click();
    URL.revokeObjectURL(link.href);
  };

  const connect = async () => {
    dispatch({ type: "sync.started", stage: "begin" });
    try {
      const session = new DeviceSession(new WebSerialTransport());
      const { capabilities, info } = await session.connect();
      const document = await session.readConfiguration();
      sessionRef.current = session;
      dispatch({ type: "device.loaded", document, metadata: { ...info, model: capabilities.deviceModel, protocolVersion: capabilities.protocolVersion, capabilities } });
    } catch (error) {
      const message = error instanceof DeviceSessionError ? `${error.code}: ${error.message}` : error instanceof Error ? error.message : "Could not connect to the pedal";
      dispatch({ type: "sync.failed", message, previousConfigurationIntact: true });
    }
  };
  const importDraft = () => fileInputRef.current?.click();
  const readImport = async (file: File) => {
    const preview = previewImport(await file.text());
    if (!preview.ok) {
      dispatch({ type: "sync.failed", message: preview.message, previousConfigurationIntact: true });
      return;
    }
    if (window.confirm(`Replace the local draft with ${preview.counts.banks} banks? Export JSON first if you want a backup.`)) dispatch({ type: "document.imported", document: preview.document });
  };
  const sync = async () => {
    if (!sessionRef.current) {
      dispatch({ type: "sync.failed", message: "No pedal is connected.", previousConfigurationIntact: true });
      return;
    }
    const result = await synchronizeDraft(sessionRef.current, state.draft.config, event => dispatch({ type: "sync.started", stage: event.stage.toLowerCase() as "begin" | "write" | "verify" | "activate" | "readback", completed: event.completed, total: event.total }));
    if (result.ok) dispatch({ type: "sync.succeeded", metadata: result.metadata as { imageCrc32: number; imageSize: number; sequence: number; activeSlot: "A" | "B" }, message: "Configuration synchronized" });
    else dispatch({ type: "sync.failed", message: `${result.code}: ${result.message}`, previousConfigurationIntact: true });
  };
  useEffect(() => {
    if (!state.dirty) return;
    const handler = (event: BeforeUnloadEvent) => { event.preventDefault(); event.returnValue = "Unsynced changes will be lost."; };
    window.addEventListener("beforeunload", handler);
    return () => window.removeEventListener("beforeunload", handler);
  }, [state.dirty]);
  const bank = selectedBank(state);

  return (
    <div className="app-shell">
      <AppHeader state={state} theme={theme} onThemeChange={changeTheme} onConnect={() => void connect()} onExport={exportDraft} onImport={importDraft} onSync={() => void sync()} />
      <input ref={fileInputRef} type="file" accept="application/json,.json" hidden onChange={event => { const file = event.target.files?.[0]; if (file) void readImport(file); event.target.value = ""; }} />
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
