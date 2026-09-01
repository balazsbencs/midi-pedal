#ifndef MIDI_PEDAL_TUSB_CONFIG_H
#define MIDI_PEDAL_TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT 0
#endif

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE 64
#endif

#define CFG_TUD_ENABLED 1
#define CFG_TUD_CDC 1
#define CFG_TUD_MIDI 1
#define CFG_TUD_MSC 0
#define CFG_TUD_HID 0
#define CFG_TUD_VENDOR 0

#define CFG_TUD_CDC_RX_BUFSIZE 4096
#define CFG_TUD_CDC_TX_BUFSIZE 4096
#define CFG_TUD_MIDI_RX_BUFSIZE 64
#define CFG_TUD_MIDI_TX_BUFSIZE 64

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

#ifdef __cplusplus
}
#endif

#endif
