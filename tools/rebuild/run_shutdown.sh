#!/bin/sh
# Run the rebuilt `engine_shutdown` module under DOSBox-X and capture its output.
#
# The run is headless: tools/dosboxx_control.py sets SDL_VIDEODRIVER=dummy and
# SDL_AUDIODRIVER=dummy for the child, the same as the driver run.
#
# Each of the eight callees prints a marker, so the output shows the call order.
#
# Usage:
#     sh tools/rebuild/run_shutdown.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/work/rebuild"
mkdir -p "$OUT"
cat > "$OUT/run_shutdown.conf" <<CONF
[dosbox]
mcp_server=9884
memsize=32
machine=svga_s3

[autoexec]
mount D "$OUT"
D:
shutmod.exe > shut_out.txt
CONF
printf 'break\nclear\nexec RUN\nsleep 10\nbreak\nscreen\nstop\n' > "$OUT/run_shutdown.dbg"
python3 "$ROOT/tools/dosboxx_control.py" script --file "$OUT/run_shutdown.dbg" \
    --port 9884 --cwd "$OUT" --timeout 180 --launch "dosbox-x -nogui -conf run_shutdown.conf"
echo "--- the module output ---"
tr -d '\000' < "$OUT/shut_out.txt"
