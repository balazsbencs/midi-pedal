import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { describe, expect, it, vi } from "vitest";

import { ThemeSwitch } from "./ThemeSwitch";
import { readInitialTheme } from "./theme";

describe("theme", () => {
  it("uses system preference only when no explicit choice exists", () => {
    const emptyStorage = { getItem: () => null };
    const darkMedia = { matches: true };
    expect(readInitialTheme(emptyStorage, darkMedia)).toBe("dark");
    expect(readInitialTheme({ getItem: () => "light" }, darkMedia)).toBe("light");
  });

  it("labels the action, not merely the current icon", async () => {
    const onChange = vi.fn();
    render(<ThemeSwitch theme="dark" onChange={onChange} />);
    await userEvent.click(screen.getByRole("button", { name: "Switch to light theme" }));
    expect(onChange).toHaveBeenCalledWith("light");
  });
});

