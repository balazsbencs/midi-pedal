# Static Browser Editor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver an accessible static Chromium editor that manages local drafts, JSON backup, and atomic USB synchronization with the Pico 2 pedal.

**Architecture:** React renders an operate-mode three-pane workspace over a framework-independent domain store. `@midi-pedal/protocol` owns all configuration and frame types; a `DeviceTransport` boundary supports WebSerial in production and deterministic simulation in tests.

**Tech Stack:** Node.js 24 LTS, pnpm 10, TypeScript 5.9, React 19.2, Vite 8, Vitest 4.1 with jsdom, Testing Library, Playwright Chromium, axe-core.

**Spec:** `docs/superpowers/specs/2026-09-01-midi-controller-pedal-design.md`

## Global Constraints

- Produce static files only; no API server, account, analytics, cloud storage, or service worker is required.
- Use `@midi-pedal/protocol` for `ConfigV1`, `ConfigDocumentV1`, validation, binary images, frames, and commands.
- Edits remain local until explicit Sync; Export JSON exports the draft, including unsynced changes.
- Support desktop Chrome, Edge, and Brave with WebSerial in secure contexts.
- Use semantic theme tokens for explicit Light/Dark modes; system preference initializes only the first visit.
- Maintain WCAG AA, keyboard operation, visible focus, color-independent states, and usability at 200% zoom.

---

### Task 1: Static React shell and persistent semantic themes

**Files:**
- Create: `editor/package.json`
- Create: `editor/tsconfig.json`
- Create: `editor/vite.config.ts`
- Create: `editor/index.html`
- Create: `editor/src/main.tsx`
- Create: `editor/src/App.tsx`
- Create: `editor/src/ui/theme/theme.ts`
- Create: `editor/src/ui/theme/tokens.css`
- Create: `editor/src/ui/theme/ThemeSwitch.tsx`
- Create: `editor/src/ui/theme/ThemeSwitch.test.tsx`

**Interfaces:**
- Consumes: no device code.
- Produces: `Theme = "light" | "dark"`, `readInitialTheme(storage, media)`, and an accessible top-level application shell.

- [ ] **Step 1: Write failing theme precedence tests**

```tsx
it("uses system preference only when no explicit choice exists", () => {
  expect(readInitialTheme(emptyStorage, darkMedia)).toBe("dark");
  expect(readInitialTheme(storageWith("light"), darkMedia)).toBe("light");
});

it("labels the action, not merely the current icon", async () => {
  render(<ThemeSwitch theme="dark" onChange={onChange} />);
  await user.click(screen.getByRole("button", { name: "Switch to light theme" }));
  expect(onChange).toHaveBeenCalledWith("light");
});
```

- [ ] **Step 2: Run and observe failure**

Run: `pnpm --filter @midi-pedal/editor test -- ThemeSwitch.test.tsx`

Expected: FAIL because editor package and theme APIs are absent.

- [ ] **Step 3: Create the Vite application and theme contract**

```ts
export function readInitialTheme(storage: Pick<Storage, "getItem">, media: Pick<MediaQueryList, "matches">): Theme {
  const stored = storage.getItem("midi-pedal.theme");
  return stored === "light" || stored === "dark" ? stored : media.matches ? "dark" : "light";
}
```

Call `readInitialTheme(localStorage, window.matchMedia("(prefers-color-scheme: dark)"))`, set `document.documentElement.dataset.theme`, persist only explicit button choices, and define tokens for canvas, panel, control, border, text, muted text, focus, accent, success, warning, and error in both modes.

- [ ] **Step 4: Add reduced-motion and contrast tests**

Use axe-core plus computed-style assertions for token pairs. The theme switch must remain visible in the top bar, use text and state—not an unlabeled icon—and have a 3 px focus outline.

- [ ] **Step 5: Verify test and production build**

Run: `pnpm --filter @midi-pedal/editor test && pnpm --filter @midi-pedal/editor build`

