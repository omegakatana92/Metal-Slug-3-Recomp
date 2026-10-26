#!/usr/bin/env bash
# Build the Linux x86_64 player.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ROMS="${1:-$ROOT/roms-src}"
cmake -S "$ROOT" -B "$ROOT/build-linux" -DCMAKE_BUILD_TYPE=Release "-DMSLUG3_ROM_DIR=$ROMS"
cmake --build "$ROOT/build-linux" --parallel "$(nproc)"
echo "built $ROOT/build-linux/Metal_Slug_3"
