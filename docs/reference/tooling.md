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
| `tools/gfx_container.py` | list and extract the members of a graphics container |
| `tools/export_assets.py` | export the graphics assets as PNG files |
| `tools/sound_bank.py` | decode the sound formats: the banks, the samples, and the songs |
| `tools/member_probe.py` | hook the container member open and report the member names |
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
names, offsets, sizes, end offsets, and flags.  The same container shape serves
the sound bank `DATA/SOUND.DAT`, whose 92 members are named `<name>.RAW`.

```sh
python3 tools/gfx_container.py info GFX/GAME.DAT
python3 tools/gfx_container.py info DATA/SOUND.DAT
python3 tools/gfx_container.py member DATA/SOUND.DAT CRY.RAW --out work/cry.raw
python3 tools/gfx_container.py member GFX/GAME.DAT GAMEL.SPR --out work/gamel.spr
python3 tools/gfx_container.py head GFX/GAME.DAT GAMEL.SPR
python3 tools/gfx_container.py font GFX/GRAF.DAT MENUL.FNT
python3 tools/gfx_container.py render GFX/GRAF.DAT MENUH.FNT 33
python3 tools/gfx_container.py lzw DATA/MAP1.DAT
python3 tools/gfx_container.py dump DATA/MAP1I.DAT --out work/map1i
python3 tools/gfx_container.py list DATA/MAP1I.DAT MAP1A.DTR OPPONENTS
```

`font` prints the font layout of a member: the count, the descriptors, the glyph
tables, and the tail.  `render` prints one glyph as characters in the confirmed
four bit format, with two pixels per byte and the high nibble first, so a zero
nibble is a transparent pixel.

`dump` writes every member under the output directory, expands the members
with the flag bit 1 set, and checks each expanded size against the recorded end
field.  It also parses the known lists of a text member into structured
records.

`list` parses one named list of a member and prints its field names, the record
size against the loader size, and every record.

`lzw` expands a plain LZW file and prints the text.  The `DATA` files
(`MAP*.DAT`, `GEN.DAT`, `LOCAL.DAT`) hold one LZW stream with no container
around it, and the reader is the one the graphics members use.

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

### `tools/sound_bank.py`

The tool decodes the three sound formats.  It uses the container and the list
parse of `tools/gfx_container.py`, so the two tools agree on the container shape
and the `SOUND_FX` list shape.

```sh
GAME=~/Documents/Screamer.app/Contents/Resources/game
python3 tools/gfx_container.py member "$GAME/DATA/MUSIC.DAT" DRUM.BNK --out work/DRUM.BNK
python3 tools/sound_bank.py bank work/DRUM.BNK
python3 tools/sound_bank.py samples "$GAME/DATA/SOUND.DAT" \
  --lists "$GAME/DATA/GEN.DAT" "$GAME/DATA/MAP1I.DAT" "$GAME/DATA/MAP2I.DAT" \
  "$GAME/DATA/MAP3I.DAT" "$GAME/DATA/MAP4I.DAT" "$GAME/DATA/MAP5I.DAT" "$GAME/DATA/MAP6I.DAT"
python3 tools/sound_bank.py samples "$GAME/DATA/SOUND.DAT" \
  --lists "$GAME/DATA/GEN.DAT" "$GAME/DATA/MAP1I.DAT" --out work/samples
python3 tools/gfx_container.py member "$GAME/DATA/MUSIC.DAT" MUSIC1.HMP --out work/MUSIC1.HMP
python3 tools/sound_bank.py song work/MUSIC1.HMP
python3 tools/sound_bank.py song work/MUSIC1.HMP --out work/midi
```

`bank` parses an `ADLIB-` instrument bank.  It reports the header, the
instrument table, and the instrument records.  Every record prints the eight OPL
register and value pairs in the driver write order: the register `0x20` at `+0x06`,
`0x40` at `+0x0A`, `0x60` at `+0x0D`, `0x80` at `+0x11`, `0xE0` at `+0x13`,
`0x23` at `+0x17`, `0x43` at `+0x1A`, and `0xC0` at `+0x1B`.

