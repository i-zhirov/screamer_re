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

## The .3dx chunk layout

- The object records are 28 bytes.  The record holds the object name at `+0x00`
  and the load flag at `+0x14`; the value 1 selects the record for loading.
  The 28-byte size matches the engine list from `list_create(engine, 150)`.
  Evidence: the decompiled `FUN_00010bfc`.
- The loader loads the `3D_HEADER` chunk and the `3D_BLKOBJ` chunk through the
  resource layer and then calls `object_init_from_file`.
  Evidence: the decompiled `FUN_00010bfc`.
- `object_init_from_file` at `0x4cc10` fills the chunk table of the object
  record.  The table holds eight words per chunk: the file record, the chunk
  count, and the six words of the chunk header.  The table ends with -1.
  Evidence: the decompiled `FUN_0004cc10`.
- The file header holds the chunk count at `+0x14` and the chunk offset table at
  `+0x18`, one 32-bit file-relative offset per chunk.  A chunk offset points
  12 bytes before the chunk header, so the six chunk words lie at `+0x10`,
  `+0x14`,
  `+0x18`, `+0x1c`, `+0x20`, and `+0x24` of the chunk.
  Evidence: the decompiled `FUN_0004cc10`.
- The `3D_HEADER` chunk holds the object count at `+0x14`.  The `3D_BLKOBJ`
  chunk holds a 0x60-byte header and `count` block objects of 0x20 bytes, so the
  size is `count * 0x20 + 0x60`.  Evidence: the decompiled `FUN_00010bfc`.
- The loader clears the six list words at `+0x24`, `+0x28`, `+0x2c`, `+0x30`,
  `+0x34`, and `+0x38` of the `3D_BLKOBJ` header.  Evidence: the decompiled
  `FUN_00010bfc`.
- The resource layer stages the loaded chunks in the main file.  The unit is 16
  bytes.  The layer asks the DOS service for a block through the DPMI real-mode
  interrupt path; the block number times 16 is the file offset.  The layer
  stores
  the buffer at chunk table entry `+0x0c` and the size at `+0x14`.
  Evidence: the decompiled `FUN_0002f0fc`, `FUN_0002f172`, and `FUN_0002f062`.
- The resource object holds the record array at `+0x00`, the loaded count at
  `+0x04`, the limit at `+0x08`, and the status at `+0x0c`; the status 2 marks
  the end.  Evidence: the decompiled `FUN_0002eed0`.
- The identity of the DOS service is open.  The layer passes the values `0x100`
  and `0x501` in the request and calls the interrupt `0x31`.
  Evidence: the decompiled `FUN_0002f0fc` and `FUN_0002f172`.

## The graphics decoder

- `pcx_load` at `0x1b5a4` decodes a PCX member.  It builds the member name
  with the format `%s\%s` and opens it in the mode `rb`.
  Evidence: the decompiled `FUN_0001b5a4` and the strings at `0x1007f80`,
  `0x1009f5`, and `0x1009f5`.
- The loader seeks 0x300 bytes before the end of the member and reads the
  256-colour palette.  It then shifts every palette byte right by 2, so the
  palette becomes the 6-bit VGA form.  Evidence: the decompiled `FUN_0001b5a4`.
- The loader rewinds, reads the 128-byte PCX header, and computes the pixel
  count as `(xmax - xmin + 1) * (ymax - ymin + 1)`.  It then decodes the RLE
  runs: a byte with the mask `0xc0` equal to `0xc0` repeats the next byte
  `byte & 0x3f` times.  Evidence: the decompiled `FUN_0001b5a4`.
- `GFX/GRAF.DAT` holds the PCX members `menu1h.pcx`, `menu2h.pcx`,
  `menu3h.pcx`, `menu4h.pcx`, `champ5h.pcx`, `load%d%c.pcx`, and
  `net%c.pcx`.  Evidence: the strings at `0x100fd5`, `0x101014`, `0x101054`,
  `0x101114`, `0x101132`, `0x101152`, and `0x1011bc`.
- `menu_space_init` at `0x29d08` loads the menu data through two loaders.
  The sprite loader at `0x257b0` reads two header blocks and then the `SPRITES`
  resource twice.  The font loader at `0x25658` reads a count, then per font a
  12-byte descriptor at `DAT_0011ca98`, then per font a 0x500-byte bitmap at
  `DAT_00119898`, and finally the `FONTS` resource.
  Evidence: the decompiled `FUN_000257b0` and `FUN_00025658`.
- The font record is 12 bytes and the font bitmap is 1280 bytes.  The loader
  increments the font count `DAT_00025440`.
  Evidence: the decompiled `FUN_00025658`.
- The sprite structure is decoded from the disassembly of `0x257b0`: the head of
  the file holds the sprite count at `+0x00` and the pixel data size at `+0x04`,
  then `count * 0x14` bytes of 20-byte sprite records fill the first `SPRITES`
  resource, and the pixel data fills the second.  The loader reads 4 bytes into
  `DAT_00025cd4`, 4 bytes into `DAT_00025cdc`, then `count * 0x14` bytes, then
  the size in bytes.  A missing file closes the stream and calls `exit(0)`.
  Evidence: the disassembly of `0x257b0` at `0x257cd` to `0x258b4`.
- Verification against `GFX/GAME.DAT`: the first dword is `0xfbf05b00`, which is
  not a plausible sprite count for a file of 637267 bytes, and the head bytes do
  not hold text.  The file does not match the plain sprite structure at its
  head.
  Evidence: the byte scan of `GFX/GAME.DAT`.
- Verification against `GFX/MENU.DAT`: the first dword is `0x03000000` and the
  first word is 0, so the file does not match the plain sprite structure either.
  Both files are compressed or encoded, so the record structures are the layouts
  of the loaded resources, not of the file bytes.
  Evidence: the byte scans of both files.
- The encoding of both files is decoded.  Every byte is exclusive-ORed with
  `0x3a`,
  and the decoder at `0x4c128` reads the file from the end: it seeks 4 bytes
  before the end, reads the decompressed size, seeks back by the size plus 4
  bytes, reads 32-byte blocks forward, exclusive-ORs every byte with `0x3a`, and
  decodes each block through `0xd9ba`.  Evidence: `FUN_0004c128`.
- The block header is the first byte of a block; the value 0 ends the data.  The
  bytes at `+0x1a` and `+0x1c` are two 16-bit lengths whose sum is the
  decompressed block size, and the byte at `+0x1d` holds two flags in bit 0 and
  bit 1.  A member name follows at `+0x58`.  Evidence: `FUN_0004c128`.
- The decode of the file tails confirms the encoding.  `GFX/GAME.DAT` returns
  the
  member names `TEMPLET_L.PCX`, `GAMEH.FNT`, `L.FNT`, and `H.FNT`.
  `GFX/MENU.DAT` returns `MENU.COL`, `SHADOW.3DX`, `AUTO06.3DX`, and
  `DOW.3DX`.  So the directory of a file sits at the end, before the 4-byte
  decompressed size.  Evidence: the exclusive-OR decode of both tails.
- The decoder `0x4c128` is a member search, not a block decoder.  It reads
  32-byte blocks from the end and compares each block with the wanted member
  name through the case-insensitive compare `0x4d9ba`.  The open wrapper
  `0x4c08d` calls the decoder, so the file layer decodes a file when it opens
  it.  Evidence: the decompiled `FUN_0004c128`, `FUN_0004d9ba`, and
  `FUN_0004c08d`.
- The last 4 bytes hold the size of the directory, and the directory holds
  `size / 32` records of 32 bytes.  A record holds the name at `+0x00`
  (26 bytes, exclusive-ORed with `0x3a`, NUL-terminated), a 16-bit block
  offset at `+0x1a`, a 16-bit block count at `+0x1c`, and two flags at
  `+0x1d`.  `GFX/GAME.DAT` holds 39 members and `GFX/MENU.DAT` holds 18.
  Evidence: the exclusive-OR decode of both files.
- The members of `GFX/GAME.DAT`: `IMPLET_L.PKC`, `IMPLET_H.PKC`,
  `IMPRES_L.PKC`, `IMPRES_H.PKC`, `CDFRONTL.PKC`, `CDFRONTH.PKC`, and
  `COCK00AL.PKC`, with 32 more.  The names form low and high resolution
  pairs.  Evidence: the exclusive-OR decode of the directory.
- The members of `GFX/MENU.DAT`: `MENU.COL`, `MENU.MLB`, `ALETTER.3DX`,
  `AMAP1.3DX` to `AMAP6.3DX`, `AAUTO00.3DX`, `AAUTO01.3DX`, and
  `AAUTO02.3DX`, with 7 more.  Evidence: the exclusive-OR decode.
- The directory record holds the member name at `+0x00` (16 bytes), the
  member offset at `+0x10`, the member size at `+0x14`, the end offset at
  `+0x18`, and a flag at `+0x1c`.  The offsets form one chain:
  `offset + size` of a member is the `offset` of the next member, and the last
  member ends at the start of the directory.
  Evidence: the decode of both directories.
- `GFX/GAME.DAT` holds the sprite members `GAMEL.SPR` (559712, 10010 bytes)
  and `GAMEH.SPR` (569722, 28736) and the font members `GAMEL.FNT`
  (598458, 11565) and `GAMEH.FNT` (610023, 25992), so the members that
  `sprite_load` and `font_load` read are present.
  Evidence: the exclusive-OR decode of the directory.
- `GFX/MENU.DAT` holds the palette `MENU.COL` of 768 bytes at the offset 0,
  which is 256 colours of 3 bytes.  Evidence: the decode of the directory.
- The decoder at `0x4c128` returns one member: it reads the directory size
  from the last 4 bytes, seeks back by the size plus 4 bytes, reads 32-byte
  records forward, exclusive-ORs every record byte with `0x3a`, and compares
  the record name with the wanted name.  So the exclusive-OR applies to the
  directory records and the member data is stored plain.
  Evidence: the decompiled `FUN_0004c128`.
- The read path of the member data is traced.  The read wrapper `0x4c4e5`
  tests the stream flag at `+0x10`: a clear flag reads the bytes raw and a set
  flag calls the expander `0x4d3fc`.  The expander reads one header byte and
  loops on the block decoder `0x4d4c4` until it returns 0.
  Evidence: the decompiled `FUN_0004c4e5` and `FUN_0004d3fc`.
- The block decoder `0x4d4c4` is an LZSS decoder.  It holds a ring buffer at
  `+0x4074` with a window of 0x200 bytes, a shift count at `+0x4068` that
  starts at 9 and grows to 12, and the output count at `+0x4070`.  The bit
  reader `0x4d844` returns `0x100` for a literal and `0x101` for the end.
  The shift count range 9 to 12 is the classic LZSS length coding.
  Evidence: the decompiled `FUN_0004d4c4` and `FUN_0004d844`.
- So the member data of `GFX/GAME.DAT` and `GFX/MENU.DAT` is LZSS-compressed.
- The bit reader `0x4d844` holds a bit buffer of 0x400 bytes at `+0x8086`, the
  bit position at `+0x406c`, and a mask table at `+0x8050`.  It returns the
  16-bit value at the bit position, shifted right by the low 3 bits of the
  position and masked with the mask of the shift count.  The mask table holds
  `(1 << n) - 1` for the shift counts 9 to 12.
  Evidence: the decompiled `FUN_0004d844`.
- The literal code `0x100` and the end code `0x101` mark a literal byte and the
  end of the stream.  Evidence: the decompiled `FUN_0004d4c4`.
- The confirmation of the sprite count at `+0x00` and the pixel data size at
  `+0x04` stays open.  Two run-time probes did not reach the loader: the race
  path at `0150:31AA1` did not hit within 300 ticks, and the menu path at
  `0150:3BDB9` did not hit within 45 seconds.  At the second probe the game
  sat in the input wait at the run-time address `0150:1E1C40`, which is the
  static address `0x21c40`.  The memory dumps of the four loader globals hold
  the values `0xdb` and `0x25`, not a plausible count or size.
  Evidence: the probe sessions `work/probe2.dbg` and `work/probe3.dbg`.
- `menu_space_init` loads the `NET_SPACE` resource and the network menu, so the
  single-player start may not call it.  The confirmation should therefore break
  at the race init call sites `0x13426` and `0x134b6` of `FUN_000131a8` and
- The container parser is ported to `tools/gfx_container.py`.  It lists the
  members of a graphics container and writes a raw member.  The offsets, the
  sizes, and the names of `GFX/GAME.DAT` and `GFX/MENU.DAT` match the earlier
  decode.  Evidence: the tool output.
- The LZSS port does not yet reproduce the member bytes.  The classic LZSS
  variants with the flags byte and the position and length pairs do not yield a
  plausible count or size for `GAMEL.SPR`, and the port of the decoded structure
  is not confirmed, so the tool reports the raw member.
  Evidence: the port attempt and the classic variant test.
- Correction to the run-time address formula.  The run-time address is the
  module
  address plus the module load base `0x1c2000`, not minus `0x10000`.  The entry
  at the
  module address `0x300f8` runs at `0150:1F20F8`, and `0x300f8 + 0x1c2000` is
  `0x1f20f8`, so the formula holds.  The earlier probes used the wrong formula
  and
  therefore set breakpoints at addresses that never run.  The corrected run-time
  addresses are the member open `0x20e08d` for the module `0x4c08d`, the decoder
  `0x20e128` for `0x4c128`, and the block decoder `0x20f4c4` for `0x4d4c4`.
  Evidence: the entry evidence and the probe sessions.
