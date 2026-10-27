#!/bin/sh
# Build the rebuilt `engine_shutdown` module with the pinned OpenWatcom V2 container.
#
# The module is `engine_shutdown` at linear 0x128ac in STARTH.EXE, 14 instructions.
# The module needs the Watcom stack-frame option `-of+`, because the original game files
# carry an EBP frame.  The build writes work/rebuild/shutmod.exe and
# work/rebuild/shutdown.map.
#
# Usage:
#     sh tools/rebuild/build_shutdown.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/tools/rebuild"
OUT="$ROOT/work/rebuild"
IMAGE="arlaneenalra/watcom-docker@sha256:16e4845ddd0df7bc877b45d103aacf66bc48aaab6d1c61d6bcaa9c1274f06a65"

mkdir -p "$OUT"
cp "$SRC/shutdown.c" "$SRC/shutdown_main.c" "$SRC/chk.asm" "$SRC/stack.asm" \
    "$SRC/cstart.c" "$OUT/"

docker run --rm -v "$OUT:/w" -w /w "$IMAGE" sh -c '
set -e
wasm -bt=dos -zq -zcm=watcom -fo=/w/chk.obj chk.asm
wasm -bt=dos -zq -zcm=watcom -fo=/w/stack.obj stack.asm
wcc386 -bt=dos -s -zq -fo=/w/cstart.obj cstart.c
wcc386 -bt=dos -s -zq -fo=/w/shutdown_main.obj shutdown_main.c
wcc386 -bt=dos -s -zq -of+ -fo=/w/shutdown.obj shutdown.c
wlink system dos4g option quiet option nodefaultlibs option start=_cstart_ \
    option map=/w/shutdown.map name shutmod.exe \
    file shutdown.obj,shutdown_main.obj,chk.obj,cstart.obj,stack.obj
'
ls -la "$OUT/shutmod.exe"
