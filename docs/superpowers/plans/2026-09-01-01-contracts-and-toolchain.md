# Shared Contracts and Toolchain Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Establish a reproducible repository and one tested configuration/protocol contract consumed by both firmware and editor.

**Architecture:** A TypeScript package owns the canonical JSON model, JSON Schema, binary encoder/reader, CRC-32, and USB frames. A hardware-independent C++ library decodes the same binary fixtures; CI rejects any cross-language drift before device or UI code depends on it.

**Tech Stack:** Node.js 24 LTS, pnpm 10, TypeScript 5.9, Vitest 4.1, C++20, CMake 3.25+, Ninja, CTest, GoogleTest 1.17, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-01-midi-controller-pedal-design.md`

## Global Constraints

- The portable format is versioned JSON; the device format is little-endian binary with explicit lengths and CRC-32.
- Capacity is exactly 128 banks, four pages, four presets, eight slots, and one expression assignment per bank.
- Reject unknown behavioral fields, invalid ASCII labels, out-of-range MIDI values, invalid relay contacts, and unsupported schema versions.
- Later plans import `@midi-pedal/protocol`; they must not duplicate TypeScript protocol types.
- Firmware parsing performs no dynamic allocation and never reads past caller-provided spans.

---

### Task 1: Reproducible root toolchain and contributor entry point

**Files:**
- Create: `.nvmrc`
- Create: `package.json`
- Create: `pnpm-workspace.yaml`
- Create: `CMakeLists.txt`
- Create: `CMakePresets.json`
- Create: `cmake/GoogleTest.cmake`
- Create: `tools/check-toolchain.mjs`
- Create: `README.md`
- Create: `docs/building.md`
- Modify: `.gitignore`

**Interfaces:**
- Consumes: Node.js 24, Corepack/pnpm 10, CMake, Ninja, and an Arm GNU toolchain visible on `PATH`.
- Produces: `pnpm check`, CMake preset `host-debug`, and documented clean-clone setup commands used by every later plan.

- [ ] **Step 1: Write the failing toolchain smoke check**

```js
// tools/check-toolchain.mjs
import { execFileSync } from "node:child_process";

const checks = [["node", ["--version"]], ["pnpm", ["--version"]], ["cmake", ["--version"]], ["ninja", ["--version"]]];
for (const [bin, args] of checks) execFileSync(bin, args, { stdio: "inherit" });
```

- [ ] **Step 2: Run it before the root scripts exist**

Run: `pnpm check`

Expected: FAIL because `package.json` does not define `check`.

- [ ] **Step 3: Add pinned project metadata and build presets**

```json
{
  "name": "midi-pedal",
  "private": true,
  "packageManager": "pnpm@10.15.0",
  "engines": { "node": ">=24 <25" },
  "scripts": {
    "check:toolchain": "node tools/check-toolchain.mjs",
    "test:ts": "pnpm -r test",
    "test:cpp": "cmake --preset host-debug && cmake --build --preset host-debug && ctest --preset host-debug",
    "check": "pnpm check:toolchain && pnpm test:ts && pnpm test:cpp"
  }
}
```

Set `.nvmrc` to `24`, make the host preset generate `build/host-debug` with Ninja, and fetch GoogleTest `v1.17.0` only for host tests.

- [ ] **Step 4: Write the initial README and build guide**

`README.md` must state the project scope, current implementation status, repository map, required hardware, supported host platforms, license intent, and the shortest commands:

```bash
corepack enable
pnpm install --frozen-lockfile
pnpm check
```

`docs/building.md` must explain Linux, macOS, and Windows installation for Node 24, pnpm 10, CMake, Ninja, Git submodules, Arm GNU Toolchain, and Pico SDK; later firmware/editor tasks append only commands they have run successfully.

- [ ] **Step 5: Verify from repository root**

Run: `corepack enable && pnpm install && pnpm check:toolchain && cmake --preset host-debug`

Expected: all version commands succeed and CMake generates `build/host-debug`.

- [ ] **Step 6: Commit**

```bash
git add .nvmrc package.json pnpm-lock.yaml pnpm-workspace.yaml CMakeLists.txt CMakePresets.json cmake tools README.md docs/building.md .gitignore
git commit -m "build: establish reproducible host toolchain"
```

### Task 2: Canonical JSON configuration model and validation

**Files:**
- Create: `packages/protocol/package.json`
- Create: `packages/protocol/tsconfig.json`
- Create: `packages/protocol/src/model.ts`
- Create: `packages/protocol/src/validate.ts`
- Create: `packages/protocol/schema/config-v1.schema.json`
- Create: `packages/protocol/test/validate.test.ts`
- Create: `protocol/fixtures/json/minimal-valid.json`
- Create: `protocol/fixtures/json/full-boundary-valid.json`
- Create: `protocol/fixtures/json/invalid-message-value.json`

**Interfaces:**
- Produces: `ConfigV1`, `ConfigDocumentV1`, `validateConfig(input: unknown): ValidationResult<ConfigDocumentV1>`, `serializeConfigDocument(document: ConfigDocumentV1): Record<string, JsonValue>`, and the canonical schema identifier `https://midi-pedal.dev/schema/config-v1.json`.
- Consumes: no firmware or editor code.

