#!/bin/sh
# Run the rebuilt `exit` module under DOSBox-X and capture its output.
#
# The run is headless: tools/dosboxx_control.py sets SDL_VIDEODRIVER=dummy and
# SDL_AUDIODRIVER=dummy for the child, the same as the driver run.
#
# The test harness sets the three hook slots and calls the module.  The hooks print
# the markers `A`, `B`, `B`, `C`, and `T`, so the output shows the chain of the
# module, and the second `B` shows the shared hook.
#
# Usage:
#     sh tools/rebuild/run_exit.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/work/rebuild"
mkdir -p "$OUT"
cat > "$OUT/run_exit.conf" <<CONF
[dosbox]
mcp_server=9882
memsize=32
machine=svga_s3

[autoexec]
mount D "$OUT"
D:
exitmod.exe > exit_out.txt
CONF
printf 'break\nclear\nexec RUN\nsleep 10\nbreak\nscreen\nstop\n' > "$OUT/run_exit.dbg"
python3 "$ROOT/tools/dosboxx_control.py" script --file "$OUT/run_exit.dbg" \
    --port 9882 --cwd "$OUT" --timeout 180 --launch "dosbox-x -nogui -conf run_exit.conf"
echo "--- the module output ---"
tr -d '\000' < "$OUT/exit_out.txt"
