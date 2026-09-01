import { mkdir, writeFile } from "node:fs/promises";
import { dirname } from "node:path";

import type { HilReportV1 } from "./types";

export async function writeReport(path: string, report: HilReportV1): Promise<void> {
  await mkdir(dirname(path), { recursive: true });
  await writeFile(path, `${JSON.stringify(report, null, 2)}\n`, "utf8");
}
