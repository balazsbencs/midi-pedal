# Fabrication package

The production Gerbers, drills, BOM, and placement files are intentionally not
generated yet: the current PCB is an unrouted outline placeholder and has not
passed ERC/DRC or independent footprint review. Once the schematic and board
are released, generate files with the KiCad CLI from the repository root:

```bash
kicad-cli sch erc hardware/midi-pedal.kicad_sch --exit-code-violations
kicad-cli pcb drc hardware/midi-pedal.kicad_pcb --exit-code-violations
kicad-cli pcb gerbers -o hardware/fabrication/gerbers/ hardware/midi-pedal.kicad_pcb
kicad-cli pcb drill -o hardware/fabrication/drill/ hardware/midi-pedal.kicad_pcb
kicad-cli sch export bom hardware/midi-pedal.kicad_sch -o hardware/fabrication/bom.csv
kicad-cli pcb export pos --format csv -o hardware/fabrication/positions.csv hardware/midi-pedal.kicad_pcb
```

Record the KiCad version, board revision, command output, and SHA-256 hashes
in this file before handing files to a fabricator. Empty output directories in
this WIP revision are deliberate.

