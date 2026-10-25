#!/bin/sh
# Run the rebuilt `memcpy` module under DOSBox-X and capture its output.
#
# The run is headless: tools/dosboxx_control.py sets SDL_VIDEODRIVER=dummy and
# SDL_AUDIODRIVER=dummy for the child, the same as the driver run.
#
# The harness copies eight bytes and prints `Y` when every byte arrived, so the output
# shows the behavior of the module.
#
# Usage:
#     sh tools/rebuild/run_memcpy.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/work/rebuild"
mkdir -p "$OUT"
cat > "$OUT/run_memcpy.conf" <<CONF
[dosbox]
mcp_server=9885
memsize=32
machine=svga_s3

[autoexec]
mount D "$OUT"
D:
memmod.exe > mem_out.txt
CONF
printf 'break\nclear\nexec RUN\nsleep 10\nbreak\nscreen\nstop\n' > "$OUT/run_memcpy.dbg"
python3 "$ROOT/tools/dosboxx_control.py" script --file "$OUT/run_memcpy.dbg" \
    --port 9885 --cwd "$OUT" --timeout 180 --launch "dosbox-x -nogui -conf run_memcpy.conf"
echo "--- the module output ---"
tr -d '\000' < "$OUT/mem_out.txt"
