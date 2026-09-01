# Carrier PCB and Enclosure Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a safe, reproducible two-layer Pico 2 carrier PCB and drilled-aluminum reference enclosure for the approved pedal interfaces.

**Architecture:** Prove each electrical function on a bench fixture before incorporating it into one KiCad 10 carrier. The carrier accepts a socketed Pico 2 and cabled panel components; hardware defaults enforce fail-open relays and prevent USB/9 V backfeed independently of firmware.

**Tech Stack:** KiCad 10.0.6 and `kicad-cli`, oscilloscope, logic analyzer, DMM, electronic load, current-limited bench supply, MIDI loop-current test fixture, continuity logger, 2D DXF/SVG panel drawings.

**Spec:** `docs/superpowers/specs/2026-09-01-midi-controller-pedal-design.md`

## Global Constraints

- Reference enclosure: Hammond 1590DD, nominal 188 × 119.5 × 33 mm body, measured before drilling.
- Front face: landscape display above a 2×2 A/B/C/D footswitch field; no additional controls.
- Connectors: 3.5 mm TRS Type-A MIDI OUT, 3.5 mm TRS relay, 6.35 mm TRS expression, 9 V center-negative DC, and accessible Pico USB.
- Two-layer PCB using components that are hand-solderable where practical; Pico 2 mounts on replaceable sockets/headers.
- Display logic is 3.3 V; a documented solder selection chooses 3.3 V or 5 V module supply; BL is always enabled; SDO is not routed.
- Both relay contacts are normally open and de-energized through unpowered, boot, reset, watchdog, and brownout states.
- Never connect unverified amp-switching voltage directly to Pico logic or ground.

---

### Task 1: Measured mechanical and connector input record

**Files:**
- Create: `hardware/README.md`
- Create: `hardware/mechanical/measured-components.csv`
- Create: `hardware/mechanical/layout-requirements.md`
- Create: `hardware/libraries/midi-pedal.pretty/`
- Create: `hardware/libraries/midi-pedal.kicad_sym`
- Create: `hardware/libraries/fp-lib-table`
- Create: `hardware/libraries/sym-lib-table`

**Interfaces:**
- Consumes: the actual display module, Pico 2, jacks, switches, DC jack, and enclosure.
- Produces: reviewed dimensions, panel keep-outs, and project-owned symbol/footprint sources used by schematic and PCB.

- [ ] **Step 1: Record every physical part before drawing cutouts**

Measure body, mounting holes, pin pitch/order, connector centerline, nut/washer envelope, insertion depth, cable bend space, display active area, and enclosure internal ribs. Record millimeters, instrument, and measurement uncertainty in CSV.

- [ ] **Step 2: Verify the chosen 1590DD fit on paper at 1:1**

Print a 188 × 119.5 mm front-face outline with the 2×2 switch centers, display active/cutout outlines, and fastener clearances. Place real parts over the print and reject any layout with less than 15 mm footswitch-nut clearance or inaccessible display fasteners.

- [ ] **Step 3: Create symbols and footprints from measured values**

Use project libraries for the ST7796S header, Pico 2 sockets, selected jacks, and any part absent from official KiCad libraries. Put pin 1, polarity, center-negative, TRS tip/ring/sleeve, and relay coil/contact meaning on fabrication layers.

- [ ] **Step 4: Run library checks**

Open each footprint over its datasheet/measured print at 1:1, verify pad drill/annular ring, courtyard, silkscreen, and 3D alignment, then have a second reviewer sign `layout-requirements.md`.

- [ ] **Step 5: Commit**

```bash
git add hardware/README.md hardware/mechanical hardware/libraries
git commit -m "docs(hardware): capture measured mechanical inputs"
```

### Task 2: Protected dual-source power bench proof

**Files:**
- Create: `hardware/bench/power.kicad_sch`
- Create: `hardware/bench/power-bom.csv`
- Create: `hardware/validation/power-bench.md`
- Create: `hardware/validation/data/power-bench.csv`

**Interfaces:**
- Consumes: USB VBUS path already present on Pico 2 and 9 V center-negative input.
- Produces: protected VSYS supply that cannot backfeed USB or the 9 V jack and survives source handover.

