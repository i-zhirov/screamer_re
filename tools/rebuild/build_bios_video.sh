#!/bin/sh
# Build the rebuilt `bios_video` module with the pinned OpenWatcom V2 container.
#
# The module is `bios_video` at linear 0x108e4 in STARTH.EXE, 22 instructions.  The build
# writes work/rebuild/bvmod.exe and work/rebuild/bios_video.map.
#
# Usage:
#     sh tools/rebuild/build_bios_video.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/tools/rebuild"
OUT="$ROOT/work/rebuild"
IMAGE="arlaneenalra/watcom-docker@sha256:16e4845ddd0df7bc877b45d103aacf66bc48aaab6d1c61d6bcaa9c1274f06a65"

mkdir -p "$OUT"
cp "$SRC/bios_video.c" "$SRC/bios_video_main.c" "$SRC/chk.asm" "$SRC/stack.asm" \
    "$SRC/cstart.c" "$OUT/"

docker run --rm -v "$OUT:/w" -w /w "$IMAGE" sh -c '
set -e
wasm -bt=dos -zq -zcm=watcom -fo=/w/chk.obj chk.asm
wasm -bt=dos -zq -zcm=watcom -fo=/w/stack.obj stack.asm
wcc386 -bt=dos -s -zq -fo=/w/cstart.obj cstart.c
wcc386 -bt=dos -s -zq -fo=/w/bios_video_main.obj bios_video_main.c
wcc386 -bt=dos -s -zq -of+ -fo=/w/bios_video.obj bios_video.c
wlink system dos4g option quiet option nodefaultlibs option start=_cstart_ \
    option map=/w/bios_video.map name bvmod.exe \
    file bios_video.obj,bios_video_main.obj,chk.obj,cstart.obj,stack.obj
'
ls -la "$OUT/bvmod.exe"
