#!/bin/sh
# Build the rebuilt `list_destroy` module with the pinned OpenWatcom V2 container.
#
# The module is `list_destroy` at linear 0x2ed77 in STARTH.EXE, 25 instructions.  The
# module needs the stack-frame option `-of+`, because the original game files carry an EBP
# frame.  The build writes work/rebuild/ldmod.exe and work/rebuild/list_destroy.map.
#
# Usage:
#     sh tools/rebuild/build_list_destroy.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/tools/rebuild"
OUT="$ROOT/work/rebuild"
IMAGE="arlaneenalra/watcom-docker@sha256:16e4845ddd0df7bc877b45d103aacf66bc48aaab6d1c61d6bcaa9c1274f06a65"

mkdir -p "$OUT"
cp "$SRC/list_destroy.c" "$SRC/list_destroy_main.c" "$SRC/chk.asm" "$SRC/stack.asm" \
    "$SRC/cstart.c" "$OUT/"

docker run --rm -v "$OUT:/w" -w /w "$IMAGE" sh -c '
set -e
wasm -bt=dos -zq -zcm=watcom -fo=/w/chk.obj chk.asm
wasm -bt=dos -zq -zcm=watcom -fo=/w/stack.obj stack.asm
wcc386 -bt=dos -s -zq -fo=/w/cstart.obj cstart.c
wcc386 -bt=dos -s -zq -fo=/w/list_destroy_main.obj list_destroy_main.c
wcc386 -bt=dos -s -zq -of+ -od -fo=/w/list_destroy.obj list_destroy.c
wlink system dos4g option quiet option nodefaultlibs option start=_cstart_ \
    option map=/w/list_destroy.map name ldmod.exe \
    file list_destroy.obj,list_destroy_main.obj,chk.obj,cstart.obj,stack.obj
'
ls -la "$OUT/ldmod.exe"
