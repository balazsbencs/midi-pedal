# Integration, Documentation, and Release Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove the complete pedal/editor system, make it reproducible for new builders, and package a traceable open-source release.

**Architecture:** A hardware-in-the-loop runner records machine-readable evidence while manual safety/mechanical checks remain explicit signed checklists. CI rebuilds firmware, editor, contract, and fabrication artifacts; the release workflow packages only outputs tied to a compatibility manifest and reviewed source revision.

**Tech Stack:** Node.js 24/pnpm 10, TypeScript, CMake/CTest, Playwright Chromium, KiCad 10.0.6 CLI, GitHub Actions, SHA-256, GitHub Releases.

**Spec:** `docs/superpowers/specs/2026-09-01-midi-controller-pedal-design.md`

## Global Constraints

- Do not mark an acceptance item passed without a command output, captured measurement, screenshot, or signed manual result.
- The README must let a new contributor orient in ten minutes and link directly to detailed build, flash, assembly, editor-hosting, protocol, and troubleshooting guides.
- Release artifacts are rebuilt from a clean tagged clone; no local untracked build output enters the release.
- Software uses MIT, hardware source uses CERN-OHL-S-2.0, and project documentation uses CC-BY-SA-4.0, each with SPDX headers or directory-level notices.
- First public compatibility tuple is config schema 1, binary image 1, USB protocol 1, firmware 0.1.x, and editor 0.1.x.

---

### Task 1: Hardware-in-the-loop runner and fixture contract

**Files:**
- Modify: `package.json`
- Modify: `pnpm-workspace.yaml`
- Create: `tools/hil/package.json`
- Create: `tools/hil/src/types.ts`
- Create: `tools/hil/src/runner.ts`
- Create: `tools/hil/src/adapters/config_serial.ts`
- Create: `tools/hil/src/adapters/midi_capture.ts`
- Create: `tools/hil/src/adapters/manual_fixture.ts`
- Create: `tools/hil/src/report.ts`
- Create: `tools/hil/test/runner.test.ts`
- Create: `tools/hil/README.md`
- Create: `hardware/test-fixtures/README.md`

**Interfaces:**
- Consumes: CDC serial device, USB/TRS MIDI capture ports, optional relay/expression test fixture, and human confirmations for power/continuity steps.
- Produces: versioned `HilReportV1` JSON with environment, firmware/config versions, test evidence, measurements, and pass/fail/skip disposition.

- [ ] **Step 1: Write a failing deterministic runner test**

```ts
it("fails the run when observed MIDI bytes differ without discarding evidence", async () => {
  const report = await runSuite(fakeRig({ midi: [0xb0, 17, 126] }), cc127Case);
  expect(report.status).toBe("FAIL");
  expect(report.cases[0]).toMatchObject({ expected: [0xb0, 17, 127], observed: [0xb0, 17, 126] });
});
```

- [ ] **Step 2: Run and observe failure**

Run: `pnpm --filter @midi-pedal/hil test`

Expected: FAIL because the runner package is absent.

- [ ] **Step 3: Implement adapter boundaries and report schema**

```ts
export interface HilCase { id: string; run(rig: Rig): Promise<CaseResult>; }
export interface CaseResult { status: "PASS" | "FAIL" | "SKIP"; startedAt: string; durationMs: number; expected?: unknown; observed?: unknown; evidence: string[]; notes: string[]; }
export interface HilReportV1 { schemaVersion: 1; commit: string; firmware: string; protocol: number; configSchema: number; environment: Record<string, string>; cases: CaseResult[]; status: "PASS" | "FAIL"; }
```

Require a reason for every SKIP and make any skipped release-blocking case fail the release gate.

Register the package in `pnpm-workspace.yaml`, give it the name `@midi-pedal/hil`, expose its CLI as a `start` script, and add these root scripts:

```json
{
  "scripts": {
    "hil:test": "pnpm --filter @midi-pedal/hil test",
    "hil:release": "pnpm --filter @midi-pedal/hil start --"
  }
}
```

- [ ] **Step 4: Implement safe manual-fixture prompts**

Prompts name the exact meter mode, probe points, supply/current limit, expected range, and stop condition. Never ask a person to change wiring while energized. Save entered measurement plus operator and timestamp.

- [ ] **Step 5: Verify simulated pass/fail/timeout reports**

Run: `pnpm --filter @midi-pedal/hil test`