Expected: tests pass and `editor/dist/index.html` uses relative static assets.

- [ ] **Step 6: Commit**

```bash
git add editor package.json pnpm-lock.yaml pnpm-workspace.yaml
git commit -m "feat(editor): establish static shell and themes"
```

### Task 2: Local draft domain and selection model

**Files:**
- Create: `editor/src/domain/editor_state.ts`
- Create: `editor/src/domain/editor_actions.ts`
- Create: `editor/src/domain/editor_reducer.ts`
- Create: `editor/src/domain/selectors.ts`
- Create: `editor/src/domain/editor_reducer.test.ts`

**Interfaces:**
- Produces: `EditorState`, pure `editorReducer`, selection `{ bank, page, preset, position }`, validation results, device checksum, dirty state, and navigation guard.
- Consumes: `ConfigDocumentV1`, `validateConfig`, and `serializeConfigDocument` from the protocol package; editor actions modify only `document.config` and retain `document.passthroughTopLevel`.

- [ ] **Step 1: Write failing dirty-state and selection tests**

```ts
it("marks a local label edit dirty without changing device metadata", () => {
  const next = editorReducer(syncedState, { type: "preset.labelChanged", value: "CHORUS" });
  expect(next.dirty).toBe(true);
  expect(next.device.imageCrc32).toBe(syncedState.device.imageCrc32);
});

it("replaces only the local draft when a validated document is imported", () => {
  const next = editorReducer(syncedState, { type: "document.imported", document: importedDocument });
  expect(next.draft).toEqual(importedDocument);
  expect(next.device).toBe(syncedState.device);
  expect(next.dirty).toBe(true);
});
```

- [ ] **Step 2: Run and observe failure**

Run: `pnpm --filter @midi-pedal/editor test -- editor_reducer.test.ts`

Expected: FAIL because reducer is absent.

- [ ] **Step 3: Implement immutable use-case actions**

Support device loaded/disconnected, bank/page/preset/position selected, bank renamed, position view changed, toggle trigger changed, message added/changed/moved/removed, expression changed, document imported, sync started/failed/succeeded, and draft reset to device. Allocate a new message's stable id as one greater than the document-wide maximum, reject exhaustion at `0xffffffff`, and never alter ids during reorder or editing.

- [ ] **Step 4: Derive dirty and validation state**

Dirty is `inspectImage(encodeImage(draft.config, device.sequence)).crc32 !== device.imageCrc32` after a device load; before connection it means any behavioral change from the initial factory-empty document. Opaque top-level metadata is retained for JSON export but never sent to the pedal and does not affect sync state. Memoize selection-level validation paths without hiding whole-document errors.

- [ ] **Step 5: Verify reducer and property tests**

Run: `pnpm --filter @midi-pedal/editor test -- editor_reducer.test.ts`

Expected: selection, import, reorder, undo-to-clean, disconnect preservation, and invalid draft cases pass.

- [ ] **Step 6: Commit**

```bash
git add editor/src/domain
git commit -m "feat(editor): manage validated local drafts"
```

### Task 3: Three-pane bank/page/preset workspace

**Files:**
- Create: `editor/src/ui/layout/AppHeader.tsx`
- Create: `editor/src/ui/layout/StatusBar.tsx`
- Create: `editor/src/ui/banks/BankList.tsx`
- Create: `editor/src/ui/presets/PageMap.tsx`
- Create: `editor/src/ui/presets/PresetCard.tsx`
- Create: `editor/src/ui/expression/ExpressionSummary.tsx`
- Create: `editor/src/ui/workspace.css`
- Create: `editor/src/ui/workspace.test.tsx`
- Modify: `editor/src/App.tsx`

**Interfaces:**
- Consumes: selectors/actions from Task 2.
- Produces: left searchable bank list, center page tabs and A/B/C/D 2×2 map, expression summary, and visible sync/validation status.

