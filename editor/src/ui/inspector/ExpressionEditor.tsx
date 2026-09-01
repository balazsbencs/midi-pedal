import type { ExpressionAssignment } from "@midi-pedal/protocol";

import type { EditorAction } from "../../domain/editor_actions";

export function ExpressionEditor({ expression, dispatch }: { expression: ExpressionAssignment; dispatch: (action: EditorAction) => void }) {
  const patch = (value: Partial<ExpressionAssignment>) => dispatch({ type: "expression.changed", patch: value });
  return (
    <section className="expression-editor" aria-labelledby="expression-editor-title">
      <div className="inspector-subheading"><div><p className="eyebrow">INPUT</p><h3 id="expression-editor-title">Expression pedal</h3></div><label className="inline-check"><input type="checkbox" checked={expression.enabled} onChange={event => patch({ enabled: event.target.checked })} /> Enabled</label></div>
      <div className="field-grid">
        <label>Label<input maxLength={12} value={expression.label} onChange={event => patch({ label: event.target.value })} /></label>
        <label>Destination<select value={expression.destination} onChange={event => patch({ destination: event.target.value as "TRS" | "USB" | "BOTH" })}><option>TRS</option><option>USB</option><option>BOTH</option></select></label>
        <label>Channel<input type="number" min={1} max={16} value={expression.channel} onChange={event => patch({ channel: Number(event.target.value) })} /></label>
        <label>CC number<input type="number" min={0} max={127} value={expression.controller} onChange={event => patch({ controller: Number(event.target.value) })} /></label>
        <label>Minimum<input type="number" min={0} max={127} value={expression.minimum} onChange={event => patch({ minimum: Number(event.target.value) })} /></label>
        <label>Maximum<input type="number" min={0} max={127} value={expression.maximum} onChange={event => patch({ maximum: Number(event.target.value) })} /></label>
      </div>
      <label className="inline-check"><input type="checkbox" checked={expression.inverted} onChange={event => patch({ inverted: event.target.checked })} /> Invert heel/toe direction</label>
    </section>
  );
}

