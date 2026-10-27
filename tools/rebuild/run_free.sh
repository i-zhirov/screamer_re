#!/bin/sh
# Run the rebuilt `free` module under DOSBox-X and capture its output.
#
# The run is headless: tools/dosboxx_control.py sets SDL_VIDEODRIVER=dummy and
# SDL_AUDIODRIVER=dummy for the child, the same as the driver run.
#
# The test harness calls the module with a null pointer and with a non-null
# pointer.  The stand-in `nfree` prints `NFREE`, so the output shows the
# behavior of the module.
#
# Usage:
#     sh tools/rebuild/run_free.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/work/rebuild"
mkdir -p "$OUT"
cat > "$OUT/run_free.conf" <<CONF
[dosbox]
mcp_server=9881
memsize=32
machine=svga_s3

[autoexec]
mount D "$OUT"
D:
free.exe > free_out.txt
CONF
printf 'break\nclear\nexec RUN\nsleep 10\nbreak\nscreen\nstop\n' > "$OUT/run_free.dbg"
python3 "$ROOT/tools/dosboxx_control.py" script --file "$OUT/run_free.dbg" \
    --port 9881 --cwd "$OUT" --timeout 180 --launch "dosbox-x -nogui -conf run_free.conf"
echo "--- the module output ---"
tr -d '\000' < "$OUT/free_out.txt"
