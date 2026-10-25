#!/bin/sh
# Build the rebuilt `exit` module with the pinned OpenWatcom V2 container.
#
# The module is the next named function in the program map: `exit` at linear
# 0x300be in STARTH.EXE with its helper at 0x300d6.  The build writes
# work/rebuild/exitmod.exe and work/rebuild/exit.map, and the map file carries
# the addresses of the rebuilt functions for the diff.  The program carries the
# name `exitmod.exe`, because the DOS shell reads `exit` as its own command.
#
# Usage:
#     sh tools/rebuild/build_exit.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/tools/rebuild"
OUT="$ROOT/work/rebuild"
IMAGE="arlaneenalra/watcom-docker@sha256:16e4845ddd0df7bc877b45d103aacf66bc48aaab6d1c61d6bcaa9c1274f06a65"

mkdir -p "$OUT"
cp "$SRC/exit.c" "$SRC/exit_main.c" "$SRC/chk.asm" "$SRC/stack.asm" \
    "$SRC/cstart.c" "$OUT/"

docker run --rm -v "$OUT:/w" -w /w "$IMAGE" sh -c '
set -e
wasm -bt=dos -zq -zcm=watcom -fo=/w/chk.obj chk.asm
wasm -bt=dos -zq -zcm=watcom -fo=/w/stack.obj stack.asm
wcc386 -bt=dos -s -zq -fo=/w/cstart.obj cstart.c
wcc386 -bt=dos -s -zq -fo=/w/exit_main.obj exit_main.c
wcc386 -bt=dos -s -zq -fo=/w/exit.obj exit.c
wlink system dos4g option quiet option nodefaultlibs option start=_cstart_ \
    option map=/w/exit.map name exitmod.exe \
    file exit.obj,exit_main.obj,chk.obj,cstart.obj,stack.obj
'
ls -la "$OUT/exitmod.exe"
