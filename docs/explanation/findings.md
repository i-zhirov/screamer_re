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

## Program map

- The program `main` sits at module address `0x10020` (object offset
  `0x20`).  The startup at
  `0xd4324` loads `argc` from `[0x27e24]` and `argv` from `[0x27e28]`
  and calls `main` with EAX=`argc` and EDX=`argv`.  Evidence:
  `tools/le.py calls STARTH.EXE 0x10020` and
  `tools/le.py disasm STARTH.EXE 0xd4324`.
- The run-time log confirms the path.  The log line
  `0150:296359  mov  edx,[002D5E28]` matches the file instruction
  `0x000d4359  mov edx, dword ptr [0x27e28]`, and the next game
  instruction is `0150:1D2020  push ebx` (`main`).  The object 1 load base
  is `0x1c2000`.  Evidence: `work/entry/run/LOGCPU.TXT` from
  `tools/entry_probe.py`.
- The entry path is `0x300f8` (jump) to `0x30170` (startup) to `0xd4324`
  (main invoker) to `0x10020` (`main`).  Evidence:
  `tools/le.py tree STARTH.EXE 0x300f8 --depth 3`.
- `main` holds 56 instructions and 15 calls.  The program calls are at
  `0x2ecf8`, `0x2f8ca`, `0x127a0`, `0x27700`, `0x108e4`, `0x10978`,
  `0x27f94`, `0x128ac`, and `0x2ed77`.  Evidence:
  `tools/le.py tree STARTH.EXE 0x10020 --depth 1`.
- See [Program map](../reference/program-map.md) for the addresses.

## Race loop and frame update

- `main` alternates `race_loop` at `0x27700` and `frame_update` at `0x27f94`.
  Evidence: the decompiled `FUN_00010020`.
- `race_loop` starts the race with `race_start` at `0x27770`, runs the inner
  loop (`motion_update`, `default_data_copy`, `joystick_update`) until the
  session state `DAT_0011ce44` reaches 4, and ends with `race_end` at `0x279a4`.
  Evidence: the decompiled `FUN_00027700`.
- `frame_update` clears the screen, runs `net_space_init` and `menu_frame`, then
  loops on `frame_step` until the frame flag `DAT_0011d234` is set, and ends
  with
  `race_frame` and `race_cleanup`.
  Evidence: the decompiled `FUN_00027f94`.
- `frame_update` clears the screen with `memset(DAT_00121528, 0, 0x3e800)`.  The
  count `0x3e800` is 256000, which is 640 times 400, so the screen buffer is 640
  by 400 bytes.  Evidence: the decompiled `FUN_00027f94` and `FUN_0008c160`.
- `input_poll` at `0x26448` counts to 8 and reads the mouse and keyboard state.
  Both loops call it.  Evidence: the decompiled `FUN_00026448`.
- `default_data_copy` at `0x10193` copies 32 dwords from object 1 into object 2,
  the data object at `0xf0000`.  Evidence: the decompiled `FUN_00010193` and the
  object table.
- `race_start` at `0x27770` sets the 640x400 screen and copies the state
  globals.
  Evidence: the decompiled `FUN_00027770`.
- See [Program map](../reference/program-map.md) for the tables.

## The object pipeline and the race frame

- `object_update_all` at `0x2786c` walks `DAT_0010f8ec` records of 0x164 bytes
  at
  `DAT_00110ab8`.  Every record passes `object_transform` (`0x1cd44`),
  `object_player_input` (`0x1c760`), `object_physics` (`0x1c810`), and
  `object_prepare` (`0x1c79c`).  Evidence: the decompiled `FUN_0002786c`.
- `object_transform` writes the world position at `+0x498`, `+0x49c`, and
  `+0x4a0`
  and the float fields at `+0x4a4` to `+0x4ac` from the position and the
  velocity.
  Evidence: the decompiled `FUN_0001cd44`.
- `object_player_input` reads the input bits `DAT_00118e1c` and the state at
  `+0x2f8`; `object_physics` and `object_prepare` run only when the state at
  `+0x414` is set.  Evidence: the decompiled `FUN_0001c760`, `FUN_0001c810`, and
  `FUN_0001c79c`.
- `object_car_link` at `0x1c84c` indexes the 28-byte record tables
  `DAT_0010653c` and `DAT_00106540` with the object field `+0x4dc`.  The 28-byte
  record size matches the list created by `list_create(engine, 150)`.
  Evidence: the decompiled `FUN_0001c84c`.
- `race_frame` at `0x2807c` copies the back buffer `DAT_0011fad0` into the video
  buffer `DAT_00121528` with `memcpy` at `0x8c150`, so `DAT_00121528` is the
  screen buffer and `DAT_0011fad0` is the back buffer.
  Evidence: the decompiled `FUN_0002807c`.