- [ ] **Step 1: Draw and independently review the power test schematic**

Use a resettable input fuse, P-channel MOSFET reverse-polarity stage, 12 V-rated transient clamp, AP63205-class fixed 5 V buck with datasheet layout network, and Schottky/ideal-diode isolation into Pico VSYS. Add test points for raw 9 V, protected input, regulated 5 V, USB VBUS, VSYS, 3V3, and ground.

- [ ] **Step 2: Run ERC before assembly**

Run: `kicad-cli sch erc hardware/bench/power.kicad_sch --exit-code-violations`

Expected: zero unapproved violations; intentional power-flag exceptions are annotated in the schematic and validation record.

- [ ] **Step 3: Assemble on a dedicated bench board with current limit**

Start at 50 mA current limit with no Pico fitted. Verify polarity stage, ramp 0→12 V, reverse 9 V, output short recovery, and no voltage at USB VBUS from 9 V input. Fit Pico only after all rail values pass.

- [ ] **Step 4: Measure all source combinations**

Record USB only, 9 V only, both, remove USB while 9 V remains, and remove 9 V while USB remains at idle and maximum display/relay load. Acceptance: VSYS stays within Pico limits, controller does not reset during overlapped handover, reverse current into either source stays below the meter's 1 mA resolution, and no component exceeds a 30 °C rise after 30 minutes.

- [ ] **Step 5: Commit evidence**

```bash
git add hardware/bench/power* hardware/validation/power-bench.md hardware/validation/data/power-bench.csv
git commit -m "test(hardware): prove protected USB and pedal power"
```

### Task 3: MIDI, relay, expression, switch, and display-interface bench proofs

**Files:**
- Create: `hardware/bench/io.kicad_sch`
- Create: `hardware/bench/io-bom.csv`
- Create: `hardware/validation/io-bench.md`
- Create: `hardware/validation/data/midi-captures/`
- Create: `hardware/validation/data/expression-sweeps.csv`

**Interfaces:**
- Consumes: Pico firmware HAL from Firmware Plan Tasks 7–8.
- Produces: proven circuits ready for carrier integration.

- [ ] **Step 1: Draw the I/O proof schematic**

Include:

```text
MIDI: MIDI Association 3.3 V current-loop output circuit, Type-A mapping
      tip=data/sink, ring=current source, sleeve=shield/reference as specified.
Relay: two Omron G5V-1-DC5 relays, transistor drivers, gate/base defaults,
       flyback diodes, TRS tip/ring switched independently to sleeve.
Expression: ring protected 3V3, tip through 1 kΩ plus 100 nF RC and clamps to ADC0,
            sleeve ground, explicit ESD path.
Switches: four NO contacts to ground, protected GPIO inputs with defined pull-ups.
Display: 9-pin header, 3V3 logic, VCC solder selector, BL tied on, SDO no-connect.
```

- [ ] **Step 2: Run ERC and peer review net names/pin order**

Run: `kicad-cli sch erc hardware/bench/io.kicad_sch --exit-code-violations`

Expected: zero unapproved violations and a signed pin-order checklist for all three TRS connectors and the display header.

- [ ] **Step 3: Prove MIDI electrical and byte behavior**

Measure loop current and polarity with a standards-representative optocoupler receiver, then capture PC and CC bytes at 31,250 baud. Verify Type-A cables/adapters, no data on sleeve, and no GPIO exposure to external short/open conditions.

- [ ] **Step 4: Prove relay isolation and defaults**

Continuity-test both contacts independently for OPEN/CLOSE/TOGGLE, simultaneous operation, reset, BOOTSEL, watchdog, brownout, and power loss. Measure isolation between contact network and controller rails; record coil current and flyback waveform.

- [ ] **Step 5: Prove expression and display interfaces**

Sweep representative 10 kΩ and 25 kΩ TRS pedals heel-to-toe, unplug/replug, reverse mechanical direction through firmware setting, and record ADC range/noise. Test the actual display at both selected module supply options only when the module rating permits; verify 3.3 V logic never exceeds the device pins.

- [ ] **Step 6: Commit**

