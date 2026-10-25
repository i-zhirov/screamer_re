#!/bin/sh
# Run the rebuilt driver under DOSBox-X and capture its output.
#
# The run is headless: tools/dosboxx_control.py sets SDL_VIDEODRIVER=dummy and
# SDL_AUDIODRIVER=dummy for the child, the same as the graphics format probes.
#
# Usage:
#     sh tools/rebuild/run.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/work/rebuild"
mkdir -p "$OUT"
cat > "$OUT/run.conf" <<CONF
[dosbox]
mcp_server=9880
memsize=32
machine=svga_s3

[autoexec]
mount D "$OUT"
D:
driver.exe > out.txt
CONF
printf 'break\nclear\nexec RUN\nsleep 10\nbreak\nscreen\nstop\n' > "$OUT/run.dbg"
python3 "$ROOT/tools/dosboxx_control.py" script --file "$OUT/run.dbg" \
    --port 9880 --cwd "$OUT" --timeout 180 --launch "dosbox-x -nogui -conf run.conf"
echo "--- the driver output ---"
tr -d '\000' < "$OUT/out.txt"