Expected: deterministic JSON for pass, byte mismatch, timeout, disconnect, invalid measurement, and forbidden skip.

- [ ] **Step 6: Commit**

```bash
git add tools/hil hardware/test-fixtures package.json pnpm-lock.yaml pnpm-workspace.yaml
git commit -m "test: add hardware-in-the-loop runner"
```

### Task 2: Executable product acceptance suite

**Files:**
- Create: `tools/hil/src/cases/switch_latency.ts`
- Create: `tools/hil/src/cases/midi_routing.ts`
- Create: `tools/hil/src/cases/relay_safety.ts`
- Create: `tools/hil/src/cases/expression.ts`
- Create: `tools/hil/src/cases/power_handover.ts`
- Create: `tools/hil/src/cases/config_recovery.ts`
- Create: `tools/hil/src/cases/display.ts`
- Create: `tools/hil/suites/release-v1.json`
- Create: `docs/acceptance.md`

**Interfaces:**
- Produces: command `pnpm hil:release -- --report artifacts/hil-report.json` implementing every acceptance criterion from the design spec.

- [ ] **Step 1: Write failing release-suite completeness test**

Read the required ids from a checked-in array and assert `release-v1.json` contains each exactly once: switch latency, all four chords, TRS bytes, USB packets, routing, queue order, relay boot/reset/watchdog/brownout/unpowered, expression, display response, USB/9 V handover, interrupted upload, editor keyboard, themes, and 200% zoom.

- [ ] **Step 2: Run and observe failure**

Run: `pnpm --filter @midi-pedal/hil test -- release-suite`

Expected: FAIL because cases and suite are absent.

- [ ] **Step 3: Implement automated MIDI/config cases**

For each switch preset, install a known test config, trigger fixture input, timestamp first edge and captured output, compare exact TRS/USB bytes/order/destination, and require normal press under 50 ms. Interrupt uploads after BEGIN, first/middle/final chunk, VERIFY, and before ACTIVATE; reboot and verify old CRC each time.

- [ ] **Step 4: Implement measured/manual safety cases**

Relay and power cases require continuity/voltage evidence at boot, reset, watchdog, brownout, unpowered, both-source operation, and each source removal. Display case captures action timestamp and first dirty-region transfer/visible change under 100 ms. Expression case records 10 kΩ and 25 kΩ sweeps, jitter, unplug, and invalid calibration.

- [ ] **Step 5: Map every design acceptance line to evidence**

`docs/acceptance.md` contains a table with spec requirement, HIL/manual case id, instrument/automation, numeric limit, last report link, and release-blocking status. No requirement may map to a prose-only assertion.

- [ ] **Step 6: Run simulated suite and commit**

Run: `pnpm --filter @midi-pedal/hil test && pnpm hil:release -- --rig simulated --report build/hil-simulated.json`

```bash
git add tools/hil docs/acceptance.md
git commit -m "test: encode v1 product acceptance suite"
```

### Task 3: Complete README, build, flashing, assembly, and troubleshooting path

**Files:**
- Modify: `README.md`
- Modify: `docs/building.md`
- Modify: `docs/flashing.md`
- Modify: `docs/editor-hosting.md`
- Modify: `docs/hardware-assembly.md`
- Create: `docs/troubleshooting.md`
- Create: `docs/architecture.md`
- Create: `docs/rehearsals/new-builder.md`
- Create: `CONTRIBUTING.md`
- Create: `tools/docs/check.mjs`
- Create: `tools/docs/verified-commands.json`

**Interfaces:**
- Produces: one verified newcomer journey from repository discovery to flashed hardware and hosted editor.

- [ ] **Step 1: Create a documentation command/link checker that initially fails**

Add `tools/docs/check.mjs` to verify local Markdown links, referenced files, fenced shell commands registered in `tools/docs/verified-commands.json`, and README links to every required guide.

Run: `node tools/docs/check.mjs`

Expected: FAIL on missing links/command registrations.

- [ ] **Step 2: Finish the README as the shortest successful path**

README order:

```text
What it is and v1 feature boundary
Photo/render and connector/switch map
Safety warning for 9 V polarity and relay contacts
Repository status and tested hardware revision
Required hardware and software
Clone with submodules
Run all host tests
Build firmware UF2
Flash with BOOTSEL
Build/run editor and connect
Links: assembly, build, flash, hosting, protocol, troubleshooting, contribute
License split and release downloads
```

