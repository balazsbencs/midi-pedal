import type { CSSProperties, KeyboardEvent } from "react";
import type { Bank } from "@midi-pedal/protocol";

interface PageMapProps {
  bank: Bank;
  selectedPage: number;
  selectedPreset: number;
  onPageSelect: (index: number) => void;
  onPresetSelect: (index: number) => void;
}

function rgb565ToHex(value: number): string {
  const red = Math.round(((value >> 11) & 0x1f) * 255 / 31);
  const green = Math.round(((value >> 5) & 0x3f) * 255 / 63);
  const blue = Math.round((value & 0x1f) * 255 / 31);
  return `#${[red, green, blue].map(channel => channel.toString(16).padStart(2, "0")).join("")}`;
}

export function PageMap({ bank, selectedPage, selectedPreset, onPageSelect, onPresetSelect }: PageMapProps) {
  const page = bank.pages[selectedPage]!;
  const selectPageFromKeyboard = (event: KeyboardEvent<HTMLButtonElement>, index: number) => {
    let next = index;
    if (event.key === "ArrowRight") next = (index + 1) % bank.pages.length;
    else if (event.key === "ArrowLeft") next = (index - 1 + bank.pages.length) % bank.pages.length;
    else if (event.key === "Home") next = 0;
    else if (event.key === "End") next = bank.pages.length - 1;
    else return;
    event.preventDefault();
    onPageSelect(next);
    event.currentTarget.parentElement?.querySelectorAll<HTMLButtonElement>('[role="tab"]')[next]?.focus();
  };
  return (
    <section className="map-section" aria-labelledby="page-map-title">
      <div className="section-heading">
        <div>
          <p className="eyebrow">LIVE SURFACE</p>
          <h2 id="page-map-title">{bank.name}</h2>
        </div>
        <span className="surface-hint">A / B / C / D</span>
      </div>
      <div className="page-tabs" role="tablist" aria-label="Pages">
        {bank.pages.map((item, index) => (
          <button key={item.id} type="button" role="tab" tabIndex={selectedPage === index ? 0 : -1} aria-selected={selectedPage === index} aria-controls="preset-map" onKeyDown={event => selectPageFromKeyboard(event, index)} onClick={() => onPageSelect(index)}>
            Page {index + 1}
          </button>
        ))}
      </div>
      <div id="preset-map" className="preset-map" role="tabpanel" aria-label={`Page ${selectedPage + 1} presets`}>
        {page.presets.map((preset, index) => {
          const letter = String.fromCharCode(65 + index);
          const selected = selectedPreset === index;
          const label = preset.position1.label || "EMPTY";
          return (
            <button
              key={preset.id}
              type="button"
              className={`preset-card ${selected ? "is-selected" : ""}`}
              data-switch={letter}
              aria-current={selected ? "true" : undefined}
              aria-label={`Preset ${letter} / switch ${letter}: ${label}`}
              style={{ "--preset-color": rgb565ToHex(preset.position1.accentRgb565) } as CSSProperties}
              onClick={() => onPresetSelect(index)}
            >
              <span className="preset-card-top">
                <span className="preset-letter" aria-hidden="true">{letter}</span>
                <span className="preset-context">{selected ? "Editing" : `Preset ${letter}`}</span>
              </span>
              <span className="preset-card-body">
                <span className="preset-label">{label}</span>
                <span className="preset-position">Position {preset.toggleOn ? "toggle" : "1"}</span>
              </span>
              <span className="preset-meta"><span>{preset.slots.length} message{preset.slots.length === 1 ? "" : "s"}</span><span aria-hidden="true">→</span></span>
            </button>
          );
        })}
      </div>
    </section>
  );
}
