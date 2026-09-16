export type Theme = "light" | "dark";

export const themeStorageKey = "midi-pedal.theme";

export function readInitialTheme(
  storage: Pick<Storage, "getItem">,
  _media: Pick<MediaQueryList, "matches">
): Theme {
  const stored = storage.getItem(themeStorageKey);
  return stored === "light" || stored === "dark" ? stored : "dark";
}

export function applyTheme(theme: Theme, documentElement: HTMLElement = document.documentElement): void {
  documentElement.dataset.theme = theme;
  documentElement.style.colorScheme = theme;
}
