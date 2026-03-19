#!/usr/bin/env bash
# Build MrHyde module for Move Anything (ARM64)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
IMAGE_NAME="move-anything-builder"

if [[ "${RUN_CHECKS:-1}" == "1" && ! -f "/.dockerenv" ]]; then
    echo "=== Running verification checks ==="
    bash "$REPO_ROOT/tests/run_all.sh"
    echo ""
fi

if [[ -z "${CROSS_PREFIX:-}" && ! -f "/.dockerenv" ]]; then
    echo "=== MrHyde Build (via Docker) ==="
    echo ""

    if ! docker image inspect "$IMAGE_NAME" &>/dev/null; then
        echo "Building Docker image (first time only)..."
        docker build -t "$IMAGE_NAME" -f "$SCRIPT_DIR/Dockerfile" "$REPO_ROOT"
        echo ""
    fi

    echo "Running build..."
    docker run --rm \
        -v "$REPO_ROOT:/build" \
        -u "$(id -u):$(id -g)" \
        -w /build \
        "$IMAGE_NAME" \
        ./scripts/build.sh

    echo ""
    echo "=== Done ==="
    exit 0
fi

CROSS_PREFIX="${CROSS_PREFIX:-aarch64-linux-gnu-}"
cd "$REPO_ROOT"

echo "=== Building MrHyde Module ==="
echo "Cross prefix: $CROSS_PREFIX"

MODULE_DIR="dist/freak"
TARBALL="dist/freak-module.tar.gz"

mkdir -p build dist
rm -rf "$MODULE_DIR"
mkdir -p "$MODULE_DIR"

COMMON_FLAGS=(
    -g -O3 -shared -fPIC -std=c++14 -DTEST
    -include stdio.h
    -Isrc
    -Isrc/dsp
    -Isrc/third_party/eurorack
)

PLAITS_SOURCES=(
    src/third_party/eurorack/plaits/dsp/engine/additive_engine.cc
    src/third_party/eurorack/plaits/dsp/engine/bass_drum_engine.cc
    src/third_party/eurorack/plaits/dsp/engine/chord_engine.cc
    src/third_party/eurorack/plaits/dsp/engine/fm_engine.cc
    src/third_party/eurorack/plaits/dsp/engine/grain_engine.cc
    src/third_party/eurorack/plaits/dsp/engine/hi_hat_engine.cc
    src/third_party/eurorack/plaits/dsp/engine/modal_engine.cc
    src/third_party/eurorack/plaits/dsp/engine/noise_engine.cc
    src/third_party/eurorack/plaits/dsp/engine/particle_engine.cc
    src/third_party/eurorack/plaits/dsp/engine/snare_drum_engine.cc
    src/third_party/eurorack/plaits/dsp/engine/speech_engine.cc
    src/third_party/eurorack/plaits/dsp/engine/string_engine.cc
    src/third_party/eurorack/plaits/dsp/engine/swarm_engine.cc
    src/third_party/eurorack/plaits/dsp/engine/virtual_analog_engine.cc
    src/third_party/eurorack/plaits/dsp/engine/waveshaping_engine.cc
    src/third_party/eurorack/plaits/dsp/engine/wavetable_engine.cc
    src/third_party/eurorack/plaits/dsp/engine2/chiptune_engine.cc
    src/third_party/eurorack/plaits/dsp/engine2/phase_distortion_engine.cc
    src/third_party/eurorack/plaits/dsp/engine2/six_op_engine.cc
    src/third_party/eurorack/plaits/dsp/engine2/string_machine_engine.cc
    src/third_party/eurorack/plaits/dsp/engine2/virtual_analog_vcf_engine.cc
    src/third_party/eurorack/plaits/dsp/engine2/wave_terrain_engine.cc
    src/third_party/eurorack/plaits/dsp/chords/chord_bank.cc
    src/third_party/eurorack/plaits/dsp/fm/algorithms.cc
    src/third_party/eurorack/plaits/dsp/fm/dx_units.cc
    src/third_party/eurorack/plaits/dsp/physical_modelling/modal_voice.cc
    src/third_party/eurorack/plaits/dsp/physical_modelling/resonator.cc
    src/third_party/eurorack/plaits/dsp/physical_modelling/string.cc
    src/third_party/eurorack/plaits/dsp/physical_modelling/string_voice.cc
    src/third_party/eurorack/plaits/dsp/speech/lpc_speech_synth.cc
    src/third_party/eurorack/plaits/dsp/speech/lpc_speech_synth_controller.cc
    src/third_party/eurorack/plaits/dsp/speech/lpc_speech_synth_phonemes.cc
    src/third_party/eurorack/plaits/dsp/speech/lpc_speech_synth_words.cc
    src/third_party/eurorack/plaits/dsp/speech/naive_speech_synth.cc
    src/third_party/eurorack/plaits/dsp/speech/sam_speech_synth.cc
    src/third_party/eurorack/plaits/dsp/voice.cc
    src/third_party/eurorack/plaits/resources.cc
    src/third_party/eurorack/plaits/user_data_receiver.cc
    src/third_party/eurorack/stm_audio_bootloader/fsk/packet_decoder.cc
    src/third_party/eurorack/stmlib/utils/random.cc
    src/third_party/eurorack/stmlib/dsp/units.cc
)

echo "Compiling DSP plugin..."
"${CROSS_PREFIX}g++" "${COMMON_FLAGS[@]}" \
    src/dsp/freak_plugin.cpp \
    src/dsp/plaits_move_engine.cpp \
    "${PLAITS_SOURCES[@]}" \
    -o build/dsp.so \
    -lm

echo "Packaging..."
cp src/module.json "$MODULE_DIR/module.json"
cp build/dsp.so "$MODULE_DIR/dsp.so"
chmod +x "$MODULE_DIR/dsp.so"
cp README.md "$MODULE_DIR/README.md"
cp src/help.json "$MODULE_DIR/help.json"
cp src/ui.js "$MODULE_DIR/ui.js"

rm -f "$TARBALL"
(
  cd dist
  tar -czf "$(basename "$TARBALL")" "$(basename "$MODULE_DIR")"
)

echo ""
echo "=== Build Complete ==="
echo "Output: $MODULE_DIR/"
echo "Tarball: $TARBALL"
echo ""
echo "To install on Move:"
echo "  ./scripts/install.sh"
