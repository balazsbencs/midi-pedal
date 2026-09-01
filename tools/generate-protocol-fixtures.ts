import { createHash } from "node:crypto";
import { readdir, readFile, writeFile } from "node:fs/promises";
import { join, basename } from "node:path";
import { encodeImage } from "../packages/protocol/src/binary.js";
import { validateConfig } from "../packages/protocol/src/validate.js";

async function main(): Promise<void> {
  const root = join(process.cwd(), "../..");
  const jsonDirectory = join(root, "protocol", "fixtures", "json");
  const binaryDirectory = join(root, "protocol", "fixtures", "bin");
  const manifestPath = join(root, "protocol", "fixtures", "manifest.json");

  const names = (await readdir(jsonDirectory)).filter(name => name.endsWith(".json") && !name.startsWith("invalid-")).sort();
  const fixtures: { name: string; json: string; binary: string; sha256: string }[] = [];
  for (const name of names) {
    const input = JSON.parse(await readFile(join(jsonDirectory, name), "utf8")) as unknown;
    const result = validateConfig(input);
    if (!result.ok) throw new Error(`${name} is invalid: ${JSON.stringify(result.errors)}`);
    const bytes = encodeImage(result.value.config, 1);
    const binaryName = `${basename(name, ".json")}.bin`;
    await writeFile(join(binaryDirectory, binaryName), bytes);
    fixtures.push({ name: basename(name, ".json"), json: `json/${name}`, binary: `bin/${binaryName}`, sha256: createHash("sha256").update(bytes).digest("hex") });
  }
  await writeFile(manifestPath, `${JSON.stringify({ schemaVersion: 1, fixtures }, null, 2)}\n`);
}

main().catch(error => { console.error(error); process.exitCode = 1; });
