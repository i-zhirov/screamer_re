#!/bin/sh
# Run the rebuilt `bios_video` module under DOSBox-X and capture its output.
#
# The run is headless: tools/dosboxx_control.py sets SDL_VIDEODRIVER=dummy and
# SDL_AUDIODRIVER=dummy for the child, the same as the driver run.
#
# The harness calls the module with the video mode 0x13, so the output shows the chain.
#
# Usage:
#     sh tools/rebuild/run_bios_video.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/work/rebuild"
mkdir -p "$OUT"
cat > "$OUT/run_bios_video.conf" <<CONF
[dosbox]
mcp_server=9887
memsize=32
machine=svga_s3

[autoexec]
mount D "$OUT"
D:
bvmod.exe > bv_out.txt
CONF
printf 'break\nclear\nexec RUN\nsleep 10\nbreak\nscreen\nstop\n' > "$OUT/run_bios_video.dbg"
python3 "$ROOT/tools/dosboxx_control.py" script --file "$OUT/run_bios_video.dbg" \
    --port 9887 --cwd "$OUT" --timeout 180 --launch "dosbox-x -nogui -conf run_bios_video.conf"
echo "--- the module output ---"
tr -d '\000' < "$OUT/bv_out.txt"