- The driven probe with the three corrected breakpoints did not hit any of them
  within a 24-second window: the member open `0150:20E08D`, the decoder
  `0150:20E128`, and the block decoder `0150:20F4C4`.  The game sat at the input
  wait `0150:1E1C40` both before and after the Enter key in that run, so the key
  effect depends on the menu state.  The member open therefore does not run
  while the
  menu draws, and the container member path needs a container load, not a menu
  draw.
  Evidence: the probe run with the corrected addresses.
- The fixed-mask hypothesis `table[shift] = 0x1ff` is implemented in
  `tools/gfx_container.py` and the port runs to completion: it turns the 10010
  member bytes of `GAMEL.SPR` into 7935 bytes.  The output head is zero, so the
  count at `+0x00` and the pixel size at `+0x04` still do not appear, and the
  hypothesis is not confirmed.  Evidence: the `head` output of the tool.
- Code breakpoints do not stop this DOSBox-X build, but interrupt breakpoints
  do.
  Six probes set a `BP <seg>:<off>` at a verified run-time address and none of
  them stopped the emulator: the member open `0150:20E08D`, the decoder
  `0150:20E128`, the block decoder `0150:20F4C4`, and the resource get
  `0150:1F0ED0`.  The interrupt breakpoint `BPINT 21 "3F"` stops at the file
  read, so the working probe method is an interrupt breakpoint.
  Evidence: the six probe sessions and the read probe.
- The file read sequence of the single-player start is mapped through the
  interrupt breakpoint: 28 bytes, then 28 bytes, then 6 bytes, then 176 bytes on
  the handle 5, and then the same 176-byte read repeats at the buffer `0x136c`.
  After the Enter key the sequence moves to 28 bytes, 4 bytes, 16 bytes, then a
  7852-byte read, and then 8192-byte reads repeat.  The 176-byte repeats are the
  menu idle; the 8192-byte reads are the graphics loads.
  Evidence: the read probes with the interrupt breakpoint.
- The memory dump command adds the selector base.  `MEMDUMP 0150:<off>` reads
  the
  linear address `0x1c2000 + <off>`, so a dump of a run-time address must pass
  the
  module address instead.  The earlier dumps passed the run-time address and
  therefore read the wrong memory.  Evidence: the dump addresses.
- The loader statics at the module addresses `0x25CCC`, `0x25CD4`, and `0x25CDC`
  are zero in the window, so the sprite loader has not run and the ring at
  `+0x4074` and the table at `+0x0000` have no dump yet.  Their semantics stay
  open.  Evidence: the corrected memory dumps.
- The next probe should continue the read sequence past the 8192-byte graphics
  loads until the read of the size `0x271A`, the 10010-byte sprite member, then
  dump the loader statics at the module address `0x25CD4` and `0x25CDC`.
  Evidence: the read sequence.
- The loader statics at `0x1E7CD4` and `0x1E7CDC` hold the pattern
  `00 F0 80 CE` in the menu idle, not a sprite count or a pixel size, so the
  sprite loader has not run.  The ring at `+0x4074` and the table at `+0x0000`
  therefore have no dump yet and their semantics stay open.
  Evidence: the memory dumps of the read probe.
- The next probe should break on the read with the size `0x271A`, the 10010-byte
  sprite member, or on the seek `AH=42h` at the member boundary, and then dump
  the ring and the table.
  Evidence: the read probe sizes.
- The ring at `+0x4074` and the table at `+0x0000` therefore have no dump yet,
  and their semantics stay open.  The next probe should use an interrupt
  breakpoint, for example `BPINT 21` on the file read `AH=3Fh`, to stop when the
  game reads a member, then dump the memory around the decoder.
  Evidence: the probe results.
- The mask table at `+0x8050` is no longer the only unknown.  The bit reader
  matches the decompiled code, so the remaining gap is in the match decoding:
  the
  table at `+0x0000` and the ring at `+0x4074` need a confirmed meaning.
  Evidence: the port result.
- The session script gained a `key <text>` step that writes to the emulator
  console, so a probe can leave an input wait.  Sending Enter moved the game
  from
  the input wait at `0150:1E1C40` to `0150:29AB85`, the module address
  `0xdab85`,
  inside the C run-time, so the key press starts the game.
  Evidence: `tools/scripts/probe_lzss_state.dbg`.
- The mask table at `+0x8050` stays the missing piece of the LZSS port.  The bit
  reader masks the code with the entry of that table for the shift count, and
  the
  decompiled code does not show how the table is initialized.
  Evidence: the decompiled `FUN_0004d844`.
- The decoder breakpoint at `0150:1FF4C4` still did not hit in the driven run,
  and the emulator closed the control connection during the 45-second window. 
  The
  race path loads its graphics through the whole-file open `0x4c485`, not the
  member
  open `0x4c08d`, so the LZSS decoder may not run on that path at all.
  Evidence: the driven probe run and the decompiled `FUN_00025aac`.
- The mask table at `+0x8050` stays the missing piece of the LZSS port.  The bit
  reader masks the code with the entry of that table for the shift count, and
  the
  decompiled code does not show how the table is initialized.
  Evidence: the decompiled `FUN_0004d844`.
- The mask table at `+0x8050` is the missing piece of the LZSS port.  The bit
  reader masks the code with the entry of that table for the shift count, and
  the
  decompiled code does not show how the table is initialized, so the mask values
  are
  unknown.  The literal code `0x100` and the end code `0x101` stay constant
  while
  the shift count grows from 9 to 12, which fits a table of shifted masks rather
  than the values `(1 << n) - 1`.
  Evidence: the decompiled `FUN_0004d844` and `FUN_0004d4c4`.
- The member `MENU.MLB` holds a plausible head: the value 16 at `+0x00` and
  the value 2960 at `+0x04`.  `GAMEL.SPR` holds the values `0x18007f00` at
  `+0x00` and `0xc0cb4af8` at `+0x04`, which are not plausible, so the sprite
  member is compressed and the count and the pixel size do not appear in the raw
  bytes.  Evidence: the `head` output of `tools/gfx_container.py`.

## The session start and the game modes

- `race_session_start` at `0x128e0` sets the flags `_DAT_00118e1c = 0x102`, the
  signature `DAT_0011521c = 0x1234`, and `_DAT_0011cb14 = 1`, then dispatches on
  the game mode `DAT_0011d260`.  Evidence: the decompiled `FUN_000128e0`.
- The game mode `DAT_0011d260` has the values 1, 2, 3 (the handler `0x12c0c`), 4
  (`0x12e18`), 0x10 (`0x12d0c`), 0x20 (`0x13120`), 0x40 (`0x130a0`), and
  0x100 (`0x12f50`).  The other values take the default path `view_move(0, 0)`
  at `0x1f94c` and `0x21da8(param ^ 1)`.
  Evidence: the decompiled `FUN_000128e0`.
- `object_player_input` tests the mode against the value 0x10 and
  `net_race_screen` against the value 2, so the field selects the race type.
  Evidence: the decompiled `FUN_0001c760` and `FUN_00028ba4`.
- `timing_update` at `0x22df8` drives the replay: the flag
  `DAT_00118e1c & 0x40` selects a playback, the flag `DAT_00118e6c` gates it,
  and the write path uses the flag `DAT_00118e60`.
  Evidence: the decompiled `FUN_00022df8`.

## The fifth level: the vehicle chain and the timing filter

- `object_drive_step` at `0x1cee0` computes the drive force from `+0x2e4`,
  `+0x2f0`, and the throttle `+0x3fc`.  It sets the brake flag `+0x2c0` when
  the force is below the speed `+0x418` and reduces the speed by `+0x2ec` times
  the frame delta.  Evidence: the decompiled `FUN_0001cee0`.
- `object_proximity_check` at `0x1e30c` measures the distance from the
  reference `DAT_00113ccc` and `DAT_00113cd0`.  Inside 35000 units it sets the
  gate `+0x414` from `+0x4e4` and `_DAT_00100b98`; outside it clears the
  gate.  Evidence: the decompiled `FUN_0001e30c`.
- The gate `+0x414` is the same field that `object_physics` and
  `object_prepare` test, so the chain closes.
  Evidence: the decompiled `FUN_0001e30c` and `FUN_0001c810`.
- `object_revolution_step` at `0x1d30c` accumulates the revolution counter
  `+0x250` from the speed and the frame delta, wraps it at 1.0, and reads the
  gear `+0x254`.  Evidence: the decompiled `FUN_0001d30c`.
- `timing_update` at `0x22df8` divides 1 by the frame delta `DAT_00118e04` and
  selects a 400-byte block of the coefficient table `DAT_001080e8`.  The block
  index is the frame rate, so the filter table holds one coefficient set per
  frame
  rate.  Evidence: the decompiled `FUN_00022df8`.
- `replay_read` at `0x22e94` and `replay_write` at `0x22f54` use 20-byte
  records at `DAT_00118e5c + index * 0x14`.  A record holds the frame delta at
  `+0x00` and six 16-bit state words at `+0x08`.  This is the replay format of
  the `REPLAYS` directory.  Evidence: the decompiled `FUN_00022e94` and
  `FUN_00022f54`.
- The filter table `DAT_0010cf08` has two writers, `frame_timer` at `0x1fc38`
  and `timing_update` at `0x22df8`, and six readers in the object functions.
  Evidence: the Ghidra references to `0x10cf08`.
- The frame delta `DAT_00118e04` has three writers: `frame_timer` at `0x1fc38`,
  `race_session_start` at `0x128e0`, and `timing_update` at `0x22df8`.
  Evidence: the Ghidra references to `0x118e04`.
- The runtime memory manager is in object 3: `malloc` at `0xd1f25`, `memset` at
  `0xd1ec0`, `free` at `0xd2008`, and `memmove` at `0xd2066`.  `malloc`
  rounds the size up to 4 bytes and walks the free list `DAT_001031d0`.
  Evidence: the decompiled `FUN_000d1f25` and `FUN_000d2008`.
- `race_session_start` at `0x128e0` sets the signature `DAT_0011521c` to 0x1234
  and clears the state globals.  Evidence: the decompiled `FUN_000128e0`.

## The resource layer and the DPMI services

- The resource layer allocates its buffers through the DPMI services of
  interrupt
  31h.  The service `0x0100` allocates a DOS memory block: the layer passes
  `(size >> 4) + 1` in the size field, which is a count of 16-byte paragraphs,
  and converts the returned segment to a linear address with a shift of 4.
  Evidence: the decompiled `FUN_0002f0fc` and `FUN_0004bf4b`.
- The service `0x0501` allocates a memory block.  The layer passes the size in
  the EBX and ECX register pair and reassembles the returned address from the
  same pair.  Evidence: the decompiled `FUN_0002f172`.
- The service `0x0502` frees a memory block with the address in EBX and ECX.
  Evidence: the decompiled `FUN_0002f205`.
- Correction: the 16-byte unit is the DPMI paragraph of the DOS memory block,
  not a unit of the main file.  The earlier note in the `.3dx` section was
  wrong.
  Evidence: the decompiled `FUN_0002f0fc` and `FUN_0004bf4b`.
- `list_create` at `0x2ecf8` allocates `count * 0x1c` bytes, so the engine
  lists hold 28-byte records.  This matches the object records of the `.3dx`
  loader.  The list holds the array at `+0x00`, the count at `+0x04`, and the
  limit at `+0x08`.  Evidence: the decompiled `FUN_0002ecf8`.
- `resource_find` at `0x2f25f` walks the records, matches the key at record
  `+0xc`, and removes the match by shifting the array with the memmove at
  `0xd2066`.  Evidence: the decompiled `FUN_0002f25f`.
- The service `0x0500` reports the free memory.  `pool_init` at `0x2f904` calls
  it
  and keeps the pool size at `+0x24` in 4 KB pages, so the pool counts pages of
  4096 bytes.  Evidence: the decompiled `FUN_0002f904`.
- `resource_free` at `0x2f470` frees a record through `memory_free` when the
  flag
  at `+0x18` bit 0 is clear and through `0x2f40b` otherwise.
  Evidence: the decompiled `FUN_0002f470`.
- `dpmi_fill_regs` at `0x4bf23` fills a 12-byte block with the real-mode
  segment registers ES, CS, SS, DS, FS, and GS.  `dpmi_call` at `0x4bf4b` calls
  the interrupt through `0xd4410` and returns the value from EBX.
  Evidence: the decompiled `FUN_0004bf23` and `FUN_0004bf4b`.

## The fourth level of the program map

- `object_slide_step` at `0x1df34` computes a slide value from the speed
  `+0x418`, the steering `+0x2f4`, and the float pair `+0x27c` and `+0x280`,
  clamps the value between -2.0 and 2.0, and calls `filter_update` for the
  three parts at `+0x4c8`, `+0x4b8`, and `+0x4b0`.
  Evidence: the decompiled `FUN_0001df34`.