`samples` parses `DATA/SOUND.DAT` and the `SOUND_FX` lists of the list
containers.  It joins the list names with the members, prints the sample records,
and derives the runtime flags of the loader at `0x1f2e0`.  The `--out` option
writes one WAV file per sample.  The samples are 8-bit unsigned mono, and the
default rate is 11025, which the HMI digital driver init at `0x1f1fb` passes.

`song` parses an HMP song.  It reports the header, the instrument table, and the
chunks.  Every chunk prints the chunk number, the track number, the size, the event
count, the tick count, and the event mix.  The `--out` option writes one standard
MIDI file per chunk, and the delta converts from the LSB-first form of the game to
the standard MSB-first form.

### `tools/member_probe.py`

The tool hooks the container member open at run time and reports the member names.
The protected-mode and linear breakpoints watch a memory change and the `BP`
breakpoint is a real-mode breakpoint, so none of them fire at the protected-mode
member open.  The tool patches a copy of the game executable with the INT 3 opcode
`0xCC` at the file offset of the member open `FUN_0004C08D` and of its call site
in the sample loader, sets the `BPINT 3` breakpoint, runs the game, and reads the
member name from the stack at every stop.

```sh
python3 tools/member_probe.py --limit 40
```

The debugger consumes the INT 3 opcode, so the guest resumes at the next byte and
the patched `push esi` is skipped.  The tool repairs the Watcom frame: it decrements
the stack pointer by four and writes the `ESI` value at the new top, which is exactly
what the skipped `push esi` does.  The member open then runs with a correct frame,
and the opcode stays in place, so every member open fires the same breakpoint.

One run observes 55 member opens in the demo window.  The first is `MENU.dtr`, so the
menu space init loads the menu data first.  The sample loads are the `SFXSBASE`
records and the two `MAP1` records, 14 members from `skidfx.RAW` to `airplane.RAW`.
The five unlisted members `CRY.RAW`, `TOUR0.RAW`, `TOUR1.RAW`, `TOUR2.RAW`, and
`TRAIN.RAW` do not appear, so the run-time trace confirms the static conclusion.
The module base varies between runs, so the hook recognizes the stop by the patched
bytes and not by the module address.  The tool restores the game copy at the end of
every run.

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

A memory watch reports a byte change in the emulator output:

```text
DEBUG: Memory breakpoint (Prot): <selector>:<offset> - <old> -> <new>
```

The `(%02X)` field in `BPLIST` is the new byte at the watched address.  A `show`
step prints the captured emulator output, so the fires of the window appear in
order.  The dump offset is the linear address, and the length is a hexadecimal
number.  The file offset of an object page maps to the linear address with the
offset plus `0x1B7800`.
Evidence: the watch-fire probe and the dump-base probe under `tools/scripts/`.

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

## The rebuild harness

`tools/rebuild/` holds the rebuild framework, the module records, and the
rebuilt modules.  The build runs in a container, because the OpenWatcom V2
release has no macOS asset.  The container holds the extracted toolchain, so
no installer run is needed.

| Item | Value |
| --- | --- |
| Container | `arlaneenalra/watcom-docker@sha256:16e4845ddd0df7bc877b45d103aacf66bc48aaab6d1c61d6bcaa9c1274f06a65` |
| OpenWatcom | V2.0 beta, the build date Oct 24 2026 |
| Container tools | `wcc386`, `wlink`, `wasm` in `/opt/watcom/binl64` |
| DOS-hosted asset | `open-watcom-2_0-c-dos.exe`, 127899553 bytes |
| Linux asset | `open-watcom-2_0-c-linux-x64`, 129121153 bytes, SHA-256 `74cee369f527889144fe840f8fff3df78ed541b907f624334c12248e1435b8b3` |
| Release tag | `Current-build` |

The files:

| File | Role |
| --- | --- |
| `driver.c` | the rebuilt driver: the eight register pairs as immediates, register first and data second, with no C run-time |
| `chk.asm` | the `__CHK` stack check helper and the `_cstart_` start symbol, which runs the segment guard, sets `ES` to `DS`, and then starts `main_` |
| `stack.asm` | the `STACK` segment that the DOS/4G extender needs (the error 1317) |
| `cstart.c` | the C start stub |
| `free.c` | the rebuilt `free` module, the smallest named module in the program map |
| `free_main.c` | the `free` test harness: the `main` markers and the stand-in `nfree` |
| `exit.c` | the rebuilt `exit` module and its helper |
| `exit_main.c` | the `exit` test harness: the hook slots and the terminator stub |
| `printf.c` | the rebuilt `printf` module, after the run-time file `bld/clib/streamio/c/printf.c` |
| `printf_main.c` | the `printf` test harness: the `__iob` table and the print-engine stub |
| `shutdown.c` | the rebuilt `engine_shutdown` module, built with the stack-frame option `-of+` |
| `shutdown_main.c` | the `engine_shutdown` test harness: the eight callee stubs |
| `memcpy.asm` | the rebuilt `memcpy` module, the only module with no call and no data reference |
| `memcpy_main.c` | the `memcpy` test harness: the source and target buffers |
| `error_report.c` | the rebuilt `error_report` module, the first game module, a switch over 20 codes with a compiler jump table that the link does not relocate |
| `error_report_main.c` | the `error_report` test harness: the message table and the variadic printer stub |
| `bios_video.c` | the rebuilt `bios_video` module: the register sets and the interrupt 10h call |
| `bios_video_main.c` | the `bios_video` test harness: the fill and call markers |
| `list_destroy.c` | the rebuilt `list_destroy` module: the guard, the `free` call, and the return |
| `list_destroy_main.c` | the `list_destroy` test harness: the set pointer, the null pointer, and the markers |
| `memory_available.c` | the rebuilt `memory_available` module: the two heap probes and the sum |
| `memory_available_main.c` | the `memory_available` test harness: the probe stand-ins and the markers |
| `frame_step.c` | the rebuilt `frame_step` module: the three calls in call order |
| `frame_step_main.c` | the `frame_step` test harness: the three callee stubs |
| `video_fill.c` | the rebuilt `video_fill` module, the active C build with `-of+`: the size pair 480 and 400 and the video buffer pointer |
| `video_fill.asm` | the alternative assembly build of `video_fill`, kept alongside for the later Watcom 10.x build; it matches the original with only the link-time addresses |
| `video_fill_main.c` | the `video_fill` test harness: the copy stub and the pointer check |
| `memset.asm` | the rebuilt `memset` module, byte-identical to the original: the dword filler |
| `memset_main.c` | the `memset` test harness: the filled buffer check |
| `dpmi_fill_regs.asm` | the rebuilt `dpmi_fill_regs` module, byte-identical: the six segment words |
| `dpmi_fill_regs_main.c` | the `dpmi_fill_regs` test harness: the segment set check |
| `memmove.asm` | the rebuilt `memmove` module, byte-identical: the dword and byte copy |
| `memmove_main.c` | the `memmove` test harness: the copied buffer check |
| `fp_helper.asm` | the rebuilt `fp_helper` module, byte-identical: the FPU rounding helper |
| `fp_helper_main.c` | the `fp_helper` test harness: the run marker |
| `vesa_set_mode.asm` | the rebuilt `vesa_set_mode` module: the VBE mode set through the real-mode interrupt path |
| `vesa_set_mode_main.c` | the `vesa_set_mode` test harness: the data area and the run marker |
| `vesa_alloc.asm` | the `vesa_set_mode` harness allocator stub: the info block in `edi` |
| `memset_runtime.asm` | the rebuilt run-time `memset` module: the value replication and the core call |
| `memset_runtime_main.c` | the `memset_runtime` test harness: the core stub and the buffer check |
| `dpmi_call.asm` | the rebuilt `dpmi_call` module: the four arguments and the DPMI entry call |
| `dpmi_call_main.c` | the `dpmi_call` test harness: the DPMI entry stub and the result check |
| `fprintf.c` | the rebuilt `fprintf` module, after the run-time file `bld/clib/streamio/c/fprintf.c` |
| `fprintf_main.c` | the `fprintf` test harness: the `__iob` table and the print-engine stub |
| `default_data_copy.asm` | the rebuilt `default_data_copy` module: the 32-dword default data copy |
| `default_data_copy_main.c` | the `default_data_copy` test harness: the source and target blocks |
| `hud_update.c` | the rebuilt `hud_update` module: the three overlay calls, gated on the session state |
| `hud_update_main.c` | the `hud_update` test harness: the session state and the overlay stubs |
| `stream_status.c` | the rebuilt `stream_status` module: the word at `stream + 0x70` |
| `stream_status_main.c` | the `stream_status` test harness: the stream record and the status check |
| `race_state_update.c` | the rebuilt `race_state_update` module: the state dispatch through an if-else chain |
| `race_state_update_main.c` | the `race_state_update` test harness: the state words and the step stubs |
| `fpu_stub.asm` | the FPU run-time stubs of the `race_state_update` harness |
| `music_bank_reset.c` | the rebuilt `music_bank_reset` module: the three field stores of the music bank record |
| `music_bank_reset_main.c` | the `music_bank_reset` test harness: the bank record check |
| `object_gear_state.c` | the rebuilt `object_gear_state` module: the gear state mapping through an if-else chain |
| `object_gear_state_main.c` | the `object_gear_state` test harness: the gear state and the field check |
| `sprintf.c` | the rebuilt `sprintf` module, after the run-time file `bld/clib/streamio/c/sprintf.c` |
| `sprintf_main.c` | the `sprintf` test harness: the `__iob` table and the format engine stub |
| `object_prepare.c` | the rebuilt `object_prepare` module: the three preparation calls |
| `object_prepare_main.c` | the `object_prepare` test harness: the step stubs and the state gate |
| `filter_update.c` | the rebuilt `filter_update` module: the one-pole filter with the table pointer |
| `filter_update_main.c` | the `filter_update` test harness: the stack-local filter table and the value check |
| `video_block_swap_b.asm` | the rebuilt `video_block_swap_b` module: the 200 by 160 dword swap |
| `video_block_swap_b_main.c` | the `video_block_swap_b` test harness: the video buffer and the swap check |
| `video_buf_load.c` | the rebuilt `video_buf_load` module: the `VIDEO_BUF` lookup and the two pointer stores |
| `video_buf_load_main.c` | the `video_buf_load` test harness: the lookup and release stubs |
| `menu_enter.c` | the rebuilt `menu_enter` module: the saved value and the two state stores |
| `menu_enter_main.c` | the `menu_enter` test harness: the state words and the frame stub |
| `player_update.c` | the rebuilt `player_update` module: the five step calls in call order |
| `player_update_main.c` | the `player_update` test harness: the five step stubs |
| `race_timeout.c` | the rebuilt `race_timeout` module: the frame delta and the timeout flag |
| `race_timeout_main.c` | the `race_timeout` test harness: the frame delta stub and the state check |
| `dpmi_int31_entry.asm` | the rebuilt `dpmi_int31_entry` module: the run-time DPMI INT 31h entry |
| `dpmi_int31_stub.asm` | the `dpmi_int31_entry` harness dispatcher stub: the register return path |
| `dpmi_int31_entry_main.c` | the `dpmi_int31_entry` test harness: the register blocks |
| `object_physics.c` | the rebuilt `object_physics` module: the gated and unconditional physics steps |
| `object_physics_main.c` | the `object_physics` test harness: the four step stubs and the state gate |
| `dpmi_real_int.asm` | the rebuilt `dpmi_real_int` module: the real-mode interrupt setup |
| `dpmi_real_int_main.c` | the `dpmi_real_int` test harness: the state words and the error path stubs |
| `object_player_input.c` | the rebuilt `object_player_input` module: the gated drive step and the revolution step |
| `object_player_input_main.c` | the `object_player_input` test harness: the input bits and the step stubs |
| `stream_layer_init.c` | the rebuilt `stream_layer_init` module: the stream layer setup calls |
| `stream_layer_init_main.c` | the `stream_layer_init` test harness: the six setup stubs |
| `view_move.c` | the rebuilt `view_move` module: the view helper and the position setters |
| `view_move_main.c` | the `view_move` test harness: the three view stubs |
| `video_block_swap_a.asm` | the rebuilt `video_block_swap_a` module: the byte-swap and dword swap |
| `video_block_swap_a_main.c` | the `video_block_swap_a` test harness: the video buffer and the swap check |
| `timing_update.c` | the rebuilt `timing_update` module: the timing advance and the filter entry |
| `timing_update_main.c` | the `timing_update` test harness: the replay stubs and the float check |
| `panel_show.c` | the rebuilt `panel_show` module: the panel state stores |
| `panel_show_main.c` | the `panel_show` test harness: the panel state and the update stub |
| `table_lookup.c` | the rebuilt `table_lookup` module: the table entry lookup |
| `table_lookup_main.c` | the `table_lookup` test harness: the lookup stub and the flag gate |
| `memory_alloc.c` | the rebuilt `memory_alloc` module: the allocator and the out-of-memory handlers |
| `memory_alloc_main.c` | the `memory_alloc` test harness: the allocator and the handler stubs |
| `sound_driver_set.asm` | the rebuilt `sound_driver_set` module: the HMI request block and the driver call |
| `sound_driver_set_main.c` | the `sound_driver_set` test harness: the request block and the driver stub |
| `main_invoker.asm` | the rebuilt `main invoker` module: the stack alignment and the startup calls |
| `main_invoker_main.c` | the `main invoker` test harness: the startup stubs |
| `graphics_load.c` | the rebuilt `graphics_load` module: the two name copies into the static name buffers |
| `graphics_load_main.c` | the `graphics_load` test harness: the two name buffers and the two copy checks |
| `stream_close.c` | the rebuilt `stream_close` module: the two buffer releases and the handle close |
| `stream_close_main.c` | the `stream_close` test harness: the stream record and the three tracking stubs |
| `memory_free.c` | the rebuilt `memory_free` module: the DPMI 0x0502 request block and the status |
| `memory_free_main.c` | the `memory_free` test harness: the two DPMI stubs and the request checks |
| `panel_draw_centered.c` | the rebuilt `panel_draw_centered` module: the measure, the centering, and the draw |
| `panel_draw_centered_main.c` | the `panel_draw_centered` test harness: the measure and draw stubs and the three width cases |
| `runtime_free.asm` | the rebuilt run-time `free` module: the free-list walk and the heap release call |
| `runtime_free_main.c` | the run-time `free` test harness: the free list and the heap stub with the `[eax] [ebx]` convention |
| `race_cleanup.c` | the rebuilt `race_cleanup` module: the twelve cleanup calls and the data addresses |
| `race_cleanup_main.c` | the `race_cleanup` test harness: the twelve stubs and the call-order check |
| `vis_tab_load.c` | the rebuilt `vis_tab_load` module: the `VIS.TAB` read and the error path |
| `vis_tab_load_main.c` | the `vis_tab_load` test harness: the file stubs and the two paths |
| `object_state_save.c` | the rebuilt `object_state_save` module: the four dword copies and the float copy |
| `object_state_save_main.c` | the `object_state_save` test harness: the object record and the five field checks |
| `input_poll.c` | the rebuilt `input_poll` module: the three gates, the counter, the read, and the apply |
| `input_poll_main.c` | the `input_poll` test harness: the five gate cases and the two stubs |
| `race_loop.c` | the rebuilt `race_loop` module: the race start, the frame loop, and the race end |
| `race_loop_main.c` | the `race_loop` test harness: the ten stubs and the explicit order checks |
| `setup_load.c` | the rebuilt `setup_load` module: the `setup.cfg` path, the read, and the error path |
| `setup_load_main.c` | the `setup_load` test harness: the format stub and the file stubs |
| `modules/*.json` | the fifty-eight module records: the sources, the compiler, the link, and the run expectations |
| `store.py` | the record store: the schema, the read, the write, the validation, the seed, the edits, and the input fingerprint |
| `harness.py` | the pinned container build and the DOSBox-X run of one module |
| `run.py` | select the records, run the declared checks, print the progress, and write the results. `--skip-unchanged` reports a record whose input fingerprint matches the previous run without running its checks |
| `view.py` | render the self-contained status page of the store |
| `docs.py` | render the `program-map.md` tables and check the drift |
| `diff_module.py` | the module diff; it compares the rebuilt code with the original code |

