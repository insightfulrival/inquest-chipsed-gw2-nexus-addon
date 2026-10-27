#!/usr/bin/env bash
# Build on Fedora/Linux for Nexus running inside GW2's Proton process.
set -euo pipefail
cd -- "$(dirname -- "$0")"
compiler="${ZIG:-zig}"
if ! command -v "$compiler" >/dev/null 2>&1; then
  echo 'Zig is required. Put Zig 0.14.1 on PATH or set ZIG=/absolute/path/to/zig.' >&2
  exit 1
fi
mkdir -p dist
"$compiler" c++ -target x86_64-windows-gnu -std=c++17 -O2 -shared -static -Wno-format-security \
  -Ivendor/imgui inquest.cpp vendor/imgui/imgui.cpp vendor/imgui/imgui_draw.cpp vendor/imgui/imgui_widgets.cpp vendor/imgui/imgui_tables.cpp \
  -luser32 -lgdi32 -limm32 -lwinmm -o dist/InquestChipset.dll
