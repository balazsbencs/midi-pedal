import { parseArgs } from "node:util";
import { resolve } from "node:path";

import { writeReport } from "./report";
import { fakeRig, runSuite } from "./runner";
import { releaseSuite } from "./cases";

const args = process.argv.slice(2).filter(argument => argument !== "--");
const { values } = parseArgs({ args, options: { report: { type: "string" }, rig: { type: "string", default: "simulated" } } });
const report = await runSuite(fakeRig(), releaseSuite);
if (values.report) await writeReport(resolve(process.cwd(), "../..", values.report), report);
console.log(JSON.stringify(report, null, 2));
if (report.status === "FAIL") process.exitCode = 1;