```bash
git add hardware/bench/io* hardware/validation/io-bench.md hardware/validation/data
git commit -m "test(hardware): prove pedal input and output circuits"
```

### Task 4: Complete carrier schematic and electrical review

**Files:**
- Create: `hardware/midi-pedal.kicad_pro`
- Create: `hardware/midi-pedal.kicad_sch`
- Create: `hardware/bom/approved-parts.csv`
- Create: `hardware/reviews/schematic-review.md`

**Interfaces:**
- Consumes: measured libraries and passed power/I/O bench schematics.
- Produces: one hierarchical schematic with approved component identities, test points, and connector labels.

- [ ] **Step 1: Integrate proven sheets without redrawing circuits**

Create hierarchical sheets for power, Pico/headers, display, switches, MIDI, relay, and expression. Preserve validated part values and net names. Add assembly option fields, manufacturer part numbers, voltage/power ratings, and sourcing notes to every BOM item.

- [ ] **Step 2: Add design-for-test and safe defaults**

Expose test points for each rail, UART MIDI, relay drives, ADC, SPI clock/data/CS/DC/RST, and ground. Mark relay drivers with explicit pull-down/default-off components. Label panel connectors by function on both schematic and silkscreen.

- [ ] **Step 3: Run ERC and BOM validation**

Run:

```bash
kicad-cli sch erc hardware/midi-pedal.kicad_sch --exit-code-violations
kicad-cli sch export bom hardware/midi-pedal.kicad_sch -o hardware/bom/generated.csv
```

Expected: zero unapproved ERC violations and every generated line maps to an approved manufacturer part or documented do-not-fit option.

- [ ] **Step 4: Conduct independent schematic review**

Reviewer checks power direction, maximum ratings, Pico pin mux, ADC protection, MIDI Type-A polarity, relay contact/coil separation, flyback orientation, TRS labels, display supply selection, no-connect flags, and all connector pin numbers. Resolve every item in `schematic-review.md`.

- [ ] **Step 5: Commit**

```bash
git add hardware/midi-pedal.kicad_pro hardware/midi-pedal.kicad_sch hardware/bom hardware/reviews/schematic-review.md
git commit -m "feat(hardware): complete reviewed carrier schematic"
```

### Task 5: Two-layer PCB layout and fabrication outputs

**Files:**
- Create: `hardware/midi-pedal.kicad_pcb`
- Create: `hardware/reviews/pcb-review.md`
- Create: `hardware/fabrication/gerbers/`
- Create: `hardware/fabrication/drill/`
- Create: `hardware/fabrication/bom.csv`
- Create: `hardware/fabrication/positions.csv`
- Create: `hardware/fabrication/README.md`

**Interfaces:**
- Consumes: reviewed schematic and measured enclosure constraints.
- Produces: fabricator-neutral two-layer board package.

- [ ] **Step 1: Place by current path and mechanical constraints**

Keep protection/regulator loops compact, USB/power paths away from ADC, relay contact copper isolated from logic, MIDI output at its jack, display SPI short with continuous return, and all panel headers keyed/labeled. Keep Pico USB physically accessible and leave SWD clearance.

- [ ] **Step 2: Route with explicit net classes**

Define logic, 3V3, 5V, raw-9V, relay-contact, and ground classes with documented width/clearance/via rules. Use a continuous ground plane where isolation rules allow; do not route relay contacts under Pico/display logic.

- [ ] **Step 3: Run DRC and inspect manufactured views**

Run: `kicad-cli pcb drc hardware/midi-pedal.kicad_pcb --exit-code-violations`

Inspect front/back copper, mask, silk, drill, edge cuts, and 3D view. Resolve all courtyard, edge, unconnected, silkscreen-over-pad, and drill issues; no blanket exclusions.

- [ ] **Step 4: Generate reproducible fabrication files**

Use checked-in `hardware/fabrication/README.md` commands based on `kicad-cli pcb gerbers`, `pcb drill`, `sch export bom`, and `pcb export pos`. Zip only generated reviewed outputs and record SHA-256.

- [ ] **Step 5: Independent PCB review and commit**

Reviewer validates footprints against real parts, polarity, pin 1, USB/enclosure access, test probes, mounting holes, panel cable direction, clearance, thermal paths, and no backfeed route.

