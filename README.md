# Move Everything - Freak Module

`Freak` is a MicroFreak-inspired sound generator for [Move Anything](https://github.com/charlesvestal/move-anything), built on Mutable Instruments Plaits core DSP.

## Highlights

- Uses vendored upstream `plaits` + `stmlib` as synthesis core
- Direct top-level Plaits params (`Pitch`, `Harmonics`, `Timbre`, `Morph`, `FM Amount`)
- Single `Mod` submenu with pages for `Assign 1`, `Assign 2`, `Pitch`, `Harmonics`, `Timbre`, and `Cutoff`
- Full destination-based modulation sources:
  - LFO, Envelope, Cycling Envelope, Random, Velocity, Poly Aftertouch
- Global multimode post filter submenu (`LP/BP/HP`, `Cutoff`, `Resonance`)
- Voice controls: mono/poly/mono legato, polyphony, unison, detune, spread, glide
- Top-level LPG controls (`Decay`, `Color`) and active-mod page star indicators in UI

## Layout

- DSP wrapper: `src/dsp/freak_plugin.cpp`, `src/dsp/plaits_move_engine.{h,cpp}`
- Vendored upstream source:
  - `src/third_party/eurorack/plaits`
  - `src/third_party/eurorack/stmlib`
  - `src/third_party/eurorack/stm_audio_bootloader/fsk`

## Build

```bash
./scripts/build.sh
```

Refresh vendored upstream Plaits first (optional):

```bash
./scripts/sync_plaits_upstream.sh
```

Artifacts:

- `dist/freak/`
- `dist/freak-module.tar.gz`

## Install to Move

```bash
./scripts/install.sh
```

Install target:

`/data/UserData/move-anything/modules/sound_generators/freak/`

## Verify

Run checks before building/releasing:

```bash
bash tests/plaits_vendor_layout_test.sh
g++ -std=c++14 -Isrc/dsp tests/plaits_mod_matrix_test.cpp -o /tmp/plaits_mod_matrix_test && /tmp/plaits_mod_matrix_test
bash tests/run_plaits_plugin_smoke.sh
python3 tests/module_metadata_test.py
```

## Licensing

Plaits and stmlib code is MIT-licensed by Emilie Gillet (Mutable Instruments).  
This repository keeps upstream license headers in vendored files.