- [ ] **Step 1: Write failing validation tests**

```ts
import { describe, expect, it } from "vitest";
import { validateConfig } from "../src/validate";
import minimal from "../../../protocol/fixtures/json/minimal-valid.json";
import invalid from "../../../protocol/fixtures/json/invalid-message-value.json";

describe("config v1", () => {
  it("accepts the safe empty 128-bank document", () => expect(validateConfig(minimal).ok).toBe(true));
  it("rejects CC values above 127 with a field path", () => {
    const result = validateConfig(invalid);
    expect(result).toMatchObject({ ok: false });
    if (!result.ok) expect(result.errors[0].path).toContain("value");
  });
  it("round-trips opaque top-level metadata without compiling it into behavior", () => {
    const input = { ...minimal, communityRating: { stars: 5 } };
    const result = validateConfig(input);
    if (!result.ok) throw new Error("fixture must validate");
    expect(serializeConfigDocument(result.value).communityRating).toEqual({ stars: 5 });
  });
});
```

- [ ] **Step 2: Verify the tests fail**

Run: `pnpm --filter @midi-pedal/protocol test -- validate.test.ts`

Expected: FAIL because `validateConfig` and model types do not exist.

- [ ] **Step 3: Define the exact discriminated model**

```ts
export type Destination = "TRS" | "USB" | "BOTH";
export type Trigger = "PRESS" | "RELEASE" | "LONG_PRESS" | "DOUBLE_TAP";
export type PositionFilter = "POSITION_1" | "POSITION_2" | "BOTH";
export type JsonValue = null | boolean | number | string | JsonValue[] | { [key: string]: JsonValue };
export type Message =
  | { type: "PC"; channel: number; program: number; destination: Destination }
  | { type: "CC"; channel: number; controller: number; value: number; destination: Destination }
  | { type: "RELAY"; contact: 1 | 2; operation: "OPEN" | "CLOSE" | "TOGGLE" }
  | { type: "NAV"; operation: "BANK_UP" | "BANK_DOWN" | "BANK_SET" | "PAGE_UP" | "PAGE_DOWN" | "PAGE_SET"; target?: number };
export interface MessageSlot { id: number; trigger: Trigger; position: PositionFilter; message: Message }
export interface PositionView { label: string; accentRgb565: number }
export interface Preset { id: number; position1: PositionView; position2: PositionView; toggleOn: Trigger | null; slots: MessageSlot[] }
export interface Page { id: number; presets: [Preset, Preset, Preset, Preset] }
export interface ExpressionAssignment { enabled: boolean; label: string; channel: number; controller: number; destination: Destination; minimum: number; maximum: number; inverted: boolean }
export interface Bank { id: number; name: string; pages: [Page, Page, Page, Page]; expression: ExpressionAssignment }
export interface ConfigV1 { schemaVersion: 1; deviceModel: "MIDI_PEDAL_PICO2"; banks: Bank[] }
export interface ConfigDocumentV1 { config: ConfigV1; passthroughTopLevel: Readonly<Record<string, JsonValue>> }
```

Enforce exactly four presets per page, at most eight slots, at most 128 banks in JSON, globally unique nonzero `u32` ids for every bank/page/preset/message, printable ASCII name limits, channel 1–16, MIDI bytes 0–127, valid NAV targets, and deterministic ids/defaults for omitted banks up to 128.

