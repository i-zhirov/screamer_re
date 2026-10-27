#!/bin/sh
# Run the rebuilt `list_destroy` module under DOSBox-X and capture its output.
#
# The run is headless: tools/dosboxx_control.py sets SDL_VIDEODRIVER=dummy and
# SDL_AUDIODRIVER=dummy for the child, the same as the driver run.
#
# The harness calls the module with a set pointer and with a null pointer, so the output
# shows the guard and the return value.
#
# Usage:
#     sh tools/rebuild/run_list_destroy.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/work/rebuild"
mkdir -p "$OUT"
cat > "$OUT/run_list_destroy.conf" <<CONF
[dosbox]
mcp_server=9888
memsize=32
machine=svga_s3

[autoexec]
mount D "$OUT"
D:
ldmod.exe > ld_out.txt
CONF
printf 'break\nclear\nexec RUN\nsleep 10\nbreak\nscreen\nstop\n' > "$OUT/run_list_destroy.dbg"
python3 "$ROOT/tools/dosboxx_control.py" script --file "$OUT/run_list_destroy.dbg" \
    --port 9888 --cwd "$OUT" --timeout 180 --launch "dosbox-x -nogui -conf run_list_destroy.conf"
echo "--- the module output ---"
tr -d '\000' < "$OUT/ld_out.txt"