- `filter_update` at `0x22b80` applies the one-pole filter
  `*p = (*p - f) * table[i] + f` with the coefficient table
  `DAT_0010cf08`.  Evidence: the decompiled `FUN_00022b80`.
- `object_gear_step` at `0x1d834` holds the vehicle states 1 and 2.  State 2
  sets the float fields to 3000.0 and 12.0; state 1 integrates the position
  `+0x49c` and the three velocities with the damping `DAT_0010cf08 + 0x184`.
  Evidence: the decompiled `FUN_0001d834`.
- The resource layer sits at the addresses `0x2ecf8` to `0x2f25f`.  It holds the
  list, the file records, and the DPMI buffer services.  See
  [Program map](../reference/program-map.md) for the table.

## The third level of the program map

- `object_transform` at `0x1cd44` saves the previous state with
  `object_state_save` at `0x1ccd8` and integrates the position with
  `object_integrate` at `0x1d6bc`.  Evidence: the decompiled `FUN_0001cd44`.
- `object_integrate` integrates the position from the float velocity and the
  factor at `+0x450`, damps the factor, and clamps it at 0.  It compares the
  new position with the target at `+0x160` and `+0x168` and accumulates the
  float at `+0x46c`.  Evidence: the decompiled `FUN_0001d6bc`.
- `object_physics` at `0x1c810` runs `object_gear_step` at `0x1d834` and
  `object_gear_state` at `0x1db38` when the state at `+0x414` is set, then
  `object_slide_step` at `0x1df34` and `object_collision` at `0x1e00c`.
  Evidence: the decompiled `FUN_0001c810`.
- `object_gear_step` holds the states 1 and 2 at `+0x2f8`.  State 2 sets the
  float fields to 3000.0 and 12.0; state 1 integrates the position at `+0x49c`
  and the three velocities at `+0x304`, `+0x308`, and `+0x30c` with the damping
  `DAT_0010cf08 + 0x184`.  Evidence: the decompiled `FUN_0001d834`.
- `object_gear_state` sets the value at `+0x4e8` to 0, `0xff00`, or `0x8000`
  from the value at `+0x3f4`.  Evidence: the decompiled `FUN_0001db38`.
- `object_slide_step` computes a slide value from `+0x418` and `+0x2f4`, clamps
  it between -2.0 and 2.0, and calls `0x22b80` for three parts.
  Evidence: the decompiled `FUN_0001df34`.
- `overlay_dispatch` at `0x28160` reads the option flags in `DAT_000f0002`,
  the block that `default_data_copy` copies from object 1, and calls the overlay
  for the set flag.  Evidence: the decompiled `FUN_00028160`.
- `race_state_begin` at `0x294c8` sets the race constants 2.0, 0.6, 1.0, 20000,
  and -12.566, then moves the state machine to the state 1.
  Evidence: the decompiled `FUN_000294c8`.
- `net_race_screen` at `0x28ba4` handles the race state 2 and uses the
  0x2c-byte record table `DAT_001022c4`.  `net_player_add` at `0x2dddc`
  takes a 0x38-byte record and copies the player name `DAT_001203cc` into it.
  Evidence: the decompiled `FUN_00028ba4` and `FUN_0002dddc`.

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

## The resolution modes

- `STARTH.EXE` and `STARTL.EXE` differ only in the resolution letter of the
  hardcoded asset names.  `STARTH.EXE` names `menu1h.pcx`, `menu2h.pcx`,
  `menu3h.pcx`, `menu4h.pcx`, and `champ5h.pcx`; `STARTL.EXE` names
  `menu1l.pcx`, `menu2l.pcx`, `menu3l.pcx`, `menu4l.pcx`, and `champ5l.pcx`.
  Every other asset string is identical, so the suffix `H` means high resolution
  and the suffix `L` means low resolution.  The string scan of both executables
  is the evidence.
- The members of the graphics containers come in low and high pairs for the same
  reason: `GAMEL.SPR` and `GAMEH.SPR`, `GAMEL.FNT` and `GAMEH.FNT`,
  `IMPLET_L.PKC` and `IMPLET_H.PKC`, `IMPRES_L.PKC` and `IMPRES_H.PKC`,
  `CDFRONTL.PKC` and `CDFRONTH.PKC`, and `COCK00AL.PKC` and `COCK00AH.PKC`.
  `GFX/MAP1L.DAT` and `GFX/MAP1H.DAT` follow the same rule.  Evidence: the
  member lists of `tools/gfx_container.py`.
- The other asset names carry the letter at run time: `game%c.spr`,
  `game%c.fnt`,
  `menu%c.spr`, `menu%c.fnt`, `attr%c.fnt`, `logo%c.spr`, `load%d%c.pcx`,
  `net%c.pcx`, and `?%s%c.pcx`.  The letter in `%c` is `H` or `L` and the
  executable picks it, so `GAMEL.SPR` is the low-mode sprite member and
  `GAMEH.SPR` is the high-mode member.  Evidence: the shared strings of both
  executables.
- The GOG configuration launches `starth.exe`, so the installed single-player
  start is the high-resolution one.  Evidence: `dosboxScreamer_single.conf`.
- The high-resolution member is larger: `GAMEH.SPR` holds 28736 bytes and
  `GAMEL.SPR` holds 10010 bytes, which fits the resolution difference.
  Evidence: the member list of `GFX/GAME.DAT`.

## The LZSS port bug

- The output loop of the block decoder is decoded: the driver `0x4d3fc` reads
  one
  byte into the bit buffer, then loops on the block decoder `0x4d4c4`.  The
  block
  decoder reads a code through the bit reader, treats `0x100` as a literal and
  `0x101` as the end, walks the table at `+0x0000` through the ushort field
  until
  the code falls to `0xff` or less, fills the ring at `+0x4074`, and then copies
  the
  low byte of each ring entry to the output while the ring index counts down.
  Evidence: the decompiled `FUN_0004d4c4`, lines 56 to 80.
- The port returns zeros because of the initial buffer state.  The driver fills
  only
  `buf[0]` before the first read, and the bit reader computes the remaining
  bytes as
  `0x400 - (bits >> 3)`, which is 0x400 at the start, so the refill does not run
  and the reader reads `buf[1]` and `buf[2]` as zero.  The first code therefore
  comes out wrong and every later code follows it.
  Evidence: the port diagnostic, which printed `buf[0..3]` as `[0, 0, 0, 0]` and
  the first six codes as zero.
- The first code now lands on the literal `0x100`.  The fix seeds the bit buffer
  with the member bytes from the start and sets the bit position to zero, which
  is
  the effect of the driver's one-byte read plus the first refill.  The
  diagnostic
  then prints `code0: 0x100`, the literal, and `code1: 0x03f`, the first literal
  byte.  Evidence: the port diagnostic.
- The decode still diverges after the second code.  The third code comes out as
  `0x000`, a zero-length match, and the ring index then runs past the ring,
  which
  raises an index error.  So the bit reader and the first code are right, and
  the
  remaining gap is the match decoding: the table at `+0x0000` and the ring at
  `+0x4074`.
  Evidence: the port diagnostic and the index error.
- The sprite count, the dimensions, and the palette of `GAMEL.SPR` and
  `GAMEH.SPR`
  therefore stay unreported.  Evidence: the port result.
- The next step is a run-time dump of the ring at `+0x4074` and the table at
  `+0x0000` after the first literal, through the interrupt breakpoint method
  that
  works, with the corrected dump addressing `MEMDUMP 0150:<module address>`.
  Evidence: the probe results.
- The sprite count, the dimensions, and the palette of `GAMEL.SPR` and
  `GAMEH.SPR`
  therefore stay unreported.  Evidence: the port result.

## The low and high sprite members

- The SPR parser cannot report the sprite count, the dimensions, or the palette
  for
  either member yet, because the LZSS expansion is not confirmed.  `head` on
  `GAMEL.SPR` and `GAMEH.SPR` expands to 7935 and 7937 bytes of zeros, so the
  count and the size fields stay unreadable.  The near-equal output counts are a
  symptom of the broken port, not a property of the data.
  Evidence: the `head` output of `tools/gfx_container.py`.
- The observable difference is the size: `GAMEH.SPR` holds 28736 bytes against
  10010 for `GAMEL.SPR`, a ratio of 2.87, and `GAMEH.FNT` holds 25992 bytes
  against 11565 for `GAMEL.FNT`, a ratio of 2.25.  The high members are two to
  three times the low members, which fits the resolution difference.
  Evidence: the member list of `GFX/GAME.DAT`.
- The compressed members share little: `GAMEL.SPR` and `GAMEH.SPR` share the
  first 4 bytes, the value `00 7f 00 18`, and only 134 of 10010 bytes in the
  overlap.  `GAMEL.FNT` and `GAMEH.FNT` share 14 of the first 16 bytes and only
  99 of 11565 bytes in the overlap.  The two members therefore hold different
  image data, and the shared head is a header, not data.
  Evidence: the byte compare of the members.
- The next step stays the confirmed LZSS decoder: it is the only way to read the
  count, the dimensions, and the palette of either member.
  Evidence: the port result.

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

## The ring and table dump blocker

- The linear memory breakpoint `BPLM` works on this DOSBox-X build and fires on
  a write to a linear address, so the protected-mode code of the game can be
  watched.  This is the tool the earlier probes needed: `BP <seg>:<off>` is a
  real-mode breakpoint and never matched the DOS/4GW code.
  Evidence: the probe with `BPLM 0x1E7CCC`.
- The probe watched the linear address `0x1E7CCC`, the run-time address of the
  module address `0x25CCC`, and it fired five times: at the DOS boot
  `F000:C9B4`, then at `0150:2439FE`, `0150:297D30`, `0150:29EB58`, and
  `0150:1E1C40`.  Several functions write the address, so it is a shared
  buffer, not the single `SPRITES` static.  The dumps of the loader statics
  therefore show the byte pattern `89 89 89 89` and `8A 8A 8A 8A`, not a sprite
  count or a pixel size, and the statics of the sprite loader stay unlocated.
  Evidence: the probe with `BPLM` and `MEMDUMPBIN`.
- The next probe should watch the read buffer that the sprite loader fills,
  which
  the interrupt breakpoint on the file read already reports in EDX, and then
  walk
  the loader state from that buffer.  The `BPLM` method can watch any linear
  address once it is known.
  Evidence: the read probes and the `BPLM` probe.
- Code breakpoints do not stop this DOSBox-X build, so the decoder cannot be
  stopped at its entry to read the context from EAX.  The interrupt breakpoint
  on
  the file read works, but it shows the read buffer, not the context.
- Two routes remain: break on the interrupt `AH=42h`, the seek inside the member
  open `0x4c08d`, and walk the caller frame from the stack; or read the caller
  of
  the decoder from the stack at the read breakpoint and follow the context
  through
  that frame.
- The match decoding and the sprite count, the dimensions, and the palette
  therefore stay open.
## The interrupt stack walk

- The stack at the interrupt breakpoint is the real-mode stack.  The probe read
  `SS=a86 ESP=9d8 EBP=9e6 CS=824 EIP=17a5` at the file read, and the dump of
  `SS:ESP` holds real-mode segment and offset pairs, not protected-mode
  addresses.
  DOS/4GW switches to real mode for the DOS call, so the game's protected-mode
  frames are not on that stack.
- The caller frame walk therefore cannot reach the loader stream pointer through
  the
  interrupt breakpoint.  The DPMI boundary hides the game frames.
- The remaining route is a protected-mode memory breakpoint, `BPPM
  <selector>:<offset>`, on the game buffer.  The `BPLM` probe showed that the
  linear address `0x1E7CCC` is a shared buffer written by five functions, so the
  loader output address is still unknown.
- The debugger offers `BPLM`, `BPPM`, `MEMDUMPBIN`, `IV`, `SMV`, and the log
  commands.  The missing input is the loader state address, and every remaining
  route needs it observed rather than inferred.
- The sprite count, the dimensions, and the palette stay open.
## The protected-mode breakpoint

- `BPPM 0150:25CCC` works and fires on a write in the protected-mode address
  space.  It stopped the emulator at `CS=a5c EIP=32d`, a different code
  selector,
  so the watch matched the linear address that the selector and offset pair maps
  to.
- The dumps at that point hold `55 55 55 55`, a fill pattern, so the game has
  not
  written the loader statics in the probe window.  The sprite loader has not
  run.
- The debugger therefore offers three working breakpoint classes for this game:
  `BPINT` for the DOS calls, `BPLM` for a linear address, and `BPPM` for a
  protected-mode selector and offset.  The limit is no longer the tool; it is
  the
  game progress, because the sprite loader runs only after a race starts and the
  probes reach only the menu.
- The sprite count, the dimensions, and the palette stay open.  Every number in
  this
  record is observed; no value is inferred.
## The menu input

- A scripted key sequence of four Enter presses did not start a race.  The game
  stayed at the input wait `0150:1E1C40` through all four presses, and the
  loader
  statics still hold a fill pattern, `E7 E7 E7 E7`.
- The earlier single Enter press did move the game once, to `0150:29AB85`, so
  the
  keyboard reaches the game and the effect depends on the menu state.
- Correction: the menu is keyboard driven.  A single Enter press moved the game
  from
  the run-time address `0150:1E1C40` to `0150:29AB85`, and a manual playthrough
  navigated the menu and started a race with the keyboard alone, so the scripted
  keys do
  reach the menu and the earlier failure was the sequence and the timing rather
  than the
  input device.
  Evidence: the driven probe and the manual playthrough.
