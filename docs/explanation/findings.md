# Findings

This page records the investigation log.  Each finding carries its evidence.
The page separates observations from open questions.

## Identity

| Fact | Value | Evidence |
| --- | --- | --- |
| Game name | Screamer | GOG metadata file, package name |
| GOG game id | 1207658726 | `goggame-1207658726.info` |
| GOG build | 34378 | package file name |
| Language | English (en-US) | `goggame-1207658726.info` |
| Installer author | GOG Sp. z o.o. | package signature |

The package signature chain is `Developer ID Installer: GOG Sp. z o.o.
(9WS36Q8886)`, issued by the Apple Developer ID Certification
Authority.

## Distribution

- The package `screamer_enUS_1_0_34378.pkg` is a XAR archive.  Its
  SHA-256 digest is
  `5e3717a3f61519a81cdee8e3b81acb7cc45e384d33ca1835d60f4e2ce5432c6f`.
  Evidence: REA `inventory_artifact` record
  `ev_eba4e5c89f9c945420917e733a3f70a7e624f21e54bcbcec0603f19db163edd5`.
- REA reports the package root hash only.  Child inventory needs a native
  macOS adapter.  Evidence: REA limitation text.
- The package holds a gzip tar payload of 451194368 bytes.  The tar holds
  106 entries under `payload/`.
- The payload root is the `Screamer.app` bundle.  The installed copy is
  `~/Documents/Screamer.app`.
- The installed `Launcher` matches the payload `Launcher`.  Both digests are
  `d2663b471ae64f7c7ac5aad90370f0110c21be1a161a15abb3ac18e9c0f3356b`.
- The `postinstall` script moves the payload into the target directory,
  clears the quarantine attribute, and changes ownership.  The payload holds
  no `support/` directory, so the support script loop runs on nothing.
- `Contents/Info.plist` names the bundle "The Dame Was Loaded" with the
  identifier `com.gog.the_dame_was_loaded`.  The GOG metadata file names
  the game "Screamer".  The bundle metadata is stale.

## Executables

- `STARTH.EXE`, `STARTL.EXE`, and `SETUP.EXE` are LE (linear
  executable) images with a 16-bit MZ stub.  The LE header sits at file
  offset `0x2a50`.  Evidence: `tools/le.py info` output and `file` output.
- The header reports CPU 80386, OS type OS/2, and module type EXE.
- Each image holds three objects: a code object, a small data object, and a
  stack object.
- `STARTH.EXE` and `STARTL.EXE` share the module name `start`.  The
  resident name table holds one symbol, `start`, with ordinal 0.
- The run-time banner is `WATCOM C/C++32 Run-Time system. (c) Copyright
  by WATCOM International Corp. 1988-1994. All rights reserved.`  The game
  therefore uses the Watcom C/C++ 32-bit toolchain.
- The programs use the DOS/4GW protected-mode extender.  The runtime code
  carries the string `dos4gw.exe`.
- The page mapping is confirmed: the object page table numbers run in order
  from 1 to 227, and the last page ends at the end of the file.  Evidence:
  the page table values and the file size `0xfcaab`.
- The fixup page table confirms the header layout: the last entry equals
  `0x17485`, and `frectab + 0x17485` equals the import module table
  offset `0x17cb7`.
- `STARTH.EXE` and `STARTL.EXE` are two builds of one program.  They
  differ in 405284 bytes and in 4 bytes of size.
- `SETUP.EXE` is the setup and configuration program.  It reads and
  writes `CONFIG/SETUP.DAT` and the `CONFIG/*.CFG` files.
- The first executable code in the code object sits at file offset
  `0x1a818`.  The code object starts with a trap stub and NOP padding.

## Runtime

- The sound support comes from HMI (Human Machine Interfaces, Inc.)
  drivers.  The banners name the Gravis Ultrasound and the OPL2.
- `DATA/MUSIC.DAT` and `CONFIG/SETUP.DAT` carry the `ADLIB-` signature
  and instrument names such as `<Blank`, `$clap`, and `$rimshot`.
- The single-player path runs `STARTH.EXE`.  The client and server paths
  run the same executable with an IPX network configuration.

## Entry point

- The LE header entry value is an offset inside the start object.  The file
  offset follows this rule:

  ```text
  file_offset = data_pages_offset
              + (page_table_index - 1 + entry_offset // page_size) * page_size
              + entry_offset % page_size
  ```

  Evidence: `tools/le.py` and the run-time log below.
