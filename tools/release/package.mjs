import { createHash } from "node:crypto";
import { cp, mkdir, readFile, readdir, stat, writeFile } from "node:fs/promises";
import { dirname, join, relative, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { parseArgs } from "node:util";

const toolRoot = resolve(dirname(fileURLToPath(import.meta.url)), "../..");

const crcTable = (() => {
  const table = [];
  for (let value = 0; value < 256; value += 1) {
    let crc = value;
    for (let bit = 0; bit < 8; bit += 1) crc = (crc & 1) ? (crc >>> 1) ^ 0xedb88320 : crc >>> 1;
    table.push(crc >>> 0);
  }
  return table;
})();

function crc32(bytes) {
  let crc = 0xffffffff;
  for (const byte of bytes) crc = (crc >>> 8) ^ crcTable[(crc ^ byte) & 0xff];
  return (crc ^ 0xffffffff) >>> 0;
}

function u16(value) { const output = Buffer.alloc(2); output.writeUInt16LE(value, 0); return output; }
function u32(value) { const output = Buffer.alloc(4); output.writeUInt32LE(value >>> 0, 0); return output; }

async function filesUnder(directory) {
  const output = [];
  async function visit(current) {
    for (const entry of (await readdir(current, { withFileTypes: true })).sort((a, b) => a.name.localeCompare(b.name))) {
      const path = join(current, entry.name);
      if (entry.isDirectory()) await visit(path);
      else output.push(path);
    }
  }
  await visit(directory);
  return output;
}

async function deterministicZip(directory, outputPath, epoch) {
  const files = await filesUnder(directory);
  const local = [];
  const central = [];
  let offset = 0;
  const date = new Date(epoch * 1000);
  const dosTime = (date.getUTCHours() << 11) | (date.getUTCMinutes() << 5) | Math.floor(date.getUTCSeconds() / 2);
  const dosYear = Math.max(1980, date.getUTCFullYear());
  const dosDate = ((dosYear - 1980) << 9) | ((date.getUTCMonth() + 1) << 5) | Math.max(1, date.getUTCDate());
  for (const path of files) {
    const data = await readFile(path);
    const name = Buffer.from(relative(directory, path).replaceAll("\\", "/"));
    const checksum = crc32(data);
    const header = Buffer.concat([Buffer.from("PK\x03\x04", "binary"), u16(20), u16(0), u16(0), u16(dosTime), u16(dosDate), u32(checksum), u32(data.length), u32(data.length), u16(name.length), u16(0), name, data]);
    local.push(header);
    central.push(Buffer.concat([Buffer.from("PK\x01\x02", "binary"), u16(20), u16(20), u16(0), u16(0), u16(dosTime), u16(dosDate), u32(checksum), u32(data.length), u32(data.length), u16(name.length), u16(0), u16(0), u16(0), u16(0), u32(0), u32(offset), name]));
    offset += header.length;
  }
  const centralBytes = Buffer.concat(central);
  const end = Buffer.concat([Buffer.from("PK\x05\x06", "binary"), u16(0), u16(0), u16(files.length), u16(files.length), u32(centralBytes.length), u32(offset), u16(0)]);
  await writeFile(outputPath, Buffer.concat([...local, centralBytes, end]));
}

async function requiredPath(root, relativePath, label) {
  const path = resolve(root, relativePath);
  try { await stat(path); } catch { throw new Error(`missing required artifact ${label}`); }
  return path;
}

/** @param {{root?: string, outDir?: string, version?: string, sourceDateEpoch?: number, changelogPath?: string}} [options] */
export async function packageRelease({ root = toolRoot, outDir, version = "0.1.0", sourceDateEpoch = Number(process.env.SOURCE_DATE_EPOCH ?? 0), changelogPath = "CHANGELOG.md" } = {}) {
  if (!outDir) throw new Error("outDir is required");
  root = resolve(root); outDir = resolve(root, outDir);
  const editorDir = await requiredPath(root, "editor/dist", "editor-dist.zip");
  const artifactPaths = [
    ["firmware.uf2", "build/pico2-release/firmware/midi_pedal.uf2", "MIT"],
    ["firmware.elf", "build/pico2-release/firmware/midi_pedal.elf", "MIT"],
    ["firmware.map", "build/pico2-release/firmware/midi_pedal.elf.map", "MIT"],
    ["CHANGELOG.md", changelogPath, "CC-BY-SA-4.0"],
    ["config-schema.json", "packages/protocol/schema/config-v1.schema.json", "MIT"],
    ["factory-empty.json", "firmware/defaults/factory-empty.json", "MIT"],
    ["factory-empty.bin", "firmware/defaults/factory-empty.bin", "MIT"],
    ["protocol.md", "docs/protocol.md", "CC-BY-SA-4.0"],
    ["flashing.md", "docs/flashing.md", "CC-BY-SA-4.0"],
    ["fabrication-bom.csv", "hardware/fabrication/bom.csv", "CERN-OHL-S-2.0"],
    ["fabrication-positions.csv", "hardware/fabrication/positions.csv", "CERN-OHL-S-2.0"],
    ["front-panel.dxf", "hardware/mechanical/front-panel.dxf", "CERN-OHL-S-2.0"],
    ["rear-panel.dxf", "hardware/mechanical/rear-panel.dxf", "CERN-OHL-S-2.0"],
    ["hil-report.json", "build/hil-simulated.json", "CC-BY-SA-4.0"],
    ["LICENSES/README.md", "LICENSES/README.md", "CC-BY-SA-4.0"]
  ];
  const resolvedArtifacts = [];
  for (const [name, source, license] of artifactPaths) resolvedArtifacts.push({ name, source: await requiredPath(root, source, name), license });
  await mkdir(outDir, { recursive: true });
  await deterministicZip(editorDir, join(outDir, "editor-dist.zip"), sourceDateEpoch);
  await Promise.all(resolvedArtifacts.map(async artifact => {
    const destination = join(outDir, artifact.name);
    await mkdir(dirname(destination), { recursive: true });
    await cp(artifact.source, destination);
  }));
  const publicFiles = [join(outDir, "editor-dist.zip"), ...resolvedArtifacts.map(item => join(outDir, item.name))];
  const artifacts = [];
  for (const path of publicFiles) {
    const bytes = await readFile(path); const name = relative(outDir, path).replaceAll("\\", "/");
    artifacts.push({ name, size: bytes.length, sha256: createHash("sha256").update(bytes).digest("hex"), license: name === "editor-dist.zip" ? "MIT" : resolvedArtifacts.find(item => item.name === name)?.license ?? "MIT" });
  }
  artifacts.sort((a, b) => a.name.localeCompare(b.name));
  const hil = JSON.parse(await readFile(join(outDir, "hil-report.json"), "utf8"));
  const manifest = {
    schemaVersion: 1, releaseVersion: version, gitCommit: process.env.GIT_COMMIT ?? "working-tree",
    buildTimeUtc: new Date(sourceDateEpoch * 1000).toISOString(),
    compatibility: { firmware: "0.1.x", editor: "0.1.x", configSchema: 1, imageFormat: 1, usbProtocol: 1, minimumChromium: 89, hardwareRevision: "unqualified-wip" },
    artifacts, hil: { status: hil.status, report: "hil-report.json" }
  };
  await writeFile(join(outDir, "manifest.json"), `${JSON.stringify(manifest, null, 2)}\n`);
  const checksums = artifacts.map(item => `${item.sha256}  ${item.name}`).join("\n") + "\n";
  await writeFile(join(outDir, "SHA256SUMS"), checksums);
  return manifest;
}

if (process.argv[1] && resolve(process.argv[1]) === resolve(fileURLToPath(import.meta.url))) {
  const args = process.argv.slice(2).filter(argument => argument !== "--");
  const { values } = parseArgs({ args, options: { version: { type: "string", default: "0.1.0" }, out: { type: "string" }, changelog: { type: "string", default: "CHANGELOG.md" } } });
  const manifest = await packageRelease({ outDir: values.out, version: values.version, changelogPath: values.changelog });
  console.log(JSON.stringify(manifest, null, 2));
}
