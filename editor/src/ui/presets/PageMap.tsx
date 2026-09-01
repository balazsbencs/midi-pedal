import type { Bank } from "@midi-pedal/protocol";

interface PageMapProps {
  bank: Bank;
  selectedPage: number;
  selectedPreset: number;
  onPageSelect: (index: number) => void;
  onPresetSelect: (index: number) => void;
}

export function PageMap({ bank, selectedPage, selectedPreset, onPageSelect, onPresetSelect }: PageMapProps) {
  const page = bank.pages[selectedPage]!;
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
          <button key={item.id} type="button" role="tab" aria-selected={selectedPage === index} aria-controls="preset-map" onClick={() => onPageSelect(index)}>
            Page {index + 1}
          </button>
        ))}
      </div>
      <div id="preset-map" className="preset-map" role="group" aria-label={`Page ${selectedPage + 1} presets`}>
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
              onClick={() => onPresetSelect(index)}
            >
              <span className="preset-letter" aria-hidden="true">{letter}</span>
              <span className="preset-label">{label}</span>
              <span className="preset-position">Position {preset.toggleOn ? "toggle" : "1"}</span>
            </button>
          );
        })}
      </div>
    </section>
  );
}

