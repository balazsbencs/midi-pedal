import type { CSSProperties } from "react";
import type { EditorAction } from "../../domain/editor_actions";
import { selectedBank, selectedPreset } from "../../domain/selectors";
import type { EditorState } from "../../domain/editor_state";
import { ExpressionEditor } from "./ExpressionEditor";
import { MessageList } from "./MessageList";
import { PositionEditor } from "./PositionEditor";
import { rgb565ToHex } from "../presets/color";

export function PresetInspector({ state, dispatch }: { state: EditorState; dispatch: (action: EditorAction) => void }) {
  const bank = selectedBank(state);
  const preset = selectedPreset(state);
  const letter = String.fromCharCode(65 + state.selection.preset);
  return (
    <aside className="inspector-pane" aria-label="Inspector" style={{ "--preset-color": rgb565ToHex(preset.position1.accentRgb565) } as CSSProperties}>
      <div className="section-heading compact inspector-heading">
        <div className="inspector-title"><span className="inspector-title-mark" aria-hidden="true">{letter}</span><div><p className="eyebrow">INSPECTOR</p><h2>Preset {letter}</h2></div></div>
        <span className="id-caption">#{preset.id.toString(16).toUpperCase()}</span>
      </div>
      <section className="preset-settings" aria-label="Preset settings">
        <PositionEditor preset={preset} position={state.selection.position} dispatch={dispatch} />
        <label className="compact-select-row" htmlFor="toggle-trigger"><span>Toggle after</span>
          <select id="toggle-trigger" value={preset.toggleOn ?? "NONE"} onChange={event => dispatch({ type: "preset.toggleChanged", value: event.target.value === "NONE" ? null : event.target.value as "PRESS" | "RELEASE" | "LONG_PRESS" | "DOUBLE_TAP" })}>
            <option value="NONE">Do not toggle</option><option value="PRESS">Press</option><option value="RELEASE">Release</option><option value="LONG_PRESS">Long press</option><option value="DOUBLE_TAP">Double tap</option>
          </select>
        </label>
      </section>
      <ExpressionEditor expression={bank.expression} dispatch={dispatch} />
      <MessageList slots={preset.slots} dispatch={dispatch} />
    </aside>
  );
}
