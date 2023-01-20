# Picostation

## __In developement__ _Raspberry Pi Pico based ODE_ for the original Playstation
<a href="https://twitter.com/paulo7x8/status/1602007862733312000"><img src="https://i.ibb.co/9hT2GQc/pico-tweet.jpg" alt="original tweet" height="400"/></a>

### Supported models:
- PU-8  (SCPH100X)
- PU-18 (SCPH55XX)

### Compatibility
- Audio CD image
- UNIROM image

### How-to
- Assemble [PCB](https://github.com/paulocode/picostation_pcb)
- Solder [Xstation QSB](https://github.com/x-station/xstation-releases) (including pin lifting)
- [Compile](https://shawnhymel.com/2096/how-to-set-up-raspberry-pi-pico-c-c-toolchain-on-windows-with-vs-code/) code, or use latest release
- Upload picostation.uf2 to your Raspberry Pi Pico (press Bootsel while you plug usb cable)
- Drop your bin/cue files on your SD Card (named UNIROM.bin / UNIROM.cue | UNIROM.cue must point to UNIROM.bin)
- Insert SD Card into PCB
- Boot your PS1, Enjoy

### Links
- PCB: https://github.com/paulocode/picostation_pcb
- FAQ: https://github.com/paulocode/picostation_faq
- How to compile (Windows): https://shawnhymel.com/2096/how-to-set-up-raspberry-pi-pico-c-c-toolchain-on-windows-with-vs-code/
