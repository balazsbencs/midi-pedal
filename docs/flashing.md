# Flashing and recovery

The Pico 2 firmware is a normal UF2 image. Always disconnect the 9 V supply
before changing USB or SWD wiring, and observe the 9 V center-negative warning
in [hardware-assembly.md](hardware-assembly.md).

## Build the image

```bash
cmake --preset pico2-release
cmake --build --preset pico2-release
```

The image to flash is:

```text
build/pico2-release/firmware/midi_pedal.uf2
```

## BOOTSEL / UF2 (recommended)

1. Unplug the pedal from USB and 9 V.
2. Hold the Pico 2 **BOOTSEL** button while connecting USB.
3. Release BOOTSEL after the removable `RP2350`/`RPI-RP2` volume appears.
4. Copy `midi_pedal.uf2` to that volume.
5. Wait for the volume to disappear and the controller to reboot.

On Linux the volume normally mounts under `/media/$USER/`; macOS shows it in
Finder; Windows assigns it a removable drive letter. Copy the file to the
volume root, not to a subdirectory. Do not unplug while the copy is in
progress. If the volume does not appear, retry with a known data-capable USB
cable and hold BOOTSEL before inserting the cable.

## picotool

With a supported `picotool` on `PATH` and the device in normal USB mode:

```bash
picotool info
picotool load -f build/pico2-release/firmware/midi_pedal.uf2
picotool reboot
```

The `-f` option is an explicit overwrite. Confirm the board identity in the
`info` output before loading an image.

## SWD debug probe

Use a 3.3 V probe only. Connect probe GND, SWDIO, SWCLK, and (if supported)
the target reset line to the Pico 2 debug pads; never apply probe voltage and
9 V pedal power at the same time unless the carrier power design explicitly
isolates them. A typical OpenOCD session is:

```bash
openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg
```

In a second terminal, load the ELF with the matching GDB:

```bash
arm-none-eabi-gdb build/pico2-release/firmware/midi_pedal.elf
(gdb) target extended-remote :3333
(gdb) monitor reset init
(gdb) load
(gdb) monitor reset run
(gdb) detach
(gdb) quit
```

Probe configuration names vary; use the config supplied by the probe vendor
if `target/rp2350.cfg` is named differently.

## Verify after flashing

Before connecting the editor:

- both isolated relay contacts measure open with a continuity meter;
- the ST7796S shows the 2×2 live screen and expression-unavailable state when
  no pedal is connected;
- the host sees one CDC configuration port and one class-compliant MIDI port;
- a MIDI monitor receives the expected three-byte CC/PC packet from a switch.

The relays are designed normally open in hardware and are driven inactive
before configuration is read. A firmware image cannot substitute for the
continuity checks in [firmware-bench.md](../hardware/validation/firmware-bench.md).

## Factory-empty recovery

If configuration synchronization is interrupted, the previous validated image
remains active. Reconnect the editor and retry the sync. To intentionally
clear the user configuration, use the editor's **Factory empty reset** command
or send `FACTORY_EMPTY_RESET` over CDC; this activates the checked-in image at
`firmware/defaults/factory-empty.bin` and does not change firmware.

If the device does not enumerate, reflash the UF2 first, then reconnect USB
and repeat the reset. Keep a JSON export of the desired configuration before
using a factory reset.