Do not duplicate full guides; keep commands exact and link deeper explanations.

- [ ] **Step 3: Verify and complete `docs/building.md`**

For Linux, macOS, and Windows, include supported tool versions, install sources, submodules, Node/pnpm, CMake/Ninja, Arm compiler, Pico SDK, KiCad CLI, host tests, protocol fixture generation, Debug/Release firmware builds, editor typecheck/test/build, HIL simulation, all-check command, expected artifact paths, and common environment errors.

- [ ] **Step 4: Verify and complete `docs/flashing.md`**

Include BOOTSEL drag/drop and picotool flows, debug-probe wiring/OpenOCD/GDB, first-boot indicators, firmware update while preserving config, factory-empty reset, corrupt-config recovery, editor protocol mismatch, Windows driver expectations, and how to confirm both USB interfaces and relay-open state after flashing.

- [ ] **Step 5: Complete assembly, hosting, troubleshooting, and contributor guides**

Troubleshooting is symptom-led: no power, high current, no USB interfaces, WebSerial denied/busy, no TRS MIDI, USB MIDI absent, noisy expression, inverted expression, relay wrong/default closed, blank/wrong-color display, failed upload, incompatible schema, and source-handover reset. Each names safe observations, recovery, and when to power off.

- [ ] **Step 6: Rehearse with a new builder and commit**

Have a person unfamiliar with the project use only README and linked docs on a clean machine/clone. Record every blocked or ambiguous step in `docs/rehearsals/new-builder.md`, fix docs, rerun link/command checks, then commit.

```bash
git add README.md CONTRIBUTING.md docs tools/docs
git commit -m "docs: complete build flash assembly and contributor journey"
```

### Task 4: Open-source licenses and community/repository policy

**Files:**
- Create: `LICENSES/MIT.txt`
- Create: `LICENSES/CERN-OHL-S-2.0.txt`
- Create: `LICENSES/CC-BY-SA-4.0.txt`
- Create: `LICENSES/README.md`
- Create: `REUSE.toml`
- Create: `CODE_OF_CONDUCT.md`
- Create: `SECURITY.md`
- Create: `.github/ISSUE_TEMPLATE/bug.yml`
- Create: `.github/ISSUE_TEMPLATE/hardware.yml`
- Create: `.github/ISSUE_TEMPLATE/config.yml`
- Create: `.github/pull_request_template.md`
- Modify: `README.md`

**Interfaces:**
- Produces: machine-checkable license boundaries and issue/reporting paths.

- [ ] **Step 1: Add a failing REUSE/license check**

Run: `reuse lint`

Expected: FAIL until license texts, file annotations, and directory mappings exist.

- [ ] **Step 2: Apply the approved license split**

Map `firmware/`, `editor/`, `packages/`, `tools/`, and CI/build files to MIT; `hardware/` design sources/fabrication/mechanics to CERN-OHL-S-2.0; Markdown/product documentation to CC-BY-SA-4.0. Third-party submodules retain upstream licenses and are excluded from project assertions.

- [ ] **Step 3: Add contribution and safety templates**

Hardware issues require board revision, power source, measured rails, assembly photos, and whether tests were performed unpowered first. Security policy covers editor/protocol parsing and unsafe hardware defects without promising private handling channels that do not exist.

- [ ] **Step 4: Verify license manifest and links**

Run: `reuse lint && node tools/docs/check.mjs`

Expected: all project-owned files have an effective license and README explains the split clearly.

- [ ] **Step 5: Commit**

```bash
git add LICENSES REUSE.toml CODE_OF_CONDUCT.md SECURITY.md .github README.md CONTRIBUTING.md
git commit -m "chore: establish open-source project policies"
```

### Task 5: Reproducible release packaging and compatibility manifest

**Files:**
- Create: `release/manifest.schema.json`
- Create: `release/manifest.json`
- Create: `tools/release/package.json`
- Create: `tools/release/package.mjs`
- Create: `tools/release/package.test.ts`
- Create: `.github/workflows/release.yml`
- Create: `CHANGELOG.md`
- Create: `docs/releasing.md`
- Modify: `package.json`

**Interfaces:**
- Produces: `pnpm release:package -- --version 0.1.0 --out build/release` and a GitHub workflow triggered by signed `v*` tags.

- [ ] **Step 1: Write failing manifest completeness tests**