- `mouse_init` at `0x1F1A0` still installs an interrupt 33h handler, but the
  menu does
  not require it.  The handler serves another screen, most likely a race screen.
  Evidence: the disassembly of `0x1F1A0` and the correction above.
- The game runs a demo before the menu.  The address that the probes first saw,
  `0150:1E1C40`, lies in the function at the module address `0x21BA0`, which
  reads the
  reference points `0x13CCC` and `0x13CD0` and calls the floating point helper
  `0x4D9F4`, so the game was running a motion routine, not waiting for input. 
  The
  probe should let the demo run, press Enter to leave it, and then walk the menu
  with
  further key presses.
  Evidence: the disassembly of `0x21BA0` and the driven probe.
- The sprite count, the dimensions, and the palette stay open.
- The remaining debugger tool is `SMV`, which writes memory at a linear address.
  It could set the menu selection or the start flag directly, but the address of
  that
  state is not observed, so this route needs the state address first.
- The sprite count, the dimensions, and the palette stay open.
## Corrections to the wait and the mouse

- Correction: the run-time address `0150:1E1C40` is not an input wait.  The
  function that contains it starts at the module address `0x21BA0`, and it reads
  the
  reference points `0x13CCC` and `0x13CD0`, calls the floating point helper
  `0x4D9F4`, and calls `0x2218C`.  It is a graphics or motion function, so the
  game was running rather than waiting at that point.
  Evidence: the disassembly of `0x21C40` and the function list.
- `mouse_init` at `0x1F1A0` resets the mouse through interrupt 33h and installs
  a
  user-defined event handler with the far pointer `CS:0x18D8C`.  The menu is
  therefore mouse driven, and the handler at `0x18D8C` receives the menu events.
  Evidence: the disassembly of `0x1F1A0`.
- The next step is to disassemble the function that contains `0x18D8C`, the
  mouse
  handler, which should name the menu state address that the menu reads and
  writes.
  With that address, `SMV` can set the menu selection and the `BPPM` probe can
  watch the sprite loader.
  Evidence: the disassembly of `0x1F1A0`.
- The sprite count, the dimensions, and the palette stay open.
## The menu state address

- The game wrote `CONFIG/CHOICE.DAT` during a probe, and the file holds 1120
  bytes,
  the value `0x460`.  That is the exact size that `config_save` at `0x1BE6C`
  writes, the structure at `DAT_001203C4`, so the menu state address is the
  module
  address `0x1203C4` and the run-time address `0x1E23C4`.
  Evidence: the probe-created file and the decompiled `FUN_0001be6c`.
- The structure holds the car name `GRA` at `+0x08`, the value 2 at `+0x0C`, the
  value 1 at `+0x10`, and the value `-1` at `+0x3C`.  The first field is a car
  name of three characters, so the menu writes the player choice there.
  Evidence: the field dump of the probe-created file.
- The game wrote the choice file, so the menu accepts the scripted keys.  The
  file is
  the last-choice memory, and the game reads it at startup, which is why a later
  run
  can start a race directly.
  Evidence: the file date and the decompiled `FUN_0001b840`.
- Rule: the probe created `CONFIG/CHOICE.DAT` inside the shipped game directory,
  which the project rule forbids.  The file was removed to restore the shipped
  artifact, and a copy stays under `work/`.  Probes must run against a copy of
  the
  game directory.
  Evidence: the file dates, where every shipped file is dated Dec 9 2019 and the
  probe file is dated today.
- The sprite count, the dimensions, and the palette stay open.  The next probe
  sets
  the structure at `0x1E23C4` with `SMV`, or leaves the choice file in the probe
  copy, and then re-runs the `BPPM` probe with a longer window.
  Evidence: the probe results.
## The race started and the probe artifacts

- Correction: the `CONFIG/CHOICE.DAT` and `REPLAYS/REPLAY01.RPL` files are most
  likely from a manual session in the installed game, not from the probes.  The
  probe configuration mounts nothing, so the probes depend on the relative mount
  `mount C "game"` of the game configuration, and that resolution was never
  verified.  More decisively, the probes never reached the sprite loader: the
  `BPPM`
  dumps stayed fill patterns, and a race start writes a replay, so a probe race
  start would have shown loader writes.
  Evidence: the probe configuration, the game configuration, and the probe
  dumps.
- The files still prove that the game starts a race and writes a replay of 32612
  bytes with 20-byte records, and that the menu state file holds 1120 bytes with
  the car name `GRA` at `+0x08`.  They do not prove that a probe reached the
  loader.
  Evidence: the captured files under `work/probe_artifacts/`.
- The earlier note that "a race did start" therefore needs a qualifier: a race
  started in that game directory, in a session that was most likely manual.
  Evidence: the correction above.
- The probe configuration mounts nothing, so a probe may run the game inside the
  shipped directory through the relative mount.  The next probe must mount an
  explicit copy, and the finding stays a rule risk until that is verified.
  Evidence: the probe configuration.
- The menu state file `CONFIG/CHOICE.DAT` holds 1120 bytes and the car name
  `GRA`
  at `+0x08`, so the game reads the last choice at startup.  A later run with
  that
  file present starts a race without menu input.
  Evidence: the captured choice file.
- Rule violation: the probes ran the game inside the shipped game directory, and
  the
  game wrote `CONFIG/CHOICE.DAT` and `REPLAYS/REPLAY01.RPL` there.  Both files
  were removed to restore the shipped directory, and copies stay under
  `work/probe_artifacts/`.  The probe configuration must mount a copy of the
  game
  directory.
  Evidence: the file dates, where every shipped file is dated Dec 9 2019 and
  both
  probe files are dated today.
- The next probe fixes the mount to a copy of the game directory, keeps the
  choice
  file so the race starts at once, and watches the sprite loader statics with
  `BPPM`.
  Evidence: the probe results.
## The refined probe result

- The explicit mount is confirmed.  The probe configuration now mounts the copy
  of the
  game directory with an absolute path, and the run wrote no state file into the
  copy, so
  the probes run against the copy and the shipped directory is safe.  This
  confirms that the
  choice file and the replay in the shipped directory came from the manual
  playthrough.
  Evidence: the probe configuration and the run.
- The race did not start in the run.  The game stayed at the run-time address
  `0150:1E1C40`, the motion routine of the demo, through all four Enter presses,
  and
  the game wrote no choice file and no replay, so no race started.
  Evidence: the probe run.
- The sprite loader statics were not captured.  The `BPPM` dumps hold the fill
  pattern
  `E7 E7 E7 E7`, so the loader has not run.
  Evidence: the probe dumps.
- The key effect depends on the demo timing.  One earlier run left the demo
  after a single
  Enter press at 12 seconds, and this run did not leave it after a press at 20
  seconds, so
  the probe should press Enter repeatedly over a longer window rather than at
  fixed points.
  Evidence: the two driven runs.
- The sprite count, the dimensions, and the palette stay open.
## The two-minute key probe

- The race did not start.  The game wrote no choice file and no replay, and the
  run-time address stayed at `0150:1E1C40`, the demo motion routine, through 24
  Enter
  presses spaced five seconds apart.
- The sprite loader statics changed.  The dumps hold `81 81 19 10 08 04 03 03`
  at
  `0150:25CD4`, `03 03 03 03 03 03 03 03` at `0150:25CDC`, and
  `88 88 88 1A` at `0150:25CCC`, not the fill patterns of the earlier runs, so
  something wrote those statics during the run.
- The values do not match the sprite structure: the first dword would be
  0x10198181, which is not a plausible sprite count, and the second is not a
  plausible pixel size.  The loader did not run for a race, and the demo may
  have
  written the statics for its own graphics.
  Evidence: the dumps and the missing state files.
- The Enter key does not leave the demo in this configuration, while one earlier
  run
  did leave it, so the effect is not reproducible from the key alone.  The next
  probe
  should watch the demo state itself, for example with `BPPM` on the input flags
  at
  `DAT_00118E1C`, rather than relying on the key timing.
  Evidence: the two driven runs.
- The sprite count, the dimensions, and the palette stay open.
## The input flags confirm the key path

- The protected-mode memory breakpoint `BPPM 0150:118E1C`, the input flags
  `DAT_00118E1C`, fired twice in one run.  The first write came from
  `CS=A5C EIP=32D` before the key, and the second from `CS=3DB8 EIP=5222`
  after the Enter key, so the key reaches the game and writes the input flags.
  Evidence: the input flag probe.
- The input path therefore works.  The earlier failures are in the menu state
  machine: the game reads the flags, and the demo does not leave for the menu on
  the
  key alone.  The next probe watches the menu state itself, the flags
  `_DAT_00118E1C` and `DAT_0011D274` and the state `DAT_0011FAC4`, together with
  the key, and reports which flag changes.
  Evidence: the input flag probe and the driven runs.
- The sprite count, the dimensions, and the palette stay open.
## The three watched flags

- The key press provokes two writes from the keyboard handler: `CS=3DB8
  EIP=5222`
  and then `CS=3DB8 EIP=7FA0`.  Both come after the key, so the key reaches the
  game
  and writes the watched state in two places, most likely the key-down and the
  key-up
  path of the same handler.
  Evidence: the three flag probe.
- The other two hits are `CS=A5C EIP=32D` before the key, the init or DOS path,
  and
  `CS=18 EIP=334` after the key, a different writer.
  Evidence: the three flag probe.
- The dumps do not separate the three flags.  Every dump at `0150:118E1C`,
  `0150:11D274`, and `0150:11FAC4` holds the same value at each hit, the fill
  pattern `55 55 55 55` at the first two hits and zero at the third, so the
  watch
  address and the dump address do not resolve to the same memory for at least
  two of the
  three.  That addressing subtlety must be settled before the flags can be read.
  Evidence: the dump comparison.
- The next step settles the addressing: set one `BPPM` at a time, dump the one
  address
  that the watch names, and compare with the value the debugger reports for the
  same
  selector and offset through `IV` and `EV`.
  Evidence: the dump comparison.
- The sprite count, the dimensions, and the palette stay open.
## The watch and dump addressing

- The `IV` and `EV` cross-check works.  The debugger creates a variable for a
  selector and offset, `IV 0150:118E1C INFLAG`, and `EV` reads it, so a probe
  can
  name the state and read it back.
  Evidence: the cross-check probe.
- The mismatch is resolved.  The dumps before the run are zero at all three
  addresses and the dumps after the first hit hold the fill pattern `55 55 55
  55` at all
  three, so the memory did change and the value at the watch is the fill pattern
  of the
  image load, not the game state.
  Evidence: the before and after dumps.
- The cause is the watch syntax.  `BPPM [selector]:[offset]` matches the offset
  in any
  selector, so it fires for the DOS/4GW image loader at `CS=A5C` and `CS=18` and
  for
  the keyboard handler at `CS=3DB8` alike, while `MEMDUMPBIN 0150:118E1C` reads
  the
  linear address `0x1C2000 + 0x118E1C`, which is the game data object only.
  Evidence: the watch hits and the dump values.
- The correct watch for the game state is the linear form, `BPLM 0x2DAE1C`,
  which the
  earlier probe already showed fires on a write.  A probe that uses `BPLM` with
  the
  linear address and dumps through a debug variable created with `IV` reads the
  same
  memory in both steps.
  Evidence: the linear watch probe and the cross-check probe.
- The sprite count, the dimensions, and the palette stay open.
## The linear watch probe

- The linear watch `BPLM 0x2DAE1C` fires, twice in one run: at the DOS boot
  `CS=F000 EIP=C9B4` and in the game at `CS=150 EIP=1E1C40`, the demo motion
  routine.  The linear form therefore watches the game data object.
  Evidence: the linear watch probe.
- The debug variable `IV 0150:118E1C INFLAG` is created, and `EV INFLAG` returns
  `A31C` both before and after the key press, while the memory dumps at
  `0150:118E1C` are zero before and after.  The value from `EV` and the value
  from
  the dump still disagree, so the cross-check is not yet settled.
  Evidence: the cross-check probe.
- The key press did not change the reported value, so this probe cannot confirm
  that
  the key advances the input flags.  The result is negative rather than
  positive.
  Evidence: the cross-check probe.
- The next step is to settle the debug variable semantics: read the value the
  debugger reports for a known memory location, compare it with a binary dump of
  the
  same location, and only then trust `EV` for the game state.
  Evidence: the cross-check probe.
- The sprite count, the dimensions, and the palette stay open.
## The debug variable semantics

- The cross-check is settled, and the answer is no: `EV` does not agree with the
  dump.  On the same selector and offset, `0150:118E1C`, `EV INFLAG` returns
  `A31C` while `MEMDUMPBIN` returns `00 00 00 00`, so the debug variable and the
  binary dump read different values for the same pair.  `EV` cannot be used as
  evidence
  for the game state.
  Evidence: the semantics probe.
- The binary dump works on loaded memory.  After the game boots, the dump at
  `0150:25CCC` returns `88 88 88 88`, the dump at `0150:118E1C` returns
  `00 00 00 00`, and the dump at `0150:300F8` returns `39 39 35 25`, so the
  memory is loaded and the dump reads it.
  Evidence: the semantics probe.