```sh
python3 tools/rebuild/harness.py build --module free
python3 tools/rebuild/harness.py run --module free
python3 tools/rebuild/run.py run --all
python3 tools/rebuild/run.py run --all --skip-unchanged
python3 tools/rebuild/store.py check
python3 tools/rebuild/docs.py render
python3 tools/rebuild/view.py html
```

The link needs `system dos4g`, `option start=_cstart_`, and the `STACK` segment.
The driver output holds the eight register pairs `20 0F`, `40 3F`, `60 01`, `80 3E`,
`E0 0F`, `23 3F`, `43 01`, and `C0 01`, which match the original driver
sequence at `0x14157`.

## The period toolchain

The original build uses the Watcom C/C++ 10.0a toolchain.  The game carries the
run-time banner `WATCOM C/C++32 Run-Time system. (c) Copyright by WATCOM
International Corp. 1988-1994.`, which matches the 10.0a run-time library byte
for byte, and the embedded stub is byte-identical to the 10.0a `WSTUB.EXE`
(the marker `WATCOM patch level .a`).  The 10.5 run-time banner ends in
`1988-1995`, so 10.5 is excluded.  The period toolchain runs alongside the
pinned OpenWatcom V2 container; it does not replace it.

| Item | Value |
| --- | --- |
| Source | the Internet Archive item `watcomcc10.0cd-rom`, image `WATCOM_C10A.ISO` |
| Image | 303226880 bytes, SHA-256 `f51ed9358e7eaf6a0d292268bbbc15832530489dd8239a98f6938c244f1736fc` |
| Compiler | `WATCOM C32 Optimizing Compiler Version 10.0a` |
| Host | DOSBox-X, because the tools are DOS programs |
| Layout | `BINB` holds `wcc386`, `wasm`, `wpp386`, `wmake`, `wlib`; `BIN` holds `wlink`, `wstub`, `w32run`, `dos4gw`; `LIB386/DOS` holds `clib3r.lib`; `H` holds the headers |