```bash
git add hardware/midi-pedal.kicad_pcb hardware/reviews/pcb-review.md hardware/fabrication
git commit -m "feat(hardware): route and release carrier PCB"
```

### Task 6: 1590DD panel and assembly design

**Files:**
- Create: `hardware/mechanical/midi-pedal-enclosure.step`
- Create: `hardware/mechanical/front-panel.dxf`
- Create: `hardware/mechanical/rear-panel.dxf`
- Create: `hardware/mechanical/drill-template.pdf`
- Create: `hardware/mechanical/assembly.step`
- Create: `hardware/mechanical/README.md`
- Modify: `docs/hardware-assembly.md`

**Interfaces:**
- Consumes: measured parts and routed PCB.
- Produces: dimensioned cutouts, fastening stack, cable plan, and full collision-checked assembly.

- [ ] **Step 1: Build the measured 3D assembly**

Import the Hammond 1590DD, PCB STEP, display, switches, jacks, Pico/USB plug envelope, headers, fasteners, nuts, and cable bend volumes. Orient the display above the 2×2 switch field with A/B top and C/D bottom.

- [ ] **Step 2: Check every service and collision path**

Verify lid closure, display viewing area, switch nut tool access, jack plugs inserted, USB cable attached, DC plug attached, PCB removal, display removal, no conductor trapped by lid, and no metal edge against unsleeved wiring.

- [ ] **Step 3: Produce dimensioned panel files**

DXF and PDF must include datum edges, hole/cutout dimensions, tolerances, countersink/no-countersink notes, labels, and a 100 mm print scale check. Place no product name until branding is separately approved.

- [ ] **Step 4: Make a paper/acrylic prototype before metal**

Print at 100%, punch/cut a disposable template, install the actual display/switches/jacks, and operate with shoes while cables are connected. Record changes and regenerate—not hand-edit—the final exports.

- [ ] **Step 5: Verify docs and commit**

`docs/hardware-assembly.md` must show mechanical order, washers/nuts, spacer heights, cable destinations, display supply selector, strain relief, and safe disassembly.

```bash
git add hardware/mechanical docs/hardware-assembly.md
git commit -m "feat(hardware): design the reference pedal enclosure"
```

### Task 7: First-article carrier qualification

**Files:**
- Create: `hardware/validation/first-article-checklist.md`
- Create: `hardware/validation/first-article-results.md`
- Create: `hardware/validation/data/first-article/`
- Modify: `hardware/fabrication/README.md`
- Modify: `docs/hardware-assembly.md`

**Interfaces:**
- Produces: qualified revision identifier or a documented board revision with corrected fabrication outputs.

- [ ] **Step 1: Inspect unpowered assembly**

Verify BOM identity, orientation, joints, shorts, resistance-to-ground on each rail, relay contact openness, enclosure clearances, connector labels, and display-voltage selector before fitting Pico/display.

- [ ] **Step 2: Power in controlled stages**

Use current-limited 9 V first without Pico, then Pico without display/relays, then each load. Record current and all rail voltages at each stage. Stop on any deviation from the bench proof rather than bypassing protection.

- [ ] **Step 3: Run full interface qualification**

Execute firmware boot, USB CDC/MIDI, TRS MIDI capture, every switch/chord, expression calibration/sweep, both relays, display snapshots, watchdog/reset, USB/9 V combinations, and 30-minute thermal soak.

- [ ] **Step 4: Decide pass or new board revision from evidence**

A bodge is acceptable only on an explicitly labeled engineering sample. Any safety, protection, footprint, connector, or fail-open defect requires a schematic/PCB revision and regenerated fabrication files before the design is called reproducible.

- [ ] **Step 5: Run hardware phase gate and commit**

Run:

```bash
kicad-cli sch erc hardware/midi-pedal.kicad_sch --exit-code-violations
kicad-cli pcb drc hardware/midi-pedal.kicad_pcb --exit-code-violations
```

```bash
git add hardware/validation hardware/fabrication docs/hardware-assembly.md
git commit -m "test(hardware): qualify the carrier first article"
```
