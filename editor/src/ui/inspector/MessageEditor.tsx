import type { ChangeEvent } from "react";
import type { Message, MessageSlot, PositionFilter, Trigger } from "@midi-pedal/protocol";

import type { SlotPatch } from "../../domain/editor_actions";

interface MessageEditorProps {
  slot: MessageSlot;
  index: number;
  count: number;
  onChange: (patch: SlotPatch) => void;
  onRemove: () => void;
  onMove: (direction: -1 | 1) => void;
}

const triggers: Trigger[] = ["PRESS", "RELEASE", "LONG_PRESS", "DOUBLE_TAP"];
const positions: PositionFilter[] = ["BOTH", "POSITION_1", "POSITION_2"];

function numberValue(event: ChangeEvent<HTMLInputElement>): number { return Number(event.target.value); }

export function MessageEditor({ slot, index, count, onChange, onRemove, onMove }: MessageEditorProps) {
  const message = slot.message;
  const setMessage = (patch: Partial<Message>) => onChange({ message: patch as Record<string, unknown> });
  const changeType = (type: Message["type"]) => {
    if (type === "PC") setMessage({ type, channel: 1, program: 0, destination: "BOTH" });
    else if (type === "CC") setMessage({ type, channel: 1, controller: 1, value: 127, destination: "BOTH" });
    else if (type === "RELAY") setMessage({ type, contact: 1, operation: "TOGGLE" });
    else setMessage({ type, operation: "PAGE_UP" });
  };
  return (
    <fieldset className="message-card">
      <legend>Message {index + 1}</legend>
      <div className="message-toolbar">
        <label>Trigger
          <select value={slot.trigger} onChange={event => onChange({ trigger: event.target.value as Trigger })}>
            {triggers.map(value => <option key={value} value={value}>{value.replaceAll("_", " ")}</option>)}
          </select>
        </label>
        <label>Position
          <select value={slot.position} onChange={event => onChange({ position: event.target.value as PositionFilter })}>
            {positions.map(value => <option key={value} value={value}>{value.replace("POSITION_", "P")}</option>)}
          </select>
        </label>
        <div className="message-order" aria-label={`Reorder message ${index + 1}`}>
          <button type="button" aria-label={`Move message ${index + 1} up`} disabled={index === 0} onClick={() => onMove(-1)}>↑</button>
          <button type="button" aria-label={`Move message ${index + 1} down`} disabled={index === count - 1} onClick={() => onMove(1)}>↓</button>
          <button type="button" className="danger-link" onClick={onRemove}>Remove</button>
        </div>
      </div>
      <label>Type
        <select value={message.type} onChange={event => changeType(event.target.value as Message["type"])}>
          <option value="CC">Control Change</option><option value="PC">Program Change</option><option value="RELAY">Relay</option><option value="NAV">Navigation</option>
        </select>
      </label>
      {(message.type === "PC" || message.type === "CC") && <div className="field-grid">
        <label>Channel<input type="number" min={1} max={16} value={message.channel} onChange={event => setMessage({ channel: numberValue(event) })} /></label>
        {message.type === "PC" ? <label>Program<input type="number" min={0} max={127} value={message.program} onChange={event => setMessage({ program: numberValue(event) })} /></label> : <>
          <label>Controller<input type="number" min={0} max={127} value={message.controller} onChange={event => setMessage({ controller: numberValue(event) })} /></label>
          <label>Value<input type="number" min={0} max={127} value={message.value} onChange={event => setMessage({ value: numberValue(event) })} /></label>
        </>}
        <label>Destination<select value={message.destination} onChange={event => setMessage({ destination: event.target.value as "TRS" | "USB" | "BOTH" })}><option>TRS</option><option>USB</option><option>BOTH</option></select></label>
      </div>}
      {message.type === "RELAY" && <div className="field-grid">
        <label>Contact<select value={message.contact} onChange={event => setMessage({ contact: Number(event.target.value) as 1 | 2 })}><option value="1">Relay 1</option><option value="2">Relay 2</option></select></label>
        <label>Operation<select value={message.operation} onChange={event => setMessage({ operation: event.target.value as "OPEN" | "CLOSE" | "TOGGLE" })}><option>OPEN</option><option>CLOSE</option><option>TOGGLE</option></select></label>
      </div>}
      {message.type === "NAV" && <div className="field-grid">
          <label>Operation<select value={message.operation} onChange={event => {
          const operation = event.target.value as Extract<Message, { type: "NAV" }>["operation"];
          onChange({ message: operation.endsWith("_SET") ? { type: "NAV", operation, target: operation === "PAGE_SET" ? 1 : 1 } : { type: "NAV", operation } });
        }}><option>BANK_UP</option><option>BANK_DOWN</option><option>BANK_SET</option><option>PAGE_UP</option><option>PAGE_DOWN</option><option>PAGE_SET</option></select></label>
        {message.operation.endsWith("_SET") && <label>Target<input type="number" min={1} max={message.operation.startsWith("PAGE") ? 4 : 128} value={message.target ?? 1} onChange={event => setMessage({ target: numberValue(event) })} /></label>}
      </div>}
    </fieldset>
  );
}
