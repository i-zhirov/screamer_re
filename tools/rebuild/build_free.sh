#!/bin/sh
# Build the rebuilt `free` module with the pinned OpenWatcom V2 container.
#
# The module is the smallest named function in the program map: `free` at linear
# 0x300ec in STARTH.EXE, three instructions.  The build writes
# work/rebuild/free.exe and work/rebuild/free.map, and the map file carries the
# address of the rebuilt module for the diff.
#
# Usage:
#     sh tools/rebuild/build_free.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/tools/rebuild"
OUT="$ROOT/work/rebuild"
IMAGE="arlaneenalra/watcom-docker@sha256:16e4845ddd0df7bc877b45d103aacf66bc48aaab6d1c61d6bcaa9c1274f06a65"

mkdir -p "$OUT"
cp "$SRC/free.c" "$SRC/free_main.c" "$SRC/chk.asm" "$SRC/stack.asm" \
    "$SRC/cstart.c" "$OUT/"

docker run --rm -v "$OUT:/w" -w /w "$IMAGE" sh -c '
set -e
wasm -bt=dos -zq -zcm=watcom -fo=/w/chk.obj chk.asm
wasm -bt=dos -zq -zcm=watcom -fo=/w/stack.obj stack.asm
wcc386 -bt=dos -s -zq -fo=/w/cstart.obj cstart.c
wcc386 -bt=dos -s -zq -fo=/w/free_main.obj free_main.c
wcc386 -bt=dos -s -zq -fo=/w/free.obj free.c
wlink system dos4g option quiet option nodefaultlibs option start=_cstart_ \
    option map=/w/free.map name free.exe \
    file free.obj,free_main.obj,chk.obj,cstart.obj,stack.obj
'
ls -la "$OUT/free.exe"
