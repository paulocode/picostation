# Picostation

## __In developement__ _Raspberry Pi Pico based ODE_ for the original Playstation
<a href="https://twitter.com/paulo7x8/status/1602007862733312000"><img src="https://i.ibb.co/9hT2GQc/pico-tweet.jpg" alt="original tweet" height="400"/></a>

### Supported models:
- PU-8  (SCPH100X)
- PU-18 (SCPH55XX)

### Compatibility
- Audio CD image
- UNIROM image
- Some games MAY load

### How-to
- Assemble [PCB](https://github.com/paulocode/picostation_pcb)
- Solder [Xstation QSB](https://github.com/x-station/xstation-releases) (including pin lifting)
- Double / triple check [each connection](https://mmmonkey.co.uk/xstation-sony-playstation-install-notes-and-pinout/), from the alt-point to the FFC connector point on the picostation board
- [Compile](https://shawnhymel.com/2096/how-to-set-up-raspberry-pi-pico-c-c-toolchain-on-windows-with-vs-code/) code, or use latest release
- Upload picostation.uf2 to your Raspberry Pi Pico (press Bootsel while you plug usb cable)
- Drop your bin/cue files on your SD Card (named UNIROM.bin / UNIROM.cue | UNIROM.cue must point to UNIROM.bin)
- Insert SD Card into PCB
- Boot your PS1, Enjoy

### Notes
- For the moment, Game/app loading is not consistent, you'll need to try several times
- If using an out of region bin/cue, you may need to use freepsxboot to bypass the region check

### Links
- PCB: https://github.com/paulocode/picostation_pcb
- FAQ: https://github.com/paulocode/picostation_faq
- Alternative solder points / checking connection: https://mmmonkey.co.uk/xstation-sony-playstation-install-notes-and-pinout/
- How to compile (Windows): https://shawnhymel.com/2096/how-to-set-up-raspberry-pi-pico-c-c-toolchain-on-windows-with-vs-code/
