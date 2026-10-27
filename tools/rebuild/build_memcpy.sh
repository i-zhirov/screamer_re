#!/bin/sh
# Build the rebuilt `memcpy` module with the pinned OpenWatcom V2 container.
#
# The module is `memcpy` at linear 0x8c150 in STARTH.EXE, 14 bytes with no call and no
# data reference.  The module is assembly, because the original routine is assembly too.
# The build writes work/rebuild/memmod.exe and work/rebuild/memcpy.map.
#
# Usage:
#     sh tools/rebuild/build_memcpy.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/tools/rebuild"
OUT="$ROOT/work/rebuild"
IMAGE="arlaneenalra/watcom-docker@sha256:16e4845ddd0df7bc877b45d103aacf66bc48aaab6d1c61d6bcaa9c1274f06a65"

mkdir -p "$OUT"
cp "$SRC/memcpy.asm" "$SRC/memcpy_main.c" "$SRC/chk.asm" "$SRC/stack.asm" \
    "$SRC/cstart.c" "$OUT/"

docker run --rm -v "$OUT:/w" -w /w "$IMAGE" sh -c '
set -e
wasm -bt=dos -zq -zcm=watcom -fo=/w/chk.obj chk.asm
wasm -bt=dos -zq -zcm=watcom -fo=/w/stack.obj stack.asm
wasm -bt=dos -zq -zcm=watcom -fo=/w/memcpy.obj memcpy.asm
wcc386 -bt=dos -s -zq -fo=/w/cstart.obj cstart.c
wcc386 -bt=dos -s -zq -fo=/w/memcpy_main.obj memcpy_main.c
wlink system dos4g option quiet option nodefaultlibs option start=_cstart_ \
    option map=/w/memcpy.map name memmod.exe \
    file memcpy.obj,memcpy_main.obj,chk.obj,cstart.obj,stack.obj
'
ls -la "$OUT/memmod.exe"
