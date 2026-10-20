#!/bin/sh
# Decompile the functions that contain the given addresses with Ghidra.
#
# The loader extension must be installed first:
#
#     nix develop .#ghidra --command sh tools/ghidra/install_loader.sh
#
# Then decompile a function:
#
#     tools/ghidra/decompile.sh 0x10020 0x2ecf8
#
# The script runs Ghidra headless on the installed game executable, imports it
# again, and prints the decompiled C of each function that contains an address.
# The project stays under `work/`, so the shipped artifacts do not change.

set -e

EXE="${SCREAMER_EXE:-$HOME/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE}"
PROJECT_DIR="work/ghidra/proj"
PROJECT_NAME="Screamer"
OUTPUT="${SCREAMER_GHIDRA_OUT:-work/ghidra/decompiled.txt}"

if [ "$#" -eq 0 ]; then
    echo "usage: $0 <address> ..." >&2
    exit 2
fi
if [ ! -f "$EXE" ]; then
    echo "no executable at $EXE; set SCREAMER_EXE" >&2
    exit 1
fi

mkdir -p "$PROJECT_DIR" "$(dirname "$OUTPUT")"
nix develop .#ghidra --command analyzeHeadless \
    "$PROJECT_DIR" "$PROJECT_NAME" \
    -import "$EXE" \
    -overwrite \
    -scriptPath tools/ghidra \
    -postScript DecompileFunctions.java "$@" 2>&1 |
    sed -n '/=== 0x/,$p' | sed 's/^INFO  DecompileFunctions.java> //' > "$OUTPUT"
echo "wrote $OUTPUT"
grep '^=== ' "$OUTPUT" || true
