#!/bin/sh
# Build the rebuilt `error_report` module with the pinned OpenWatcom V2 container.
#
# The module is `error_report` at linear 0x10978 in STARTH.EXE, the first game module
# of the rebuild.  The build writes work/rebuild/errmod.exe and work/rebuild/error_report.map.
#
# Usage:
#     sh tools/rebuild/build_error_report.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/tools/rebuild"
OUT="$ROOT/work/rebuild"
IMAGE="arlaneenalra/watcom-docker@sha256:16e4845ddd0df7bc877b45d103aacf66bc48aaab6d1c61d6bcaa9c1274f06a65"

mkdir -p "$OUT"
cp "$SRC/error_report.c" "$SRC/error_report_main.c" "$SRC/chk.asm" \
    "$SRC/stack.asm" "$SRC/cstart.c" "$OUT/"

docker run --rm -v "$OUT:/w" -w /w "$IMAGE" sh -c '
set -e
wasm -bt=dos -zq -zcm=watcom -fo=/w/chk.obj chk.asm
wasm -bt=dos -zq -zcm=watcom -fo=/w/stack.obj stack.asm
wcc386 -bt=dos -s -zq -fo=/w/cstart.obj cstart.c
wcc386 -bt=dos -s -zq -fo=/w/error_report_main.obj error_report_main.c
wcc386 -bt=dos -s -zq -fo=/w/error_report.obj error_report.c
wlink system dos4g option quiet option nodefaultlibs option start=_cstart_ \
    option map=/w/error_report.map name errmod.exe \
    file error_report.obj,error_report_main.obj,chk.obj,cstart.obj,stack.obj
'
ls -la "$OUT/errmod.exe"
