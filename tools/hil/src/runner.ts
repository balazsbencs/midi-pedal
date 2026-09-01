import type { CaseResult, HilCase, HilReportV1, Rig } from "./types";

export interface FakeRigOptions { midi?: number[]; measurements?: Record<string, unknown>; confirmations?: Record<string, boolean> }

export function fakeRig(options: FakeRigOptions = {}): Rig {
  return {
    async captureMidi() { return [...(options.midi ?? [])]; },
    async measure(name: string) { return options.measurements?.[name]; },
    async confirm(prompt: string) { return options.confirmations?.[prompt] ?? false; }
  };
}

export async function runCase(rig: Rig, testCase: HilCase): Promise<CaseResult> {
  const started = Date.now();
  const startedAt = new Date(started).toISOString();
  try {
    const observation = await testCase.run(rig);
    if (observation.status === "SKIP" && !observation.skipReason && !observation.notes.some(note => /^skip( reason)?:/i.test(note))) throw new Error(`case ${testCase.id} must provide a skip reason`);
    return { id: testCase.id, startedAt, durationMs: Date.now() - started, ...observation };
  } catch (error) {
    if (error instanceof Error && /must provide a skip reason/.test(error.message)) throw error;
    return { id: testCase.id, startedAt, durationMs: Date.now() - started, status: "FAIL", evidence: [], notes: [error instanceof Error ? error.message : String(error)] };
  }
}

export async function runSuite(rig: Rig, tests: HilCase | HilCase[]): Promise<HilReportV1> {
  const list = Array.isArray(tests) ? tests : [tests];
  const cases = await Promise.all(list.map(testCase => runCase(rig, testCase)));
  const blockedSkip = cases.some((item, index) => item.status === "SKIP" && list[index]!.releaseBlocking);
  return {
    schemaVersion: 1, commit: process.env.GIT_COMMIT ?? "working-tree", firmware: process.env.FIRMWARE_VERSION ?? "0.1.0",
    protocol: 1, configSchema: 1, environment: { node: process.version, rig: process.env.HIL_RIG ?? "simulated" },
    cases, status: cases.some(item => item.status === "FAIL") || blockedSkip ? "FAIL" : "PASS"
  };
}