- [ ] **Step 1: Write failing spatial and keyboard tests**

```tsx
it("orders presets A/B/C/D as the physical 2x2 surface", () => {
  renderWorkspace();
  expect(screen.getAllByRole("button", { name: /Preset/ }).map(x => x.dataset.switch)).toEqual(["A", "B", "C", "D"]);
});

it("selects page tabs and presets from the keyboard", async () => {
  render(<PageMap bank={testBank} selectedPage={0} selectedPreset={0} onPageSelect={onPage} onPresetSelect={onPreset} />);
  screen.getByRole("tab", { name: "Page 2" }).focus();
  await user.keyboard("{Enter}");
  expect(onPage).toHaveBeenCalledWith(1);
  screen.getByRole("button", { name: /switch B/i }).focus();
  await user.keyboard("{Enter}");
  expect(onPreset).toHaveBeenCalledWith(1);
});
```

- [ ] **Step 2: Run and observe failure**

Run: `pnpm --filter @midi-pedal/editor test -- workspace.test.tsx`

Expected: FAIL because workspace components are absent.

- [ ] **Step 3: Implement the approved hierarchy**

Use landmarks (`header`, `nav`, `main`, `aside`, `footer`), real buttons for banks/pages/presets, `aria-current` for selection, labels plus text for position state, and a status line that distinguishes valid/invalid and synced/unsynced.

- [ ] **Step 4: Implement laptop/zoom adaptations**

At narrower available width, keep bank list and page map side-by-side first, then move the inspector below; at 200% zoom allow one-column flow without horizontal scrolling. Preserve A/B/C/D reading order.

- [ ] **Step 5: Run component and axe tests**

Run: `pnpm --filter @midi-pedal/editor test -- workspace.test.tsx`

Expected: role/name queries pass, axe reports no violations, search filters 128 banks, long names do not overlap controls, and color is not the sole state cue.

- [ ] **Step 6: Commit**

```bash
git add editor/src/ui editor/src/App.tsx
git commit -m "feat(editor): add pedal-shaped preset workspace"
```

### Task 4: Preset and expression inspectors

**Files:**
- Create: `editor/src/ui/inspector/PresetInspector.tsx`
- Create: `editor/src/ui/inspector/PositionEditor.tsx`
- Create: `editor/src/ui/inspector/MessageList.tsx`
- Create: `editor/src/ui/inspector/MessageEditor.tsx`
- Create: `editor/src/ui/inspector/ExpressionEditor.tsx`
- Create: `editor/src/ui/inspector/inspector.test.tsx`
- Modify: `editor/src/App.tsx`

**Interfaces:**
- Consumes: exact PC/CC/RELAY/NAV unions from protocol package.
- Produces: state-specific label/accent editing, toggle trigger, ordered max-eight messages, and per-bank expression mapping.

- [ ] **Step 1: Write failing message-form tests**

```tsx
it("shows only parameters belonging to the selected message type", async () => {
  renderInspector(withMessage({ type: "RELAY", contact: 1, operation: "CLOSE" }));
  expect(screen.getByLabelText("Relay contact")).toBeVisible();
  expect(screen.queryByLabelText("MIDI channel")).toBeNull();
});

it("prevents a ninth message and explains the eight-slot limit", () => {
  render(<MessageList slots={eightSlots} onAdd={onAdd} onChange={onChange} />);
  expect(screen.getByRole("button", { name: "Add message" })).toBeDisabled();
  expect(screen.getByText("Maximum 8 messages per preset")).toBeVisible();
});
```

- [ ] **Step 2: Run and observe failure**

Run: `pnpm --filter @midi-pedal/editor test -- inspector.test.tsx`

Expected: FAIL because inspectors are absent.

- [ ] **Step 3: Implement controlled fields with inline validation**

