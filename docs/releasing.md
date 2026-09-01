# Releasing

Releases use SemVer. The first compatibility tuple is firmware/editor `0.1.x`,
configuration schema 1, binary image format 1, and USB protocol 1. A change to
field meaning, byte order, command lifecycle, or relay/power safety requires a
version bump and compatibility note before it can ship.

## Automatic GitHub releases

Every push to `main` runs `.github/workflows/release.yml`. The workflow uses
the conventional commit subject to calculate the next version from the latest
reachable `vX.Y.Z` tag:

```text
BREAKING CHANGE or feat!: major
feat: minor
fix, perf, refactor, docs, test, build, ci, chore: patch
```

The first release uses the version in the root `package.json`. Subsequent
releases are tagged automatically, so no manual tag or release-draft step is
needed. A rerun of a completed workflow is idempotent when the commit already
has a release tag.

Each release contains the BOOTSEL image `firmware.uf2`, debugger/programmer
images, the generated `CHANGELOG.md`, `SHA256SUMS`, the compatibility
`manifest.json`, and `flashing.md`. The `firmware.uf2` file is the normal
drag-and-drop image for the Pico 2; see [Flashing and recovery](flashing.md).

The simulated HIL report is included in the package and remains `FAIL` while
the repository's physical qualification evidence is incomplete. This keeps
the artifact traceable without presenting an unqualified hardware revision as
stage-ready.

## Local rehearsal

Build from a clean clone, set a stable UTC epoch, and package twice:

```bash
export SOURCE_DATE_EPOCH="$(git show -s --format=%ct HEAD)"
pnpm install --frozen-lockfile
pnpm check
cmake --preset pico2-release
cmake --build --preset pico2-release
pnpm --filter @midi-pedal/editor build
pnpm hil:release -- --rig simulated --report build/hil-simulated.json || true
pnpm release:package -- --version 0.1.0 --out build/release-a
pnpm release:package -- --version 0.1.0 --out build/release-b
cmp build/release-a/SHA256SUMS build/release-b/SHA256SUMS
```

The local packager includes the checked-in `CHANGELOG.md` by default. To
rehearse the exact generated release notes, first write notes with
`node tools/release/next.mjs --notes build/release-notes.md`, then pass
`--changelog build/release-notes.md` to `pnpm release:package`.

The simulated HIL report is expected to be `FAIL` while hardware evidence is
missing. It is useful for checking report shape, not for publication.

## Publication gate

Before tagging `v0.1.0`, require a reviewed schematic/PCB revision, generated
Gerbers/drills, signed first-article results, and a real HIL report with no
release-blocking skips. Inspect `manifest.json`, `SHA256SUMS`, firmware map
size, editor archive contents, licenses, and the compatibility tuple. Publish
only after a human reviews the artifact list and release notes.

## Rollback and corrections

Firmware can be rolled back through BOOTSEL while preserving the active
configuration image if the protocol/image tuple remains compatible. Keep a JSON
export before any reset. If a fabrication or safety defect is found, withdraw
the affected artifact, increment the hardware revision, regenerate all outputs,
and document the correction rather than silently replacing files.
