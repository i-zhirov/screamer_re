# Tooling

## Project tools

| Tool | Purpose |
| --- | --- |
| `tools/le.py` | parse an LE image and disassemble x86 code |
| `tools/extract_cd_image.py` | extract the data track of the CD image |
| `tools/inventory.py` | build a file inventory with SHA-256 digests |
| `tools/dosboxx_control.py` | drive the DOSBox-X debugger over its control socket |
| `tools/entry_probe.py` | find the program entry in a DOSBox-X instruction log |
| `tools/pipeline_check.py` | run the pipeline end to end and check the stages agree |

### `tools/le.py`

The tool parses the MZ stub, the LE image header, the object table, and the
object page table.  It reports the header metadata, the objects, the entry
point, and the mapping from a linear address to a file offset.  The mapping
follows the LE page table, so the last page ends at the end of the file.

The disassembly uses the capstone shared library through ctypes.  The tool
looks for the library in these paths:

- `/opt/homebrew/lib/libcapstone.5.dylib`
- `/opt/homebrew/lib/libcapstone.dylib`
- `/usr/local/lib/libcapstone.5.dylib`
- `/usr/local/lib/libcapstone.dylib`

```sh
python3 tools/le.py info   <file>
python3 tools/le.py map    <file> <linear-address>
python3 tools/le.py disasm <file> <linear-address> [--count N]
```

### `tools/extract_cd_image.py`

The tool reads the CUE sheet, keeps the data track, and writes a plain ISO
9660 image with 2048-byte sectors.  It strips the 16-byte sync and header
and the 288-byte EDC/ECC tail of each 2352-byte sector.

```sh
python3 tools/extract_cd_image.py <cue-file> <output-iso>
```

Mount the result with `hdiutil`:

```sh
mkdir -p work/mnt
hdiutil attach -readonly -nobrowse -mountpoint work/mnt work/screamer_data_track.iso
hdiutil detach work/mnt
```

### `tools/inventory.py`

The tool prints a Markdown table with path, size, SHA-256 digest, and a
coarse role for each file.

```sh
python3 tools/inventory.py <directory> [--title TITLE]
```

## Host tools

| Tool | Version | Use |
| --- | --- | --- |
| `python3` | 3.14.7 | project tools |
| `capstone` | 5.0.9 | x86 disassembly library |
| `dosbox-x` | 2026.10.01 | DOS emulator with a debugger |
| `objdump` | LLVM 21.1.8 | object inspection |
| `otool`, `nm`, `strings`, `xxd` | macOS | binary inspection |
| `hdiutil` | macOS 15.5 | ISO mount |
| `xar`, `tar` | macOS | package extraction |

## Debugger control

The bundled DOSBox 0.74-2 carries no debugger.  The project uses DOSBox-X
from Homebrew.  DOSBox-X connects to a control socket on `127.0.0.1:<port>`
when the `[dosbox] mcp_server` option is set.  The protocol is line based:

```text
REQ <id> PING                 -> BEGIN <id> OK / PONG / END <id>
REQ <id> BREAK                -> break into the debugger
REQ <id> EXEC <debug command>  -> run one debugger command
```

`tools/dosboxx_control.py` is the server side of the protocol.  It listens
on the port, sends requests, and prints the responses.  It holds three
commands:

```sh
python3 tools/dosboxx_control.py listen --port 9876
python3 tools/dosboxx_control.py run --port 9876 \
  --command BREAK --command "EV CS EIP"
python3 tools/dosboxx_control.py script --file SCRIPT \
  --port 9876 --cwd WORKDIR \
  --launch "dosbox-x -nogui -break-start -conf probe.conf -conf game.conf"
```

The `script` command runs a session script.  It starts DOSBox-X when
`--launch` is given, and it attaches to a running DOSBox-X when `--launch`
is absent.  The script holds one command per line:

| Command | Effect |
| --- | --- |
| `break` | break into the debugger |
| `clear` | clear the captured emulator output |
| `exec <command>` | run one debugger command |
| `expect <text>` | fail when the last response lacks the text |
| `note <text>` | print a note |
| `read <register> ...` | read registers |
| `sleep <seconds>` | wait |
| `stop` | end the script |
| `wait <marker> [timeout]` | wait for a marker in the emulator output |

The tool exits with code 0 when every step succeeds.  It exits with code 1
when a step fails.  `tools/scripts/example_session.dbg` is a working example.

```sh
python3 tools/dosboxx_control.py script \
  --file tools/scripts/example_session.dbg \
  --port 9876 --cwd work/entry/run \
  --launch "dosbox-x -nogui -break-start -conf ../probe.conf -conf game/dosboxScreamer_single.conf"
```

`tools/entry_probe.py` runs the game in DOSBox-X, breaks into the debugger,
sets an interrupt breakpoint on the game stub's DOS4GW exec call
(`BPINT 21 "4B"`), and starts a CPU instruction log.  The tool reads the
growing log, reports the first game instruction, and then stops the emulation.
The tool needs a terminal for the DOSBox-X debugger, so it runs the emulator
under a pseudo terminal.  It writes only under `work/`.  It exits with code 0
when it finds the entry and code 1 when it does not.

```sh
python3 tools/entry_probe.py \
  --game-dir ~/Documents/Screamer.app/Contents/Resources/game
```

A run takes about 15 seconds.  It reports the entry, for example:

```text
found: True
selector: 0150
eip: 1F20F8
instruction: jmp  short 001F2170 ($+76)
```

Useful debugger commands: `EV CS EIP` reads the current address, `CPU` shows
the CPU state, `GDT` lists the descriptors, `BPLIST` lists breakpoints,
`BPDEL *` removes all breakpoints, `LOGS <count>` writes an instruction log
to `LOGCPU.TXT` in the DOSBox-X working directory.

### `tools/pipeline_check.py`

The tool runs every stage of the pipeline and checks that the stages agree: the
syntax gate, the inventory, the CD image, the LE parser, the entry probe, the
debugger session script, and the cross-checks between the file view and the
run-time view.  The check pins the reference values from the reference pages.
The tool exits with code 0 when every check passes.

```sh
python3 tools/pipeline_check.py \
  --game-dir ~/Documents/Screamer.app/Contents/Resources/game
```

A run takes about 15 seconds and reports 24 checks.  Use `--skip-runtime` to run
the static stages only.

The `dosbox-debugger` agent skill carries the generic workflow: the
connection direction, the terminal requirement, the command list, and the
common traps.  Use the skill for another DOS program.  Use this page for the
project commands.

## Run the game

The installed bundle runs the game:

```sh
open ~/Documents/Screamer.app
```

The bundle carries a universal DOSBox binary with `x86_64`, `i386`, and
`ppc7400` slices.  On Apple silicon the `x86_64` slice runs through
Rosetta 2.  The host accepts x86_64 binaries, and the bundled DOSBox runs.

The single-player path runs:

```sh
cd ~/Documents/Screamer.app/Contents/Resources
DYLD_LIBRARY_PATH=./dosbox ./dosbox/dosbox \
  -conf game/dosboxScreamer.conf \
  -conf game/dosboxScreamer_single.conf
```

## REA status

REA opens Mach-O targets, for example
`~/Documents/Screamer.app/Contents/MacOS/Launcher`.  REA does not open the
DOS LE executables.  Each `open_binary` call on `STARTH.EXE` and
`DOS4GW.EXE` fails with this error:

```text
MCP error -32602: Structured content does not match the tool's output schema
```

REA also reports the PKG root hash only.  The package child inventory
requires a native macOS adapter.

The project therefore carries its own LE parser in `tools/le.py`.  Use REA for
Mach-O targets and for package identity; use the project tools for DOS LE
targets.

## Limits

- The capstone bindings load through ctypes.  A host without the capstone
  library cannot disassemble.
- `tools/le.py` covers LE images only.  It rejects LX images and does not
  decode the embedded HMI drivers.
- The tool does not apply fixups.  It reads the fixup section offsets only.
- The tool does not decompress iterated pages.
