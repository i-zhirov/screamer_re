#!/bin/sh
# Build the rebuilt driver with the pinned OpenWatcom V2 container.
#
# The container holds the extracted OpenWatcom V2 toolchain, so no installer run is
# needed.  The sources stay in tools/rebuild, and every build writes only under
# work/rebuild.
#
# Usage:
#     sh tools/rebuild/build.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/tools/rebuild"
OUT="$ROOT/work/rebuild"
IMAGE="arlaneenalra/watcom-docker@sha256:16e4845ddd0df7bc877b45d103aacf66bc48aaab6d1c61d6bcaa9c1274f06a65"

mkdir -p "$OUT"
cp "$SRC/driver.c" "$SRC/chk.asm" "$SRC/stack.asm" "$SRC/cstart.c" "$OUT/"

docker run --rm -v "$OUT:/w" -w /w "$IMAGE" sh -c '
set -e
wasm -bt=dos -zq -zcm=watcom -fo=/w/chk.obj chk.asm
wasm -bt=dos -zq -zcm=watcom -fo=/w/stack.obj stack.asm
wcc386 -bt=dos -s -zq -fo=/w/cstart.obj cstart.c
wcc386 -bt=dos -s -zq -fo=/w/driver.obj driver.c
wlink system dos4g option quiet option nodefaultlibs option start=_cstart_ \
    name driver.exe file driver.obj,chk.obj,cstart.obj,stack.obj
'
ls -la "$OUT/driver.exe"