Setup (writes only under `work/`):

```sh
python3 tools/rebuild/watcom10_setup.py
```

Build one module with the period toolchain (needs DOSBox-X):

```sh
python3 tools/rebuild/harness_dos.py build --module free
```

The period link uses the standard run-time library (the default libs), because
that is the flow that produces a runnable DOS/4G executable.  The module symbol
comes from the linker map, so the diff reads it as usual.  The period link
takes one `file` directive per object, and the response file is generated by the
harness.  The known limits are the `-of+` epilogue (`mov esp,ebp`) and the
register-save order, which differ from the original; see the last findings
section.

The module rebuild:

```sh
python3 tools/rebuild/diff_module.py \
    --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE \
    --original-address 0x300ec \
    --rebuilt work/rebuild/free.exe \
    --rebuilt-map work/rebuild/free.map \
    --rebuilt-symbol free_

python3 tools/rebuild/diff_module.py \
    --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE \
    --original-address 0x300be \
    --rebuilt work/rebuild/exitmod.exe \
    --rebuilt-map work/rebuild/exit.map \
    --rebuilt-symbol exit_

python3 tools/rebuild/diff_module.py \
    --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE \
    --original-address 0x300d6 --count 6 \
    --rebuilt work/rebuild/exitmod.exe \
    --rebuilt-map work/rebuild/exit.map \
    --rebuilt-symbol exit_flush_

python3 tools/rebuild/diff_module.py \
    --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE \
    --original-address 0x3009b \
    --rebuilt work/rebuild/prtfmod.exe \
    --rebuilt-map work/rebuild/printf.map \
    --rebuilt-symbol printf_
```