- [ ] **Step 4: Implement AJV validation and normalized defaults**

Return `{ ok: true, value: ConfigDocumentV1 }` or `{ ok: false, errors: { path: string; code: string; message: string }[] }`. Set AJV to `allErrors: true`, reject additional properties anywhere below the root, and sort errors by path then code. At the root, separate JSON-safe unknown keys into `passthroughTopLevel`; the binary encoder ignores them and `serializeConfigDocument` merges them back only when they do not collide with `schemaVersion`, `deviceModel`, or `banks`. Reject non-JSON values, prototype-pollution keys, and any unsupported field nested in behavioral data.

- [ ] **Step 5: Run boundary tests**

Run: `pnpm --filter @midi-pedal/protocol test`

Expected: valid fixtures pass; invalid MIDI, label, capacity, target, and schema fixtures fail with stable paths.

- [ ] **Step 6: Commit**

```bash
git add packages/protocol protocol/fixtures/json pnpm-lock.yaml pnpm-workspace.yaml
git commit -m "feat(protocol): define configuration schema"
```

### Task 3: Deterministic binary configuration image

**Files:**
- Create: `packages/protocol/src/crc32.ts`
- Create: `packages/protocol/src/binary.ts`
- Create: `packages/protocol/test/binary.test.ts`
- Create: `protocol/fixtures/bin/minimal-valid.bin`
- Create: `protocol/fixtures/bin/full-boundary-valid.bin`
- Create: `protocol/fixtures/manifest.json`

**Interfaces:**
- Consumes: normalized behavioral `ConfigV1` from `ConfigDocumentV1.config` in Task 2; opaque top-level metadata never enters the device image.
- Produces: `encodeImage(config: ConfigV1, sequence: number): Uint8Array`, `ImageMetadata = { formatVersion: number; imageSize: number; sequence: number; bankCount: number; crc32: number }`, `inspectImage(bytes: Uint8Array): ImageMetadata`, and `decodeImage(bytes: Uint8Array): ConfigV1` for editor readback/tests.

- [ ] **Step 1: Write failing header and determinism tests**

```ts
it("writes the fixed 32-byte v1 header", () => {
  const image = encodeImage(minimalConfig, 7);
  expect(new TextDecoder().decode(image.slice(0, 4))).toBe("MPDL");
  expect(readU16(image, 4)).toBe(1);
  expect(readU16(image, 6)).toBe(32);
  expect(readU32(image, 12)).toBe(7);
});

it("encodes identical input byte-for-byte", () => {
  expect(encodeImage(minimalConfig, 1)).toEqual(encodeImage(minimalConfig, 1));
});
```

- [ ] **Step 2: Run and observe failure**

Run: `pnpm --filter @midi-pedal/protocol test -- binary.test.ts`

Expected: FAIL because `encodeImage` is absent.

- [ ] **Step 3: Implement the fixed layout**

Use little-endian fields:

```text
Header (32 bytes): magic[4], formatVersion:u16, headerSize:u16,
imageSize:u32, sequence:u32, bankCount:u16, reserved:u16,
indexOffset:u32, payloadOffset:u32, crc32:u32.
Index: 128 × u32 payload-relative bank offsets; 0xffffffff means empty.
Payload: length-prefixed bank records; each record contains its u32 stable id,
bank view, expression assignment, four page ids, 16 fixed preset ids/records,
and 0–8 packed slots with a u32 stable id each.
CRC-32: IEEE polynomial over the complete image with header crc32 zeroed.
```

Reject images above 768 KiB and any offset, count, length, enum, ASCII string, or CRC that lies outside the documented range.

- [ ] **Step 4: Generate golden fixtures from JSON**

Add a package script `fixtures` that reads `protocol/fixtures/json`, validates, encodes with sequence 1, writes `.bin`, and updates SHA-256 values in `manifest.json`.

- [ ] **Step 5: Verify round-trip, corruption, and reproducibility**

Run: `pnpm --filter @midi-pedal/protocol fixtures && pnpm --filter @midi-pedal/protocol test && git diff --exit-code protocol/fixtures`

Expected: all tests pass and a second fixture generation produces no diff.