- `video_block_swap_a` at `0x1075f` and `video_block_swap_b` at `0x107a1`
  byte-swap every dword and swap two regions of the screen buffer.  The flags
  `0x10` and `0x20` of `DAT_0011d274` gate them.  The purpose is open; the
  rear-view mirror and the split screen are candidates.
  Evidence: the decompiled `FUN_0001075f` and `FUN_000107a1`.
- `race_state_update` at `0x29564` switches on `DAT_0011fac4`.  Case 1 advances
  `_DAT_0012038c` by the frame delta and copies `DAT_0011facc` into
  `DAT_0011fac4` and `DAT_001203b8`.  The value `DAT_001203b8` then selects
  the overlay in `race_frame`.  Evidence: the decompiled `FUN_00029564` and
  `FUN_0002807c`.
- `fade_update` at `0x29818` ramps `DAT_0011d224` toward 1.0 or down to 0.0 by
  the frame delta, so `DAT_0011d224` is a fade level and `DAT_0011fac8` is the
  direction.  Evidence: the decompiled `FUN_00029818`.
- `race_frame` sets the frame flag `DAT_0011d234` when
  `_DAT_00100ee0 < _DAT_0011d24c` and `DAT_0011fac4` is 0.  `frame_update`
  loops until that flag is set.  Evidence: the decompiled `FUN_0002807c` and
  `FUN_00027f94`.
- `fp_helper` at `0x4d9f4` is a Watcom floating-point helper.  It takes no
  arguments in the decompiled output and appears between float operations.
  Evidence: the decompiled `FUN_00021e6c` and `FUN_0004d9f4`.

## Subsystem initialisation

- `engine_init` at `0x127a0` runs the subsystem setup in this order:
  `config_load`, `list_file_load`, `setup_load`, `controls_load`, `mouse_init`,
  `keyboard_hook`, `palette_build`, `video_buf_load`, `vesa_set_mode`,
  `hmi_sound_init`, `engine_interface_init`, and `stream_layer_init`.
  Evidence: the decompiled `FUN_000127a0` and its callees.
- `config_load` at `0x1bf38` reads `screamer.cfg`.  On failure it reports
  `Files damaged. Re-install Screamer.` and exits with `-1`.  Evidence: the
  decompiled `FUN_0001bf38`.
- `setup_load` at `0x1bb58` reads `CONFIG/SETUP.CFG` into a 144-byte
  structure, which matches the shipped file size.  On failure it reports
  `Run SETUP before.`.
  Evidence: the decompiled `FUN_0001bb58` and `docs/reference/inventory.md`.
- `keyboard_hook` at `0x14e20` installs a handler on interrupt 9 and drives the
  keyboard LEDs through port `0x60`.  `engine_init` calls it with `1` and
  `engine_shutdown` with `0`.  Evidence: the decompiled `FUN_00014e20`.
- `vesa_set_mode` at `0x8bdec` calls the DOS/4GW real-mode interrupt
  dispatcher with `0x4f01`, the VBE get-mode-information call.  Evidence: the
  decompiled `FUN_0008bdec`.
- `mouse_init` at `0x1f1a0` calls interrupt 33h.  Evidence: the decompiled
  `FUN_0001f1a0`.
- `palette_build` at `0x15720` fills a 50-entry table with floating-point ramp
  values.  Evidence: the decompiled `FUN_00015720`.
- See [Program map](../reference/program-map.md) for the table.

## The .3dx resource loader

- The `.3dx` loader is at module address `0x10bfc`.  It walks the object list
  and
  loads every record with the load flag at `+0x14` set.  Evidence: the
  decompiled
  `FUN_00010bfc` and the `3D_HEADER` and `3D_BLKOBJ` string references from
  `tools/ghidra/FindReferences.java`.
- The file name is the record name plus the suffix `3dx`.  Evidence: the format
  `%s.3DX` at `0x1003ac`, the `rb` mode at `0x1003b3`, and the decompiled
  `FUN_00010bfc`.
- The loader reports failures with return codes: `2` when the open fails, `3`
  when the
  `3D_HEADER` chunk is missing, and `5` when the `3D_BLKOBJ` chunk is missing.
  Evidence: the decompiled `FUN_00010bfc`.
- The error reporter at `0x10978` dispatches on the code through a jump table
  and
  prints the `3dx` messages.  Evidence: the Ghidra references to `0x100074`,
  `0x1000a3`, `0x1000d1`, and `0x1000ff`, all from `FUN_00010978`.
- `resource_get` at `0x2eed0` looks a resource up by name and loads it from the
  file when missing.  The same function serves `3D_HEADER`, `3D_BLKOBJ`,
  `MELODIC_BANK`, `DRUM_BANK`, and `NET_SPACE`.  Evidence: the decompiled
  `FUN_0002eed0`, `FUN_0002f9e0`, and `FUN_00029d08`.
