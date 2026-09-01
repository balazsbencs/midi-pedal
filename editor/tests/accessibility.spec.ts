import { test, expect } from "@playwright/test";

test("workspace exposes labelled controls and no horizontal overflow", async ({ page }) => {
  await page.goto("/");
  await expect(page.getByRole("heading", { name: "Editor" })).toBeVisible();
  await expect(page.getByRole("button", { name: "Switch to dark theme" })).toBeVisible();
  await expect(page.getByRole("navigation", { name: "Banks" })).toBeVisible();
  await expect(page.getByRole("button", { name: /Preset A/ })).toBeVisible();
  expect(await page.evaluate(() => document.documentElement.scrollWidth <= window.innerWidth)).toBe(true);
});

test("theme control remains keyboard reachable", async ({ page }) => {
  await page.goto("/");
  const theme = page.getByRole("button", { name: "Switch to dark theme" });
  await theme.focus();
  await expect(theme).toBeFocused();
  await page.keyboard.press("Enter");
  await expect(page.getByRole("button", { name: "Switch to light theme" })).toBeFocused();
});