The diff reads the original address from the command line and the rebuilt address
from the linker map.  It exits with code 0 when the instruction sequences match.
The rebuilt `free` holds `85 c0`, `0f 85 <rel32>`, and `c3`; the original holds
`85 c0`, `0f 85 14 1f 0a 00`, and `c3`, so the instruction sequences match and
the tail-jump address differs.  The `exit` function and its helper match in the same
way: 8 of 8 instructions and 6 of 6 instructions, and the differing operands are the
addresses of the three hook slots and of the two calls.

The rebuilt `printf` holds 34 bytes and 13 instructions, the same as the original, and
no EBP frame.  Three instructions come in a different order, because the V2 register
allocator spills the argument list before it loads the arguments, so the verdict is
`the modules hold the same instructions in a different order`.

The `exit` test program carries the name `exitmod.exe`, because the DOS shell reads
`exit` as its own command and leaves the session.  The `printf` test program carries
the name `prtfmod.exe`, and its output file carries the name `prtf_out.txt`, because
DOS keeps only the first eight characters of a file name.

## The compiler options

The original game files were built with the Watcom stack-frame option `-of+`, and the
run-time library files were not.  A game file rebuild therefore adds `-of+`:

```sh
wcc386 -bt=dos -s -zq -of+ -fo=/w/module.obj module.c
```

