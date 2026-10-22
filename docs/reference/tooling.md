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
| `tools/ghidra/install_loader.sh` | install the LX/LE loader into Ghidra |
| `tools/ghidra/decompile.sh` | decompile functions by address with Ghidra |
| `tools/ghidra/DecompileFunctions.java` | the Ghidra headless decompile script |
| `tools/ghidra/FindReferences.java` | the Ghidra headless reference listing script |

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
python3 tools/le.py calls  <file> <linear-address> [--max N]
python3 tools/le.py tree   <file> <linear-address> [--depth N] [--max-nodes N]
python3 tools/le.py funcs  <file> [--from ADDR] [--to ADDR] [--limit N]
python3 tools/le.py near   <file> <linear-address>
```

The `calls` command decodes one function body from its start and lists the call
sites.  A leading unconditional jump is followed, because the linker puts a jump
over
the run-time banner string at the entry point.  The `tree` command follows the
call
targets recursively.  The `funcs` command lists the candidate function starts
from
the Watcom prologue `push ebp; mov ebp, esp`.  The `near` command prints the
candidate function start nearest below an address.

The body sweep stops at the first `ret`, `retf`, `iret`, `hlt`, or `int3`, or
after `--max` instructions.  A function that ends with a tail call has no
terminator, so the sweep continues into the next function.  An indirect call has
no
static target.  See [Program map](program-map.md) for the recovered structure.

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

## gfx_container.py

`tools/gfx_container.py` lists and extracts the members of a Screamer graphics
container.  It parses the directory at the end of the file and reports the
member
names, offsets, sizes, end offsets, and flags.

```sh
python3 tools/gfx_container.py info GFX/GAME.DAT
python3 tools/gfx_container.py member GFX/GAME.DAT GAMEL.SPR --out work/gamel.spr
python3 tools/gfx_container.py head GFX/GAME.DAT GAMEL.SPR
python3 tools/gfx_container.py font GFX/GRAF.DAT MENUL.FNT
python3 tools/gfx_container.py render GFX/GRAF.DAT MENUH.FNT 33
```

`font` prints the font layout of a member: the count, the descriptors, the glyph
tables, and the tail.  `render` prints one glyph as characters in the confirmed
four bit format, with two pixels per byte and the high nibble first, so a zero
nibble is a transparent pixel.

`export_assets.py` writes the graphics assets as PNG files and builds a
whiteboard
page that shows every asset with its name and its size:

```sh
python3 tools/export_assets.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game
python3 tools/export_assets.py --game-dir GAME --out work/assets
python3 tools/export_assets.py --game-dir GAME --embed
```

The tool writes one PNG per sprite member, one glyph sheet per font member, and
one
PNG per PCX member, and then writes `index.html`.  The sprite sheets stack the
sprites of a member one under the other and use the `MENU.COL` palette.  The
font
sheets draw the four bit glyphs at 32 by 40 pixels per cell in grey.  The PCX
files
use the palette at the end of the file.  The page links the images by default
and
embeds them with `--embed`, which makes the page self contained and much larger.
`--scale` scales the sprite and font images by whole numbers and the default is
four.  `--font-scale` and `--font-cap` set the separate font page, which draws
every
font block in four readings and adds one image per descriptor at the width and
the
height of that descriptor.  The tool uses the standard library only.

`head` prints the first 16 bytes of a member and tests the sprite structure: the
count at `+0x00`, the pixel data size at `+0x04`, and whether
`8 + count * 0x14 + size` equals the member size.  The member data is
LZW-compressed, so the raw member does not match the plain structure.  The
expansion follows the instructions and is confirmed against the runtime records
of
`LOGOH.SPR`.

## Debugger limits and the alternatives

The DOSBox-X debugger does offer more than the probes used.  `BP <seg>:<off>` is
a
real-mode breakpoint, which is why it never hit the protected-mode DOS/4GW code
of
this game.  The debugger also offers `BPLM <linear address>` for a linear memory
breakpoint, `BPPM <selector>:<off>` for a protected-mode memory breakpoint,
`BPM <seg>:<off>` for a memory change breakpoint, `MEMDUMPBIN` for a binary
memory
dump, `IV` to name a memory address, `SMV` to write memory at a linear address,
and
`LOG`, `LOGS`, `LOGL`, and `HEAVYLOG` for CPU logs.  The next probe should use
`BPLM` and `MEMDUMPBIN` rather than `BP` and the text `MEMDUMP`.

The alternatives, if DOSBox-X still falls short:

| Emulator | Debugger | Runs DOS/4GW | macOS arm64 | Cost |
| --- | --- | --- | --- | --- |
| DOSBox-X | the command set above; real-mode breakpoints and linear memory breakpoints | yes | yes, Homebrew | none, already in place |
| Bochs (`bochsdbg`) | the strongest: linear and physical breakpoints, watchpoints, memory dumps, a command file for scripts | yes, it boots DOS | yes, source or Homebrew | medium: a bootable DOS disk image and an install step |
| QEMU plus GDB | strong: the gdb stub with hardware breakpoints, memory dumps, and GDB Python scripting | yes, with FreeDOS | yes, Homebrew | medium: a bootable DOS image and a GDB script |
| 86Box | accurate hardware; the debugger is weaker than Bochs and QEMU | yes | yes, Homebrew | medium: a bootable DOS image |

The recommendation is to try `BPLM` and `MEMDUMPBIN` in DOSBox-X first, because
the
work is already in place.  Bochs is the fallback with the best debugger for
protected-mode DOS code, and QEMU with GDB is the fallback with the best
scripting.

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
debugger session script, the cross-checks between the file view and the run-time
view, and the radare2 cross-check.  The check pins the reference values from the
reference pages.  The tool exits with code 0 when every check passes.  The
radare2 stage is skipped when `r2` is not on PATH.

```sh
python3 tools/pipeline_check.py \
  --game-dir ~/Documents/Screamer.app/Contents/Resources/game