Use labels, descriptions, and field-linked error ids. Keep numeric fields editable as strings until blur/submit, then validate ranges. Default new MIDI destinations to BOTH. Reordering uses Up/Down buttons first; pointer drag is optional progressive enhancement and cannot be the only method.

- [ ] **Step 4: Implement expression calibration presentation**

Separate global heel/toe calibration from per-bank assignment. Show sample value and calibration span only when a device session supplies them; reject spans below 10% and explain how to repeat calibration.

- [ ] **Step 5: Verify every union and boundary**

Run: `pnpm --filter @midi-pedal/editor test -- inspector.test.tsx`

Expected: PC/CC/relay/navigation forms, toggle states, ASCII lengths, values, destinations, move/remove, expression inversion/range, and keyboard behavior pass.

- [ ] **Step 6: Commit**

```bash
git add editor/src/ui/inspector editor/src/App.tsx
git commit -m "feat(editor): edit presets messages and expression"
```

### Task 5: WebSerial transport and device-session state machine

**Files:**
- Create: `editor/src/device/DeviceTransport.ts`
- Create: `editor/src/device/WebSerialTransport.ts`
- Create: `editor/src/device/SimulatedTransport.ts`
- Create: `editor/src/device/DeviceSession.ts`
- Create: `editor/src/device/device_session.test.ts`
- Create: `editor/src/ui/device/ConnectionControl.tsx`
- Create: `editor/src/ui/device/ConnectionStatus.tsx`

**Interfaces:**
- `DeviceTransport.open/write/read/close` carries bytes only.
- `DeviceSession.connect`, `readConfiguration`, `syncConfiguration`, `readExpressionSample`, `setCalibration`, `factoryEmptyReset`, and `disconnect` carry typed commands.

- [ ] **Step 1: Write failing timeout/retry/disconnect tests**

```ts
it("retries the same request id three times then preserves the draft", async () => {
  const transport = new SimulatedTransport().dropResponses(3);
  await expect(session.syncConfiguration(draft)).rejects.toMatchObject({ code: "TIMEOUT" });
  expect(store.getState().draft).toEqual(draft);
  expect(transport.requestIds()).toEqual([1, 1, 1]);
});
```

- [ ] **Step 2: Run and observe failure**

Run: `pnpm --filter @midi-pedal/editor test -- device_session.test.ts`

Expected: FAIL because session and transports are absent.

- [ ] **Step 3: Implement `DeviceSession` over framed protocol**

Use 1000 ms timeout and three total attempts with the same request id. Serialize commands, decode incremental frames, match responses by request id, reject incompatible protocol/device models before reading config, and preserve the local draft on every failure.

- [ ] **Step 4: Implement WebSerial permission and disconnect handling**

Request a user-selected port only from a button click, open CDC at 115200 8N1, pipe reads without assuming chunk boundaries, listen for disconnect, release reader/writer locks, and map DOM exceptions to `PERMISSION_DENIED`, `PORT_BUSY`, `DISCONNECTED`, or `UNSUPPORTED_BROWSER`.

- [ ] **Step 5: Verify all simulated session states**

Run: `pnpm --filter @midi-pedal/editor test -- device_session.test.ts`

Expected: capability mismatch, permission denial, noise/corrupt frame, retry, disconnect during read/upload/verify, and clean reconnect pass.

- [ ] **Step 6: Commit**

```bash
git add editor/src/device editor/src/ui/device
git commit -m "feat(editor): connect to the pedal over WebSerial"
```

### Task 6: JSON import/export and atomic synchronization UX

**Files:**
- Create: `editor/src/domain/files.ts`
- Create: `editor/src/domain/synchronize.ts`
- Create: `editor/src/ui/files/ImportDialog.tsx`
- Create: `editor/src/ui/sync/SyncProgress.tsx`
- Create: `editor/src/ui/sync/SyncResult.tsx`
- Create: `editor/src/ui/sync/sync.test.tsx`
- Modify: `editor/src/ui/layout/AppHeader.tsx`