```ts
it("refuses a package missing any public artifact", async () => {
  await expect(packageRelease(incompleteInputs)).rejects.toThrow("editor-dist.zip");
});
```

Require firmware UF2/ELF/map, editor zip, schema, default JSON, protocol doc, Gerbers/drills, BOM/positions, panel DXF/PDF, source revision, HIL report, licenses, and SHA-256 file.

- [ ] **Step 2: Run and observe failure**

Run: `pnpm --filter @midi-pedal/release-tools test`

Expected: FAIL because packager and manifest are absent.

- [ ] **Step 3: Implement manifest and deterministic packaging**

Manifest fields include release version, git commit, UTC build time from `SOURCE_DATE_EPOCH`, hardware revision, firmware version, config schema, image format, USB protocol, editor version, minimum Chromium, artifact filename/size/SHA-256/license, and HIL report status.

Name `tools/release/package.json` as `@midi-pedal/release-tools`, expose Vitest through its `test` script, and add this root script:

```json
{
  "scripts": {
    "release:package": "pnpm --filter @midi-pedal/release-tools package --"
  }
}
```

- [ ] **Step 4: Build release workflow**

On a signed tag: checkout submodules, run frozen installs/tests, build firmware/editor, run KiCad ERC/DRC and fabrication generation, require uploaded real HIL report matching commit/hardware revision, package artifacts, verify checksums, and create a draft GitHub Release. Publishing remains a human review action.

- [ ] **Step 5: Document versioning and rollback**

`docs/releasing.md` defines SemVer, compatibility changes, schema/protocol bump rules, hardware revision naming, release-candidate process, artifact review, firmware rollback, editor rollback, and correction of bad fabrication files.

- [ ] **Step 6: Verify twice and commit**

Run packaging twice with identical `SOURCE_DATE_EPOCH` and compare artifact hashes; all deterministic archives must match.

```bash
git add release tools/release .github/workflows/release.yml CHANGELOG.md docs/releasing.md package.json pnpm-lock.yaml
git commit -m "build: package reproducible project releases"
```

### Task 6: Final release rehearsal and v0.1.0 readiness record

**Files:**
- Create: `docs/releases/v0.1.0-readiness.md`
- Create: `docs/releases/v0.1.0-builder-report.md`
- Create: `docs/releases/v0.1.0-known-limitations.md`
- Modify: `CHANGELOG.md`
- Modify: `README.md`

**Interfaces:**
- Produces: auditable go/no-go decision for the first public release; does not publish automatically.

- [ ] **Step 1: Run all clean-clone software and hardware checks**

```bash
pnpm install --frozen-lockfile
pnpm check
cmake --preset pico2-release
cmake --build --preset pico2-release
pnpm --filter @midi-pedal/editor test:e2e
kicad-cli sch erc hardware/midi-pedal.kicad_sch --exit-code-violations
kicad-cli pcb drc hardware/midi-pedal.kicad_pcb --exit-code-violations
pnpm hil:release -- --report artifacts/hil-report.json
reuse lint
node tools/docs/check.mjs
```

Expected: zero failed checks and no release-blocking skipped HIL case.

- [ ] **Step 2: Rehearse the user journey from manufactured artifacts**

A new builder orders/uses the released BOM and fabrication files, assembles with the guide, builds or downloads UF2, flashes, hosts editor files, connects, imports a sample JSON, synchronizes, and controls TRS MIDI, USB-MIDI, expression, and both relays. Record time, ambiguity, deviations, and outcomes.

- [ ] **Step 3: Verify recovery and safety one final time**

Repeat wrong/incompatible config import, interrupted sync, USB disconnect, watchdog, factory-empty reset, both-source handover, and relay continuity during power loss. Record instruments and raw evidence links.

- [ ] **Step 4: Write the readiness decision**

Every spec acceptance item is `PASS` with evidence or `BLOCKED` with an owner and release withheld. Known limitations may include only approved non-goals; they cannot excuse a failed safety, data-integrity, accessibility, build, or flashing requirement.

- [ ] **Step 5: Update public entry points and commit**

README names the validated hardware revision and links release downloads, build, flashing, assembly, editor, troubleshooting, acceptance, limitations, and source licenses. CHANGELOG lists only shipped behavior.

```bash
git add docs/releases README.md CHANGELOG.md
git commit -m "docs: record v0.1.0 release readiness"
```
