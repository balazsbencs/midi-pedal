# Real-device browser checklist

Record browser version, OS, firmware commit, editor commit, and device serial
before starting. Run the matrix in current stable desktop Chrome, Edge, and
Brave from an HTTPS deployment.

- [ ] Connect, grant WebSerial permission, read capabilities and active CRC.
- [ ] Edit a label and expression assignment; confirm local dirty status.
- [ ] Export JSON; import it back and confirm opaque metadata is retained.
- [ ] Sync a changed draft; observe BEGIN, WRITE, VERIFY, ACTIVATE, READBACK.
- [ ] Unplug USB during WRITE; reconnect and verify the previous CRC remains active.
- [ ] Complete a second sync and compare active CRC with the compiled image.
- [ ] Confirm USB-MIDI output and TRS output independently, then with BOTH routing.
- [ ] Switch Light/Dark mode, reload, and confirm the explicit choice persists.
- [ ] Resize to a laptop viewport and browser zoom 200%; confirm no required horizontal scroll.
- [ ] Use Factory empty reset only after exporting the desired JSON backup.

Attach screenshots, browser console output, and a USB/MIDI monitor capture for
any failure. A checklist entry without evidence is not a release qualification
pass.

