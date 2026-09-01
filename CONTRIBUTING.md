# Contributing

Read the approved design and implementation plans before changing protocol,
timing, pin assignments, relay behavior, or power paths. Run `pnpm check` and
the focused package tests for every change; firmware changes should also build
the `pico2-release` preset when the Arm toolchain is available.

Keep commits focused by boundary (protocol, firmware, editor, hardware, docs),
preserve generated fixtures/lockfiles, and do not commit local `build/`,
`editor/dist/`, or dependency caches. New behavior needs a test and a note in
the relevant plan or protocol document.

Hardware contributions must include board revision, exact part/module identity,
power source and current limit, measured rails, connector/pin interpretation,
assembly photos, and evidence that inspection was performed unpowered first.
Do not report ERC/DRC, relay isolation, or power-handover results without raw
tool output or instrument captures.

