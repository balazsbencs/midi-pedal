import { execFile } from "node:child_process";
import { readFile, writeFile } from "node:fs/promises";
import { promisify } from "node:util";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { parseArgs } from "node:util";

const execFileAsync = promisify(execFile);
const toolRoot = resolve(dirname(fileURLToPath(import.meta.url)), "../..");

const sectionNames = new Map([
  ["feat", "Features"],
  ["fix", "Bug Fixes"],
  ["perf", "Performance"],
  ["refactor", "Refactoring"],
  ["docs", "Documentation"],
  ["test", "Tests"],
  ["build", "Build System"],
  ["ci", "CI"],
  ["chore", "Chores"],
  ["revert", "Reverts"]
]);

function parseVersion(value) {
  const match = /^(\d+)\.(\d+)\.(\d+)$/.exec(value);
  if (!match) throw new Error(`expected a stable SemVer, received ${value}`);
  return { major: Number(match[1]), minor: Number(match[2]), patch: Number(match[3]) };
}

function versionString(version) {
  return `${version.major}.${version.minor}.${version.patch}`;
}

function isBreaking(subject, body) {
  return /(^|\n)BREAKING(?: CHANGE|-CHANGE)\s*:/m.test(body) ||
    /^[a-z][\w-]*(?:\([^)]*\))?!:\s+/.test(subject);
}

/** @param {string} subject @param {string} body @param {string} hash */
export function parseConventionalCommit(subject, body = "", hash = "") {
  const match = /^([a-z][\w-]*)(?:\(([^)]+)\))?(!)?:\s+(.+)$/.exec(subject.trim());
  if (!match) return null;
  return {
    hash,
    type: match[1],
    scope: match[2] ?? "",
    description: match[4].trim(),
    breaking: Boolean(match[3]) || isBreaking(subject, body)
  };
}

/** @param {string} currentVersion @param {{type: string, breaking: boolean}[]} commits */
export function calculateNextVersion(currentVersion, commits) {
  const version = parseVersion(currentVersion);
  if (commits.some(commit => commit.breaking)) {
    version.major += 1;
    version.minor = 0;
    version.patch = 0;
  } else if (commits.some(commit => commit.type === "feat")) {
    version.minor += 1;
    version.patch = 0;
  } else {
    version.patch += 1;
  }
  return versionString(version);
}

function sectionFor(commit) {
  return sectionNames.get(commit.type) ?? "Other Changes";
}

function formatCommit(commit) {
  const scope = commit.scope ? `**${commit.scope}:** ` : "";
  const breaking = commit.breaking ? " **(breaking)**" : "";
  const hash = commit.hash ? ` ([${commit.hash.slice(0, 7)}])` : "";
  return `- ${scope}${commit.description}${breaking}${hash}`;
}

/** @param {string} version @param {{type: string, scope: string, description: string, breaking: boolean, hash: string}[]} commits @param {string|null} previousTag */
export function renderChangelog(version, commits, previousTag) {
  const groups = new Map();
  for (const commit of commits) {
    const section = sectionFor(commit);
    if (!groups.has(section)) groups.set(section, []);
    groups.get(section).push(commit);
  }
  const preferredOrder = ["Breaking Changes", ...sectionNames.values(), "Other Changes"];
  const lines = [`# v${version}`, "", previousTag ? `Changes since ${previousTag}:` : "Changes included in the initial automated release:", ""];
  const breaking = commits.filter(commit => commit.breaking);
  if (breaking.length) {
    lines.push("## Breaking Changes", "", ...breaking.map(formatCommit), "");
  }
  for (const section of preferredOrder) {
    const items = groups.get(section);
    if (!items?.length || section === "Breaking Changes") continue;
    lines.push(`## ${section}`, "", ...items.map(formatCommit), "");
  }
  if (!commits.length) lines.push("No commits found since the previous release.", "");
  return `${lines.join("\n").trim()}\n`;
}