**Interfaces:**
- Produces: `exportDraft(document: ConfigDocumentV1): Blob`, `previewImport(text: string): ImportPreview`, `SyncStage = "BEGIN" | "WRITE" | "VERIFY" | "ACTIVATE" | "READBACK"`, and `synchronizeDraft(session: DeviceSession, config: ConfigV1, onProgress?: (event: { stage: SyncStage; completed: number; total: number }) => void): Promise<SyncResult>`.

- [ ] **Step 1: Write failing lifecycle tests**

```ts
it("exports the unsynced document with opaque metadata intact", async () => {
  const blob = exportDraft(importedDocumentWithCommunityRating);
  expect(JSON.parse(await blob.text()).communityRating).toEqual({ stars: 5 });
});

it("reports the previous pedal configuration intact after verification failure", async () => {
  const result = await synchronizeDraft(sessionThatFailsVerify, changedDocument.config);
  expect(result).toMatchObject({ ok: false, stage: "VERIFY", previousConfigurationIntact: true });
});

it("previews a valid import and rejects invalid input without a replacement document", () => {
  expect(previewImport(validConfigText)).toMatchObject({ ok: true, counts: { banks: 128, pages: 512, presets: 2048 } });
  expect(previewImport('{"schemaVersion":1,"banks":"invalid"}')).toMatchObject({ ok: false, document: undefined });
});

it("reports every uploaded chunk and succeeds only after active CRC readback", async () => {
  const progress: string[] = [];
  const result = await synchronizeDraft(successfulSession, changedDocument.config, event => progress.push(event.stage));
  expect(progress).toEqual(["BEGIN", "WRITE", "VERIFY", "ACTIVATE", "READBACK"]);
  expect(result).toMatchObject({ ok: true, activeCrc32: expectedImageCrc32 });
});
```

The `Sync to pedal` component test also renders an invalid draft and asserts its button is disabled with `aria-describedby` pointing to the validation summary; reducer tests assert a rejected preview never dispatches `document.imported`.

- [ ] **Step 2: Run and observe failure**

Run: `pnpm --filter @midi-pedal/editor test -- sync.test.tsx`

Expected: FAIL because file/sync use cases are absent.

- [ ] **Step 3: Implement deterministic import/export**

Export `serializeConfigDocument(document)` as UTF-8 pretty JSON with two-space indentation and a trailing newline. Import parses text through `validateConfig`, checks schema/device model, preserves safe top-level metadata, validates all behavioral fields, and shows counts/errors before the user confirms replacement.

- [ ] **Step 4: Implement upload lifecycle**

Encode once, BEGIN with length/sequence/CRC, WRITE 1024-byte chunks, VERIFY, ACTIVATE, GET_CONFIG_INFO, and compare active CRC. Disable conflicting edits while writing/verifying but leave navigation/read-only inspection available.

- [ ] **Step 5: Add unsaved-navigation protection**

Register `beforeunload` only while dirty; internal document replacement requires a focused confirmation dialog naming Export JSON as the safe escape.

- [ ] **Step 6: Verify and commit**

Run: `pnpm --filter @midi-pedal/editor test -- sync.test.tsx`

```bash
git add editor/src/domain editor/src/ui/files editor/src/ui/sync editor/src/ui/layout/AppHeader.tsx
git commit -m "feat(editor): import export and atomically sync drafts"
```

### Task 7: Complete states, responsive behavior, and Impeccable audit/polish

**Files:**
- Create: `editor/src/ui/states/EmptyState.tsx`
- Create: `editor/src/ui/states/ErrorSummary.tsx`
- Create: `editor/src/ui/states/CompatibilityError.tsx`
- Create: `editor/tests/accessibility.spec.ts`
- Create: `editor/tests/responsive.spec.ts`
- Modify: `editor/src/ui/workspace.css`
- Modify: `editor/src/ui/theme/tokens.css`

