# Picostation

## __In developement__ _Raspberry Pi Pico based ODE_ for the original Playstation
<a href="https://twitter.com/paulo7x8/status/1602007862733312000"><img src="https://i.ibb.co/9hT2GQc/pico-tweet.jpg" alt="original tweet" height="400"/></a>

### Supported models:
- PU-8  (SCPH100X)
- PU-18 (SCPH55XX)

### Compatibility
<b>NOTE: rename your image to UNIROM.bin/.cue</b><br>
- Audio CD image
- UNIROM image
- Some games may load (see <a href="https://github.com/paulocode/picostation/wiki/Game-Compatibility-List">Game Compatibility List</a> wiki page)

### How-to
- see <a href="https://github.com/paulocode/picostation/wiki/How-to">How-to</a> wiki page

### Notes
- For the moment, Game/app loading is not consistent, you'll need to try several times
- If using an out of region multi-track bin/cue, you may need to use freepsxboot to bypass the region check

### To-do (see <a href="https://github.com/paulocode/picostation/issues">issues</a>)
- Resolve region unlock not always working: should be OK for single track games with commit <a href="https://github.com/paulocode/picostation/commit/babc0722b1dbb14aaa5118330c816ad6ce5f6fcf">babc072</a> (thanks to <a href="https://github.com/OldBoredEE/picostation">OldBoredEE</a>)
- Stabilize image loading
- Make an interface for image choice/loading
- Make it possible to update the pico via SD card

### Links
- PCB: https://github.com/paulocode/picostation_pcb
- FAQ: https://github.com/paulocode/picostation_faq
- Slow Solder Board (SSB) solder points / checking connection: https://mmmonkey.co.uk/xstation-sony-playstation-install-notes-and-pinout/
- How to compile (Windows): https://shawnhymel.com/2096/how-to-set-up-raspberry-pi-pico-c-c-toolchain-on-windows-with-vs-code/
- PCB pinout: <a href="https://i.ibb.co/RvjvDyp/pinout.png"><img src="https://i.ibb.co/mDNDc8C/pinout.png" alt="pinout" border="0"></a>
- 3D Printable mount (550X) by <a href="https://twitter.com/SadSnifit">@Sadsnifit</a> : https://www.printables.com/fr/model/407224-picostation-mount-for-scph-5502