```

A run takes about 20 seconds and reports 28 checks.  Use `--skip-runtime` to run
the static stages only.

The check runs in three places:

- the `.githooks/pre-push` hook runs the syntax gate and the full check before
  every push.  Enable it once per clone with
  `git config core.hooksPath .githooks`.  Set `SCREAMER_GAME_DIR` to use another
  game directory,
- GitHub Actions runs the syntax gate on every push and pull request,
- GitHub Actions runs the full check on a self-hosted runner that holds the
  game and DOSBox-X.  Register the runner with the labels `macOS` and
  `screamer-artifacts`, and set the repository variable `SCREAMER_SELF_HOSTED`
  to `true` to enable the job.

The `dosbox-debugger` agent skill carries the generic workflow: the
connection direction, the terminal requirement, the command list, and the
common traps.  Use the skill for another DOS program.  Use this page for the
project commands.

## Development environment

`flake.nix` pins the project tools for a Nix shell:

```sh
nix develop
r2 -v
```

The shell provides Python 3.14.8, capstone 5.0.9, and radare2 6.2.4. 
`flake.lock`
pins the nixpkgs revision.  DOSBox-X stays on Homebrew, because the run-time
probe depends on the debugger control socket of the tested version.

## Static cross-check with radare2

radare2 carries a native LE parser (`bin_le`).  Use it to cross-check the
project
parser and to recover functions that the `le.py` prologue scan misses:

```sh
r2 -A -q -c "ie" -c "iS" -c "afl" -c "q" STARTH.EXE
r2 -A -q -c "s 0x10020; pdf" -c "q" STARTH.EXE
```

The addresses are module addresses.  They agree with `tools/le.py` and with the
run-time log.  See
[Program map](program-map.md) for the recovered structure and
[Findings](../explanation/findings.md) for the cross-check table.

## Decompilation with Ghidra

Ghidra needs an LX/LE loader for the DOS/4GW executables.  Without the loader,
Ghidra reads the file as a 16-bit MZ image and misses the 32-bit code.  The
`ghidra` dev shell pins Ghidra 12.0.4, because the loader extension is released
for the 12.0.x series.  Ghidra 12.1.x removed an API the loader uses.

Install the loader once per user:

```sh
nix develop .#ghidra --command sh tools/ghidra/install_loader.sh
```

The script downloads the `yetmorecode/ghidra-lx-loader` release into
`work/ghidra/`
and installs it into the Ghidra user directory.  The Nix store is read-only, so
the
extension lives outside the store.  The user directory is
`~/Library/ghidra/ghidra_12.0.4_NIX` on this host.

Decompile the functions that contain the given addresses:

```sh
nix develop .#ghidra --command sh tools/ghidra/decompile.sh 0x10020 0x2ecf8
```

The script imports the executable into the `work/ghidra/proj` project, runs the
`tools/ghidra/DecompileFunctions.java` script, and writes the decompiled C to
`work/ghidra/decompiled.txt`.  Set `SCREAMER_EXE` to use another executable.

List every reference to an address, for example to resolve a string that a fixup
points at:

```sh
nix develop .#ghidra --command analyzeHeadless work/ghidra/proj Screamer \
  -import ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE -overwrite \
  -scriptPath tools/ghidra \
  -postScript FindReferences.java 0x1000a3
```

The loader applies the LE fixups, so a reference that is a low offset in the
file
resolves to the full address.  radare2 needs `-e bin.relocs.apply=true` for the
same
result.

The loader reports the same addresses as `tools/le.py` and radare2, so the three
views agree.  See [Program map](program-map.md) for the named functions.

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
