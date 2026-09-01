import type { Message, MessageSlot } from "@midi-pedal/protocol";

import type { EditorAction } from "../../domain/editor_actions";
import { MessageEditor } from "./MessageEditor";

export function MessageList({ slots, dispatch }: { slots: MessageSlot[]; dispatch: (action: EditorAction) => void }) {
  return (
    <section className="messages-section" aria-labelledby="messages-title">
      <div className="inspector-subheading"><div><p className="eyebrow">ACTIONS</p><h3 id="messages-title">Message order</h3></div><span className="count-badge">{slots.length}/8</span></div>
      <div className="message-list">
        {slots.map((slot, index) => <MessageEditor key={slot.id} slot={slot} index={index} count={slots.length} onChange={patch => dispatch({ type: "slot.updated", index, patch })} onRemove={() => dispatch({ type: "slot.removed", index })} onMove={direction => dispatch({ type: "slot.moved", from: index, to: index + direction })} />)}
        {slots.length === 0 && <p className="muted-copy">No actions yet. Add one below.</p>}
      </div>
      <div className="add-actions">
        {(["CC", "PC", "RELAY", "NAV"] as const).map(type => <button key={type} type="button" disabled={slots.length >= 8} onClick={() => dispatch({ type: "slot.added", messageType: type })}>+ {type}</button>)}
      </div>
    </section>
  );
}

