# Move Everything - Freak Module

`Freak` is a MicroFreak-inspired sound generator for [Move Anything](https://github.com/charlesvestal/move-anything), built on Mutable Instruments Plaits core DSP.

## Highlights

- Uses vendored upstream `plaits` + `stmlib` as synthesis core
- `Main` submenu for core macro params (`Model`, `Pitch`, `Harmonics`, `Timbre`, `Morph`, `FM Amount`, `Aux Mix`, `LPG Decay`, `LPG Color`)
- Single `Mod` submenu with pages for `Pitch`, `Harmonics`, `Timbre`, `Cutoff`, `Assign 1`, and `Assign 2`
- Full destination-based modulation sources:
  - LFO, Envelope, Cycling Envelope, Random, Velocity, Poly Aftertouch
- Global multimode post filter submenu (`LP/BP/HP`, `Cutoff`, `Resonance`)
- Voice controls: mono/poly/mono legato, polyphony, unison, detune, spread, glide
- Mono + mono-legato support unison stacks (single-note behavior, no chords)
- Active modulation pages are marked with `*` in the `Mod` submenu
- `Aux Mix` blends Plaits outputs (`0=main`, `0.5=equal`, `1=aux`)
- Assign targets include `volume` and `pan` (in addition to macro/filter/pitch targets)
- Audio release tail timing follows LPG decay (not ADSR release time)

## UI Map

Root levels:

- `Main`
- `Filter`
- `Mod`
- `Mod Sources`
- `Voice`

Root knob mapping stays fixed to:

- `model`, `harmonics`, `timbre`, `morph`, `fm_amount`, `lpg_decay`, `lpg_color`, `filter_cutoff`

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
bash tests/run_all.sh
```

## Licensing

Plaits and stmlib code is MIT-licensed by Emilie Gillet (Mutable Instruments).  
This repository keeps upstream license headers in vendored files.