- `object_init_from_file` at `0x4cc10` binds the loaded file to the object
  record
  and copies the chunk table.  Evidence: the decompiled `FUN_0004cc10`.
- The loader's callees are named in [Program map](../reference/program-map.md).

## Function names

- The Watcom startup at `0xd4324` calls `main` at `0x10020` with `argc` in EAX
  and `argv` in EDX.  Evidence: `tools/ghidra/decompile.sh 0x10020`.
- `main` calls `list_create` with `0x96` (150) and stores the result in the
  global `DAT_001062ac`.  The list holds 150 records of `0x1c` (28) bytes.
  Evidence: the decompiled `FUN_0002ecf8`.
- `main` calls `memory_available` and compares the result with `6400000`.
  Below that value it calls `printf` and `memory_available` again, which reports
  the shortfall.  Evidence: the decompiled `FUN_0002f8ca` and the strings
  `Not enough memory to play SCREAMER` and `You need another %d bytes`.
- `main` calls `engine_init` at `0x127a0`.  That function calls 17 setup
  functions and loads the `MENU` music from `%s\DATA\MUSIC.DAT`.  Evidence: the
  decompiled `FUN_000127a0`.
- The main loop runs `race_loop` and `frame_update` until the quit flag
  `DAT_00107ed8` is 1.  Evidence: the decompiled `FUN_00010020`.
- `race_loop` starts a race through `FUN_00027770` and loops until the session
  state `DAT_0011ce44` reaches 4.  Evidence: the decompiled `FUN_00027700`.
- `frame_update` clears the key state, reads the clock, advances a 6-frame
  counter,
  waits for the frame flag `DAT_0011d234`, and handles input.  Evidence: the
  decompiled `FUN_00027f94`.
- `main` calls `error_report` at `0x10978` on the session error path, with the
  error code in EAX.  The reporter dispatches through a jump table to one
  message
  stub per code.  Evidence: the decompiled `FUN_00010020` and `FUN_00010978`.
- `main` calls `engine_shutdown` at `0x128ac` after the loop.  That function is
  the counterpart of `engine_init`: it stops the music with `FUN_00014e20(0)`.
  Evidence: the decompiled `FUN_000128ac`.
- Four of the callees are Watcom run-time helpers: `memory_alloc` (`0x30050`),
  `printf` (`0x3009b`), `exit` (`0x300be`), and `free` (`0x300ec`).  They sit
  at the end of the object, after the program code.  Evidence: the decompiled
  bodies and the out-of-memory handler pointers `DAT_001274ac` and
  `DAT_001274a8`.
- The engine strings name the resource interface: `%s.3dx` object files, the
  `3D_HEADER`, `3D_BLKOBJ`, `GOBJ_LIST`, and `OBJ_LIST` chunks, the track
  resources `OSTACLES`, `TRACK_DATA`, `OPPONENTS`, `ANIMATIONS`, and
  `CARS_DATA`,
  the sprite and visual-list interface, and the HMI banks `MELODIC_BANK` and
  `DRUM_BANK`.  Evidence: `r2 -q -c "iz"`.
- See [Program map](../reference/program-map.md) for the table.

## Static cross-check with radare2

radare2 6.2.4 reads the same LE image through its `bin_le` plugin.  Its view
agrees with `tools/le.py` and with the run-time log:

| Check | `tools/le.py` | radare2 | Match |
| --- | --- | --- | --- |
| entry point | module `0x300f8` | `0x000300f8` | yes |
| page 0 mapping | file `0x1a800` to module `0x10000` | paddr `0x1a800`, vaddr `0x10000` | yes |
| `main` | module `0x10020` | `fcn.00010020` | yes |
| `main` caller | `0xd4324`, call at `0xd4364` | `fcn.000d4324`, xref at `0xd4364` | yes |
| `main` prologue | `push ebx; push edx; push ebp; mov ebp,esp` | the same | yes |
| `main` callees | 13 unique targets | 13 identical targets | yes |
| function count | 642 (prologue scan) | 923 (call analysis) | different methods |

Evidence: `r2 -A -q -c "ie" -c "iS" -c "afl"` and
`r2 -A -q -c "s 0x10020; pdf"` from the Nix dev shell.  The function counts
differ because the two methods differ: the prologue scan finds the Watcom
prologue,
and radare2 also recovers functions from call targets.

## Pipeline confirmation

`tools/pipeline_check.py` runs every stage and checks the cross-stage links.
The check reports 28 checks and 0 failures.  Evidence: the tool output.

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
7. cross-check the static view with radare2 in the Nix dev shell,
8. record every claim with its evidence and its limits.

REA covers the package identity and the Mach-O targets.  REA does not
open the DOS LE executables, so the project carries its own LE parser.
The bundled DOSBox 0.74-2 has no debugger, so the project uses DOSBox-X.
