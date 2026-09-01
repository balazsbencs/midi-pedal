import type { ExpressionAssignment } from "@midi-pedal/protocol";

export function ExpressionSummary({ expression }: { expression: ExpressionAssignment }) {
  return (
    <section className="expression-summary" aria-labelledby="expression-summary-title">
      <div><p className="eyebrow">BANK EXPRESSION</p><h2 id="expression-summary-title">{expression.enabled ? expression.label : "Expression disabled"}</h2></div>
      <p className="summary-detail">{expression.enabled ? `CC ${expression.controller} · channel ${expression.channel} · ${expression.destination}` : "Enable it in the inspector when a pedal is connected."}</p>
    </section>
  );
}