- The entry code is a short jump over the Watcom run-time banner string:
  `jmp short +0x76` at `0x03a8f8`, then `sti`, `and esp,-4`,
  `mov ebx,esp`, `mov ah,0x30`, `int 0x21`.  Evidence: `tools/le.py
  disasm STARTH.EXE 0x300f8`.
- All three executables share the entry pattern.  The entry file offsets are
  `0x3a8f8` (`STARTH.EXE`), `0x3a6b8` (`STARTL.EXE`), and `0x20628`
  (`SETUP.EXE`).
- A DOSBox-X instruction log of `STARTH.EXE` shows the game code start at
  selector `0x0150`, EIP `0x1f20f8`, with all registers zero.  The first
  instructions are `jmp short 001f2170`, `sti`, `mov ebx,esp`, and `int 21`
  with `EAX=0x3000`.  Evidence: `tools/entry_probe.py` and the log line
  `0150:1F20F8  jmp  short 001F2170 ($+76)  EAX:00000000 ...`.
- The run-time module load base is `0x1c2000`: `0x1f20f8 - 0x200f8 =
  0x1c2000`.  The module-relative entry is `0x300f8`, which equals the
  object base `0x10000` plus the header entry `0x200f8`.  Evidence: the log
  line above and the LE object table.
- The DOS4GW stub of `STARTH.EXE` loads the extender as an overlay:
  `int 21` with `EAX=0x4b03` (AH `0x4b`, AL `0x03`).  Evidence: the
  first log line `9fe0:00000002  int  21  EAX:00004B03`.

## Pipeline confirmation

`tools/pipeline_check.py` runs every stage and checks the cross-stage links.
The check reports 24 checks and 0 failures.  Evidence: the tool output.

- The file view and the run-time view agree.  The run-time entry `0150:1f20f8`
  matches the file entry at `0x3a8f8`, and the run-time jump target `0x1f2170`
  matches the file target.
- The module load base is `0x1c2000`.  The base is page aligned.  Evidence:
  `0x1f20f8 - 0x300f8 = 0x1c2000`.
- The run-time startup calls the DOS version service with `EAX=0x3000`
  (`AH=0x30`).  Evidence: the log line after the entry.
- The stub exec break `9fe0:0002` is outside the BIOS, and the log shows
  `int 21` with `EAX=0x4b03` (`AH=0x4b`, `AL=0x03`, load overlay).  Evidence:
  the first log line.

## Open questions

### Entry alignment (resolved)

The entry alignment question is resolved.  The header entry is an offset
inside the start object, not a module-relative address.  The run-time log
shows the entry instruction `jmp short +0x76`, and that instruction sits at
file offset `0x3a8f8`.  See the Entry point section above.

### Data formats

The project has no decoder for these formats:

- the HMI `ADLIB-` music bank (`DATA/MUSIC.DAT`, `CONFIG/SETUP.DAT`),
- the map data (`DATA/MAP*.DAT`, `DATA/GEN.DAT`, `DATA/LOCAL.DAT`),
- the map index data (`DATA/MAP*I.DAT`),
- the graphics data (`GFX/*.DAT`),
- the tables (`TABS/VIS.TAB`, `TABS/PERSP01.TAB`),
- the raw sample data (`DATA/SOUND.DAT`),
- the HMI `.386` drivers,
- the replay format.

### Duplicate data

`GFX/MAP5H.DAT` and `GFX/MAP5L.DAT` hold the same bytes.  One file is
therefore redundant.  The reason is not known.

### CD audio

The CD image holds seven audio tracks.  The project holds no extractor for
audio tracks.  The game plays the tracks through the CD audio path.

## Method

The investigation uses these steps:

1. inspect the installer package with `pkgutil`, `xar`, and `tar`,
2. compare the installed bundle with the payload,
3. inventory the files with SHA-256 digests,
4. parse the LE headers and the tables with `tools/le.py`,
5. disassemble the code at selected addresses with capstone,
6. run the game in DOSBox-X and drive its debugger over the control
   socket with `tools/dosboxx_control.py` and `tools/entry_probe.py`,
7. record every claim with its evidence and its limits.

REA covers the package identity and the Mach-O targets.  REA does not
open the DOS LE executables, so the project carries its own LE parser.
The bundled DOSBox 0.74-2 has no debugger, so the project uses DOSBox-X.
