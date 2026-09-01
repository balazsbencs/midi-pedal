import { useEffect, useState } from "react";

import { ThemeSwitch } from "./ui/theme/ThemeSwitch";
import { applyTheme, readInitialTheme, themeStorageKey, type Theme } from "./ui/theme/theme";
import "./ui/theme/tokens.css";

export function App() {
  const [theme, setTheme] = useState<Theme>(() =>
    readInitialTheme(window.localStorage, window.matchMedia("(prefers-color-scheme: dark)"))
  );

  useEffect(() => applyTheme(theme), [theme]);

  const changeTheme = (next: Theme) => {
    setTheme(next);
    window.localStorage.setItem(themeStorageKey, next);
  };

  return (
    <div className="app-shell">
      <header className="app-header">
        <div>
          <p className="eyebrow">MIDI PEDAL</p>
          <h1>Editor</h1>
        </div>
        <ThemeSwitch theme={theme} onChange={changeTheme} />
      </header>
      <main className="shell-placeholder">
        <p className="eyebrow">LOCAL WORKSPACE</p>
        <h2>Connect a pedal to begin</h2>
        <p>Drafts stay in this browser until you explicitly sync them.</p>
      </main>
    </div>
  );
}

