#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$REPO_ROOT"

if [[ ! -d "dist/freak" ]]; then
    echo "Error: dist/freak not found. Run ./scripts/build.sh first."
    exit 1
fi

TARGET_DIR="/data/UserData/move-anything/modules/sound_generators/freak"

echo "=== Installing MrHyde Module ==="
echo "Copying module to Move..."
ssh ableton@move.local "mkdir -p ${TARGET_DIR}"
scp -r dist/freak/* "ableton@move.local:${TARGET_DIR}/"

echo "Setting permissions..."
ssh ableton@move.local "chmod -R a+rw ${TARGET_DIR}"

echo ""
echo "=== Install Complete ==="
echo "Module installed to: ${TARGET_DIR}/"
echo "Restart Move Anything to load the module."
