# Benchmark results
 
Measurements for the pico-st7735 driver, taken on the device itself. I ran this on 2026-10-07.
 
## Setup
 
| | |
|---|---|
| Board | Raspberry Pi Pico (RP2040) |
| Display | <!-- TODO: module name / tab colour --> ST7735, 128x160, SPI |
| SPI clock | 7,812,500 Hz actual (8 MHz requested, read back with `spi_get_baudrate`), unless stated |
| Pico SDK | Pico SDK Version: 2.2.0 (Stable) |
|

 
## Method
 
- **Timer:** `time_us_64()` on the RP2040. Each result is the average of 20 runs (5 for full-screen fills). Run-to-run differences are at an average of like 10 us.
- **Ideal time:** the wire-only minimum, `(11 + 2 x pixels) x 8 bits / baud`. Each pixel is 2 bytes (RGB565) and 11 bytes set the address window (3 command bytes plus 8 parameter bytes).
- **Efficiency:** `ideal / measured`, the share of time the SPI bus carries useful data.
- **"First version":** the straightforward original implementation I made when I started the project, where every pixel set its own address window and every byte was its own SPI call. Its numbers were measured before any optimization and then reproduced with the original code kept as a reference during development.
- No external instrument (logic analyzer, oscilloscope) was used. Everything below is measured by the chip's own timer.
## Final results (7.8125 MHz)
 
| Operation | Pixels | First version | Now | Speedup | Ideal | Efficiency (first -> now) |
|---|---|---|---|---|---|---|
| `fill_screen` | 20,480 | 74,413 us | 46,092 us | 1.6x | 41,954 us | 56% -> 91% |
| `draw_char` (5x7) | 35 | 865 us | 104 us | 8.3x | 83 us | 10% -> 80% |
| `draw_char_scaled` (x3) | 315 | 7,763 us | 749 us | 10.4x | 656 us | 8% -> 88% |
| `draw_rect` (30x30) | 900 | 22,108 us | 2,072 us | 10.7x | 1,855 us | 8% -> 90% |
| `draw_string` (21 chars) | 882 | 18,157 us* | 2,147 us | 8.5x | 1,818 us | 10% -> 85% |
| `draw_pixel` (single) | 1 | 25 us | not optimized | | | |
 
\* 21 calls of the original `draw_char` (735 pixels; the gaps between characters are not painted). `draw_string` paints all 882 pixels.
 
## How each step changed the numbers
 
### Full-screen fill
 
| Version | Time | Efficiency |
|---|---|---|
| Original, one byte per SPI call | 74,405 us | 56.4% |
| Row-batched, 8-bit frames | 50,026 us | 83.8% |
| Row-batched, 16-bit frames | 46,094 us | 91.0% |
| Theoretical limit (wire only) | 41,943 us | 100% |
 
### Shapes and text
 
| Operation | First version | After: one window + streamed 16-bit pixels | After: batched window parameters |
|---|---|---|---|
| `draw_char` | 865 us | 107 us | 104 us |
| `draw_char_scaled` (x3) | 7,763 us | 752 us | 749 us |
| `draw_rect` (30x30) | 22,107 us | 2,076 us | 2,072 us |
 
Setting the window once per shape and streaming the pixels did nearly all the work. Batching the window parameters (5 SPI calls instead of 11, same 11 bytes on the wire) saved about 3 us per window.
 
### Why I think the first version was at 8-10%
 
For the 30x30 rectangle, the first version sent 900 x 13 = 11,700 bytes, of which only 1,800 were pixel data (15% useful). Even those 11,700 bytes would have needed 11,981 us at wire speed, but took 22,108 us (54% efficient). 15% x 54% is about 8%, matching the measured efficiency.
 
## String drawing
 
| Method | Pixels painted | Time | Per pixel |
|---|---|---|---|
| 21 x original `draw_char` (per-pixel) | 735 | 18,157 us | 24.7 us |
| Loop of 21 x current `draw_char` | 735 | 2,118 us | 2.88 us |
| `draw_string` (one window per line) | 882 | 2,147 us | 2.43 us |
 
`draw_string` runs at 9,781 characters per second at scale 1 and 84.7% of ideal. In total time it is about equal to a loop of `draw_char` (1.4% slower), but it paints 20% more pixels (the gaps between characters) and handles wrapping and newlines. A hypothesis for its lower efficiency compared with rectangles (90%) is that the CPU builds each pixel row before it is sent, which is not overlapped with the transfer. Haven't tested this yet.
 
## Clock sweep (full-screen fill)
 
The clock was changed at runtime with `spi_set_baudrate`, and the display was fully re-initialized at the default clock before each step.
 
