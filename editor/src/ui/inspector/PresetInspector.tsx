import type { EditorAction } from "../../domain/editor_actions";
import { selectedBank, selectedPreset } from "../../domain/selectors";
import type { EditorState } from "../../domain/editor_state";
import { ExpressionEditor } from "./ExpressionEditor";
import { MessageList } from "./MessageList";
import { PositionEditor } from "./PositionEditor";

export function PresetInspector({ state, dispatch }: { state: EditorState; dispatch: (action: EditorAction) => void }) {
  const bank = selectedBank(state);
  const preset = selectedPreset(state);
  const letter = String.fromCharCode(65 + state.selection.preset);
  return (
    <aside className="inspector-pane" aria-label="Inspector">
      <div className="section-heading compact"><div><p className="eyebrow">INSPECTOR</p><h2>Preset {letter}</h2></div><span className="id-caption">#{preset.id.toString(16).toUpperCase()}</span></div>
      <label htmlFor="bank-name">Bank name <span className="field-hint">20 ASCII max</span></label>
      <input id="bank-name" maxLength={20} value={bank.name} onChange={event => dispatch({ type: "bank.nameChanged", value: event.target.value })} />
      <PositionEditor preset={preset} position={state.selection.position} dispatch={dispatch} />
      <label htmlFor="toggle-trigger">Toggle after
        <select id="toggle-trigger" value={preset.toggleOn ?? "NONE"} onChange={event => dispatch({ type: "preset.toggleChanged", value: event.target.value === "NONE" ? null : event.target.value as "PRESS" | "RELEASE" | "LONG_PRESS" | "DOUBLE_TAP" })}>
          <option value="NONE">Do not toggle</option><option value="PRESS">Press</option><option value="RELEASE">Release</option><option value="LONG_PRESS">Long press</option><option value="DOUBLE_TAP">Double tap</option>
        </select>
      </label>
      <MessageList slots={preset.slots} dispatch={dispatch} />
      <ExpressionEditor expression={bank.expression} dispatch={dispatch} />
    </aside>
  );
}