**Interfaces:**
- Produces: complete disconnected, permission, dirty, invalid, syncing, verification failure, incompatibility, and success experiences.

- [ ] **Step 1: Write failing Playwright state tests**

At 1440×900, 1366×768, 1280×800, and 200% zoom, assert no horizontal page overflow, visible primary controls, logical focus order, Light/Dark snapshots, and recovery actions for each required state.

- [ ] **Step 2: Run and observe failure**

Run: `pnpm --filter @midi-pedal/editor test:e2e -- accessibility.spec.ts responsive.spec.ts`

Expected: failures for states and responsive paths not yet represented.

- [ ] **Step 3: Implement all state and responsive requirements**

Errors name the problem, whether the pedal config is intact, and the next action. Use `aria-live="polite"` for connection/sync status and `role="alert"` only for actionable failures. At constrained width, inspector follows the map and bank navigation remains reachable.

- [ ] **Step 4: Run Impeccable audit, then polish the verified findings**

Invoke `$impeccable audit editor`, record its score/findings in `docs/ui-audit.md`, then invoke `$impeccable polish editor` to fix P0–P2 findings without changing the approved three-pane structure. Run the bundled detector exactly once after the final UI edits and record intentional exceptions.

- [ ] **Step 5: Verify desktop and narrow views in one bounded pass**

Run: `pnpm --filter @midi-pedal/editor test && pnpm --filter @midi-pedal/editor test:e2e && pnpm --filter @midi-pedal/editor build`

Expected: component, axe, responsive, theme, state, and production-build checks pass with no P0/P1 audit finding open.

- [ ] **Step 6: Commit**

```bash
git add editor docs/ui-audit.md
git commit -m "fix(editor): harden and polish the complete workspace"
```

### Task 8: Static hosting and real-device browser verification

**Files:**
- Create: `docs/editor-hosting.md`
- Create: `editor/tests/real-device-checklist.md`
- Create: `.github/workflows/editor.yml`
- Modify: `README.md`
- Modify: `docs/building.md`

**Interfaces:**
- Produces: static `editor/dist`, CI artifact, hosting guidance, and a repeatable Chrome/Edge/Brave real-device checklist.

- [ ] **Step 1: Test a subpath deployment before documenting it**

Run: `pnpm --filter @midi-pedal/editor build && pnpm dlx serve@14 editor/dist -l 4173`

Open both `/` and a configured subpath build; confirm assets resolve and WebSerial reports the secure-context requirement instead of a generic failure.

- [ ] **Step 2: Write hosting instructions**

Document HTTPS, static MIME types, cache policy (`index.html` no-cache; hashed assets immutable), subpath base configuration, no required rewrite/API, Cloudflare Pages/GitHub Pages/nginx examples, and browser permission behavior.

- [ ] **Step 3: Run the real-device matrix**

In current stable Chrome, Edge, and Brave: connect, read config, edit, export, import, sync, unplug during chunk upload, reconnect, verify old configuration, complete sync, send USB-MIDI, switch theme, reload, and confirm persistence. Record browser/OS/firmware versions and results.

- [ ] **Step 4: Add editor CI**

Install from frozen lockfile, typecheck, test, build, run Playwright Chromium, upload `editor/dist`, and enforce that production output contains no backend URL or source map unless explicitly enabled for a development build.

- [ ] **Step 5: Run the editor phase gate**

Run: `pnpm --filter @midi-pedal/editor typecheck && pnpm --filter @midi-pedal/editor test && pnpm --filter @midi-pedal/editor test:e2e && pnpm --filter @midi-pedal/editor build`

Expected: all checks pass and hosting/real-device steps are reproducible from README links.

- [ ] **Step 6: Commit**

```bash
git add README.md docs/building.md docs/editor-hosting.md editor/tests/real-device-checklist.md .github/workflows/editor.yml
git commit -m "docs: publish and verify the static editor"
```
