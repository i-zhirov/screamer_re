#!/bin/sh
# Run the rebuilt `printf` module under DOSBox-X and capture its output.
#
# The run is headless: tools/dosboxx_control.py sets SDL_VIDEODRIVER=dummy and
# SDL_AUDIODRIVER=dummy for the child, the same as the driver run.
#
# The test harness calls the module with a format string.  The engine stub prints `P`
# and returns the count 4, so the output shows the chain of the module.
#
# Usage:
#     sh tools/rebuild/run_printf.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/work/rebuild"
mkdir -p "$OUT"
cat > "$OUT/run_printf.conf" <<CONF
[dosbox]
mcp_server=9883
memsize=32
machine=svga_s3

[autoexec]
mount D "$OUT"
D:
prtfmod.exe > prtf_out.txt
CONF
printf 'break\nclear\nexec RUN\nsleep 10\nbreak\nscreen\nstop\n' > "$OUT/run_printf.dbg"
python3 "$ROOT/tools/dosboxx_control.py" script --file "$OUT/run_printf.dbg" \
    --port 9883 --cwd "$OUT" --timeout 180 --launch "dosbox-x -nogui -conf run_printf.conf"
echo "--- the module output ---"
tr -d '\000' < "$OUT/prtf_out.txt"
