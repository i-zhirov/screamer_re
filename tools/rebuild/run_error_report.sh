#!/bin/sh
# Run the rebuilt `error_report` module under DOSBox-X and capture its output.
#
# The run is headless: tools/dosboxx_control.py sets SDL_VIDEODRIVER=dummy and
# SDL_AUDIODRIVER=dummy for the child, the same as the driver run.
#
# The harness calls the codes 0, 2, and 20, so the output holds one marker per call
# plus the final marker.
#
# Usage:
#     sh tools/rebuild/run_error_report.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/work/rebuild"
mkdir -p "$OUT"
cat > "$OUT/run_error_report.conf" <<CONF
[dosbox]
mcp_server=9886
memsize=32
machine=svga_s3

[autoexec]
mount D "$OUT"
D:
errmod.exe > err_out.txt
CONF
printf 'break\nclear\nexec RUN\nsleep 10\nbreak\nscreen\nstop\n' > "$OUT/run_error_report.dbg"
python3 "$ROOT/tools/dosboxx_control.py" script --file "$OUT/run_error_report.dbg" \
    --port 9886 --cwd "$OUT" --timeout 180 --launch "dosbox-x -nogui -conf run_error_report.conf"
echo "--- the module output ---"
tr -d '\000' < "$OUT/err_out.txt"
