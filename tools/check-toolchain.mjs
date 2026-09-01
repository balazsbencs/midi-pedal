import { execFileSync } from "node:child_process";

const checks = [
  ["node", ["--version"]],
  ["pnpm", ["--version"]],
  ["cmake", ["--version"]],
  ["ninja", ["--version"]],
];

for (const [binary, args] of checks) {
  execFileSync(binary, args, { stdio: "inherit" });
}
