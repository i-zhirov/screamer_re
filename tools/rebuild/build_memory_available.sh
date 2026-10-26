#!/bin/sh
# Build the rebuilt `memory_available` module with the pinned OpenWatcom V2 container.
#
# The module is `memory_available` at linear 0x2f8ca in STARTH.EXE, 27 instructions.
# The module needs the stack-frame option `-of+` and the unoptimized mode `-od`, which give
# the closest shape.  The build writes work/rebuild/mamod.exe and
# work/rebuild/memory_available.map.
#
# Usage:
#     sh tools/rebuild/build_memory_available.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/tools/rebuild"
OUT="$ROOT/work/rebuild"
IMAGE="arlaneenalra/watcom-docker@sha256:16e4845ddd0df7bc877b45d103aacf66bc48aaab6d1c61d6bcaa9c1274f06a65"

mkdir -p "$OUT"
cp "$SRC/memory_available.c" "$SRC/memory_available_main.c" "$SRC/chk.asm" \
    "$SRC/stack.asm" "$SRC/cstart.c" "$OUT/"

docker run --rm -v "$OUT:/w" -w /w "$IMAGE" sh -c '
set -e
wasm -bt=dos -zq -zcm=watcom -fo=/w/chk.obj chk.asm
wasm -bt=dos -zq -zcm=watcom -fo=/w/stack.obj stack.asm
wcc386 -bt=dos -s -zq -fo=/w/cstart.obj cstart.c
wcc386 -bt=dos -s -zq -fo=/w/memory_available_main.obj memory_available_main.c
wcc386 -bt=dos -s -zq -of+ -od -fo=/w/memory_available.obj memory_available.c
wlink system dos4g option quiet option nodefaultlibs option start=_cstart_ \
    option map=/w/memory_available.map name mamod.exe \
    file memory_available.obj,memory_available_main.obj,chk.obj,cstart.obj,stack.obj
'
ls -la "$OUT/mamod.exe"