| Requested | Actual clock | Fill time | Ideal | Efficiency | Overhead | Per 16-bit frame | In clock periods |
|---|---|---|---|---|---|---|---|
| 8 MHz | 7.81 MHz | 46,101 us | 41,954 us | 91.0% | 4,147 us | 0.203 us | 1.58 |
| 16 MHz | 15.63 MHz | 23,156 us | 20,977 us | 90.6% | 2,179 us | 0.106 us | 1.66 |
| 21 MHz | 20.83 MHz | 17,425 us | 15,733 us | 90.3% | 1,692 us | 0.083 us | 1.72 |
| 32 MHz | 31.25 MHz | 11,689 us | 10,489 us | 89.7% | 1,200 us | 0.059 us | 1.83 |
| 63 MHz | 62.50 MHz* | 5,908 us | 5,244 us | 88.8% | 664 us | 0.032 us | 2.03 |
 
*The display output was clean through 31.25 MHz. At 62.5 MHz the picture broke (bright and garbled), so that row describes the SPI bus, not a working display.* Sort of expected since the datasheet for the ST7735 gives a max clock write speed of 15MHz (from 1 / 66 ns minimum serial clock write cycle ).
 
Overhead is measured time minus ideal time; per-frame overhead divides it by the 20,480 pixel frames in a fill. A straight-line fit through these points gives about 1.5 clock periods plus roughly 8 ns per frame. Because the overhead shrinks with the clock instead of staying constant in microseconds, it is consistent with the SPI peripheral inserting a short gap between frames, not with the CPU being late feeding the FIFO. This, again, is a hypothesis I have; it hasn't been confirmed.
 
## Raw output
 
First baseline (original code, before any optimization):
 
```
-- baseline (avg of 20 runs) --
draw_pixel: 25 us
draw_char (35 px): 865 us
draw_char_scaled x3 (315): 7763 us
draw_rect 30x30 (900 px): 22107 us
```
 
Fill-screen versions (the third column is the 16-bit-frame version, run at 7.8125 MHz; I mislabelled it as "16MHz" in the printout :/ ):
 
```
-- fill_screen benchmark tests--
Actual SPI baud rate: 7812500 Hz
run 0: slow 74446 us | fast 50037 us | 16MHz 46113 us
run 1: slow 74405 us | fast 50026 us | 16MHz 46094 us
run 2: slow 74405 us | fast 50026 us | 16MHz 46094 us
run 3: slow 74405 us | fast 50026 us | 16MHz 46094 us
run 4: slow 74405 us | fast 50026 us | 16MHz 46094 us
```
 
Final code, side by side with the reference implementation:
 
```
-- slow vs fast --
fill_screen: 74413 -> 46092 us (1.6x)
draw_char: 865 -> 104 us (8.3x)
draw_char_scaled3: 7763 -> 749 us (10.4x)
draw_rect 30x30: 22108 -> 2072 us (10.7x)
string (21 chars): per-pixel 18157 us | draw_char loop 2118 us | draw_string 2147 us
draw_string: 9781 chars/sec, ideal 1818 us, 84.7% efficient
```
 
Clock sweep:
 
```
-- clock sweep --
req 8000000 -> actual 7812500 Hz: fill 46101 us (ideal 41954 us, 91.0%)
req 16000000 -> actual 15625000 Hz: fill 23156 us (ideal 20977 us, 90.6%)
req 21000000 -> actual 20833333 Hz: fill 17425 us (ideal 15733 us, 90.3%)
req 32000000 -> actual 31250000 Hz: fill 11689 us (ideal 10489 us, 89.7%)
req 63000000 -> actual 62500000 Hz: fill 5908 us (ideal 5244 us, 88.8%)
```
 
## Experiments not reported
 
- **Comparison with Adafruit's ST7735 library** (Arduino IDE, arduino-pico core): `fillScreen` took about 493.5 ms at every SPI clock setting from 1 to 16 MHz. A time that does not respond to the clock means the comparison was not measuring what it was meant to, so I couldn't draw any conclusion.
- **A control test of the Arduino SPI call** returned about 64 us for a transfer that needs about 21 ms on the wire, so the call returned before the data finished. It was discarded.
## Limitations
 
- One display module, one board, and one wiring setup. The maximum reliable clock depends on all three.
- Timings come from the on-chip timer only; bus timing was not verified with external instruments.
- `draw_pixel` and `draw_filled_circle` use the single-pixel path and are not covered by the efficiency figures.
- The ideal time counts only the wire time for pixel data and the address window; it ignores the reset and initialization time.
## Reproducing
 
Build `examples/benchmark`, flash it, and read the serial output at 115200 baud over USB. The program prints measured time, ideal time, efficiency and microseconds per pixel for each operation. Numbers within roughly 10 to 20 us of the table above are expected on the same hardware and SDK version.

