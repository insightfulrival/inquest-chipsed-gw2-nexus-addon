#!/usr/bin/env bash
# Download the pinned Linux compiler locally, build, and install into Nexus.
set -euo pipefail
cd -- "$(dirname -- "$0")"
if [[ "$(uname -s)" != Linux || "$(uname -m)" != x86_64 ]]; then
  echo 'This setup script supports x86_64 Linux only.' >&2
  exit 1
fi
build_only=false
install_args=()
while (($#)); do
  case "$1" in
    --build-only) build_only=true; shift ;;
    --game)
      if (($# < 2)); then echo '--game requires a directory.' >&2; exit 1; fi
      install_args+=(--game "$2"); shift 2 ;;
    -h|--help)
      echo 'Usage: ./setup-fedora.sh [--game "/path/to/Guild Wars 2"] [--build-only]'
      exit 0 ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done
for dependency in python3 curl tar xz sha256sum; do
  if ! command -v "$dependency" >/dev/null 2>&1; then
    echo 'Install prerequisites: sudo dnf install git python3 curl tar xz coreutils' >&2
    exit 1
  fi
done
version=0.14.1
if [[ -z "${ZIG:-}" ]]; then
  if command -v zig >/dev/null 2>&1 && [[ "$(zig version)" == "$version" ]]; then
    ZIG="$(command -v zig)"
  else
    tool_dir="$PWD/.tools/zig-x86_64-linux-$version"
    if [[ ! -x "$tool_dir/zig" ]]; then
      mkdir -p .tools
      archive="$(mktemp "$PWD/.tools/zig-download.XXXXXXXX.tar.xz")"
      trap 'rm -f -- "$archive"' EXIT
      curl --fail --location --retry 3 --connect-timeout 30 --output "$archive" \
        "https://ziglang.org/download/$version/zig-x86_64-linux-$version.tar.xz"
      printf '%s  %s\n' '24aeeec8af16c381934a6cd7d95c807a8cb2cf7df9fa40d359aa884195c4716c' "$archive" | sha256sum --check --status
      tar -xJf "$archive" -C .tools
    fi
    ZIG="$tool_dir/zig"
  fi
fi
export ZIG
if [[ "$("$ZIG" version)" != "$version" ]]; then
  echo 'This project is tested with Zig 0.14.1. Select it with ZIG=/path/to/zig.' >&2
  exit 1
fi
./build.sh
if $build_only; then
  echo 'Built dist/InquestChipset.dll. Install later with: python3 install.py'
else
  python3 install.py "${install_args[@]}"
fi
