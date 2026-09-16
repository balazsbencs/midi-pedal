import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { describe, expect, it, vi } from "vitest";

import { ThemeSwitch } from "./ThemeSwitch";
import { readInitialTheme } from "./theme";

describe("theme", () => {
  it("defaults to the studio-console dark theme and honors explicit choices", () => {
    const emptyStorage = { getItem: () => null };
    expect(readInitialTheme(emptyStorage, { matches: false })).toBe("dark");
    expect(readInitialTheme({ getItem: () => "light" }, { matches: true })).toBe("light");
  });

  it("labels the action, not merely the current icon", async () => {
    const onChange = vi.fn();
    render(<ThemeSwitch theme="dark" onChange={onChange} />);
    await userEvent.click(screen.getByRole("button", { name: "Switch to light theme" }));
    expect(onChange).toHaveBeenCalledWith("light");
  });
});
