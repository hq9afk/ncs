#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

if [ ! -d build ]; then
    meson setup build
fi

meson compile -C build