- [ ] **Step 6: Commit**

```bash
git add packages/protocol protocol/fixtures
git commit -m "feat(protocol): encode deterministic device images"
```

### Task 4: Versioned USB configuration frames

**Files:**
- Create: `packages/protocol/src/commands.ts`
- Create: `packages/protocol/src/frame.ts`
- Create: `packages/protocol/test/frame.test.ts`
- Create: `protocol/fixtures/frames/get-capabilities-request.bin`
- Create: `protocol/fixtures/frames/crc-corrupt-then-valid.bin`
- Create: `protocol/fixtures/frames/unsupported-version.bin`
- Create: `docs/protocol.md`

**Interfaces:**
- Produces: `encodeFrame(frame: Frame): Uint8Array`, incremental `FrameDecoder.push(chunk): DecodeEvent[]`, `DecodeEvent = { type: "frame"; frame: Frame } | { type: "error"; code: StatusCode }`, `Command`, `StatusCode`, and typed request/response payload codecs.
- Frame fields: magic `MPCF`, protocol version 1, request id `u32`, command `u16`, flags `u16`, payload length `u32`, payload, CRC-32.

- [ ] **Step 1: Write failing split-frame and CRC tests**

```ts
it("reassembles arbitrary serial chunks", () => {
  const bytes = encodeFrame({ requestId: 42, command: Command.GET_CAPABILITIES, flags: 0, payload: new Uint8Array() });
  const decoder = new FrameDecoder();
  expect(decoder.push(bytes.slice(0, 5))).toEqual([]);
  expect(decoder.push(bytes.slice(5))).toEqual([{ type: "frame", frame: {
    requestId: 42, command: Command.GET_CAPABILITIES, flags: 0, payload: new Uint8Array()
  } }]);
});

it("rejects a corrupt payload without losing the following frame", () => {
  const corrupt = encodeFrame({ requestId: 1, command: Command.GET_CONFIG_INFO, flags: 0, payload: new Uint8Array([1]) });
  corrupt[corrupt.length - 1] ^= 0xff;
  const valid = encodeFrame({ requestId: 2, command: Command.GET_CAPABILITIES, flags: 0, payload: new Uint8Array() });
  const events = new FrameDecoder().push(new Uint8Array([...corrupt, ...valid]));
  expect(events[0]).toEqual({ type: "error", code: StatusCode.CRC_MISMATCH });
  expect(events[1]).toMatchObject({ type: "frame", frame: { requestId: 2 } });
});
```

- [ ] **Step 2: Run and observe failure**

Run: `pnpm --filter @midi-pedal/protocol test -- frame.test.ts`

Expected: FAIL because framing APIs are absent.

- [ ] **Step 3: Implement commands and incremental decoding**

Define commands `GET_CAPABILITIES`, `GET_CONFIG_INFO`, `READ_CONFIG`, `BEGIN_UPLOAD`, `WRITE_CHUNK`, `VERIFY_UPLOAD`, `ACTIVATE_UPLOAD`, `GET_EXPRESSION_SAMPLE`, `SET_EXPRESSION_CALIBRATION`, and `FACTORY_EMPTY_RESET`. Cap payloads at 4096 bytes and upload chunks at 1024 bytes. Cache the latest response for each request id so retries are idempotent.

- [ ] **Step 4: Document every byte and error**

`docs/protocol.md` must specify byte order, CRC coverage, commands, payloads, request lifecycle, retry behavior, status codes, 1000 ms host timeout, three retry limit, and compatibility rules.

- [ ] **Step 5: Verify stream fuzz cases**

Run: `pnpm --filter @midi-pedal/protocol test -- frame.test.ts`

Expected: PASS for byte-at-a-time input, concatenated frames, noise before magic, oversize payload, corrupt CRC, duplicate request, and unsupported version.

- [ ] **Step 6: Commit**

```bash
git add packages/protocol protocol/fixtures/frames docs/protocol.md
git commit -m "feat(protocol): define USB configuration frames"
```

### Task 5: Allocation-free C++ image reader against golden fixtures

