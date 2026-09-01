# Mechanical layout requirements

These are design constraints, not a claim that the current outline has passed
fit checking.

- Display above the switch field in landscape orientation; visible screen is
  480×320 and the panel header is on the rear/inside edge.
- Footswitch order is A/B on the top row and C/D on the bottom row, matching
  the firmware chord map and editor map.
- Keep at least 15 mm between footswitch nut envelopes and neighboring
  hardware. Verify with the actual shoes/cables before drilling metal.
- Keep the Pico USB connector accessible without opening the enclosure.
- Keep MIDI, relay, expression, and DC jacks reachable with right-angle plugs
  and their specified cable bend radius.
- Leave SWD/debug access and Pico socket removal clearance.
- No unsleeved conductor may touch the aluminum lid or an enclosure edge.
- Display module VCC selection is a documented assembly option; logic remains
  3.3 V, BL is hardwired on, and SDO is intentionally no-connect.

Second-review sign-off: __________________  Date: __________