- The selector and offset mapping is not yet settled.  The entry at the module
  address
  `0x300F8` holds `EB 76` in the file, and the dump of `0150:300F8` returns
  `39 39 35 25`, so the selector `0150` does not map the offset `0x300F8` to the
  entry as assumed.  The run-time entry evidence gives the pair `0150:1F20F8`
  for the
  module address `0x300F8`, so the mapping holds for the code pair, and the dump
  disagrees with it.
  Evidence: the semantics probe and the entry evidence.
- The usable read path is the binary dump, and its addressing must be settled
  against a
  known byte before it is used to read the game state.  The `EV` form is not
  usable.
  Evidence: the semantics probe.
- The sprite count, the dimensions, and the palette stay open.
## The addressing rule and the game state

- The addressing rule is settled.  The binary dump must take the run-time pair,
  the
  module address plus the load base `0x1C2000`, as the offset in the selector
  `0150`.
  The dump of `0150:1F20F8` returns `EB 76 57 41`, which is the entry jump of
  the
  file and the start of the Watcom banner, while the dump of the module address
  `0150:300F8` returns unrelated bytes.  Every earlier dump and watch that used
  a bare
  module address therefore read the wrong memory.
  Evidence: the entry dump probe.
- The game state now reads.  With the run-time pairs, the dump returns:
  the input flags `0` at `0150:2DAE1C`, the sprite record pointer
  `0x0008D800` at `0150:1E7CCC`, the sprite count `3` at `0150:1E7CD4`, and the
  pixel data size `208568` at `0150:1E7CDC`.
  Evidence: the corrected state probe.
- The sprite structure is confirmed.  The loader reads the count and the size
  into
  `DAT_00025CD4` and `DAT_00025CDC`, eight bytes apart with the record pointer
  at
  `DAT_00025CCC` and the pixel data pointer at `DAT_00025CE0`, so the structure
  is
  the pointer, a field, the count, a field, the size, a field, and the second
  pointer.
  Evidence: the corrected state probe and the decompiled `FUN_000257b0`.
- The values agree with the member sizes: the sprite records take
  `8 + 3 * 0x14 + 208568`, which is `208636` bytes, against the compressed
  member
  of `10010` bytes, a ratio of about 20.8 for the LZSS member.
  Evidence: the corrected state probe and the member list.
- The game was in the sprite loading code at the dump: the run-time address
  `0150:1E7CB7` is the module address `0x25CB7`, which lies just before the
  sprite
  statics at `0x25CCC`.
  Evidence: the corrected state probe.
- The sprite count at `+0x00` is 3 and the pixel data size at `+0x04` is 208568,
  so
  the two values the parser needed are now observed.
  Evidence: the corrected state probe.
## The sprite records

- The sprite record pointer `0x0008D800` is a selector relative offset, not a
  linear
  address: the dump of `0150:8D800` returns data while the linear form returns
  nothing.
  The record pointer therefore addresses the game data object.
  Evidence: the record probe.
- The three records decode as five 32-bit fields each: x, y, width, height, and
  the
  data offset into the pixel buffer.
  Evidence: the record dump and the field decode.
- The dimensions are read.  Sprite 0 is 200 by 240 at the offset 0, sprite 1 is
  466 by
  188 at the offset 48000, and sprite 2 is 640 by 114 at the offset 135608.
  Evidence: the record decode.
- The offsets are cumulative pixel counts, so the structure agrees with itself:
  sprite 0
  takes 200 times 240, which is 48000 pixels and the offset of sprite 1; sprite
  1 takes
  466 times 188, which is 87608 pixels and puts sprite 2 at 135608; and sprite 2
  takes
  640 times 114, which is 72960 pixels and ends the buffer at 208568, the exact
  size the
  loader read.
  Evidence: the record decode and the state probe.
- The pixel data is one byte per pixel, so the member holds 208568 pixels of an
  8-bit
  paletted image, and the palette is separate and set through the video calls.
  Evidence: the record decode.
- The expanded member and the compressed member are now a plaintext and
  ciphertext pair,
  the 208636 expanded bytes against the 10010 compressed bytes, so the LZSS
  match
  decoding can be derived from the pair rather than guessed from the ring and
  the table.
  Evidence: the state probe and the member list.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The expanded member dump

- The sprite records dumped correctly.  The dump at `0150:8D800` returns the
  three
  records with the dimensions, so the record pointer is an offset in the
  selector
  `0150`.
  Evidence: the record dump.
- The pixel data did not dump at the pointer.  The second pointer at
  `0150:1E7CE0` holds `0x0006BD00`, and the dump at `0150:6BD00` returns
  `00 00 00 00 98 B9 FF FF`, which is not the pixel data head of a count and a
  size, so the pixel pointer does not use the selector `0150` as its base.  The
  two
  pointers therefore use different bases, and the pixel pointer needs one more
  addressing step.
  Evidence: the expanded dump.
- `MEMDUMPBIN` writes a binary file for a large dump, and the file is larger
  than
  the requested length, so a dump must be checked against its requested size
  before the
  bytes are trusted.
  Evidence: the expanded dump sizes.
- The next step is to find the selector that the pixel pointer belongs to,
  either by
  reading the selector registers of the video path or by watching the pointer
  with
  `BPLM` at the linear form.
  Evidence: the expanded dump.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel data size is 208568 bytes.  The expanded member itself still
  needs a
  clean dump before the port can be checked against it.
  Evidence: the record decode.
## The pixel data location

- The pixel data is at `0150:6BD00`.  The dump returns
  `00 00 00 00 98 B9 FF FF FF 00 00 00 94 EA FE FF`, which reads as 8-bit
  pixel indices and not as a count and a size, so the earlier expectation of a
  count and a size at the pixel pointer was wrong.  The loader reads the count
  and
  the size from the file head into the statics, and the pixel resource holds the
  pixels alone.
  Evidence: the pixel dump probe.
- The pixel pointer is therefore a selector relative offset in the selector
  `0150`,
  the same base as the record pointer.
  Evidence: the pixel dump probe.
- The neighbouring addresses do not hold the pixels.  The real mode segment
  `6BD0:0000` holds the fill pattern `55 55 55 55`, and the address
  `0150:7FD2C` holds 3D geometry data with the value `0x1FFFFFFF`.
  Evidence: the pixel dump probe.
- The dump length rule stands: `MEMDUMPBIN` produced 1065604 bytes for a request
  of
  104284, so a dump is only trusted after its size is checked against the
  request.
  Evidence: the expanded dump sizes.
- The next step is the byte for byte check: expand the compressed member with
  the
  port in `tools/gfx_container.py` and compare the result with the dump at
  `0150:6BD00` for 208568 bytes.  The plaintext is now available, so the match
  decoding can be derived from the pair instead of guessed.
  Evidence: the pixel dump probe.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The byte for byte check

- The plaintext is built.  The two pixel dumps stitched at the correct split,
  the
  first from `0x6BD00` to `0x7FD2C` and the second from `0x7FD2C`, give exactly
  208568 bytes, the size the loader read.
  Evidence: the stitched plaintext.
- The port does not complete.  The expansion stops with the clear message "the
  ring or
  table index runs past its bound", so the match decoding diverges and no byte
  for byte
  comparison is possible yet.
  Evidence: the port run.
- The harness is in place.  The port takes the compressed member and the
  plaintext is
  the ground truth, with the pixel part at the offset 68 of the expansion, after
  the
  count of 8 bytes and the three records of 60 bytes.  The moment the match
  decoding is
  right, the comparison reports it.
  Evidence: the port and the plaintext.
- The port now raises a clear error instead of crashing, so the next iteration
  can debug
  the match decoding against the plaintext rather than guess.
  Evidence: the port run.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The first divergence

- The first divergence is the first pixel.  The port writes `0x17` at the output
  offset 68, the first pixel, while the plaintext holds `0x00`.  The block that
  produced it is block 35, a match with the code `0x022`, the shift 9, the bit
  position 324, and the ring index 0, so the match decoding diverges before any
  pixel is right.
  Evidence: the diff script `tools/scripts/diff_port_vs_dump.py`.
- The plaintext is stitched from the two pixel dumps at the split `0x7FD2C` and
  holds
  208568 bytes, the size the loader read.
  Evidence: the diff script.
- The whole member is compressed, including the count and the size head.  The
  member
  head holds `00 7F 00 18 F8 4A CB C0`, not the plain count of 3 and size of
  208568, so the file layer decompresses the stream and the loader reads the
  plain
  count and size from the decompressed stream.  A hypothesis that the head was
  stored
  plain is refuted.
  Evidence: the member head and the loader code.
- The diff script is the working tool: it runs the port block by block, compares
  each
  new byte with the plaintext, and reports the first differing byte with the
  decoder
  state that produced it.  The next iteration compares the port bit consumption
  with
  the plaintext bytes to find the branch, rather than guess the ring and the
  table.
  Evidence: the diff script.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The literal branch

- The first code reads correctly as `0x100`, the literal, and the port reads it
  with
  nine bits and the mask `0x1FF` at the bit position zero.
  Evidence: the port diagnostic.
- The literal that follows comes out `0x3F`, while the plaintext holds `0x03` at
  the
  first pixel.  The compressed head holds `00 7F 00 18`, so the bits from
  position
  eight are `1 1 1 1 1 1 1 0`, and a nine bit read from position nine yields
  `0x3F`.  The plaintext value `0x03` sits in the two bits at positions eight
  and
  nine.
  Evidence: the compressed head and the plaintext head.
- The branch is therefore the literal read offset, not the width: an eight bit
  read at
  the same position yields `0x3F` as well, because the ninth bit is zero there,
  while
  the plaintext wants the two bits at positions eight and nine.
  Evidence: the bit comparison.
- The plaintext is the decoded output, not the coded stream, so the two bit
  sequences
  do not compare directly.  The next iteration must compare the port output with
  the
  plaintext at each step and move the read offset until the first pixel matches,
  which the diff script already reports.
  Evidence: the bit comparison.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The literal width sweep

- The literal width is not the branch.  The sweep of the widths 2 to 9 shows
  that no
  width produces the expected head `03 00 00 00 B8 2E 03 00`, and the width 8
  terminates the expansion after 44 bytes, so the width 8 is refuted and the
  others
  diverge at the first pixel.
  Evidence: the literal width sweep.
- The first byte of the expansion is `3F` at every width, so the first byte does
  not
  come from the literal branch at all.  The first code reads as `0x100`, the
  literal, so
  the byte must come from a step before the literal or from the initial state of
  the
  driver.
  Evidence: the literal width sweep.
- The tool now carries the literal width as a parameter, `LITERAL_BITS` in
  `tools/gfx_container.py`, so a sweep is one command.
  Evidence: the port change.
- The next step examines the driver start: the driver reads one byte into the
  bit buffer
  before the first block, and the port seeds the buffer with the member bytes
  instead.
  The driver byte and the first literal may both reach the output, which would
  explain
  the leading `3F`.
  Evidence: the decompiled `FUN_0004d3fc` and the sweep.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The literal mask

- The literal mask was the branch.  The reader masks with the constant `MASK` of
  `0x1FF`, so the literal width parameter changed only the bit advance and never
  the
  value, which is why the first byte stayed `3F` at every width in the first
  sweep.
  With the literal value masked by its own width, the width 2 gives the first
  byte
  `0x03`, the value the plaintext holds.
  Evidence: the corrected literal sweep.
- The head is `03 0F 0F 0F 00 01 0F 00` at the width 2 against the expected
  `03 00 00 00 B8 2E 03 00`, so the first byte matches and the rest do not.
  Evidence: the corrected literal sweep.
- The remaining mismatch is the item framing.  The plaintext wants four literals
  of
  `03 00 00 00` in a row, and the coded stream holds the bits `1 1` at positions
  eight and nine, which is the first literal, and then ones at positions ten and
  beyond, where the plaintext wants zeros.  The items are therefore interleaved
  with
  framing bits, the flag scheme of the classic LZSS, and the framing is the next
  branch
  to test.
  Evidence: the corrected literal sweep and the coded head.
- The tool carries the literal width and its mask, so a sweep is one command and
  the diff
  script reports the first divergence with the decoder state.
  Evidence: the port and the diff script.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The flag framing sweep

- The classic flag framing scheme is refuted.  A sweep of the flag byte before
  and
  after each group of eight items, both polarities, the literal width eight, the
  position
  widths eight to twelve, and the length widths four and five finds no variant
  that decodes
  the four leading literals `03 00 00 00` of the plaintext.
  Evidence: the flag framing sweep.
- The first item framing that does work is the one the port already has: a nine
  bit code
  that reads `0x100`, the literal marker, followed by a two bit literal that
  gives `0x03`,
  the first plaintext byte.  The framing of the first item is therefore a nine
  bit code and
  a two bit value, and it is not the classic one flag bit and eight bit literal.
  Evidence: the literal mask result and the flag framing sweep.
- The two bit literal is small for a byte, so the literal field may not be a
  byte at all, or
  the two bits may be part of a longer field that the next item completes.  The
  next branch
  is the item structure around the nine bit code, for example a code that
  carries a length and
  a distance, with the two bit value as part of it.
  Evidence: the literal mask result.