**Files:**
- Create: `firmware/CMakeLists.txt`
- Create: `firmware/src/core/config_types.hpp`
- Create: `firmware/src/core/crc32.hpp`
- Create: `firmware/src/core/crc32.cpp`
- Create: `firmware/src/core/image_reader.hpp`
- Create: `firmware/src/core/image_reader.cpp`
- Create: `firmware/tests/image_reader_test.cpp`
- Modify: root `CMakeLists.txt`

**Interfaces:**
- Consumes: `std::span<const std::byte>` binary images and the Task 3 fixtures.
- Produces: `ImageReader::inspect`, `ImageReader::load_bank(uint8_t, BankConfig&)`, and `ImageError`; no heap allocation.

- [ ] **Step 1: Write the failing C++ fixture test**

```cpp
TEST(ImageReader, LoadsGoldenBankWithoutAllocation) {
  auto bytes = read_fixture("protocol/fixtures/bin/minimal-valid.bin");
  midi::ImageReader reader(bytes);
  ASSERT_EQ(reader.inspect().error, midi::ImageError::None);
  midi::BankConfig bank{};
  ASSERT_TRUE(reader.load_bank(0, bank));
  EXPECT_EQ(bank.pages[0].presets.size(), 4);
}
```

- [ ] **Step 2: Run and observe failure**

Run: `cmake --preset host-debug && cmake --build --preset host-debug && ctest --preset host-debug -R image_reader`

Expected: compile failure because `ImageReader` is absent.

- [ ] **Step 3: Implement checked readers and fixed-capacity types**

Use `std::array` for four pages, four presets, eight slots, and 128 offsets. Every read must pass through `read_u8/u16/u32(offset)` returning an error on overflow. Validate magic, version, header size, image size, bank count, index/payload offsets, CRC, record lengths, globally unique nonzero stable ids, enum ranges, and printable ASCII before populating `BankConfig`.

- [ ] **Step 4: Add corrupt-image table tests**

Cover truncated header/index/record, wrapped offset, invalid enum, oversized slot count, bad label byte, wrong version, wrong magic, and bad CRC. Assert the exact `ImageError` and leave output structures zero-initialized on failure.

- [ ] **Step 5: Run host contract tests**

Run: `pnpm --filter @midi-pedal/protocol fixtures && cmake --build --preset host-debug && ctest --preset host-debug --output-on-failure`

Expected: TypeScript and C++ agree on every fixture.

- [ ] **Step 6: Commit**

```bash
git add firmware CMakeLists.txt
git commit -m "feat(firmware): decode versioned configuration images"
```

### Task 6: Cross-language CI and contract documentation

**Files:**
- Create: `.github/workflows/contracts.yml`
- Modify: `README.md`
- Modify: `docs/building.md`
- Modify: `docs/protocol.md`

**Interfaces:**
- Produces: required CI job `contracts` and the verified contributor commands later plans inherit.

- [ ] **Step 1: Add a CI change that initially fails on generated drift**

Configure Ubuntu CI to install Node 24, pnpm from `packageManager`, CMake, and Ninja; run fixture generation without committing the resulting diff.

- [ ] **Step 2: Run the equivalent locally**

Run: `pnpm install --frozen-lockfile && pnpm --filter @midi-pedal/protocol fixtures && git diff --exit-code protocol/fixtures`

Expected: FAIL until all generated fixtures from Tasks 2–4 are committed.

- [ ] **Step 3: Complete the CI workflow**

```yaml
- run: pnpm install --frozen-lockfile
- run: pnpm --filter @midi-pedal/protocol test
- run: pnpm --filter @midi-pedal/protocol fixtures
- run: git diff --exit-code protocol/fixtures
- run: cmake --preset host-debug
- run: cmake --build --preset host-debug
- run: ctest --preset host-debug --output-on-failure
```

- [ ] **Step 4: Verify README links and clean-clone commands**

Add direct README links to `docs/building.md`, `docs/protocol.md`, the approved spec, and the roadmap. Verify every copied command from a fresh temporary clone using `git clone --recurse-submodules`.

- [ ] **Step 5: Run the complete phase gate**

Run: `pnpm check && git diff --exit-code protocol/fixtures`

Expected: all TypeScript/C++ tests pass and generated contracts are clean.

- [ ] **Step 6: Commit**

```bash
git add .github README.md docs protocol/fixtures
git commit -m "ci: enforce shared protocol contracts"
```
