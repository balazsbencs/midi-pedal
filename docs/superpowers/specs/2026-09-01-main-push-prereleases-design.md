# Main-Push Prerelease Automation Design

## Problem

The repository currently has a tag-triggered release workflow, but active
development happens on `main`. Contributors need a repeatable firmware download
from GitHub Releases after each `main` push, with release notes derived from
Conventional Commits and an explicit flashing path for builders.

The repository is still hardware-unqualified. Automatic `main` releases must
therefore be useful for software and firmware iteration without being confused
with a qualified production release.

## Goals

- Run the release packaging flow for every push to `main`.
- Publish one uniquely versioned GitHub prerelease for every processed push.
- Derive the semantic-version base bump and release notes from Conventional
  Commit messages.
- Make `firmware.uf2` an obvious downloadable release asset, while retaining
  the complete deterministic bundle and checksums.
- Keep the existing qualified `v*` tag release path separate from development
  prereleases.
- Document downloading, checksum verification, BOOTSEL flashing, and the
  hardware-qualification status of each automatic release.

## Non-goals

- Automatically declaring the unqualified carrier hardware stage-ready.
- Replacing the real HIL/safety gate for stable tagged releases.
- Adding a backend, account, wireless update mechanism, or device-side updater.
- Publishing a stable `latest` release from an arbitrary `main` push.

## Decisions

### Trigger and serialization

Add a dedicated GitHub Actions workflow for `push` events on `main` without a
path filter, so documentation, firmware, editor, and tooling commits all go
through the same release path. Use a non-cancelling concurrency group for the
main release lane; queued pushes are processed in order instead of allowing
two jobs to publish the same development channel concurrently.

The workflow checks out recursive submodules with full history, grants only
`contents: write`, bootstraps pnpm before enabling the setup-node pnpm cache,
and installs the pinned ARM compiler/C++ runtime packages with a noninteractive
apt command.

### Version and Conventional Commits

Use semantic-release-compatible Conventional Commit analysis. The first
development release starts from the repository's `0.1.0` baseline. Commit
severity selects the next semantic base: breaking changes are major, `feat`
commits are minor, and every other processed commit (including `fix`, `perf`,
`docs`, `test`, `refactor`, `chore`, and `ci`) is at least a patch. This
fallback is intentional: every push must produce a release even when a push
contains documentation or maintenance work.

Development tags use the `vX.Y.Z-dev.N` form, where `N` is monotonically
advanced from the existing development tags. The GitHub Release is marked
`prerelease: true` and is never treated as the stable `latest` release.

### Changelog and release notes

Generate grouped release notes with the Conventional Commit analyzer and
release-notes generator. Generate a release-specific `CHANGELOG.md` in the
workflow workspace and include it in the release bundle and release assets;
the GitHub Release body is the same generated changelog. The workflow does not
push a bot changelog commit back to `main`, avoiding release-loop and
non-fast-forward races when multiple developers push concurrently.

The checked-in `CHANGELOG.md` remains the human/project overview; generated
release notes are the authoritative history for each published development
artifact.

### Build and package gates

Before publishing, the workflow runs the same reproducible software checks as
the existing release path: frozen pnpm install, protocol/editor checks,
native C++ tests, Pico 2 release configure/build, factory-image reproducibility,
simulated HIL report generation, editor static build, and deterministic release
packaging. Simulated HIL is recorded in the manifest but is not treated as a
qualification pass for a development prerelease.

The existing tag workflow keeps its qualified HIL requirement and remains the
path for stable releases. Its package dependency setup is kept aligned with
the main workflow.

### Release assets

Publish every file emitted by `pnpm release:package`, including:

- `firmware.uf2` for BOOTSEL drag-and-drop flashing;
- `firmware.elf`, `firmware.map`, and `firmware.hex` for diagnostics/probes;
- `editor-dist.zip`;
- configuration schema, factory-empty images, protocol documentation,
  fabrication/mechanical source files, the HIL report, generated changelog,
  license notices, `manifest.json`, and `SHA256SUMS`.

The release notes call out `firmware.uf2` first and identify the commit,
development version, simulated-HIL status, and hardware-qualification warning.

### Documentation

Extend `docs/flashing.md` with a release-download section that points to the
repository Releases page, explains the `firmware.uf2` asset, verifies
`SHA256SUMS`, and warns that `main` prereleases are development firmware.
Retain the existing BOOTSEL, picotool, SWD/GDB, post-flash checks, and recovery
instructions. Add a direct Releases link and the same qualification warning to
the README. Update `docs/releasing.md` to describe the automatic prerelease
lane and the separate stable tag lane.

## Data flow

```text
push main
  -> serialized CI/release workflow
  -> frozen checks + native tests + Pico 2 build + editor build
  -> simulated HIL report (recorded, not a qualification pass)
  -> Conventional Commit version + generated changelog
  -> deterministic package + checksums
  -> GitHub prerelease with firmware.uf2 and complete bundle
```

## Failure handling

- Any build, test, packaging, checksum, or artifact-generation failure stops
  before release creation.
- A missing or malformed simulated HIL report stops packaging; a valid report
  with hardware-dependent failures is published only as a clearly marked WIP
  prerelease.
- GitHub release creation uses the workflow's scoped `GITHUB_TOKEN`; no token
  or credentials are embedded in artifacts.
- The stable tag workflow remains the recovery path for a reviewed,
  hardware-qualified artifact.

## Verification

- Parse every workflow as YAML and assert pnpm setup precedes setup-node caching.
- Run the repository's documented host checks and Pico 2 release build.
- Run the release packager twice with the same `SOURCE_DATE_EPOCH` and compare
  checksums.
- Verify generated changelog grouping against representative `feat`, `fix`,
  `docs`, and breaking Conventional Commits.
- Inspect a completed GitHub prerelease for `firmware.uf2`, `SHA256SUMS`,
  `manifest.json`, generated changelog, and the flashing link.