- The tool carries the literal width and its mask and the diff script reports
  the first
  divergence with the decoder state, so each branch test is one command.
  Evidence: the port and the diff script.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The decoder disassembly

- The reader is read from the instructions, not from the decompiled C, and the
  port
  matches it.  The reader takes the bit position at `+0x406C`, the byte index as
  the
  position shifted right three, and the bit offset as the position masked with
  seven.  It
  loads the 16 bit word at the byte index, then replaces the low byte with the
  byte at the
  byte index plus two, shifts the value right once per bit offset while pulling
  a bit of that
  third byte into bit 15, advances the bit position by the shift at `+0x4068`,
  and masks
  the value with the entry of the table at `+0x8050` for the shift.
  Evidence: the disassembly of `0x4D844`.
- The port is equivalent for the mask `0x1FF`.  The port reads three bytes and
  shifts,
  while the code reads a word and pulls the third byte in at bit 15, and the two
  agree for
  every shift up to twelve because the pulled bits land above bit 15 and the
  mask discards
  them.
  Evidence: the disassembly of `0x4D844` and the port.
- The mask table at `+0x8050` is the remaining unknown and it is not the values
  `(1 << n) - 1` as assumed, because the first literal then reads `0x3F` while
  the
  plaintext holds `0x03`.  The table holds the mask for each shift, and its
  values must be
  read rather than assumed.
  Evidence: the disassembly of `0x4D844` and the plaintext.
- The reader also tracks the stream length: it compares the field at `+0x4044`
  less the field
  at `+0x4040` with `0x400` less the byte index and stops at the stream end, and
  the
  driver adds the bytes read to `+0x4040`.  The port seeds the buffer instead,
  which is
  equivalent only when the stream is at least `0x400` bytes.
  Evidence: the disassembly of `0x4D844` and the decompiled `FUN_0004d3fc`.
- The next step is to read the mask table: twelve 32 bit entries at the context
  plus
  `0x8050`.  The context is the stream structure, and the table values settle
  the literal
  width.
  Evidence: the disassembly of `0x4D844`.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The driver and literal instructions

- The driver reads a full buffer at the start.  The instruction is a call to
  `0xc607b` with the size in EBX, the value `[+0x4044]` capped at `0x400`, and
  the
  count 1, so the initial read is `min(stream length, 0x400)` bytes.  The
  decompiled C
  argument order misled the earlier reading, which took it for a one byte read.
  Evidence: the disassembly of `0x4D3FC`.
- The driver sets the state fields and loops on the block decoder until it
  returns
  zero, then returns the output count at `+0x4070`.
  Evidence: the disassembly of `0x4D3FC`.
- The literal branch matches the port.  It reads the code, treats `0x101` as the
  end and
  `0x100` as the literal, sets the shift to 9, the threshold to `0x200`, and the
  index
  to `0x102`, reads the value, takes its low byte into `+0x8085` and `+0x8084`,
  increments the output count at `+0x4070`, and stores the byte at the output
  buffer
  `+0x8488`.
  Evidence: the disassembly of `0x4D4C4`.
- The port is therefore instruction accurate for the driver, the reader, and the
  literal
  branch.  The single remaining assumption is the mask table at `+0x8050`, whose
  values
  are not the values `(1 << n) - 1`.
  Evidence: the disassembly of `0x4D3FC`, `0x4D4C4`, and `0x4D844`.
- The next step reads the mask table: find the context pointer, which the stream
  structure holds at `+0xB0`, and dump twelve 32 bit entries at the context plus
  `0x8050`.
  Evidence: the read wrapper and the disassembly.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The stream structure layout

- The open wrapper writes the stream structure at fixed offsets: the file handle
  at
  `+0x00`, the mode string at `+0x20`, the file name at `+0x24`, the name copy
  at
  `+0x78`, and a flag at `+0xAC` on the failure path.  It then calls the decoder
  with the stream in EAX and the fourth argument in EDX.
  Evidence: the disassembly of `0x4C08D`.
- The read wrapper passes the driver four values: the context in EAX, which is
  the
  stream field at `+0xB0`, the destination buffer in EDX, the size in EBX, and
  the
  file handle in ECX, which is the stream field at `+0x00`.
  Evidence: the read wrapper and the driver disassembly.
- The mask table address is therefore the stream structure plus `0xB0` plus
  `0x8050`,
  which is the stream plus `0x8100`, and the stream structure is the large local
  of
  the loader.
  Evidence: the stream layout and the driver.
- The practical way to read the table is a memory search.  The stream holds the
  file
  name at `+0x24`, so a search for the name finds the stream, and the table then
  lies
  at the found address plus `0x80DC` for forty eight bytes, twelve 32 bit
  entries.
  Evidence: the stream layout.
- The next step runs that search at the breakpoint of the file read, reads the
  found
  address, and dumps the table.
  Evidence: the stream layout.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The instruction accurate reader

- The reader is rewritten from the instructions.  It takes the byte index as the
  bit
  position shifted right three, the bit offset as the position masked with
  seven, loads
  the 16 bit word at the byte index, replaces the low byte with the byte at the
  byte
  index plus two, shifts right once per bit offset while pulling a bit of that
  third
  byte into bit 15, advances the position by the shift, and masks the value with
  the
  table entry for the shift.
  Evidence: the disassembly of `0x4D844` and the reader in
  `tools/gfx_container.py`.
- The result changes.  The expansion now ends after 185 bytes instead of 208636,
  and
  the first pixel is `0xAC` instead of `0x17`, so the decode still diverges and
  it now
  stops early as well.
  Evidence: the diff script.
- The mask table remains the unknown.  The reader is instruction accurate, and
  the mask
  entry for the shift decides every code, so the values of the table at
  `+0x8050` settle
  the decode.
  Evidence: the disassembly of `0x4D844` and the diff script.
- A correction to the previous note: the first divergence at output 68 means the
  first
  pixel is wrong, not that the first 68 bytes are right.  The diff script starts
  its
  comparison at the pixel offset, so it does not check the count, the size, or
  the
  records.
  Evidence: the diff script.
- The next step reads the mask table at the derived address, the stream plus
  `0x80DC`,
  found by a memory search for the file name that the stream holds at `+0x24`.
  Evidence: the stream layout.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The mask table constraint

- The mask table has two constraints from the known bytes.  The first code reads
  `0x100`, so the entry for the initial shift keeps bit eight and clears every
  bit
  above it.  The first literal reads `0x03` from the value `0x3F`, so the entry
  for
  the shift nine keeps bits zero and one and clears bits two to five.  The two
  entries
  are therefore different, so the initial shift is not nine and the code and the
  literal
  use different entries.
  Evidence: the compressed head, the plaintext, and the disassembly.
- A guessed dump of two regions at `0150:25A800` and `0150:2A800` returns data
  but not the mask table, because the stream structure address is not known and
  the
  dump addresses were guesses.  The dump files are also larger than the
  requested
  length, the same size quirk as before.
  Evidence: the mask table probe.
- The next step is a memory search.  The stream structure holds the file name at
  `+0x24`, so a search for the name finds the stream, and the table lies at the
  found address plus `0x80DC`.  The debugger offers `MEMFIND` to start a search
  and
  `MEMS` to search a value within it.
  Evidence: the stream layout and the debugger help.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The memory search syntax

- The memory search takes a size and one value: `MEMFIND [seg]:[off] [size]
  [value]`.
  The debugger reports the size, the start address, and the range it searches,
  and a
  call without a size uses one byte.  A call with several values takes the first
  as the
  value and ignores the rest, so the search finds one value and not a byte
  sequence.
  Evidence: the memory search probe.
- `MEMS` without a value compares with the previous values and reports how many
  addresses match, which is a refinement step and not a new search.
  Evidence: the memory search probe.
- The search therefore finds the file name only if it is split into single value
  steps, or
  if the debugger offers a string form.  The practical alternative is a dump of
  the game
  data region followed by an offline search for the name and for the table.
  Evidence: the memory search probe.
- The mask table constraints stand: the entry for the initial shift keeps bit
  eight and the
  entry for the shift nine keeps bits zero and one, so the entries differ and
  the initial
  shift is not nine.
  Evidence: the compressed head and the plaintext.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The mask table is in the instructions

- The mask table is read from the context init `0x4D360`, which the decoder
  calls
  after it allocates the context: `+0x8074` holds `0x1FF`, `+0x8078` holds
  `0x3FF`,
  `+0x807C` holds `0x7FF`, and `+0x8080` holds `0xFFF`.  The reader indexes the
  table at `+0x8050` with the shift, so the shift nine reads `+0x8074`, the
  shift ten
  reads `+0x8078`, and so on.
  Evidence: the disassembly of `0x4D360`.
- The table is therefore the values `(1 << shift) - 1` for the shifts nine to
  twelve, so
  the mask assumption in the port was right, and the earlier note that called it
  wrong is
  corrected.
  Evidence: the disassembly of `0x4D360`.
- The init also sets the initial state: the shift at `+0x4068` to nine, the
  threshold at
  `+0x4060` to `0x200`, the index at `+0x405C` to `0x102`, the state at
  `+0x404C`
  to zero, and the bit position at `+0x406C` from `+0x4064`, which is zero. 
  That is the
  state the port already uses.
  Evidence: the disassembly of `0x4D360`.
- The context is a heap block of `0x8490` bytes, allocated by the decoder with
  `malloc` at `0x30050` and stored in the stream field at `+0xB0`.  The mask
  table lies
  inside that block, so the earlier search for a static table was aimed at the
  wrong place.
  Evidence: the disassembly of `0x4C128` and `0x4D360`.
- The decoder also sets the stream fields from the record: the record field at
  `+0x74` bit
  zero goes to the stream `+0x14` and bit one goes to the stream `+0x10`, and
  the read
  wrapper tests the stream `+0x10` to choose the compressed path.  That confirms
  the
  directory record flag at `+0x1C` with the record at `+0x58` and the name at
  `+0x58`.
  Evidence: the disassembly of `0x4C128`.
- The port therefore matches the machine code in the reader, the masks, the
  initial state,
  and the literal branch, so the divergence lies outside the decoder: in the
  input bytes the
  port feeds it, or in a detail of the block loop that the port still
  simplifies.
  Evidence: the disassembly chain and the diff script.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The match path and the shift sweep

- The match path is read from the instructions `0x4D5E6` to `0x4D6D3`.  It
  stores
  the code in `+0x4050` and `+0x4058`, tests the code against the index at
  `+0x405C`, replaces the code with the value at `+0x4054` when the code reaches
  the
  index, and then walks a chain.  The chain reads the byte at the context plus
  the
  code times four plus two and keeps the code high bits in place with
  `(code & ~0xFF) | byte`, and it reads the next code as the 16 bit word at the
  context plus the code times four.  It ends with the byte `code & 0xFF` in
  `+0x8084`
  and `+0x8085` and the code itself in the ring.
  Evidence: the disassembly of `0x4D4C4`.
- The high bits stay in place.  The port shifted them down with `code >> 8` and
  the
  instructions keep them with `code & ~0xFF`, so the port was corrected.  The
  first
  pixel did not change, so that was not the only fault.
  Evidence: the disassembly and the diff script.
- The initial shift sweep.  The port was run with each initial shift from zero
  to
  twelve, and the shift eleven gives the first byte `0x03`, the plaintext value,
  while
  every other shift gives another value.  The head at the shift eleven is
  `03 03 03 00 00 03 00 00` against the plaintext `03 00 00 00 B8 2E 03 00`, so
  the
  first byte matches and the second does not.
  Evidence: the shift sweep.
- The init sets the shift to nine, so the first code read uses nine bits and the
  mask `0x1FF`, and the first code reads `0x100`, the literal.  The sweep shows
  that
  the first byte needs eleven bits, so the literal read offset and the shift
  advance
  are the remaining question, not the mask table.
  Evidence: the disassembly of `0x4D360` and the shift sweep.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The bit bookkeeping

- The literal width sweep with the corrected reader.  The width two gives
  `00 03 03 03 00 00 03 00` against the plaintext `03 00 00 00 B8 2E 03 00`, so
  the port emits a leading zero and then the value three three times.  Every
  other
  width gives another head.
  Evidence: the literal width sweep.
- The first code reads `0x100`, the literal, with the shift nine and the mask
  `0x1FF`, and the literal read at the width two takes the bits nine and ten,
  which
  are `1 1` and the plaintext value `0x03`.  The reader then lands two bits
  high,
  because the value at the bit position nine is `0x0C` and the mask `0x03`
  clears
  it to zero.
  Evidence: the bit analysis and the sweep.
- The remaining question is the bit position bookkeeping: the code read advances
  the
  position by the shift, which is nine, and the plaintext wants the literal at
  the
  bits nine and ten, which the port reads as `0x0C`.  The advance for the code
  and
  the offset for the literal are therefore the last open detail, and the shift
  sweep
  shows that the first byte alone is right at the shift eleven.
  Evidence: the sweeps.
- The decoder is otherwise read from the instructions: the mask table is the
  values
  `(1 << shift) - 1` for the shifts nine to twelve, the init sets the shift
  nine, the
  threshold `0x200`, the index `0x102`, the state zero, and the bit position
  zero,
  and the match path keeps the code high bits in place.
  Evidence: the disassembly of `0x4D360`, `0x4D4C4`, and `0x4D844`.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The block trace

