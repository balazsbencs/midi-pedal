import { createHash } from "node:crypto";
import { readFile, writeFile } from "node:fs/promises";
import { resolve } from "node:path";

import { encodeImage } from "../packages/protocol/src/binary.js";
import { validateConfig } from "../packages/protocol/src/validate.js";

async function main(): Promise<void> {
  const root = resolve(process.cwd(), "../..");
  const sourcePath = resolve(root, "firmware/defaults/factory-empty.json");
  const outputPath = resolve(root, "firmware/defaults/factory-empty.bin");

  const input: unknown = JSON.parse(await readFile(sourcePath, "utf8"));
  const result = validateConfig(input);
  if (!result.ok) throw new Error(`factory-empty.json is invalid: ${JSON.stringify(result.errors)}`);

  const image = encodeImage(result.value.config, 0);
  await writeFile(outputPath, image);
  console.log(`wrote ${outputPath} (${image.length} bytes, sha256=${createHash("sha256").update(image).digest("hex")})`);
}

main().catch(error => { console.error(error); process.exitCode = 1; });