async function git(cwd, args) {
  const { stdout } = await execFileAsync("git", args, { cwd, maxBuffer: 10 * 1024 * 1024 });
  return stdout;
}

function parseGitLog(output) {
  return output.split("\x1e").map(record => record.replace(/^\n+/, "")).filter(record => record.trim()).map(record => {
    const [hash, subject, body = ""] = record.split("\x1f");
    return { hash, subject, body, parsed: parseConventionalCommit(subject ?? "", body, hash) };
  });
}

function normalizeCommits(log) {
  return log.map(item => item.parsed ?? {
    hash: item.hash,
    type: "other",
    scope: "",
    description: item.subject.trim(),
    breaking: false
  });
}

function semverTags(tags) {
  return tags
    .map(tag => tag.trim())
    .filter(tag => /^v\d+\.\d+\.\d+$/.test(tag))
    .map(tag => ({ tag, version: tag.slice(1), parsed: parseVersion(tag.slice(1)) }));
}

function compareVersions(a, b) {
  return a.major - b.major || a.minor - b.minor || a.patch - b.patch;
}

/**
 * Compute the release that should be made for the current HEAD. A stable tag
 * already pointing at HEAD makes the result idempotent, which is important
 * when a workflow run is manually re-run.
 */
export async function getReleaseMetadata(cwd = toolRoot) {
  const packageJson = JSON.parse(await readFile(resolve(cwd, "package.json"), "utf8"));
  const exactTags = semverTags((await git(cwd, ["tag", "--points-at", "HEAD"])).split("\n"));
  const mergedTags = semverTags((await git(cwd, ["tag", "--merged", "HEAD", "--list", "v[0-9]*"])).split("\n"));
  if (exactTags.length) {
    const current = exactTags.sort((a, b) => compareVersions(b.parsed, a.parsed))[0];
    const previous = mergedTags
      .filter(item => item.tag !== current.tag && compareVersions(item.parsed, current.parsed) < 0)
      .sort((a, b) => compareVersions(b.parsed, a.parsed))[0] ?? null;
    const log = parseGitLog(await git(cwd, previous ? ["log", `${previous.tag}..HEAD`, "--format=%H%x1f%s%x1f%b%x1e"] : ["log", "HEAD", "--format=%H%x1f%s%x1f%b%x1e"]));
    const commits = normalizeCommits(log);
    return {
      version: current.version,
      tag: current.tag,
      previousTag: previous?.tag ?? null,
      commits,
      releaseNotes: renderChangelog(current.version, commits, previous?.tag ?? null),
      alreadyReleased: true
    };
  }

  const previous = mergedTags.sort((a, b) => compareVersions(b.parsed, a.parsed))[0] ?? null;
  const log = parseGitLog(await git(cwd, previous ? ["log", `${previous.tag}..HEAD`, "--format=%H%x1f%s%x1f%b%x1e"] : ["log", "HEAD", "--format=%H%x1f%s%x1f%b%x1e"]));
  const commits = normalizeCommits(log);
  const baseVersion = previous?.version ?? packageJson.version;
  const version = previous ? calculateNextVersion(baseVersion, commits.length ? commits : [{ type: "chore", breaking: false }]) : baseVersion;
  return {
    version,
    tag: `v${version}`,
    previousTag: previous?.tag ?? null,
    commits,
    releaseNotes: renderChangelog(version, commits, previous?.tag ?? null),
    alreadyReleased: false
  };
}

if (process.argv[1] && resolve(process.argv[1]) === resolve(fileURLToPath(import.meta.url))) {
  const args = process.argv.slice(2).filter(argument => argument !== "--");
  const { values } = parseArgs({ args, options: { output: { type: "string" }, notes: { type: "string" } } });
  const metadata = await getReleaseMetadata();
  if (values.output) await writeFile(resolve(process.cwd(), values.output), `${JSON.stringify(metadata, null, 2)}\n`);
  if (values.notes) await writeFile(resolve(process.cwd(), values.notes), metadata.releaseNotes);
  console.log(JSON.stringify(metadata, null, 2));
}
