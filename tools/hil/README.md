# Hardware-in-the-loop runner

`@midi-pedal/hil` records machine-readable evidence without pretending that a
simulator proves electrical safety. The default release suite deliberately
marks hardware-dependent cases as `SKIP` with a reason; because those cases are
release-blocking, the overall report is `FAIL` until a real rig supplies
captures and signed manual measurements.

Run the deterministic simulated report with:

```bash
pnpm --filter @midi-pedal/hil test
pnpm hil:release -- --rig simulated --report build/hil-simulated.json
```

Adapters for CDC configuration, MIDI capture, and safe manual prompts are
separate from the report schema. Attach raw captures and the firmware/editor
commit to any report used for release qualification.