The option reproduces the original `push ebp`, `mov ebp,esp` frame.  One difference
stays: for a function that does not change ESP, OpenWatcom V2 adds `mov esp,ebp` to the
epilogue, and the original compiler omits it.  A run-time or library file rebuild
takes the default, no frame, because those modules carry a register-save prologue.

The `engine_shutdown` module is the first game file to test the option:

```sh
python3 tools/rebuild/diff_module.py \
    --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE \
    --original-address 0x128ac \
    --rebuilt work/rebuild/shutmod.exe \
    --rebuilt-map work/rebuild/shutdown.map \
    --rebuilt-symbol engine_shutdown_
```

The rebuild holds 54 bytes and 15 instructions against the original 52 bytes and 14
instructions, and the diff names the difference: `the rebuilt module holds one
instruction the original does not: mov esp, ebp`.  The frame and the eight calls match,
the run writes the markers `1` to `8` in order, and no compiler option removes the
restore, so the module stays open as a byte-level match.

The `memcpy` module is the first exact match:

```sh
python3 tools/rebuild/diff_module.py \
    --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE \
    --original-address 0x8c150 \
    --rebuilt work/rebuild/memmod.exe \
    --rebuilt-map work/rebuild/memcpy.map \
    --rebuilt-symbol memcpy_
```

The diff reports 14 of 14 bytes equal and the verdict `the modules are
byte-identical`.  The module copies through `rep movsd`, which reads through `DS:ESI`
and writes through `ES:EDI`, so the harness start code sets `ES` to `DS`; without that
step the copy reads the source and writes to the wrong segment, and the target stays
zero.  The run then prints the target bytes `A` to `H` and the flag `Y`.

The start stub also runs a segment guard before `main_`.  The guard writes a pattern to
the stack through `ES:EDI` and reads it back through `DS:ESI` and through `SS:ESP`.  When a
read differs, the segments do not describe the same memory, so the harness spells
`SEGFAIL` on the screen through the BIOS teletype, which needs no segment setup, and exits
with the code 1.  The guard is silent when the segments agree.

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
| `show` | print the captured emulator output |
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
view, the radare2 cross-check, the container member probe, the sound bank decode,
the record run, and the store check.  The check pins the reference values from
the reference pages.  The tool exits with code 0 when every check passes.  The
radare2 stage is skipped when `r2` is not on PATH, and the record run is skipped
when Docker is missing.

```sh
python3 tools/pipeline_check.py \
  --game-dir ~/Documents/Screamer.app/Contents/Resources/game
```

The record run of stage 11 builds and runs every module record, so it needs
more time as the module count grows.  Use `--record-timeout` to raise the
budget of that one subprocess; the default is 1200 seconds:

```sh
python3 tools/pipeline_check.py \
  --game-dir ~/Documents/Screamer.app/Contents/Resources/game \
  --record-timeout 1800
```

A run takes about 15 minutes and reports one check per module record in stage
11 plus the fixed stages.  Use `--skip-runtime` to skip the run-time stages:
it takes about 16 seconds.

The record run calls `run.py run --all --skip-unchanged`.  A record whose input
fingerprint still matches the fingerprint of its recorded result runs no check
and reports `PASS` with the detail `the inputs are unchanged since the previous
run`; the stage recomputes the fingerprint and fails a record whose stored
fingerprint does not match.  The first run after the framework change records
the fingerprints, so it still runs every module once; the runs after that skip
the unchanged modules and functions and take only the hashing time.

The check runs in three places:

- the `.githooks/pre-commit` hook regenerates `work/rebuild/status.html` before
  every commit, and the CI `status` job regenerates the page and publishes it as
  the `rebuild-status` artifact,
- GitHub Actions runs the syntax gate and the unit suite on every push and pull
  request,
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
