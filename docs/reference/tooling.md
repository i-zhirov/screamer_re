# Tooling

## Project tools

| Tool | Purpose |
| --- | --- |
| `tools/le.py` | parse an LE image and disassemble x86 code |
| `tools/extract_cd_image.py` | extract the data track of the CD image |
| `tools/inventory.py` | build a file inventory with SHA-256 digests |

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
| `objdump` | LLVM 21.1.8 | object inspection |
| `otool`, `nm`, `strings`, `xxd` | macOS | binary inspection |
| `hdiutil` | macOS 15.5 | ISO mount |
| `xar`, `tar` | macOS | package extraction |

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
