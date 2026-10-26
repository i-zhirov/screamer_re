#!/bin/sh
# Run the rebuilt `memory_available` module under DOSBox-X and capture its output.
#
# The run is headless: tools/dosboxx_control.py sets SDL_VIDEODRIVER=dummy and
# SDL_AUDIODRIVER=dummy for the child, the same as the driver run.
#
# The harness calls the module once, and the two probe stubs count their calls and return
# fixed values, so the output shows the chain.
#
# Usage:
#     sh tools/rebuild/run_memory_available.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/work/rebuild"
mkdir -p "$OUT"
cat > "$OUT/run_memory_available.conf" <<CONF
[dosbox]
mcp_server=9889
memsize=32
machine=svga_s3

[autoexec]
mount D "$OUT"
D:
mamod.exe > ma_out.txt
CONF
printf 'break\nclear\nexec RUN\nsleep 10\nbreak\nscreen\nstop\n' > "$OUT/run_memory_available.dbg"
python3 "$ROOT/tools/dosboxx_control.py" script --file "$OUT/run_memory_available.dbg" \
    --port 9889 --cwd "$OUT" --timeout 180 --launch "dosbox-x -nogui -conf run_memory_available.conf"
echo "--- the module output ---"
tr -d '\000' < "$OUT/ma_out.txt"