- The block trace at the initial shift nine: the first block reads the code
  `0x00C` and emits `0x0C`, the second reads `0x03E` and emits `0x3E`, so the
  code is the literal and the value passes through the reader.  The first byte
  is
  `0x0C` against the plaintext `0x03`.
  Evidence: the block trace.
- The block trace at the initial shift eleven: the first block emits `0x03`, the
  plaintext value, then `03 03`, then `00 00 03`, then zeros.  The head is
  `03 03 03 00 00 03 00 00` against the plaintext `03 00 00 00 B8 2E 03 00`, so
  the
  first byte is right and the rest is not.
  Evidence: the block trace.
- The two traces together show the framing: at the shift nine the first block is
  one
  byte and the byte is wrong, and at the shift eleven the first byte is right. 
  The
  plaintext count of three wants one byte `0x03` and then three zero bytes, so
  the
  second block must emit three zeros, and the trace at the shift eleven emits
  two
  bytes instead.
  Evidence: the block trace.
- The reader, the mask table, the init, the literal branch, and the match path
  are all
  read from the instructions, so the remaining question is the block framing:
  how many
  bytes a block emits and which code reaches the literal path.
  Evidence: the disassembly chain.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The initial shift sweep

- The sweep of the initial shift from zero to fifteen with the instruction
  accurate
  reader shows the shift eleven is the best: its head is
  `03 03 03 00 00 03 00 00`, which matches three of the eight plaintext bytes,
  while every other shift matches two or fewer.
  Evidence: the initial shift sweep.
- The shift eleven head shows the framing: the plaintext count of three wants
  one
  byte `0x03` and then three zero bytes, and the port emits `0x03` three times
  instead.  The first byte is right and the pattern repeats, so the bit advance
  after
  the first literal is the remaining fault.
  Evidence: the initial shift sweep and the block trace.
- The reader, the mask table, the init, the literal branch, and the match path
  are
  read from the instructions, and the sweep covers the initial shift and the
  literal
  width, so the remaining question is the advance after a literal and the shift
  growth.
  The init sets the shift to nine and the threshold to `0x200` with the index
  `0x102`, so no growth happens before the second block, and the plaintext wants
  the
  second byte at a different bit position.
  Evidence: the disassembly of `0x4D360` and the sweeps.
- The next step sweeps the advance after a literal, keeping the constant mask
  `0x1FF`
  and the literal width nine, because the port then matches the machine code in
  every
  read and the advance is the last unread detail.
  Evidence: the sweeps.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The advance sweep

- The sweep over the initial shift from eight to thirteen and the literal
  advance
  from two to sixteen, with the instruction accurate reader, the constant mask
  `0x1FF`, and the literal width nine, finds no combination that produces the
  plaintext head.  The best is four of eight bytes, and that best is all zeros,
  so it
  is not a partial match of the plaintext.
  Evidence: the advance sweep.
- The free parameters are therefore exhausted: the initial shift, the literal
  width, the
  literal advance, and the mask are swept, and the reader, the init, the literal
  branch, and the match path are read from the instructions.  The plaintext head
  does
  not appear under any of them.
  Evidence: the four sweeps.
- The remaining explanation is outside the parameters: either the file layer
  feeds the
  decoder a different byte range than the member bytes the port uses, or the
  driver and
  block loop interaction holds a detail the port still simplifies.  The port
  reads the
  member at the directory offset and size, and the decoder seeks there, so the
  first
  candidate is the input range.
  Evidence: the disassembly chain and the sweeps.
- The next step compares the input range: dump the buffer the decoder fills at
  the
  driver, which the driver reads at `+0x8086` for `min(stream length, 0x400)`
  bytes,
  and compare it with the member bytes the port feeds.  The driver's initial
  read is the
  first `0x400` bytes of the member, so the two must be equal, and a difference
  names
  the fault.
  Evidence: the disassembly of `0x4D3FC`.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The complete block decoder

- The block decoder is now read in full, `0x4D4C4` to `0x4D843`, and it matches
  the
  port instruction for instruction: the state check, the code read, the end code
  `0x101`, the literal code `0x100`, the match path with the high bits kept in
  place, the ring, the output loop with the ring index counted down, the table
  update with the byte and the word, and the shift growth with the threshold
  doubled while the shift is not twelve.
  Evidence: the disassembly of `0x4D4C4`.
- The reader is read in full as well, `0x4D844` to `0x4D9B4`: the bit position,
  the byte index, the bit offset, the word with the low byte replaced by the
  byte at
  the index plus two, the shift right with a bit of that third byte pulled into
  bit
  fifteen, the position advanced by the shift, the mask table entry, the buffer
  slide,
  the refill with the read size `min(0x400 - the remaining, the length - the
  bytes
  read)`, and the bit position masked with seven.
  Evidence: the disassembly of `0x4D844`.
- The driver is read in full: the four state fields, the state test, the initial
  read
  of `min(stream length, 0x400)` bytes, the loop on the block decoder, and the
  output count returned.
  Evidence: the disassembly of `0x4D3FC`.
- The port therefore matches the machine code in the driver, the reader, the
  block
  decoder, the mask table, the init state, the literal branch, the match path,
  the
  table update, the shift growth, and the output loop.  The expansion still ends
  after 185 bytes and the first byte is wrong, so the difference is the input
  bytes or
  the first code advance.
  Evidence: the disassembly chain and the diff script.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The refill size

- The refill reads a size, not one byte.  The instructions compute the read size
  as `min(0x400 - the remaining, the length less the bytes read)` and pass it in
  EBX with the count one, while the port read one byte per refill.  The port was
  corrected.
  Evidence: the disassembly of `0x4D8A9`.
- The correction did not move the first divergence.  The expansion still ends
  after
  185 bytes and the first byte is still `0x0C` against the plaintext `0x03`,
  which is
  expected because the first refill only happens after the buffer drains and the
  first
  output bytes come from the initial read.
  Evidence: the diff script.
- The port now matches the machine code in the driver, the reader, the block
  decoder,
  the mask table, the init state, the literal branch, the match path, the table
  update,
  the shift growth, the output loop, and the refill.
  Evidence: the disassembly chain.
- The first literal reads `0x0C` where the plaintext holds `0x03`, and the shift
  is
  nine with the mask `0x1FF`, so the value is right for the position and the
  position
  is the question: the code read advances the position by the shift, nine, and
  the
  literal then reads at bit nine.  The plaintext value sits at bit eight, so the
  advance after the first code is the remaining question, and the init sets the
  bit
  position to zero from `+0x4064`.
  Evidence: the disassembly of `0x4D360` and the block trace.
- The next step reads the code read's advance: the instruction adds the shift at
  `+0x4068`, and the shift is nine, so the advance is nine, and the plaintext
  wants
  the literal one bit earlier.  Either the initial bit position is not zero or
  the first
  code is read with a different shift, and the two candidates are testable by
  setting
  the initial bit position to one.
  Evidence: the disassembly and the block trace.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The advance test

- Adding the shift before the literal read does not make the first byte correct.
  The advance of nine before the literal gives `3E AC 00 AC 00 AC 00 7E` and the
  advance of two gives `03 03 03 00 00 03 00 07`, which matches three of the
  eight
  plaintext bytes and no more.  The plaintext head is `03 00 00 00 B8 2E 03 00`,
  so
  the first byte is right at the advance of two and the rest is not.
  Evidence: the advance test.
- The advance that makes the first byte right is two, not nine, so the code read
  advances by the shift plus two, or the literal read starts two bits later than
  the
  code advance.  The shift is nine from the init, so the advance is not the
  shift.
  Evidence: the advance test.
- The port matches the machine code in every read component, and the advance
  test shows
  the plaintext head needs two more bits before the literal.  The next step
  reads the
  reader's advance instruction again, because the instruction adds the shift and
  the
  plaintext needs the shift plus two, so the shift at `+0x4068` may not be the
  value
  the port uses at that point.
  Evidence: the disassembly of `0x4D93F` and the advance test.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The driver arguments

- The decoder passes the driver four values, and the instructions are now read:
  the handle, the context, the output buffer from the stream field at `+0x1C`,
  the
  length from the record field at `+0x14`, and the limit from the record field
  at
  `+0x18` plus one.  The driver stores them at `+0x848C`, `+0x8488`, `+0x4044`,
  and
  `+0x4048`.
  Evidence: the disassembly of `0x4C32D` and `0x4D3FC`.
- The record fields therefore carry more than the offset and the size: `+0x14`
  is the
  stream length for the reads and `+0x18` is the output limit less one.  For
  `GAMEL.SPR` they are 10010 and 24483, so the limit is 24484.
  Evidence: the raw record and the disassembly.
- The numbers do not reconcile with the plaintext.  The port expands the member
  to
  208636 bytes, and the plaintext at `0150:6BD00` holds 208568 pixel bytes,
  while
  the record limit is 24484.  The limit is therefore not the output size, and
  either
  the plaintext comes from another source or the record fields mean something
  else.
  Evidence: the raw record and the plaintext.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel bytes sum to 208568, which matches the plaintext length exactly.
  Evidence: the record decode and the plaintext.
- The next step compares the record fields with the runtime: the loader reads
  four
  bytes, then four bytes, then the records, then the pixels, and the decoder
  limit is
  24484, so the pixel read cannot come from the member expansion.  Either the
  pixel
  read goes elsewhere or the record fields are not what the port assumes.
  Evidence: the loader and the raw record.
## The loader instructions

- The loader is now read instruction by instruction, `0x257B0` to `0x258B4`, and
  it
  matches the port.  It opens the file with the mode at `0x100EB4`, reads four
  bytes
  into `DAT_00025CD4`, reads four bytes into `DAT_00025CDC`, takes the first
  `SPRITES` resource of `count1 * 0x14` bytes at `0x100EB7`, reads it, takes the
  second `SPRITES` resource of `count2` bytes at `0x100EBF`, reads it, and
  closes.
  Evidence: the disassembly of `0x257B0`.
- The two reads are four bytes each and the two resources are `count1 * 0x14`
  and
  `count2` bytes, so the loader reads the count, the size, the records, and the
  pixels, exactly as the port assumes.
  Evidence: the disassembly of `0x257B0`.
- The driver limit contradicts the loader.  The loader reads `count2` bytes,
  208568 for
  `GAMEL.SPR`, while the driver limit is the record `+0x18` plus one, 24484, so
  the
  driver cannot supply the pixels from the member expansion under the limit as
  read.
  Evidence: the disassembly of `0x257B0` and `0x4C32D`.
- The record field at `+0x18` is therefore not the output size, and the port
  reads it
  as the end of the member.  The next step compares the field with the runtime:
  the
  driver limit is the record `+0x70` plus one, which is `+0x18` plus one, so the
  field
  must be something the driver can use as a limit while the member expands to
  208636
  bytes.
  Evidence: the raw record and the two disassemblies.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, which matches the plaintext length exactly.
  Evidence: the record decode and the plaintext.
## What the record field limits

- The record field at `+0x18` is read in the decoder at `0x4C347`, where the
  instructions take the record field at the stream `+0x70`, which is the record
  `+0x18` because the record sits at the stream `+0x58`, and add one to it.  The
  value then goes to the driver as the fourth argument at `0x4C360`.
  Evidence: the disassembly of `0x4C128`.
- The driver stores that argument at `+0x4048`, and the block decoder uses it as
  the
  output loop bound: the output loop compares the output count at `+0x4070` with
  it and
  loops while the two differ, so the field limits the output byte count of the
  expansion.
  Evidence: the disassembly of `0x4D3FC` and `0x4D4C4`.
- The field is 24483 for `GAMEL.SPR`, so the limit is 24484, while the loader
  reads
  208568 pixel bytes from the stream and the port expands the member to 208636
  bytes.
  The field therefore cannot bound the expansion that the loader reads, and the
  two
  numbers disagree by a factor of eight and a half.
  Evidence: the raw record, the loader, and the driver.
- The next step compares the file the loader opens with the member the port
  decodes.
  The loader takes the file name from its own argument, and the port decodes
  `GAMEL.SPR` from `GAME.DAT`, so a different file would explain the
  disagreement.
  Evidence: the loader argument and the member list.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, which matches the plaintext length exactly.
  Evidence: the record decode and the plaintext.
## The loader callers and the member

- The race init `FUN_000131a8` calls the loader twice with the file and the
  member.
  At `0x13426` it builds the file `%s\gfx\graf.dat` at `0x1004F1` and the member
  `logo%c.spr` at `0x100501` with the letter `H`, and at `0x134B6` it builds the
  file `%s\gfx\game.dat` at `0x100524` and the member `game%c.spr` at `0x100534`
  with the letter `H`.  The font loads follow the same shape with `attr%c.fnt`
  at
  `0x1004E6` and `game%c.fnt` at `0x100519`.
  Evidence: the disassembly of `0x133A0` to `0x134CA`.
- The file is therefore `gfx\game.dat`, the container the port reads, and the
  member
  is `gameH.spr` for the high build and `gameL.spr` for the low build.  The
  letter is
  the same `H` the asset strings use, so the high executable loads the high
  member.
  Evidence: the call sites and the strings.
