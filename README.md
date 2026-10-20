# HEX-Q VANTA

Lightweight six-band parametric EQ (VST3, Windows 64-bit) built with C++ / JUCE.
First product of the HEX-Q plugin family.

## Diagnostic build (v1.0.1)

This build writes a step-by-step trace to `%TEMP%\HEX-Q-VANTA-log.txt` (press Win+R, type `%TEMP%`, open the file).
If FL Studio reports a crash, the last lines of that file show where it happened. The trace is for troubleshooting
and will be removed once the plugin is stable.

## Status - read this first

| Item | Status |
|---|---|
| DSP maths + EQ engine (`Source/DSP`) | **Compiled and tested offline** (g++, 31/31 checks pass, see below) |
| JUCE plugin, editor, parameters | **Written but NOT compiled or tested yet** - first build may show errors |
| Running inside FL Studio | **Not tested** |

Nothing in this repository claims more than that. Fix compile errors first, then work through the checklist below.

## Build (no compiler on your PC needed)

1. Create a free GitHub account and a new repository.
2. Upload **all files of this folder to the repository root** (`CMakeLists.txt` must be visible on the repo's front page; keep the `.github` folder).
3. Open the **Actions** tab, run **Build HEX-Q VANTA (Windows)** (~10 min; also starts on every upload).
4. Download the artifact **HEX-Q-VANTA-Windows**: it holds `HEX-Q-VANTA-Setup.exe` and a manual-install zip.

### Local build (optional)
Requirements: Windows 10/11, Visual Studio 2022 (Desktop C++), CMake 3.22+, Git.
```
cmake -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
JUCE 8.0.4 is downloaded automatically by CMake. Output: `build/HexQVanta_artefacts/Release/VST3/HEX-Q VANTA.vst3`

## Install in FL Studio
1. Run `HEX-Q-VANTA-Setup.exe` (Windows SmartScreen may warn because the installer is unsigned: More info -> Run anyway).
   Manual alternative: copy the `HEX-Q VANTA.vst3` folder to `C:\Program Files\Common Files\VST3\`.
2. FL Studio: Options -> Manage plugins -> Start scan.
3. Add **HEX-Q VANTA** to a mixer insert slot (Effects).

## Using it
- Click a node (1-6) or a band button to select a band.
- **Drag a node**: horizontal = frequency, vertical = gain. **Shift** = fine.
- **Mouse wheel over a node**: Q. Q also has its own field.
- **Double-click a node**: band on/off. **BAND ON** button does the same.
- Fields (FREQ / GAIN / Q): drag up/down, mouse wheel, click the number to type (`1.2k`, `-3`), double-click to reset.
- **BYPASS** (header): clean A/B, 15 ms crossfade. **RESET ALL**: every parameter back to default.
- The curve is the exact combined response of the enabled bands (same coefficients as the audio engine). There is no spectrum analyser.

## Defaults
| Band | Type | Freq | Gain | Q | Default state |
|---|---|---|---|---|---|
| 1 | High-Pass | 25 Hz | - | 0.707 | **off** |
| 2 | Low Shelf | 120 Hz | 0 dB | 0.707 | on |
| 3 | Bell | 220 Hz | 0 dB | 1.0 | on |
| 4 | Bell | 500 Hz | 0 dB | 1.0 | on |
| 5 | High Shelf | 8 kHz | 0 dB | 0.707 | on |
| 6 | Low-Pass | 20 kHz | - | 0.707 | **off** |

Bands 1 and 6 start switched off: a 25 Hz high-pass is not neutral, and the spec requires no audible change on load. Any band can be any type (High-Pass, Low Shelf, Bell, High Shelf, Low-Pass). Ranges: 20 Hz-20 kHz (log), +/-18 dB, Q 0.1-10.

## Architecture
```
Source/
  DSP/         Biquad.h, EqEngine.h      pure C++, no JUCE, no allocation  (unit-tested)
  Parameters/  VantaParams.*             IDs, ranges, defaults, text formatting
  HexQ/        shared design system      Theme (palette/fonts), LookAndFeel, Header, Footer
  Vanta/       VANTA-only UI             ResponseGraph, BandButton
  PluginProcessor.*  PluginEditor.*
tests/dsp_test.cpp                       offline DSP self-test
```
- Filters: RBJ cookbook biquads, transposed direct form II, double precision state.
- Smoothing: frequency / Q (log domain) and gain are smoothed (10 ms) and coefficients are refreshed every 32 samples. Band on/off and type changes use a 12 ms fade. Bell/shelf bands at exactly 0 dB are skipped (zero CPU). Fully bypassed = zero CPU.
- No latency, no oversampling, no FFT. Audio thread: no allocation (buffers pre-sized in `prepareToPlay`, parameter pointers cached).
- Graph: redraws only when a parameter or selection changes (AsyncUpdater), never from the audio thread.

### Reusing the design system for the next HEX-Q plugin
Copy `Source/HexQ/`. Create a `hexq::Palette` and change only `accent`; build the editor from `HexQHeader`, `HexQFooter` and `HexQLookAndFeel`. All colours live in `HexQTheme.h`.

## Test report

### Verified offline (`tests/dsp_test.cpp`, g++ 13, -O2) - 31 / 31 passed
- Default settings: output bit-identical to input; 0 dB bell/shelf is flat.
- All five filter types match their analytic response (peak, cutoff, shelf plateau, slopes).
- Frequency / gain / Q limits clamp correctly; 35,000 combinations (7 sample rates 22.05-192 kHz x types x freq x gain x Q) are stable.
- Engine output measured with sine sweeps matches the analytic curve (single bell and all six bands, error < 0.1 dB).
- No clicks on gain jumps, fast frequency sweeps, band on/off and type change.
- Identical results for host block sizes 1 ... 4096; mono = stereo path; channels independent.
- Random automation at 6 sample rates: finite and bounded. Exact zero output after silence (no denormal tails).
- **0 heap allocations** while processing; CPU about 0.2 % of one core for 6 active bands, stereo, 48 kHz (this machine - a rough indication only).

### Still to be verified on your machine (not tested)
- [ ] Plugin compiles with JUCE (CI build is green)
- [ ] Loads in FL Studio, appears under Effects, no crash on open/close/re-open
- [ ] Mono and stereo tracks
- [ ] Host automation of band parameters (right-click a knob -> Link to controller / automation clip)
- [ ] Save project, reopen: all settings restored (including bypass and band on/off)
- [ ] Global bypass: no click, A/B level matches
- [ ] Sample rates 44.1 / 48 / 96 kHz and buffer sizes 64-1024 in FL
- [ ] Real CPU use with several instances (FL's CPU meter)
- [ ] UI: node dragging feel, readability at your screen scaling
