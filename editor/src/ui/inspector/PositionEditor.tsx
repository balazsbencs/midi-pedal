import type { Preset } from "@midi-pedal/protocol";

import type { EditorAction } from "../../domain/editor_actions";
import type { Position } from "../../domain/editor_state";
import { accentChoices, rgb565ToHex } from "../presets/color";

interface PositionEditorProps {
  preset: Preset;
  position: Position;
  dispatch: (action: EditorAction) => void;
}

export function PositionEditor({ preset, position, dispatch }: PositionEditorProps) {
  const view = position === 1 ? preset.position1 : preset.position2;
  return (
    <section className="position-editor" aria-labelledby="position-editor-title">
      <div className="inspector-subheading">
        <div><p className="eyebrow">PRESENTATION</p><h3 id="position-editor-title">Position {position}</h3></div>
        <div className="position-switch" role="group" aria-label="Toggle position">
          {[1, 2].map(item => <button key={item} type="button" className={position === item ? "is-selected" : ""} aria-pressed={position === item} onClick={() => dispatch({ type: "selection.positionChanged", position: item as Position })}>P{item}</button>)}
        </div>
      </div>
      <label htmlFor="position-label">Display label <span className="field-hint">12 ASCII max</span></label>
      <input id="position-label" maxLength={12} value={view.label} onChange={event => dispatch({ type: "preset.labelChanged", position, value: event.target.value })} />
      <fieldset className="accent-field">
        <legend>Accent color</legend>
        <div className="accent-swatches">
          {accentChoices.map(choice => {
            return <button key={choice.name} type="button" aria-label={`Set accent to ${choice.name}`} aria-pressed={view.accentRgb565 === choice.value} style={{ backgroundColor: rgb565ToHex(choice.value) }} onClick={() => dispatch({ type: "preset.accentChanged", position, value: choice.value })} />;
          })}
        </div>
      </fieldset>
      <label htmlFor="position-accent">RGB565 value</label>
      <input id="position-accent" type="number" min={0} max={65535} value={view.accentRgb565} onChange={event => dispatch({ type: "preset.accentChanged", position, value: Number(event.target.value) })} />
    </section>
  );
}
