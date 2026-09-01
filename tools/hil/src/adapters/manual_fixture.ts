export interface ManualMeasurement { name: string; value: string; unit: string; operator: string; timestamp: string; evidence: string[] }

export function safePrompt(name: string, prompt: string): string {
  return [
    `MANUAL CHECK: ${name}`,
    prompt,
    "Use the specified meter mode and probe points.",
    "Do not change wiring while energized; power off before moving probes.",
    "Stop immediately on unexpected current, heat, polarity, or continuity."
  ].join("\n");
}