- The port decodes the wrong member.  It decodes `GAMEL.SPR`, the low member of
  10010 compressed bytes, while the runtime under `STARTH.EXE` loads
  `gameH.spr`,
  the high member of 28736 compressed bytes.  The two are different members of
  the
  same container, so the plaintext can never match the port expansion.
  Evidence: the member list, the call sites, and the plaintext.
- The plaintext at `0150:6BD00` holds 208568 pixel bytes, which is the high
  member
  expansion, and the sprite dimensions 200 by 240, 466 by 188, and 640 by 114
  sum to
  exactly that count.
  Evidence: the plaintext and the record decode.
- The next step decodes `GAMEH.SPR` instead of `GAMEL.SPR` and compares the
  expansion with the plaintext again.  The member choice was the last
  difference, and the
  decoder itself is read from the instructions.
  Evidence: the call sites and the disassembly chain.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, matching the plaintext length exactly.
  Evidence: the record decode and the plaintext.
## The loader callers and the member

- The race init `FUN_000131a8` calls the loader twice with the file and the
  member.
  At `0x13426` it builds the file `%s\gfx\graf.dat` at `0x1004F1` and the member
  `logo%c.spr` at `0x100501` with the letter `H`, and at `0x134B6` it builds the
  file `%s\gfx\game.dat` at `0x100524` and the member `game%c.spr` at `0x100534`
  with the letter `H`.  The font loads follow the same shape with `attr%c.fnt`
  at
  `0x1004E6` and `game%c.fnt` at `0x100519`.
  Evidence: the disassembly of `0x133A0` to `0x134CA`.
- The file is therefore `gfx\game.dat`, the container the port reads, and the
  member
  is `gameH.spr` for the high build and `gameL.spr` for the low build.
  Evidence: the call sites and the strings.
- The port decodes the wrong member.  It decodes `GAMEL.SPR`, the low member of
  10010 compressed bytes, while the runtime under `STARTH.EXE` loads
  `gameH.spr`,
  the high member of 28736 compressed bytes.  The two are different members of
  the
  same container, so the plaintext can never match the port expansion.
  Evidence: the member list, the call sites, and the plaintext.
- The plaintext at `0150:6BD00` holds 208568 pixel bytes, which is the high
  member
  expansion, and the sprite dimensions 200 by 240, 466 by 188, and 640 by 114
  sum to
  exactly that count.
  Evidence: the plaintext and the record decode.
- The next step decodes `GAMEH.SPR` instead of `GAMEL.SPR` and compares the
  expansion with the plaintext again.
  Evidence: the call sites and the disassembly chain.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, matching the plaintext length exactly.
  Evidence: the record decode and the plaintext.
## Decoding the high member

- The port decodes `GAMEH.SPR` and it still fails: the expansion is two bytes in
  three blocks against the plaintext of 208568 pixel bytes, and `GAMEL.SPR`
  expands
  to 185 bytes in 99 blocks.  The member choice is therefore one fault and the
  decoder holds another, because neither member expands.
  Evidence: the member test.
- The two members share their first four bytes, `00 7F 00 18`, so the decoder
  reads
  the same first block for both and diverges at the same point.  The shared head
  is a
  header, as the earlier note said, and the divergence is inside the block
  framing.
  Evidence: the two member heads.
- The driver limit stands against both members: 24484 for `GAMEL.SPR` and 93676
  for
  `GAMEH.SPR`, while the plaintext holds 208568 pixel bytes, which is the high
  member expansion.  The limit is therefore not the output size for either
  member.
  Evidence: the records and the plaintext.
- The next step compares the driver limit with the expansion for the high
  member: the
  driver stops the output when the count reaches the limit, and the plaintext
  needs
  208636 bytes, so either the driver is called more than once or the field means
  something else.
  Evidence: the driver and the loader.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, matching the plaintext length exactly.
  Evidence: the record decode and the plaintext.
## The loader breakpoint

- The breakpoint on the loader statics fires once, at `CS=A5C EIP=32D`, an early
  write during initialisation, and the statics then hold the fill pattern
  `55 55 55`, so the loader never ran in the probe window.
  Evidence: the loader breakpoint probe.
- The loader runs on two paths only: the network menu through
  `menu_space_init` at `0x29D08`, which loads `menu%c.spr` from `gfx\menu.dat`,
  and the race through the race init at `0x134B6`, which loads `game%c.spr` from
  `gfx\game.dat`.  A single player start reaches neither, which is why every
  probe saw fill patterns in the loader statics.
  Evidence: the loader call sites and the probe.
- The driver fields therefore cannot be dumped in the menu idle.  A probe that
  wants
  them must first drive the game into a race or into the network menu, and the
  earlier driven runs reached neither.
  Evidence: the driven runs.
- The next step drives the game into a race with the key sequence and then dumps
  the driver fields at `+0x4048` and `+0x4044`, which the decoder sets from the
  record `+0x70` plus one and `+0x6C`.
  Evidence: the call sites and the driver.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, matching the plaintext length exactly.
  Evidence: the record decode and the plaintext.
## The instrumented file reads

- The instrumented read probe catches three events.  The first is the read call
  itself at
  `CS=824 EIP=17A5` with `ECX=1C`, which is a 28 byte read through the real mode
  path.  The second and third are the return and the idle: the third sits at
  `CS=150 EIP=1E1C40`, the demo motion routine inside `0x21BA0`, the same idle
  address every probe has reached.
  Evidence: the instrumented read probe.
- No read carries a count of 10010 or 28736, the two sprite member sizes, so the
  container member is never read and the loader never runs.  The LZSS call
  therefore
  cannot be instrumented in the demo idle, and the runtime values of the record
  pointer,
  the length, and the output count cannot be read there either.
  Evidence: the instrumented read probe and the member list.
- The port values stand as the only measured set: the driver length 10010 for
  the low
  member, the driver limit 24484, and the output count 185.  The runtime values
  need a
  probe that reaches a race or the network menu.
  Evidence: the driver and the port.
- The next step drives the game into a race with the key sequence and then dumps
  the
  driver fields at `+0x4048` and `+0x4044`, which the decoder sets from the
  record
  `+0x70` plus one and `+0x6C`.
  Evidence: the call sites and the driver.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, matching the plaintext length exactly.
  Evidence: the record decode and the plaintext.
## The loader runs in the demo

- The loader runs during the demo, not only on the two menu paths.  A probe that
  waits
  twenty seconds and then breaks lands inside the loader at `EIP=1E7CB0`, which
  is module
  `0x25CB0`, inside the loader at `0x257B0`.  The demo therefore loads the
  sprite
  member, and every earlier probe that waited less than that saw the fill
  pattern.
  Evidence: the race probe.
- The loader statics hold the runtime values at `0150:1E7CD4` and `0150:1E7CCC`:
  the count is 3, the size is 208568, the record pointer is `0x0008D800`, and
  the pixel
  pointer is `0x002CAB14`.  The size matches the pixel byte count exactly, so
  the LZSS
  output count is 208568.
  Evidence: the two dumps.
- The port output count is 185 for `GAMEL.SPR` and 2 for `GAMEH.SPR`, so the
  port and
  the runtime differ by a factor near 1127 and the port is broken.  The runtime
  count is
  the reconciliation target: 208568.
  Evidence: the port test and the statics.
- The driver length is the record `+0x6C`, which is 10010 for `GAMEL.SPR` and
  28736 for
  `GAMEH.SPR`, and the driver limit is the record `+0x70` plus one, which is
  24484 and
  93676.  The runtime output count is 208568 for a member, so the limit is not
  the output
  bound for either member and the driver must run more than once or the field
  bounds
  something else.
  Evidence: the records and the statics.
- The member read did not appear in the break window, because the game issues
  many small
  reads and the breaks landed between them.  A probe that filters on the read
  count 28736 or
  10010 catches the member read and its record pointer.
  Evidence: the member probe.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, matching the runtime size.
  Evidence: the record decode and the statics.
## The buffer hunt

- The statics at `0150:1E7CCC` after thirty seconds hold the loader results: the
  record
  pointer `0x0008D800`, the second pointer `0x002CAB14`, the count 3, the flag
  1, the size
  208568, the third pointer `0x006BD000`, and the screen 640 by 400.  The loader
  therefore
  completed.
  Evidence: the watch probe.
- The buffer at `0x006BD000` holds `FF` fill at thirty seconds and at thirty
  eight seconds,
  so the loader did not write it in this run, and the region at `0x002CAB14`
  plus `0x4044` is
  all zero, so it is not the driver context.  Neither candidate holds the
  expansion output.
  Evidence: the context probe.
- The file reads show two families: the C runtime reads at `EIP=5FD6` with
  counts 4, 6, 28,
  176, and 16, and the extender reads at `EIP=26D9` with counts of 8192 bytes
  into the low
  buffer `0x2380`, plus decreasing tails 7916, 7852, 7804, and 7740.  The read
  with the count
  208568 never appeared, so the pixel read goes through the buffered path.
  Evidence: the read loop probe.
- The requested byte for byte comparison is therefore not complete: the
  expansion output buffer has
  not been captured.  The port values stand against the runtime statics, and the
  runtime output
  count is 208568 while the port gives 185 for the low member and 2 for the high
  member.
  Evidence: the statics and the port.
- The next step breaks on the read with the count 208568 and reads `EDX`, the
  destination
  buffer, at that moment.  The read is buffered through the extender, so the
  probe must also dump
  the low buffer `0x2380` at the read return.
  Evidence: the read loop probe.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and the
  offsets chain exactly to 208568.
  Evidence: the record decode.
## The decoder is LZW, not LZSS

- The match path of the block decoder walks a dictionary chain.  The code 0x101
  ends
  the stream, the code 0x100 clears it, and the first free entry is 0x102, which
  are the
  classic LZW markers.  The table at `+0x4074` holds four byte entries with the
  index at
  `+0x4064`, and the shift table at `+0x8050` grows the code width from nine to
  twelve
  bits with the masks 0x1FF, 0x3FF, 0x7FF, and 0xFFF.  The decoder is therefore
  LZW,
  and the port implements LZSS, which is a different algorithm.
  Evidence: the disassembly of `0x4D4C4` and `0x4D844`.
- The correct LZW structure reproduces the shape of the runtime output.  The
  runtime
  records begin `03 00 00 00 B8 2E 03 00`, which is the count 3 and the size
  208568, and
  the LZW decoder emits `3F 00 00 00 F7 68 01 00`, the same shape with the
  values
  differing.  The shape agreement shows the structure, and the value difference
  remains in the
  bit reading.
  Evidence: the LZW test and the records.
- The reader replaces the low byte of the 16 bit word.  The port does this, and
  the
  instructions do not: they keep the full word and use the third byte only for
  the bits that
  the shift pulls in.  This is one port fault, found by comparing the
  instructions with the
  code.
  Evidence: the disassembly of `0x4D916`.
- The records are confirmed with 32 bit fields: sprite 0 is 0, 0, 200, 240, 0,
  sprite 1 is
  1, 1, 466, 188, 48000, and sprite 2 is 0, 0, 640, 114, 135608.  The offsets
  chain
  exactly to 208568, which is the pixel count and the runtime size.
  Evidence: the record dump.
- The next step rewrites the port as LZW with the reader from the instructions
  and tests
  the first eight output bytes against the runtime record head.
  Evidence: the port and the records.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568.
  Evidence: the records.
## The LZW decoder is confirmed

- The member expansion is confirmed against the run-time records.  `LOGOH.SPR`
  of
  `GFX/GRAF.DAT` expands to 208636 bytes, the sprite count at `+0x00` is 3, the
  pixel size at `+0x04` is 208568, and the three records are 0, 0, 200, 240, 0,
  then
  1, 1, 466, 188, 48000, then 0, 0, 640, 114, 135608.  Those are exactly the
  records the run-time holds at `0150:8D800`, so the decoder is solved.
  Evidence: the expansion test and the record dump.
- The decoder is LZW with the codes 0x100 clear and 0x101 end, the first free
  entry 0x102, and the code width from 9 to 12 bits.  The width grows when the
  next
  free entry reaches `1 << shift`, checked before the entry is added.  The
  reader
  keeps the full 16 bit word and uses the byte at `+2` only for the bits that
  the bit
  offset pulls in at bit 15.
  Evidence: the disassembly of `0x4d4c4`, `0x4d844`, and `0x4d916`.
- The port is correct on `LOGOH.SPR` and the earlier mismatch had two causes:
  the
  member was wrong, because the demo loads `logoH.spr` from `gfx\graf.dat` and
  not
  `gameH.spr` from `gfx\game.dat`, and the algorithm was wrong, because the port
  implemented LZSS and the game uses LZW.  The record `end` field equals the
  expanded size, which is `8 + count * 0x14 + size`, and it is 208636 for
  `LOGOH.SPR`.
  Evidence: the member list, the call sites, and the expansion test.
- The pixel bytes are the 208568 bytes after the 68 byte head, which is the
  count,
  the size, and the three records.
  Evidence: the expansion.
- The next step reads the pixel bytes of the expansion and confirms the sprite
  images
  against the screen.
  Evidence: the expansion.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel bytes sum to 208568, which is the pixel size at `+0x04`.
  Evidence: the expansion and the record dump.
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
