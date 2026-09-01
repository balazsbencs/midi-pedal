import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { describe, expect, it, vi } from "vitest";

import type { Bank } from "@midi-pedal/protocol";

import { PageMap } from "./presets/PageMap";

function testBank(): Bank {
  const presets = ["A", "B", "C", "D"].map((letter, index) => ({
    id: index + 1,
    position1: { label: `${letter} ONE`, accentRgb565: 0x07ff },
    position2: { label: `${letter} TWO`, accentRgb565: 0xf800 },
    toggleOn: null,
    slots: []
  })) as unknown as Bank["pages"][number]["presets"];
  const page = { id: 1, presets };
  return {
    id: 1,
    name: "TEST BANK",
    pages: [page, { ...page, id: 2 }, { ...page, id: 3 }, { ...page, id: 4 }],
    expression: { enabled: false, label: "EXPR", channel: 1, controller: 11, destination: "BOTH", minimum: 0, maximum: 127, inverted: false }
  };
}

describe("pedal workspace", () => {
  it("orders presets A/B/C/D as the physical 2x2 surface", () => {
    render(<PageMap bank={testBank()} selectedPage={0} selectedPreset={0} onPageSelect={vi.fn()} onPresetSelect={vi.fn()} />);
    expect(screen.getAllByRole("button", { name: /Preset/ }).map(button => button.dataset.switch)).toEqual(["A", "B", "C", "D"]);
  });

  it("selects page tabs and presets from the keyboard", async () => {
    const onPageSelect = vi.fn();
    const onPresetSelect = vi.fn();
    render(<PageMap bank={testBank()} selectedPage={0} selectedPreset={0} onPageSelect={onPageSelect} onPresetSelect={onPresetSelect} />);
    screen.getByRole("tab", { name: "Page 2" }).focus();
    await userEvent.keyboard("{Enter}");
    expect(onPageSelect).toHaveBeenCalledWith(1);
    screen.getByRole("button", { name: /Preset B/ }).focus();
    await userEvent.keyboard("{Enter}");
    expect(onPresetSelect).toHaveBeenCalledWith(1);
  });
});
