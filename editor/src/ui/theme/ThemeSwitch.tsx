import type { Theme } from "./theme";

interface ThemeSwitchProps {
  theme: Theme;
  onChange: (theme: Theme) => void;
}

export function ThemeSwitch({ theme, onChange }: ThemeSwitchProps) {
  const next = theme === "dark" ? "light" : "dark";
  return (
    <button
      type="button"
      className="theme-switch"
      aria-label={`Switch to ${next} theme`}
      aria-pressed={theme === "dark"}
      onClick={() => onChange(next)}
    >
      <span aria-hidden="true">{theme === "dark" ? "☀" : "☾"}</span>
      <span className="sr-only">{`Switch to ${next} theme`}</span>
    </button>
  );
}
