export function rgb565ToHex(value: number): string {
  const red = Math.round(((value >> 11) & 0x1f) * 255 / 31);
  const green = Math.round(((value >> 5) & 0x3f) * 255 / 63);
  const blue = Math.round((value & 0x1f) * 255 / 31);
  return `#${[red, green, blue].map(channel => channel.toString(16).padStart(2, "0")).join("")}`;
}

export function hexToRgb565(hex: string): number {
  const value = Number.parseInt(hex.slice(1), 16);
  const red = (value >> 16) & 0xff;
  const green = (value >> 8) & 0xff;
  const blue = value & 0xff;
  return ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
}

export const accentChoices = [
  { name: "Cyan", value: 0x2f3a },
  { name: "Coral", value: 0xf2eb },
  { name: "Violet", value: 0xccdf },
  { name: "Yellow", value: 0xfe48 },
  { name: "Green", value: 0x4fb4 }
] as const;
