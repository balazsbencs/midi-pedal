export type CaseStatus = "PASS" | "FAIL" | "SKIP";

export interface Rig {
  captureMidi(): Promise<number[]>;
  measure(name: string): Promise<unknown>;
  confirm(prompt: string): Promise<boolean>;
}

export interface HilCase {
  id: string;
  releaseBlocking?: boolean;
  run(rig: Rig): Promise<CaseObservation>;
}

export interface CaseObservation {
  status: CaseStatus;
  expected?: unknown;
  observed?: unknown;
  evidence: string[];
  notes: string[];
  skipReason?: string;
}

export interface CaseResult extends CaseObservation {
  id: string;
  startedAt: string;
  durationMs: number;
}

export interface HilReportV1 {
  schemaVersion: 1;
  commit: string;
  firmware: string;
  protocol: number;
  configSchema: number;
  environment: Record<string, string>;
  cases: CaseResult[];
  status: "PASS" | "FAIL";
}

