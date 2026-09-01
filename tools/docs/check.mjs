import { readFile, readdir, stat } from "node:fs/promises";
import { dirname, extname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "../..");
const markdownRoots = [root, join(root, "docs")];
const markdownFiles = new Set();

async function collect(directory) {
  for (const entry of await readdir(directory, { withFileTypes: true })) {
    if (entry.name === "node_modules" || entry.name === ".git" || entry.name === "dist" || entry.name === "build" || entry.name === "third_party") continue;
    const path = join(directory, entry.name);
    if (entry.isDirectory()) await collect(path);
    else if (extname(entry.name).toLowerCase() === ".md") markdownFiles.add(path);
  }
}
for (const directory of markdownRoots) await collect(directory);

const errors = [];
for (const file of markdownFiles) {
  const text = await readFile(file, "utf8");
  for (const match of text.matchAll(/\[[^\]]+\]\(([^)]+)\)/g)) {
    const target = match[1].split("#", 1)[0];
    if (!target || /^(https?:|mailto:)/i.test(target)) continue;
    const resolved = resolve(dirname(file), target);
    try { await stat(resolved); } catch { errors.push(`${file}: missing link target ${target}`); }
  }
}

const commands = JSON.parse(await readFile(join(root, "tools/docs/verified-commands.json"), "utf8"));
for (const item of commands.commands) {
  const text = await readFile(join(root, item.document), "utf8");
  if (!text.includes(item.command)) errors.push(`${item.document}: missing registered command ${item.command}`);
}
const readme = await readFile(join(root, "README.md"), "utf8");
for (const required of ["docs/building.md", "docs/flashing.md", "docs/editor-hosting.md", "docs/hardware-assembly.md", "docs/protocol.md", "docs/troubleshooting.md", "CONTRIBUTING.md"]) {
  if (!readme.includes(`(${required})`)) errors.push(`README.md: required guide link missing ${required}`);
}
if (errors.length) { console.error(errors.join("\n")); process.exitCode = 1; }
else console.log(`documentation checks passed (${markdownFiles.size} Markdown files)`);
