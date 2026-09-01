# I/O bench review sheet

Before carrier integration, draw and review these measured circuits on the
bench sheet:

- MIDI OUT: standards-representative 3.3 V current-loop driver and Type-A TRS
  mapping; no external jack conductor directly reaches a GPIO.
- Relay: two independent mechanical dry contacts, transistor drivers,
  flyback diodes, and default-off pull components. Tip and ring switch to
  sleeve independently and remain isolated from logic.
- Expression: protected 3V3 ring, tip through the RC/protection network to
  ADC0/GP26, sleeve ground; voltage-divider pedals only.
- Switches: four normally-open contacts to ground with pull-ups and GPIO
  protection.
- Display: nine-position header matching `GND,VCC,SCL,SDA,RST,DC,CS,BL,SDA-0`;
  SDO/SDA-0 is no-connect, BL is tied on, and logic is 3.3 V.

ERC, pin-order review, and component identities are pending.

