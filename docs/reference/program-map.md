# Program map

This page records the recovered program structure.  It grows with the
investigation.

## Startup path

The path from the DOS stub to the program `main`:

<!-- begin generated: Startup path -->
| Step | Module address | Object offset | Run-time address | Evidence |
| --- | ---: | ---: | --- | --- |
| entry jump | `0x300f8` | `0x200f8` | `0150:1f20f8` | `jmp short +0x76` over the banner string |
| startup | `0x30170` | `0x20170` | `0150:1f2170` | `sti`, stack setup, DOS version check |
| main invoker | `0xd4324` | `0xc4324` | `0150:296324` | loads `argc` and `argv`, calls `main` |
| program `main` | `0x10020` | `0x20` | `0150:1d2020` | 56 instructions, 15 call sites |
<!-- end generated: Startup path -->

A module address is the address in the LE module address space.  `tools/le.py`
reports this space, and radare2 reports it as the section `vaddr`.  The object 1
relocation base is `0x10000`, so the object offset is the module address minus
`0x10000`.

The object 1 load base is `0x1c2000` at run time.  A run-time address is
`0x1c2000 + object offset`.  The run-time evidence is the log line
`0150:296359  mov  edx,[002D5E28]`, which matches the file instruction
`0x000d4359  mov edx, dword ptr [0x27e28]`: the module address `0xd4359`
minus the object base `0x10000` is the object offset `0xc4359`, and
`0x1c2000 + 0xc4359 = 0x296359`.

### Main invoker

`0xd4324` prepares the run-time and calls the program `main`:

```text
0x000d4324: 52                   push edx
0x000d4325: 55                   push ebp
0x000d4326: 89e5                 mov  ebp, esp
0x000d4328: 8b1598420000         mov  edx, [0x4298]
0x000d432e: 83c203               add  edx, 3
0x000d4331: 80e2fc               and  dl, 0xfc
0x000d4334: e81f240100           call 0xe6758           ; stack check
0x000d4339: 39c2                 cmp  edx, eax
0x000d433b: 7308                 jae  0xd4345
0x000d433d: 89d0                 mov  eax, edx
0x000d433f: 29c4                 sub  esp, eax
0x000d4341: 89e0                 mov  eax, esp
0x000d4343: eb02                 jmp  0xd4347
0x000d4345: 31c0                 xor  eax, eax
0x000d4347: 8b1598420000         mov  edx, [0x4298]
0x000d434d: 01d0                 add  eax, edx
0x000d434f: a39c420000           mov  [0x429c], eax
0x000d4354: e808240100           call 0xe6761           ; stack setup
0x000d4359: 8b15287e0200         mov  edx, [0x27e28]     ; argv
0x000d435f: a1247e0200           mov  eax, [0x27e24]     ; argc
0x000d4364: e8b7bcf3ff           call 0x10020           ; main
0x000d4369: e850bdf5ff           call 0x300be           ; run-time exit
0x000d436e: 89ec                 mov  esp, ebp
0x000d4370: 5d                   pop  ebp
0x000d4371: 5a                   pop  edx
0x000d4372: c3                   ret
```

The arguments follow the Watcom register convention: the first argument in
EAX (`argc`) and the second in EDX (`argv`).

## Program main

`main` sits at module address `0x10020` (object offset `0x20`).  It holds 56
instructions and 15 call sites.

| Call site | Target | Target size | Role |
| --- | --- | ---: | --- |
| `0x1002d` | `0x30050` | 31 | run-time stack check |
| `0x1003b` | `0x2ecf8` | 46 | unknown |
| `0x10045` | `0x2f8ca` | 27 | unknown |
| `0x10056` | `0x3009b` | 13 | run-time helper |
| `0x10068` | `0x2f8ca` | 27 | repeated call |
| `0x10075` | `0x3009b` | 13 | repeated call |
| `0x10084` | `0x127a0` | 71 | unknown |
| `0x10092` | `0x27700` | 34 | unknown |
| `0x100a6` | `0x108e4` | 22 | unknown |
| `0x100ae` | `0x10978` | 16 | unknown |
| `0x100b6` | `0x300be` | 8 | run-time helper |
| `0x100bb` | `0x27f94` | 63 | unknown |
| `0x100c2` | `0x128ac` | 14 | unknown |
| `0x100d2` | `0x2ed77` | 25 | unknown |
| `0x100d7` | `0x300ec` | 3 | run-time helper |

The body reads two globals: `[0x7ed8]` and `[0x62ac]`.  The loop at
`0x10089` calls `0x27700`, and a non-zero result leads to `0x108e4`,
`0x10978`, and the run-time free at `0x300be`.

The roles of the program functions are unknown.  The next step maps them.

## Program functions

The names come from the Ghidra decompilation, the radare2 call graph, and the
run-time log.  The names of the run-time functions describe their shape, not the
original Watcom symbol names.

<!-- begin generated: Program functions -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x10020` | `main` | called by the Watcom startup at `0xd4364` with `argc` in EAX and `argv` in EDX<br>the original at `0x10020` holds 56 instructions and 194 bytes: the frame, the allocation of `0x4c` bytes, the list of `0x96` records at `DAT_000062ac`, the memory check against `0x61a800`, the two low-memory messages at object 3 offsets 4 and 0x29, `engine_init`, the `race_loop` and `frame_update` loop on `DAT_00007ed8`, the error path through `bios_video(3)`, `error_report`, and `exit`, `engine_shutdown`, and the cleanup | derived |
| `0x108e4` | `bios_video` | builds a record with the argument and calls interrupt 10h through the run-time | inferred |
| `0x10978` | `error_report` | dispatches on the code through a jump table to one message stub per code | observed |
| `0x127a0` | `engine_init` | calls 17 setup functions, loads `MENU` from `%s\DATA\MUSIC.DAT`, and reads the video mode<br>the original at `0x127a0` holds 71 instructions and 266 bytes: it calls `config_load` at `0x1bf38`, `list_file_load` at `0x15460` with the `GEN`, `MENU`, and `CD_TRACKS` constants and the table at `0x1f00`, `setup_load` at `0x1bb58`, `controls_load` at `0x1b304`, `mouse_init` at `0x1f1a0`, `dpmi_real_int` at `0x25db0`, `sound_driver_set` at `0x262a8`, `sprintf` at `0x4d32c` with the format at `0x43b` and the config string at `0x7f44`, `music_bank_load` at `0x2f9e0` with the bank at `0x62b0` and the engine list `DAT_000062ac`, `music_start` at `0x2ffc4`, `palette_build` at `0x15720`, `keyboard_hook` at `0x14e20` with 1, `engine_interface_init` at `0x10810` with `0x6530` and `0x6350`, `stream_layer_init` at `0x4c810`, `video_buf_load` at `0x4c8f0`, and `vesa_set_mode` at `0x8bdec` with 1 | derived |
| `0x128ac` | `engine_shutdown` | counterpart of `engine_init`: stops the music with `FUN_00014e20(0)` and tears the subsystems down | derived |
| `0x23a94` | `race_radar` | the original at 0x23a94 holds 208 instructions and 870 bytes up to the ret at 0x23df9, so the record keeps the 872-byte extent and sets no function size<br>the only caller at 0x2368c is the end of the HUD draw at 0x23420; the frame update at 0x1ff70 calls 0x23420 at 0x2001a after the view groups, so the function draws a race HUD element<br>the zoom counter DAT_0011d2a4 selects the scale: 0 returns, 1 uses 0x30, 2 uses 0x60, and 3 uses 0x90, and the input map at 0x2126d advances the counter from 0 through 3<br>the function sets the 144x144 panel at (8, 248) in DAT_00115ce4 through DAT_00115cf0, clears it through 0x25d28, and renders the world from DAT_0011fdcc, DAT_0011fdd4, and DAT_0011fde8 through 0x1057c<br>the mode byte DAT_0011d260 selects the record source: bit 0x10 scans the 0x2a8-byte traffic records at DAT_00113cd8 up to DAT_00115218, bits 0 and 1 scan the 0x590-byte car records at DAT_00110ab8 up to DAT_00110ab8 + DAT_0010f8ec * 0x590, and bit 4 plots the ghost record at DAT_00118ee8 and DAT_00118ef0 for the index DAT_00118ed4 ^ 1<br>a marker is the rotated record delta times the zoom scale around the panel centre (39, 159), doubled and truncated; the function stores the position in DAT_0011cb18 and DAT_0011cb1c and flushes the sprite list, and the final marker sits at (0x4c, 0x13c) with the mode 4, so the module names the function race_radar<br>the harness covers the zoom 0 return, the traffic path, the car path, the ghost path, the two ghost guards, the empty mode, the traffic precedence, and the empty traffic list; the behavior check passes | inferred |
| `0x27700` | `race_loop` | the original at `0x27700` holds 34 instructions and 110 bytes: it calls `race_start` at `0x27770`, returns a non-zero start result, and otherwise runs the loop body `motion_update`, `default_data_copy`, `joystick_update`, the gated `input_poll`, `frame_timer`, `input_map`, `object_update_all`, and `video_fill` until `DAT_0011ce44` reaches 4, then calls `race_end` at `0x279a4` and returns 0 | derived |
| `0x27f94` | `frame_update` | clears the 20 key-state dwords at `DAT_0001fb08`, resets `DAT_00020988` and `DAT_0011d24c`, probes the heap, runs the menu init and the menu frame, advances the 6-frame counter `DAT_000207f8`, and runs the frame loop until the frame flag `DAT_0011d234` is 1<br>the original at 0x27f94 holds 63 instructions and 212 bytes: the five register saves, the frame with the 0x8-byte local area, the clear of the 20 key-state dwords at `DAT_0001fb08`, the reset of `DAT_00020988` and `DAT_0011d24c`, the memory probe at 0x2f8ca with `DAT_000062ac`, the menu init at 0x29d08 and the menu frame at 0x29f80, the 6-frame counter `DAT_000207f8`, the loop with `frame_step` at 0x2aa84 and `race_frame` at 0x2807c, the input poll at 0x26448 behind the context word at `DAT_00002dec` + 0x48 and the flags `DAT_0011d2c0` and `DAT_0011d2cc`, the music call at 0x2ff14 with `DAT_000062b0`, `video_fill` at 0x2aa98, and the exit path with the `memset` at 0x8c160, the second `video_fill`, and `race_cleanup` at 0x2a1b0<br>the harness prints the markers 0, 1, 2, 3, and D: the full path with the input and music calls, the counter wrap that skips both, the input skip with the music call, and the three-turn loop; the behavior check passes, the rebuild holds 53 instructions and 189 bytes, and the identity check stays open at 19 of 189 bytes and 14 of 53 instructions because OpenWatcom V2 saves the registers in another order and uses registers for the key-clear loop | derived |
| `0x2ecf8` | `list_create` | the original at `0x2ecf8` holds 46 instructions and 127 bytes: it calls `pool_init` at `0x2f904`, allocates `count * 0x1c` bytes through `malloc` at `0xd1f25`, stores the pointer at `+0`, the cursor 0 at `+4`, and the count at `+8`, zeroes the block through `memset` at `0xd1ec0`, and returns the list pointer; a failed allocation stores the pointer 0 and the count 0 | derived |
| `0x2ed77` | `list_destroy` | releases the block when the pointer is set; counterpart of `list_create` | observed |
| `0x2f8ca` | `memory_available` | probes the heap, frees the probe blocks, and returns the total; `main` compares it with `6400000` | derived |
| `0x30050` | `memory_alloc` | allocates the size and retries through the two out-of-memory handlers when the allocator fails; the size 0 becomes 1 | derived |
| `0x3009b` | `printf` | holds the register-save prologue `push ebx`, `push edx`, `sub esp,4`, the `va_start` address of the first variable argument, the address of the standard-output entry, the format argument, and the print-engine call at `0xd429f`; the rebuilt module at `tools/rebuild/printf.c` holds the same 13 instructions | observed |
| `0x300be` | `exit` | calls the function-pointer slots at `0x2f84` and `0x2f88` and then the helper at `0x300d6`; the helper calls the slot `0x2f88` again and the slot `0x2f8c` and jumps to the terminator at `0x30387`; `main` calls it with the error code | observed |
| `0x300ec` | `free` | forwards a non-null pointer to the run-time `free` at `0xd2008` and returns at once for a null pointer; the rebuilt module at `tools/rebuild/free.c` holds the same three instructions | observed |
<!-- end generated: Program functions -->

`main` calls the run-time helpers because the Watcom run-time is linked into the
same object.  The four run-time helpers sit at the end of the object, after the
program code.  The rebuilt `exit` module at `tools/rebuild/exit.c` holds the same two
functions; the OpenWatcom V2 run-time source `bld/clib/startup/c/exit.c` holds the
same shape, with a shared handler slot and a terminator that never returns.

The game files carry an EBP frame and the run-time files do not, because the original
build passed the Watcom stack-frame option `-of+` to the game files only.  A game file
with a frame and no local variable, for example `engine_shutdown` at `0x128ac`, also
gains a `mov esp,ebp` in the epilogue that the original compiler omits.

The smallest functions in the object are six empty stubs of 5 bytes at `0x23ed0`,
`0x24068`, `0x24070`, `0x24078`, `0x24090`, and `0x24098`.  Each holds the four
instructions `push ebp`, `mov ebp,esp`, `pop ebp`, and `ret`, and each carries
one call site from a mode dispatch at `0x20eca` or `0x20f98` that switches on the
mode value at `0x1d260`.  The stubs carry no name.

## Race loop and frame update

`main` alternates `race_loop` at `0x27700` and `frame_update` at `0x27f94`
until the quit flag `DAT_00107ed8` is 1.

### race_loop callees

<!-- begin generated: race_loop callees -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x10193` | `default_data_copy` | copies 32 dwords from `DAT_00010113` in object 1 into `DAT_000f0002` in object 2 | observed |
| `0x1fc38` | `frame_timer` | waits for the frame counter, computes the frame delta, and clamps it between 0.2 and 0.02<br>the original at 0x1fc38 holds 76 instructions and 299 bytes: the two register saves, the frame with the 0xc-byte local area, the wait on the counter DAT_000007f9, the raw delta from the counter and the double 0.01 at 0x100c58, the smooth step with the float constants 9.0 at 0x100c60 and 10.0 at 0x100c64, the clamps at the doubles 0.2 at 0x100c68 and 0.02 at 0x100c70, the frame rate from the double 0.3 at 0x100c78 and the float 65535.0 at 0x100c84, the two truncating calls at 0x4d9f4, the filter table pointer DAT_0010cf08, and the phase word DAT_0002153c<br>the module waits until the counter reaches 2, clears it, stores the old delta in DAT_00118e14, forms the raw delta counter * 0.01 in DAT_00118e08 and the smooth delta (old * 9.0 + raw) / 10.0 in DAT_00118e04, accumulates the raw delta in DAT_00118e10, clamps the smooth delta between 0.02 and 0.2, sets the filter table pointer to 0x80e8 + 400 * rate where rate is the truncated 1 / delta, advances the phase DAT_00118e0c by delta * 0.3 with a wrap at 1.0, and stores the phase scaled by 65535 in DAT_0002153c<br>the harness prints the markers 0 through 5 and D for the plain delta, the upper clamp with the truncated frame rate 4, the lower clamp with the frame rate 50, the phase wrap from 1.0, a phase step below 1.0, and a phase step across 1.0; the behavior check passes, the rebuild holds 62 instructions and 278 bytes, and the identity check stays open at 27 of 62 instructions and 24 of 278 bytes because the V2 toolchain saves the frame pointer first and uses another instruction order<br>the two integer conversions use the local truncating __CHP stub in frame_timer_fpu.asm, because the shared fpu_stub.asm leaves round-to-nearest and the original helper at 0x4d9f4 truncates toward zero | derived |
| `0x211e0` | `input_map` | reads the control bits `DAT_00118e1c` and dispatches the game action<br>the original at 0x211e0 holds 62 instructions and 239 bytes up to the ret at 0x212ce: it tests bit 0x20 of the control byte 0x18e1d and sets bit 1 of the exit byte 0x1ce44 when one of the flags 0x3b, 0x1e, 0x3, or 0x19880 is set; otherwise it returns at once when the input word 0x18e1c holds 0x401c; it calls 0x21338 and clears 0x7f9 when the flag 0x1b is set and the mode 0x1d260 is not 0x10; it advances the counter 0x1d2a4 from 0 through 3 while the flag 0x34 is set and 0x18f90 is zero; and it calls 0x101b4 with 1 and sets the input bits 0x220 when the flag 0x3 is set and the control byte holds bit 0 | observed |
| `0x21338` | `options_screen` | the only caller is input_map at 0x2124f: it calls this function when the flag 0x1b is set and the mode 0x1d260 is not 0x10, so the function is the escape menu; the label table at 0x1f74 holds CD, FX, and MU, the pointer table at 0x1f80 selects the audio slots 0x1d2c0, 0x1d2bc, and 0x1d2c4, and the state table at 0x1f20 holds the seven positions, so the module names the function options_screen<br>the original at 0x21338 holds 464 instructions and 1715 bytes up to the ret at 0x219ea; the record keeps the 1716-byte extent and sets no function size; the body clears the screen twice, fades the three volume locals by the frame delta, draws the menu rows with the format 03 25 64 00, draws the CD, FX, and MU bars through 0x23dfc, and leaves the loop when the key byte 0x03 is set | inferred |
| `0x21e6c` | `motion_update` | scales the object position with the time base, calls `FUN_0001f94c`, and advances the time base with a wrap at 1.0<br>the original at 0x21e6c holds 53 instructions and 202 bytes: the frame, the 0xc-byte local area, the guard on the time base below zero, the three scaled values with the helper at 0x4d9f4, the view_move call at 0x1f94c, the video_present call at 0x27ec8, the advance by DAT_00118e08 times the 2.0 constant at 0x100d51, and the wrap at 1.0 that sets the time base to -1.0 | derived |
| `0x24da0` | `joystick_update` | the original at `0x24da0` holds 201 instructions and 630 bytes: it walks the six 28-byte control records at `0x197d8`, searches the earlier records for a duplicate device and axis, reads an axis through `0x10263` and a button port through `0x10257`, scales the axis by the 28-byte table at `0x192a4`, truncates toward zero, clamps the result, and stores the six dwords at `0x19880`<br>the record keeps the 2176-byte extent to the next entry, so the identity check uses `--count 201` | derived |
| `0x26448` | `input_poll` | the original at `0x26448` holds 27 instructions and 108 bytes up to the first return, plus the early-exit block at `0x264b4`: it tests the flags at `DAT_0016669e`, `DAT_00166684`, and `DAT_00166688`, counts `DAT_001666bc` to 8, reads through `0x260e0`, and applies the input through `0x263cc` when device bit 9 is clear and `DAT_001666ac` is set; it returns 0 on every early exit and `DAT_00166696` on the normal path | observed |
| `0x27770` | `race_start` | sets the 640x400 screen rectangle, loads the choice file, copies the state globals, and calls `race_session_start(1)`<br>the original at `0x27770` holds 66 instructions and 251 bytes: the five register saves, the frame, the 4-byte local, the four stores 0, 0x280, 0, and 0x190 at `0x15ce4` through `0x15cf0`, the call at `0x1bbc8`, the five state copies, the context reads at `+0x48` and `+8`, the call at `0x8bd54` with the table value `DAT_001021d8[index]`, the two clears, the `MAP%d` build at `0x4d32c` with `DAT_001207f8 + 1`, the mode store `0x100`, the `DAT_0011d274` copy, and the `race_session_start(1)` call at `0x128e0` | derived |
| `0x2786c` | `object_update_all` | the original at 0x2786c holds 84 instructions and 309 bytes up to the ret at 0x279a0: the six register saves, the frame, the five loops over the DAT_0010f8ec records at DAT_00110ab8 with the 0x590-byte stride and the shared counter DAT_0010634c, the seven calls at 0x27f44, 0x19ab0, 0x205c4, 0x20954, 0x8ac78, 0x81ab0, and 0x27a34, and the epilogue<br>the five passes call object_transform at 0x1cd44, object_player_input at 0x1c760, object_physics at 0x1c810, object_prepare at 0x1c79c, and object_car_link at 0x1c84c on each record; the seven calls pass 1 to opponent_update, 0x109a0 to camera_update, and 0x6350 to sound_effects_update | derived |
| `0x279a4` | `race_end` | tears the race down through 13 calls<br>the original at `0x279a4` holds 36 instructions and 141 bytes: the frame with one local, the thirteen calls at `0x1f8b4` (value 0), `0x1f8dc`, `0x2fe68` (`DAT_000062b0`, `DAT_00006350`), `0x264ec`, `0x1b180`, `0x115f8` (`DAT_00006530`), `0x11304` (`DAT_00006530`), the gated `0x15874` on `DAT_0018e3c == 0`, `0x4cf2c`, `0x1f6f8`, `0x1b58c`, `0x25640`, and `0x258b8`, then the copies `DAT_001d274 = DAT_001d22c` and `DAT_001d230 = DAT_0018e3c`<br>the only caller is `race_loop` at `0x27761`, after the loop exit test; the plan row records 141 bytes, 36 instructions, 13 calls, and 0 FPU | observed |
| `0x2aa98` | `video_fill` | calls the video copy at 0x8beb8 with 480, 400, and the video buffer pointer | derived |
<!-- end generated: race_loop callees -->

### frame_update callees

<!-- begin generated: frame_update callees -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x2807c` | `race_frame` | blits the video buffer, reads the input bits, and dispatches on the game state `DAT_001203b8`<br>the original at 0x2807c holds 48 instructions and 189 bytes: it copies 0x3e800 bytes from DAT_0011fad0 to DAT_00121528 through memcpy at 0x8c150, runs video_block_swap_a at 0x1075f and video_block_swap_b at 0x107a1 behind the flags 0x10 and 0x20 of DAT_0011d274, calls overlay_dispatch at 0x28160, race_heading_update at 0x29610, race_state_update at 0x29564, camera_update at 0x8ac78 with 0x109a0, and sound_update at 0x81a70 with 0x6350, selects the overlay for the state DAT_001203b8 (0 -> 0x2e0b4, 1 -> none, 2 -> 0x2e21c, 3 -> 0x2e58c, 4 -> 0x2e724), calls fade_update at 0x29818 with the float DAT_00120378, and sets DAT_0011d234 to 1 when DAT_0011d24c is above the constant at 0xee0 and DAT_0011fac4 is 0<br>the case targets come from the compiler jump table at cs:[eax*4 + 0x18068]: entry 0 is 0x180ea, entry 1 is 0x18104, entry 2 is 0x180f1, entry 3 is 0x180f8, and entry 4 is 0x180ff<br>the behavior check passes: the harness covers the state 5 skip path, the state 0 path with the swap A, the state 2 path with the swap B, the state 3 path with both swaps, the state 4 path, the fade argument, and the frame-flag conditions<br>the identity check stays open: the rebuilt module uses an if chain instead of the compiler jump table and saves the registers in another order, so 10 of 48 instructions and 23 of 189 bytes match | derived |
| `0x29d08` | `menu_space_init` | the original at 0x29d08 holds 156 instructions and 632 bytes: the five register saves, the EBP frame with the two local name buffers, the NET_SPACE resource request at 0x2eed0 with the size 0x2000, the choice_load call at 0x1bbc8, the two zero stores at DAT_0002082c and DAT_0002087c, the state_cells_set call at 0x8bd54 with 0xffff54e0, the font table init at 0x25620, the sound slot init at 0x257a0, the rectangle constants at DAT_00015ce4 through DAT_00015cf0, the four sprintf calls at 0x4d32c for the graf path, the sprite member, the font member, and the menu path, the sprite load at 0x257b0, the font load at 0x25658, the cleanup at 0x15150 on a non-zero result, the graphics name copy at 0x10ba4, the material list load at 0x4cd9c, the screen mode set at 0x4c850, the OPT_PCX resource request at 0x2eed0 with the size 0x3e800, the memset at 0x8c160, the video_fill call at 0x2aa98, the four member list loads at 0x1b394 into DAT_0001fb80, DAT_0001fd10, DAT_0001fea0, and DAT_00020030, the two list file loads at 0x15460, the sound bank load at 0x1f2e0, the objects init at 0x2aab8, the view_move call at 0x1f94c, the input apply at 0x263cc, the two music calls at 0x2fcec and 0x2febc behind the flag DAT_0001d2cc, and the epilogue; the harness prints the markers 0, 1, 2, 3, and D for the normal path, the two load failures, the music path, and the two zero resources; the behavior check passes, the rebuild holds 151 instructions and 637 bytes, and the identity check stays open at 72 of 632 bytes and 46 of 151 instructions because OpenWatcom V2 saves the registers after the frame and uses other local offsets | observed |
| `0x29f80` | `menu_frame` | the original at 0x29f80 holds 151 instructions and 560 bytes: the six register saves, the frame, the 0x30c-byte local area, the pcx_load call at 0x1b5a4 with the file gfx\graf.dat at object-3 offset 0xfe0 and the member menu1h.pcx at 0xfd5, the cleanup at 0x15150 on a non-zero result, the two video_present calls at 0x27ec8 with the scale 0 and the offset 0, the memcpy at 0x8c150 of 0x3e800 bytes from DAT_001fad0 to DAT_0021528, the video swaps at 0x1075f and 0x107a1 behind the flags 0x10 and 0x20 of DAT_001d274, the video_fill call at 0x2aa98, the constants at DAT_001d234, DAT_001fac0, DAT_001fac4, DAT_00203b8, DAT_0018e08 at 0.08, DAT_000007f9 at 8, DAT_001fab8 at 0x35e, DAT_001d240 at 1, DAT_0020378 and DAT_0020380 at 0.6, DAT_001fac8 at 1, and DAT_001fb64 at -1, the zero values at DAT_001fb58, DAT_001d224, DAT_001d248, DAT_00203a8, DAT_002039c, DAT_00203ac, DAT_00203a0, DAT_00109a0, DAT_0020384, DAT_00109a4, DAT_0020388, DAT_0020394, DAT_00109a8, DAT_002038c, and DAT_002037c, the two screen offsets at DAT_0020824 and DAT_0020828 from the fields at DAT_0015ccc +0x134 and +0x120 with the integer division by 2 and the constants 0x190 and 0x320, the longest of the strings at DAT_001fc1c, DAT_001fc20, and DAT_001fc24 at DAT_00203b4, the run init at 0x2dff4, and table_lookup at 0x1f718 with 0 | observed |
| `0x2a1b0` | `race_cleanup` | the original at `0x2a1b0` holds 24 instructions and 97 bytes: the frame, the twelve cleanup calls at `0x264ec`, `0x1be6c`, `0x25640`, `0x258b8`, `0x115f8`, `0x11304`, `0x11a08`, `0x4cecc`, `0x1b58c`, `0x2f326`, `0x1f6f8`, and `0x2fe68`, and the epilogue; the two argument call pairs receive the imported data addresses and the last two calls receive the pointer at `DAT_000062ac` and the address `DAT_000062b0` | observed |
| `0x2aa84` | `frame_step` | calls `default_data_copy`, `joystick_update`, and `frame_timer` | derived |
| `0x2aab8` | `menu_objects_init` | the original at 0x2aab8 holds 395 instructions and 1420 bytes up to the ret at 0x2b043, so the record keeps the 1420-byte extent and sets no function size<br>the only caller at 0x29f0c is menu_space_init at 0x29d08; the function loads the object list and fills the car, map, and member tables, so the module names it menu_objects_init<br>the function copies AAUTO00 from DAT_00002edc and AMAP1 from DAT_00002ef0, looks ALETTER up through 0x11060, loads 79 slots at DAT_001d2d0 through 0x10ed4, fills AAUTO00 through AAUTO06 in two loads each at DAT_001d550 with the field +0x20 at 0x6000 and 0xe000 and +0x3c at 0, and fills AMAP1 through AMAP6 twice at DAT_001d908<br>the cardat list at DAT_001fd10 supplies 14 names and five numbers each, the trackdat list at DAT_001fea0 supplies 12 names and one number each, the two lists bind through 0x11850, and the last call at 0x2b037 runs the init at 0x2dff4<br>the data at object-3 offset 0x3f0 holds OBJ_LIST, at 0x3ca 3D_BLKOBJ, at 0x108f ALETTER, at 0x2edc AAUTO00, and at 0x2ef0 AMAP1 | derived |
| `0x2ff14` | `music_play` | the original at `0x2ff14` holds 39 instructions and 111 bytes: a negative word at `+0x04` sets the state 6, a zero ready result from `0xd4239` sets the state 5, and otherwise the function starts the track through `0xd38e6`, selects it through `0x2febc` when the word at `+0x0c` is 1, and sets the state 0; it returns the state it wrote | derived |
| `0x8c160` | `memset` | fills `count >> 2` dwords with the value byte replicated | observed |
<!-- end generated: frame_update callees -->

`frame_update` clears the screen with `memset(DAT_00121528, 0, 0x3e800)`.
The count `0x3e800` is 256000, which is 640 times 400: the screen buffer is
640 by 400 bytes.  Evidence: the decompiled `FUN_00027f94` and `FUN_0008c160`.

`input_poll` is shared by both loops.  `default_data_copy` and
`motion_update` are shared by `race_loop` and `frame_step`.

## Second level: object update, motion update, and race frame

### object_update_all callees

`object_update_all` at `0x2786c` walks `DAT_0010f8ec` records of 0x590 bytes at
`DAT_00110ab8` and runs five passes over every record.  It then runs the seven
subsystem calls.

<!-- begin generated: object_update_all callees -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x19ab0` | `player_update` | runs the five steps at `0x19ae8`, `0x19e78`, `0x19f00`, `0x1a144`, and `0x212d0` on the player structure `DAT_00110a20` | inferred |
| `0x1c760` | `object_player_input` | runs the proximity check, the drive step gated on the state at `+0x2f8` and the input bits `DAT_00118e1c`, and the revolution step | derived |
| `0x1c79c` | `object_prepare` | runs the preparation steps, gated on the state at `+0x414` | derived |
| `0x1c810` | `object_physics` | runs the physics and collision steps, gated on the state at `+0x414` | derived |
| `0x1c84c` | `object_car_link` | the original at 0x1c84c holds 238 instructions and 863 bytes up to the first ret: the frame, the 20-byte local area, the two pointers from the 28-byte entry at DAT_0010653c for the index at +0x4dc, the target pointer from the entry at +4, the position copy, the five scaled and truncated values, the render array stores, the flags at DAT_0010d268, and the epilogue<br>the harness holds the object record, the three entry pairs of the 28-byte table, the render arrays, and the six constant globals; it covers the state copy, the two flag bits, the angle threshold, the negative truncation, the 11-bit mask, and the table selection; the behavior check passes | derived |
| `0x1cd44` | `object_transform` | writes the world position at `+0x498`, `+0x49c`, and `+0x4a0` and the float fields at `+0x4a4` to `+0x4ac` from the position and the velocity<br>the original at 0x1cd44 holds 61 instructions and 244 bytes: it calls object_integrate at 0x1d6bc and object_state_save at 0x1ccd8, writes the world position at +0x498 and +0x4a0 from +0x258 and +0x260 plus +0x454 and +0x45c, copies +0x238 to +0x49c while +0x2f8 is zero, writes the float sums at +0x4a4, +0x4a8, and +0x4ac, stores +0x238 minus 2 at +0x2c4, and stores +0x4a8 plus +0x4c8 at +0x4bc | derived |
| `0x205c4` | `ai_update` | the original at 0x205c4 holds 260 instructions and 911 bytes up to the ret at 0x20952; the loop walks DAT_0010d6f8 records of 0x40 bytes at DAT_0010d700; a record runs when the flag at +0x38 is set; a running record advances the position at +0 and +8 by the speed at +0x30 times the direction at +0x24 and +0x2c times DAT_00118e04, turns the angles at +0xc, +0x10, and +0x14 by the doubles 0.008, 0.004, and 0.0015, and falls the speed by the frame delta times 2000.0; the collision check at 0x11c00 through DAT_00108038 and DAT_00108040 saves the height at +4 in DAT_0010c72c, restores the saved position, flips the sign bit of the bytes at +0x27 and +0x2f, and scales the speed by 0.3; the vertical speed at +0x34 falls by the frame delta times 500.0, the height takes the truncated sum, the height clamps to DAT_0010c72c with a reversed vertical speed through the double 0.8, and a magnitude below 50.0 clears the vertical speed; a stopped record with a collision sets the height to -0x4e20, a stopped record without a collision rotates DAT_0010c734 and DAT_0010c73c through 0x2218c with the angle at +0x10 plus -pi / 2 into the angle at +0xc, and the run flag clears; a record with the flag at +0x20 publishes the position and height into the target record from DAT_0010653c at the 0x1c-byte entry named by +0x18 and scales the three angles by 1024.0 / pi and masks them with 0x7ff into +0x14, +0x18, and +0x1c | derived |
| `0x20954` | `opponent_update` | the original at 0x20954 holds 134 instructions and 454 bytes up to the ret at 0x20b19; the flag DAT_0010cf58 gates the loop; the loop walks DAT_0010f178 records of 0xbc bytes at DAT_0010f180; a record runs when both absolute position differences from DAT_00113ccc and DAT_00113cd0 stay below 0xcb20, or when the argument is not zero; a running record calls 0x22378 when the argument is not zero, calls 0x20b1c with the +0xc float scaled by DAT_00118e04, copies +0x78 to +0x30, copies +0x54, +0x58, and +0x5c to +0x20, +0x24, and +0x28, writes them to the target record from the pointer table DAT_0010653c at +8, +0xc, and +0x10, scales the floats at +0x2c, +0x30, and +0x34 by 1024.0 / pi and masks with 0x7ff into +0x14, +0x18, and +0x1c, and calls 0x21b9c with the record and with 1 when the argument is zero | derived |
| `0x27a34` | `hud_update` | gated on the session state `DAT_0011ce44`: the overlays 0 and 1 run when the state is not 3, and the overlay 2 runs when bit 1 is set | inferred |
| `0x27f44` | `race_timeout` | computes the frame delta between `DAT_000107fd` and `DAT_0011d228`, and sets the timeout flag `DAT_00101e04` when the delta is greater than 1000 | derived |
| `0x81ab0` | `sound_effects_update` | the original at 0x81ab0 holds 51 instructions and 235 bytes up to the ret at 0x81b9a: it saves all registers and the four segment registers, stores the driver pointer 0x6350 in the global 0x7451c, calls the two setup helpers at 0x83ca8 and 0x83354, joins the shared body at 0x81ac6, calls the source update at 0x8a958, returns when the active count 0x72488 is zero or negative, calls the mix prepare at 0x8aa60, returns when the voice count 0x7aa9c is zero or negative, mixes the voices through 0x83500, sets the song pointer 0x7251c and the frame repeat 0x72c10, and runs the per-frame loop with 0x8af08, 0x8a8d8, 0x835c0, 0x836ac, and 0x843a8 until the frame repeat reaches zero; the neighbour sound_update at 0x81a70 joins the same body at 0x81ac6 | observed |
<!-- end generated: object_update_all callees -->

### motion_update callees

<!-- begin generated: motion_update callees -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x1f94c` | `view_move` | calls the view helper with the second argument, the position setter with the constant `0x62b0` and the shifted third argument, and the position setter with the doubled and shifted first argument | derived |
| `0x27ec8` | `video_present` | the original at `0x27ec8` holds 40 instructions and 122 bytes: the frame, the 0x310-byte local area, the loop over the 0x300 source bytes at `DAT_0010cf5c` from `0xcf5c` to `0xd25c`, the arithmetic `(byte - offset) * scale + offset` with the `0x4d9f4` rounding helper, the `palette_set` call at `0x81a05`, and the `ret 8` epilogue; the scale and the offset arrive on the stack | inferred |
| `0x4d9f4` | `fp_helper` | rounds the value on the FPU stack to an integer | observed |
<!-- end generated: motion_update callees -->

### race_frame callees

`race_frame` at `0x2807c` runs every frame of a race.

<!-- begin generated: race_frame callees -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x1075f` | `video_block_swap_a` | byte-swaps and swaps dwords between the front of the video buffer and the region `0x9f` dwords ahead, 80 passes of 400 dwords | derived |
| `0x107a1` | `video_block_swap_b` | swaps 200 rows of 160 dwords between the front of the video buffer and the region `0xf960` dwords ahead | derived |
| `0x28160` | `overlay_dispatch` | reads the option flags in `DAT_000f0002` and calls the overlay for the set flag<br>the first-return body at `0x28160` holds 42 instructions and 152 bytes: the option-flag test at `DAT_000f0002`, the mode check against `0x20`, the state switch through the compiler table at `cs:[eax*4 + 0x1813c]`, the case 0 stack pop, and the epilogue; the case targets are `0x281ab`, `0x281f8`, `0x282a2`, `0x282e6`, `0x28358`, `0x2837d`, and `0x283da`, and the full function extends to `0x289bc` (2140 bytes). The rebuild keeps the dispatcher, implements case 0, and represents the other case bodies as step functions | derived |
| `0x29564` | `race_state_update` | switches on `DAT_0011fac4`; case 1 advances `DAT_0002038c` by the frame timing and copies `DAT_0011facc` into `DAT_0011fac4` and `DAT_000203b8` | derived |
| `0x29610` | `race_heading_update` | the original at 0x29610 holds 136 instructions and 519 bytes up to the ret at 0x29816; it scales DAT_00120388, DAT_00120394, DAT_00120384, and DAT_0012038c by the float 1024.0 at object-3 offset 0xf33 and the float pi at 0xf37, truncates toward zero through 0x4d9f4, and masks with 0x7ff; the first two headings wrap by 0x800 when they differ by at least 0x400; a set DAT_0011d240 makes the factor the power of DAT_0012037c raised to 2.0 through 0xc21e2, and a clear flag copies DAT_0012037c; the four outputs land at DAT_00109a0, DAT_00109a4, DAT_00109a8, DAT_00109ac, DAT_00109b0, and DAT_00109b4 from DAT_0012038c and from the three integer pairs 0x2039c/0x203a8, 0x203a0/0x203ac, and 0x203a4/0x203b0; a zero DAT_0012037c returns, and a negative value after DAT_0012037c - DAT_00118e08 * DAT_00120380 copies DAT_00120394 into DAT_00120388, copies the three second integers into the first ones, and clears DAT_0012037c | derived |
| `0x29818` | `fade_update` | ramps the fade value toward 1.0 or down to 0.0 by the frame delta and calls `video_present`<br>the body at `0x29818` holds 48 instructions and 154 bytes: the five register saves, the frame, the state read from `DAT_0011fac8`, the fade-in path, the fade-out path, the `video_present` call at `0x27ec8`, and the `ret 4` epilogue. State 0 does nothing. State 1 adds `DAT_0018e08 * delta` and clamps at 1.0. The other states subtract the same product and clamp at 0.0. A clamp clears the state. The active paths call `video_present(fade, 0)`. | observed |
| `0x2a558` | `race_results_screen` | the original at 0x2a558 holds 343 instructions and 1323 bytes up to the ret at 0x2aa82, so the record keeps the 1324-byte extent and sets no function size<br>the only caller at 0x2a335 is the race-menu handler at 0x2a214; the named ancestors are net_race_screen at 0x28ba4 and net_race_lobby at 0x2d7c4, so the module takes the section race_frame callees<br>the function clears the video buffer, loads load<N>H.pcx and champ5h.pcx from gfx\graf.dat, marks the local result record at 0x13cd8 as finished with the race time DAT_0011d268, collects the finished records, sorts them by the time at +0x2c, and draws the position, the player name, the time, and the car name through text_draw at 0x25020, so the module names the function race_results_screen<br>the screen index comes from the chosen player record at 0x2099c field +0x14 and a value above 5 drops by 6 and sets the flag 0x08; the input word DAT_00118e1c bits 0x1020 skip the results, and the wait loop exits on the bytes at 3 and 0x1e or polls the input behind DAT_0011d2c0 and DAT_0011d2cc | inferred |
| `0x2b9cc` | `map_object_update` | the original at 0x2b9cc holds 310 instructions and 1121 bytes up to the first ret at 0x2be2c; the body continues to the shared epilogue at 0x2be6f, so the record keeps the 1224-byte extent and sets no function size<br>the only caller at 0x29b86 is the race state update at 0x29918, which runs this function on each of the 0x50 records at DAT_001db78; the map placement at 0x2b044 fills those records from a map string, so the module names it map_object_update<br>a record with the field +0x08 equal to -1 binds the list name 0x6350 through 0x11850 and returns; otherwise the function decays the timers +0x60 and +0x58, looks the object up in DAT_001d2d0 and DAT_0010653c, and sets the object frame +0x20 from DAT_001fab8<br>the function interpolates the object positions +0x08, +0x0c, and +0x10 from the record fields +0x40, +0x44, and +0x48 toward +0x34, +0x38, and +0x3c with the cosine of the progress +0x58 scaled by pi/2, adds the cosine and sine of the progress scaled by 2 pi times the amplitudes +0x4c and +0x50, and adds a wave of 150.0 from the normalized position and the phase DAT_001d248<br>the three record angles +0x10, +0x14, and +0x18 scale by 1024.0 / pi into the object angles +0x14, +0x18, and +0x1c with the mask 0x7ff; the final block decays the angle +0x10 with 4 pi when the flag +0x0c and DAT_001fac0 are zero, or raises it, sets DAT_001fb64 to -2 above pi/2, and clamps it at pi<br>the harness holds one record, one object block, the entry and slot tables, the frame delta, the phase, and the constants; it covers the unused record, the full path, the two clamps, the pi return, the mode set, the interpolation, the overlay path, and the zero-angle return; the behavior check passes | inferred |
| `0x2c430` | `name_entry_screen` | the original at 0x2c430 holds 230 instructions and 885 bytes up to the ret at 0x2c7a4, so the record keeps the 888-byte extent and sets no function size<br>the only caller at 0x2bebe is case 4 of the state init dispatcher at 0x2be94; the matching update at 0x295e7 calls 0x2ce84, and race_state_update at 0x29564 is a race_frame callee at 0x2807c, so the module takes the section race_frame callees<br>the init loads menu2h.pcx from gfx\graf.dat, clears DAT_0011d240 and DAT_0011d244, and sets DAT_00120378 at 1.5, DAT_00120380 at 8.0, and DAT_0011fac8 at 1<br>the index is the first byte of the player name DAT_001203cc minus 0x41; the input at 0x2893a increments DAT_0011d23c, wraps above 0x19, and writes the letter into the name at DAT_0011d244, so the module names the function name_entry_screen<br>the function builds 26 records at DAT_001db78 with the letters A to Z, the ring position at radius 3500.0, the delay i * 0.04, and the angle i * 0.24166098; the fields match the map_object_update reader at 0x2b9cc<br>the harness covers the letters C, Z, A, and B, the failed load, and the record bound; the behavior check passes | inferred |
| `0x2cf68` | `championship_screen` | the original at 0x2cf68 holds 370 instructions and 1357 bytes up to the ret at 0x2d4b4, so the record keeps the 1360-byte extent and sets no function size<br>the only caller at 0x295f1 is the state dispatcher race_state_update at 0x29564 case 5; the dispatcher is a race_frame callee at 0x2807c, so the module takes the section race_frame callees<br>the selected record DAT_001203dc picks one 0x2c-byte record from the table at object-3 offset 0x22bc; the table holds the background names champ0 through champ7 at +0x18, and the loader at 0x2c7a8 reads champ<N>H.pcx for the state 5 screen<br>the text list DAT_00120030 is the champ list that menu_space_init loads at 0x29d08; the record type 3 branch draws the four headers, six rows with the car number, the race time, and the points, sums the points, and draws the two summary lines, so the module names the function championship_screen<br>the record type 0 branch draws the centred champ list group and calls net_race_screen when the frame timer DAT_0011d24c exceeds 10.0; the footer panel appears when the low seven bits of DAT_000007fd are below 0x48 | inferred |
| `0x2d7c4` | `net_race_lobby` | the original at 0x2d7c4 holds 198 instructions and 690 bytes up to the first ret at 0x2da75; the case blocks continue to the ret at 0x2ddda, so the record keeps the 1560-byte extent and sets no size<br>the race_state_update dispatcher at 0x29564 reaches this function through the jump-table entry that calls 0x2d7c4 at 0x29605<br>the function reads the 0x38-byte player records at object-3 offset 0x2099c, draws the local player and the remote players through panel_draw_centered at 0x2e7cc, text_draw at 0x25020, and panel_show at 0x2dfac, processes the network events through 0xc739c, 0xc8208, 0xc712c, 0xc6fc8, 0xc7248, 0xc71dc, and 0xc76f0, calls net_player_add at 0x2dddc with 0x8844, and loads menu3h.pcx from gfx\graf.dat with the palette; so the module names the function net_race_lobby | derived |
| `0x2dfac` | `panel_show` | stores the doubled position pair and the size, sets the visible flag, clears the count, and updates the panel | inferred |
| `0x2e0b4` | `race_panel_0` | the overlay for the race state 0<br>the original at 0x2e0b4 holds 103 instructions and 357 bytes: it centers half of the object width at +0x134 and half of the object height at +0x120 on the 0x140 screen width, moves each axis position by the frame delta DAT_0018e08 times a step constant while the target stays above it, snaps the position down when the target stays below it, converts the x sum with the offset at 0x12d8 and each axis through the truncating helper at 0x4d9f4, draws the panels through panel_show at 0x2dfac, and draws the title at 0xbe with the font 0 and the color 2 through panel_draw_centered at 0x2e7cc when the state at DAT_00020988 is 1 or 2 and the low seven bits of the frame counter DAT_000007fd stay below 0x48 | derived |
| `0x2e21c` | `race_panel_2` | the overlay for the race state 2<br>the original at 0x2e21c holds 265 instructions and 879 bytes: it draws the title pointer DAT_0011fc30 at y 2 with the font 3 and the color 2 through panel_draw_centered at 0x2e7cc, measures the inline text at +0x14 of the 0x44-byte record that the index DAT_001203c4 selects from the table at 0x1d550 through text_measure at 0x254a8, halves the measured width and height with a signed division, centers the half width on the 0x140 screen width, draws the doubled position with the font 0 and the color 2 through text_draw at 0x25020, selects a second text pointer through the field at +0x30 and the table at 0x1fc14 and draws it the same way at y 0xc8 minus five half heights, builds a string of DAT_001203b4 letters A, measures it with the font 1, centers the half width plus 0x40, draws the title pointer DAT_0011fc1c at y 0xd2 minus five half heights, formats the field at +0x34 with the format %d KMH through sprintf at 0x4d32c and draws it at (x + saved) * 2, draws the title pointers DAT_0011fc20 and DAT_0011fc24 one half height apart with the font 1 and the color 2 through text_draw, and draws the panels at (x + saved, y + 1) with the sizes field + 4 and field + 9 through panel_show at 0x2dfac, draws the panel at (8, 0x64) with the size 2 when the index is not zero and DAT_0012039c stays below DAT_001203a8 and DAT_0012037c stays above 0.6 and with the size 0 otherwise, and draws the panel at (0xee, 0x64) with the size 3 when DAT_001207f0 plus 11 differs from the index and DAT_0012039c stays above DAT_001203a8 and DAT_0012037c stays above 0.6 and with the size 1 otherwise; the threshold 0.6 is the double constant at object-3 offset 0x12e7 | derived |
| `0x2e58c` | `race_panel_3` | the overlay for the race state 3<br>the original at 0x2e58c holds 120 instructions and 408 bytes: it draws the title pointer DAT_0011fc34 at y 2 with the font 3 and the color 2 through panel_draw_centered at 0x2e7cc, measures the text at +0x14 of the 0x34-byte record that the index DAT_001203c8 selects from the table at 0x1d908 through text_measure at 0x254a8, halves the measured width and height with a signed division, centers the half width on the 0x140 screen width, sets y to 0xc8 minus the doubled half height, draws the doubled position with the font 2 and the color 2 through text_draw at 0x25020, draws the title pointer DAT_0011fc54 at y 0x20 with the font 1 and the color 2 when DAT_0012081c stays below DAT_00120818 and the low six bits of DAT_000007fd stay below 0x20, draws the panel at (8, 0x64) with the size 2 when the index is not zero and DAT_0012039c stays below DAT_001203a8 and DAT_0012037c stays above 0.6 and with the size 0 otherwise, and draws the panel at (0xee, 0x64) with the size 3 when DAT_00120818 plus 2 differs from the index and DAT_0012039c stays above DAT_001203a8 and DAT_0012037c stays above 0.6 and with the size 1 otherwise through panel_show at 0x2dfac; the threshold 0.6 is the double constant at object-3 offset 0x12ef | derived |
| `0x2e724` | `race_panel_4` | the overlay for the race state 4<br>the original at 0x2e724 holds 50 instructions and 167 bytes: it reads the title pointer at DAT_0011fc38, calls panel_draw_centered at 0x2e7cc with the y 2, the font 3, and the color 2, formats the name at DAT_001203cc with the format \001%s through sprintf at 0x4d32c, draws the buffer at (0x106, 0x98), shifts x by 21 pixels for each unit of DAT_0011d244, and draws the separator \001- at (2 * (0x83 + DAT_0011d244 * 21), 0xb0) through draw at 0x25020 | derived |
| `0x2e7cc` | `panel_draw_centered` | the original at `0x2e7cc` holds 34 instructions and 93 bytes: it measures the text through `0x254a8`, halves the measured width with a signed division, centers the half width on the `0x140` screen width, and draws through `0x25020` with the doubled position | derived |
| `0x2e82c` | `race_record_save` | the original at 0x2e82c holds 347 instructions and 1185 bytes; the record keeps the 1228-byte extent and sets no function size<br>the three callers at 0x1fecf, 0x29022, and 0x2a2a8 pass the selected track index from DAT_001203bc; the callers sit in the loading-screen, net-race-screen, and race-menu paths, so the module takes the section race_frame callees<br>the function inserts the lap time at DAT_0011d26c into the five slots of the 0x50-byte lap records at DAT_00120430 and, when DAT_0011d2b0 is 2, the race time at DAT_0011d268 into the race records at DAT_00120610; both tables keep the entries in ascending time order, and the screen at 0x2d4b8 reads the same two tables<br>a successful race insert builds %s\replays\temp.rpl, checks that the file exists, selects the first free replay number from the +0x28 flags, builds %s\replays\REPLAY%d%d.RPL from the track index and the number, deletes the target, and renames the temporary file, so the module names the function race_record_save<br>the input word DAT_00118e1c gates the save: the bits 0x1020 return before any table write | inferred |
| `0x81a70` | `sound_update` | a sound library update with the driver block `DAT_00106350`<br>the original at 0x81a70 holds a 62-byte entry of 16 instructions up to the jump at 0x81aac: it stores the driver pointer 0x6350 in the global 0x7451c, saves all registers and the four segment registers, calls the reset helper at 0x82e0c, clears the active count 0x72488, loads the destination 0x40330 in edi, and jumps to the shared service body at 0x81ac6; the neighbour sound_effects_update at 0x81ab0 holds the shared body; the plan row counted 299 bytes because the body sweep follows the jump to the first ret at 0x81b9a | observed |
| `0x8ac78` | `camera_update` | calls the transform at 0x8b610 with the camera pointer and the projection at 0x82c14, copies the position at +0x3c..+0x44 to saved_position, clears the velocity at +0x6c..+0x74 and the position, and stores the nine values of the second basis at +0x48..+0x68 scaled by the float at 0x7ad90 into scaled_basis<br>the original saves all registers and the four segment registers and stores the argument in the global 0x7a7ac; the rebuild without -of+ differs in the prologue and the register use, so the identity check stays open | derived |
| `0x8c150` | `memcpy` | copies `param_2` dwords to `param_1` | observed |
<!-- end generated: race_frame callees -->

`race_frame` copies the back buffer `DAT_0011fad0` into the video buffer
`DAT_00121528` with `memcpy`.  It sets the frame flag `DAT_0011d234` when
`_DAT_00100ee0 < _DAT_0011d24c` and `DAT_0011fac4` is 0; `frame_update`
loops until that flag is set.

The race state `DAT_001203b8` selects the overlay: 0 calls `race_panel_0`, 2
calls `race_panel_2`, 3 calls `race_panel_3`, and 4 calls `race_panel_4`.

## Third level: object transform, physics, and overlay dispatch

### object_transform callees

<!-- begin generated: object_transform callees -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x1ccd8` | `object_state_save` | the original at `0x1ccd8` holds 29 instructions and 105 bytes: it copies the four dword fields at `+0x454`, `+0x45c`, `+0x498`, and `+0x4a0` into `+0x160`, `+0x168`, `+0x16c`, and `+0x174`, and the float at `+0x418` into `+0x178` | derived |
| `0x1d6bc` | `object_integrate` | integrates the position at `+0x454` and `+0x45c` from the float velocity at `+0x460` and `+0x468` and the factor at `+0x450` with the frame delta `DAT_00118e04`, damps the factor with `DAT_0010cf08 + 0x13c`, and clamps it at 0<br>the original at 0x1d6bc holds 82 instructions and 339 bytes up to the first ret at 0x1d80e: it truncates each scaled velocity sum with the helper at 0x4d9f4, multiplies the factor by the damping at DAT_0010cf08 + 0x13c, clears the factor below the 10.0 constant at object 3 offset 0xb24, accumulates the dwell timer +0x46c while both positions equal the target at +0x160 and +0x168, and snaps both positions by the scale at DAT_0010cf08 + 0x180 at the 3.0 constant at offset 0xb28; the rebuild keeps the 376-byte extent and uses a module-local FPU stub with a truncating __CHP<br>the harness covers the integration, the factor clamp, the dwell accumulation, the snap, both truncation directions, the threshold boundary, and the damping reduction; the behavior check passes | derived |
<!-- end generated: object_transform callees -->

### object_physics callees

<!-- begin generated: object_physics callees -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x1d834` | `object_gear_step` | the original at `0x1d834` holds 133 instructions and 635 bytes. It branches on the state at `+0x2f8` through the jump table at `0x1d824`. State 2 sets the state to 1, the timer `+0x2fc` to 3000.0, the counter `+0x300` to 0, the velocities `+0x304` and `+0x308` to 12.0, and the velocity `+0x30c` to 0. State 1 integrates the position `+0x49c` and the three velocities with the damping at `DAT_0010cf08 + 0x184`, subtracts 6000.0 times the frame delta from the timer, and on the target compare at `+0x238` increments the counter, snaps the position, reflects the timer by 0.4, and clears the three integrated values. State 3 sets the state to 4 and the velocity `+0x308` to 24.0. State 4 integrates that velocity into `+0x318` and clears the state at the 12.566371 threshold. The record keeps the 772-byte extent.<br>the harness holds the object record, the two globals, the damping table, and the four constant globals. It covers the state 1 and state 2 integration, the two wrap branches, the counter reset, the negative truncation, the two state 3 and state 4 branches, and the default path. The behavior check passes. The module uses an if-else chain because the V2 switch jump table faults in the harness. It uses a local truncating FPU stub because the shared stub rounds to nearest. | derived |
| `0x1db38` | `object_gear_state` | sets `+0x4e8` to 0, `0xff00`, or `0x8000` from the value at `+0x3f4` | derived |
| `0x1df34` | `object_slide_step` | computes the slide from `+0x418` and `+0x2f4`, clamps it between -2.0 and 2.0, and calls `0x22b80` for the parts at `+0x4c8`, `+0x4b8`, and `+0x4b0`<br>the original at 0x1df34 holds 65 instructions and 215 bytes: it forms the slide from `+0x418`, `+0x2f4`, `+0x27c`, and `+0x280`, clamps it between -2.0 and 2.0 with the slots at `0x100b4c` and `0x100b50`, and calls `filter_update` at 0x22b80 for the parts at `+0x4c8`, `+0x4b8`, and `+0x4b0` with the values `slide * 0.5`, `slide * 0.02`, and `-abs(slide * 0.02)`; the guard is the low bit of the state at `+0x25c` | derived |
| `0x1e00c` | `object_collision` | the original at `0x1e00c` holds 193 instructions and 765 bytes up to the first `ret`: the gate `DAT_00108058`, the target scan over the 0x590-byte records at object-3 offset 0x10ab8 with the counter `DAT_00113cc8`, the two default ids at object-3 offset 0x1e30, the rotation of the deltas at `+0x498` and `+0x4a0`, the three formed values from `+0x4f0`, `+0x4e4`, and `+0x4f4`, the release at 0x1f880, and the epilogue; the record keeps the 768-byte extent<br>the constants live in object 3: the float at offset 0xb64 is 9.0, the double at 0xb68 is 2.4, the double at 0xb70 is 13568.0, the float at 0xb78 is 35000.0, the float at 0xb7c is 23552.0, and the double at 0xb80 is 32767.0; the two-entry table at offset 0x1e30 holds 2 and 10 for the type at `+0x2e0` against 0xc<br>the original truncates with the Watcom helper at 0x4d9f4, so the module uses a local truncating FPU stub; the harness supplies the stubs for `table_lookup` at 0x1f718, `0x1f7e4`, `0x1f880`, `0x2218c`, `0x22bec`, `0xc667d`, and `0xc6920`<br>the harness covers the disabled gate, the release path, the scan hit and miss, both default ids, the distance and type compares, the four state flags, the copy at `+0x418`, the negative angle, and the three collision value stores; the behavior check passes | derived |
<!-- end generated: object_physics callees -->

### overlay_dispatch callees

<!-- begin generated: overlay_dispatch callees -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x1f718` | `table_lookup` | returns the table entry for the index when the table flag is set, and `-1` otherwise | inferred |
| `0x289bc` | `overlay_queue_update` | the original at 0x289bc holds 117 instructions and 415 bytes up to the ret at 0x28b5a. The first loop walks the signed source DAT_00102108 and stops at the first negative entry. A source entry with the value 0 or with a clear byte at DAT_00100002[i] clears the flag DAT_00120884[i] and moves the value into the queue DAT_0012082c through the count DAT_0012087c. The queue write also clears the next slot. A source entry above 0 with a set byte at DAT_00100002[i] sets the flag to 1 and waits. The count stops at 4, so the queue holds five entries. The second loop walks the nine 0x18-byte records at DAT_00102e04 with the five-byte key at +0, the value pointer at +0xc, the value at +0x10, and the xor flag at +0x14. It compares the low byte of each of the five queue entries and stops at a zero queue entry without a mismatch. It runs the record on a full match with a store or an xor through the pointer. It calls table_lookup at 0x1f718 with 8 and clears the count and the first queue entry. It clears the queue when all nine records mismatch. | derived |
| `0x28b5c` | `menu_enter` | saves `DAT_0011fb58`, clears the fade `DAT_0011d224`, sets `DAT_0011fb64` to -1, calls `menu_frame`, and restores the saved value | derived |
| `0x28ba4` | `net_race_screen` | the first-return body at `0x28ba4` holds 34 instructions and 118 bytes: the state guard, the menu index and the two table reads, the menu-id switch, and the epilogue; the case bodies extend to `0x293a7` (646 instructions and 2052 bytes), and the state 2 overlay body starts at `0x28fd6`.  The rebuild keeps the dispatcher and represents the case bodies and the overlay body as step functions | inferred |
| `0x294c8` | `race_state_begin` | the original at `0x294c8` holds 35 instructions and 117 bytes: it stores the constants 2.0, 0.6, 1.0, 20000, and -12.566371 at `DAT_00120378`, `DAT_00120380`, `DAT_0012037c`, `DAT_001203b0`, and `DAT_0012038c`, sets `DAT_0011fac8` to -1, copies the old state into `DAT_001203b8`, stores the parameter at `DAT_0011facc`, and sets `DAT_0011fac4` to 1 | derived |
| `0x2dddc` | `net_player_add` | the original at 0x2dddc holds 111 instructions and 394 bytes: it calls the slot source at 0xc6fbc, stores the argument and the index at +0x04 and +0x08 of the 0x38-byte record array at object-3 offset 0x2099c, copies the name from DAT_001203cc at +0x0c, stores the cells DAT_001203c4 and DAT_001203c8 at +0x10 and +0x14, reads the context pointer DAT_00102dec for +0x18 and the lookup at +0x1c, stores DAT_0011d274 and DAT_0011d230 at +0x20 and +0x24, copies the default block at 0xc7100 to +0x28, calls the register routine at 0xc7478 with (record, 0xc, name, 0x74, 9), and returns the index | derived |
<!-- end generated: overlay_dispatch callees -->

## The session start, the replay records, and the graphics decoders

### race_session_start

`race_session_start` at `0x128e0` sets the flags `_DAT_00118e1c = 0x102`, the
signature `DAT_0011521c = 0x1234`, and `_DAT_0011cb14 = 1`, clears the state
globals, and then dispatches on the game mode `DAT_0011d260`.

| Mode | Handler | Name | Confidence |
| --- | --- | --- | --- |
| 1, 2 | `0x12c0c` | `game_mode_1_start` | inferred |
| 4 | `0x12e18` | `game_mode_4_start` | inferred |
| 0x10 | `0x12d0c` | `game_mode_10_start` | inferred |
| 0x20 | `0x13120` | `game_mode_20_start` | inferred |
| 0x40 | `0x130a0` | `game_mode_40_start` | inferred |
| 0x100 | `0x12f50` | `game_mode_100_start` | inferred |
| other | default | `view_move(0, 0)` at `0x1f94c` and `0x21da8(param ^ 1)` | derived |

`DAT_0011d260` is the game mode.  `object_player_input` tests it against the
value 0x10 and `net_race_screen` against the value 2, so the same field selects
the race type.

### The replay records

`timing_update` at `0x22df8` drives the replay.  It tests the flag
`DAT_00118e1c & 0x40` for a playback and the flag `DAT_00118e6c`; the write path
uses `DAT_00118e60`.

<!-- begin generated: The replay records -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x22e94` | `replay_read` | reads the 20-byte record at `DAT_00118e5c + index * 0x14` and returns 1 at the end markers<br>the original at 0x22e94 holds 58 instructions and 190 bytes: the record address from the index at DAT_00118e64, the two dword copies at +0x00 and +0x04, the six signed 16-bit copies at +0x08, +0x0a, +0x0c, +0x0e, +0x10, and +0x12, the index advance, and the two end-marker tests; the behavior check passes; the identity check stays open because the prologue and the register saves differ | derived |
| `0x22f54` | `replay_write` | writes the 20-byte record at `DAT_00118e5c + index * 0x14` and advances the index with a wrap at the end marker<br>the original at 0x22f54 holds 64 instructions and 228 bytes: the five register saves, the frame, the four-byte local area, the two float copies at +0x00 and +0x04, the six signed 16-bit copies at +0x08 through +0x12, the index advance at DAT_00118e64, the wrap test against DAT_00118e68, and the second-marker update at DAT_00118e70; the behavior check passes; the identity check stays open because the register saves and the record address differ | observed |
<!-- end generated: The replay records -->

A record holds the frame delta at `+0x00`, a state word at `+0x04`, and six
16-bit state words at `+0x08`.  The frame delta `DAT_00118e04` and the state
`_DAT_00119880` are the fields that `timing_update` writes into the record and
reads back.

### The graphics decoders

<!-- begin generated: The graphics decoders -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x10ba4` | `graphics_load` | the original at `0x10ba4` holds 40 instructions and 88 bytes and copies the two name arguments into the object-3 buffers at offsets `0x7f80` and `0x7fb4`, which the loader relocates to linear `0x107f80` and `0x107fb4`; a buffer holds up to `0x33` characters and the terminator | derived |
| `0x1b5a4` | `pcx_load` | seeks 0x300 bytes before the end, reads the palette, shifts every palette byte right by 2, reads the 128-byte PCX header, and decodes the RLE runs<br>the original at 0x1b5a4 holds 108 instructions and 401 bytes: the sprintf call at 0x4d32c, the fopen call at 0x4c08d, the two seek calls at 0x4c5c7, the palette read and the 0x300-byte shift loop, the 0x80-byte header read, the size arithmetic, the RLE decode loop, and the stream_close call at 0x4c38d | observed |
| `0x2a364` | `loading_screen` | the original at 0x2a364 holds 127 instructions and 498 bytes up to the ret at 0x2a555; the first call at 0x2f326 receives the pointer DAT_000062ac and the name NET_SPACE; a second argument above 6 drops by 6 and sets the bit 0x08 of DAT_0001d274, and the function restores the saved dword at the end; the function sets DAT_000203bc to the argument minus 1, calls 0x8bd54 with the table value DAT_000021d8[context word +8], copies that word to DAT_0001d2b8, clears 0x3e800 bytes at DAT_00021528 through memset at 0x8c160, and fills the screen through 0x2aa98; it builds load%d%c.pcx with the letter H through sprintf at 0x4d32c, loads gfx\graf.dat into DAT_0001fad0 with the palette at DAT_0000cf5c through pcx_load at 0x1b5a4, runs the cleanup at 0x15150 on a non-zero result, sets the palette through 0x81a05, shows the message through 0x258d0, copies the pixels through memcpy at 0x8c150, runs the swaps at 0x1075f and 0x107a1 behind the flags 0x10 and 0x20, draws DAT_0001fc3c at y 0x50 with font 3 and color 0 through 0x2e7cc, fills the screen again, builds map%d at DAT_0001d294, publishes the arguments at DAT_0001d2b0 and DAT_0001d2b4 and the context words at +0x38 and +0x48, reads the 0x50-byte record from DAT_00020430 at the argument minus 1 when the low byte of DAT_0001d260 holds none of 0x64 and stores 0 otherwise, runs race_cleanup at 0x2a1b0, runs 0x1fd70, and on a non-zero result calls 0x108e4 with 3, 0x10978, and 0x300be; the tail calls 0x1be6c | derived |
<!-- end generated: The graphics decoders -->

`pcx_load` takes a file name, an entry name, and an output palette.  It builds
the name with the format `%s\%s`, opens it in the mode `rb`, seeks 0x300 bytes
before the end for the palette, converts the palette to 6-bit values, reads the
128-byte header, computes the pixel count as `(xmax - xmin + 1) * (ymax - ymin +
1)`,
and decodes the RLE runs: a byte with the mask 0xc0 equal to 0xc0 repeats the
next byte `byte & 0x3f` times.

### The graphics assets

| Asset | Content | Loader |
| --- | --- | --- |
| `%s\gfx\graf.dat` | the PCX images `menu1h.pcx`, `menu2h.pcx`, `menu3h.pcx`, `menu4h.pcx`, `champ5h.pcx`, `load%d%c.pcx`, and `net%c.pcx` | `pcx_load` |
| `%s\gfx\menu.dat` | the sprites `menu%c.spr` and the fonts `menu%c.fnt` | `menu_space_init` |
| `OPT_PCX` | an optional PCX image | `race_cleanup` |

Evidence: the strings at `0x100f4d`, `0x100f5d`, `0x100f68`, `0x100f73`,
`0x100fd5`, `0x101014`, `0x1011a2`, `0x1011bc`, and the decompiled
`FUN_0001b5a4`, `FUN_00029d08`, and `FUN_0002a364`.

### The TABS table loaders

`engine_init` calls `FUN_0004c810` at `0x1287a`, and that caller loads the two
TABS tables.

<!-- begin generated: The TABS table loaders -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x4c810` | `stream_layer_init` | stores the stream pointer, clears the state word, and runs the six stream layer setup calls | derived |
| `0x4cf94` | `persptab_load` | opens `TABS\persp01.tab` with the mode `rb`, reads `fread(buffer, 1, 0x40740, stream)` into the `PERSPTAB` cell, and advances the cell by `0x40000`<br>the original at `0x4cf94` holds 43 instructions and 155 bytes: it opens `TABS\persp01.tab` with the mode `rb` through `fopen` at `0xc5fe5`, runs the BIOS video call with 3, the error report with `0x16`, and the exit with `0x16` on failure, calls `resource_read` at `0x2efc2` with the second argument `0x1747` and the size `0x40740`, stores the result in `DAT_000214ec`, reports a null result through `0x2edad`, reads `0x40740` bytes through `fread` at `0xc607b`, advances the cell by `0x40000`, and closes the handle through `0xc6245` | derived |
| `0x4d030` | `vis_tab_load` | the original at `0x4d030` holds 31 instructions and 101 bytes: it opens `TABS\VIS.TAB` with the mode `rb` through `fopen` at `0xc5fe5`, runs the error report, BIOS video, and exit path with the value `0x16` on failure, reads `0x27500` bytes into the buffer at `DAT_0007c1c4` through `fread` at `0xc607b`, and closes the handle through `0xc6245` | derived |
<!-- end generated: The TABS table loaders -->

The call order is `persptab_load` first and `vis_tab_load` second, so the
`PERSPTAB` table reads first.  The run-time open order confirms it: the game opens
`TABS\persp01.tab` before `TABS\VIS.TAB`.
Evidence: the call site scan, `python3 tools/le.py disasm STARTH.EXE 0x4c810`,
`0x4cf94`, `0x4d030`, and `0x127a0`, and `tools/scripts/probe_open_window.dbg`.

### The sound loaders

`engine_init` loads the music bank and starts the music, and the menu space init
loads the sample bank.

<!-- begin generated: The sound loaders -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x1f2e0` | `sound_bank_load` | builds `%s\data\sound.dat`, walks the 0x24-byte sample records at `DAT_0015224`, builds `%s.RAW` for each name, and opens the member through the member open `0x4c08d`; a failed open reports the file error and runs the exit 0<br>the rebuild copies the 0x74-byte record template, reads every member through the `SOUND_FX` resource at `0x2eed0`, and maps the high nibble of `+0x20` to the type 0, 2, 1, and 3; the five behavior paths cover the setup gate, the two records, the four status values, the two failed opens, and the zero resource | derived |
| `0x2f9b0` | `music_bank_reset` | clears the fields `+0x04`, `+0x10`, and `+0x14` of the music bank struct | derived |
| `0x2f9e0` | `music_bank_load` | builds `%s\DATA\MUSIC.DAT`, copies the name, and opens the member `melodic.bnk` (or `drum.bnk`) through the member open `0x4c08d`<br>the body continues past the first 72-instruction block to the full 688-byte extent: it opens melodic.bnk and drum.bnk, reads both through resource_read, installs them for the driver values 0xa002 and 0xa009, and reports the statuses 1, 2, and 3 | derived |
| `0x2ffc4` | `music_start` | the original at `0x2ffc4` holds 41 instructions and 140 bytes: it reads the status word at `+0x10` of the music bank struct, sends the statuses 1 to 6 through the jump table at object offset `0x1ffac` to the messages `0x1577`, `0x1596`, `0x15b6`, `0x15ca`, `0x15e2`, and `0x15f5`, prints the message through `0x3009b`, and calls `exit(-1)` at `0x300be` for a non-zero status; a zero status returns at once<br>the only caller is `engine_init` at `0x1284d`, after `music_bank_load` at `0x1283f`; `music_bank_load` writes the status at `0x2f9e0` and `music_bank_reset` clears it at `0x2f9b0` | derived |
<!-- end generated: The sound loaders -->

The callers: the Watcom startup calls `music_bank_reset` at `0x10013`, so the
music bank struct starts empty.  `engine_init` calls `music_bank_load` at `0x1283f`
and `music_start` at `0x1284d`.  `sound_bank_load` is called from the config
load at `0x15a64` and from the menu space init at `0x29f07`.
Evidence: the caller scan of `0x1f2e0`, `0x2f9b0`, `0x2f9e0`, and `0x2ffc4`
and `python3 tools/le.py disasm STARTH.EXE 0x127a0` and `0x1f2e0`.

## Fifth level: the vehicle states, the timing filter, and the runtime

### The vehicle state chain

<!-- begin generated: The vehicle state chain -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x136cc` | `player_car_init` | the original at 0x136cc holds 277 bytes and 87 instructions up to the early ret at 0x137e0; the full body continues to the ret at 0x142ea, so the store keeps the 3104-byte extent and sets no size<br>the six callers at 0x12c83, 0x12d2c, 0x12e2e, 0x12f2a, 0x130fa, and 0x13181 pass the player object at object-3 offset 0xf91c and a load flag; the function copies the selected 0x40-byte CARS_DATA record from 0xd25c + DAT_0011d2a0 * 0x40 to +0x470, uses the string at +0x474, four bytes into the file name, so AUTO00 gives 00, reads the animation flag at +0x47c, builds COCK%s%c with A or B, loads the cockpit through 0x1b23c, reads the CARS list through 0x15460, and loads the SHADOW object at +0x468 and the animation object at +0x460 through 0x11060; the data at object-3 offset 0x5a5 holds the timer format %02d:%02d:%02d, so the module names the function player_car_init<br>the rebuild covers the 87-instruction prefix; the zero load flag returns 0 and the success path returns 0 after the two object loads; the state reset after 0x13dcf is not in the module | derived |
| `0x142ec` | `car_setup` | the two callers at `0x12c0c` and `0x12f50` walk the car count `DAT_0000f8ec` and call the function for each 0x590-byte record at `0x10ab8 + index * 0x590`; the call target `0x11060` searches the 0x78-record table at `0x6530` by name through `stricmp` at `0x4d9ba` and loads the object through `0x10bfc`; the data at object-3 offset 0x5dc holds `SHADOW` and the records at 0x1d5c hold the wheel names `flx`, `fli`, `blx`, `rli`, `frx`, `fri`, `brx`, and `rri`, so the module names the function `car_setup`<br>the original at `0x142ec` holds 226 bytes and 65 instructions up to the early `ret` at `0x143cd`: the frame, the index store at `+0`, the load-flag gate, the three signed compares on `DAT_0000f8f0`, `DAT_0000d6dc`, and `DAT_0011d2a0`, the two `0x11060` calls with the names at 0x5dc and `0xd25c + index * 0x40`, the model index store at `+0x2e0`, and the two error returns; the full body continues to the `ret` at `0x1496c`, so the store keeps the 1668-byte extent and sets no size | derived |
| `0x15c1c` | `player_car_link` | the original at 0x15c1c holds 306 instructions and 1160 bytes up to the ret at 0x160a3, so the record keeps the 1160-byte extent and sets no function size<br>the five callers at 0x20195, 0x20279, 0x2034e, 0x203bb, and 0x2041f pass the player object at object-3 offset 0xf91c in eax; they are the five view groups that run player_car_view_update at 0x16f34 and the player physics group at 0x15a80 on the same object, so the record takes the section The vehicle state chain<br>the function reads the 28-byte entries at DAT_0010653c; the object fields +0x460 and +0x468 select the link, render, and target pointers; the function writes the screen positions, the three 11-bit angles, the render values from +0x4e0 and +0x4e8, the pitch ratio from +0x108 and +0x10c, the speed difference from +0x50c, the target copy, and the link flags from +0x7b0 and +0x7b4<br>the call at 0x15c7e to 0x18ccc projects the world position DAT_00110a20 and DAT_00110a28 with the screen limit 20.0 into +0x118, +0x11c, and +0x120; the object, the five callers, and the link role of the sibling object_car_link at 0x1c84c support the name player_car_link<br>the harness holds the player object, the four entry pairs, the render arrays, the constants, and the screen projection stub; it covers the full path, the state skip, the second target, the pitch divisor, the speed mask, the angle mask, the second axis, and the second link pair; the behavior check passes | derived |
| `0x1629c` | `player_steering_update` | the original at 0x1629c holds 267 instructions and 997 bytes up to the ret at 0x16680, so the record keeps the 1036-byte extent and sets no function size<br>the only caller is the player update group at 0x15a80, which calls this function at 0x15aac after the brake step at 0x16150; the group runs on the player object at object-3 offset 0xf91c, so the module takes the section The vehicle state chain<br>the function selects the filter index 0x50, 0x55, or 0x58 from the state at +0x148 and the flags at +0x110, +0x4f4, and +0x170, forms the limit (1 - sqrt(+0x50c / +0x480)) * 0x100720 + 0x100728, scales the six control values at 0x119880 by 1/1024 and the limit, and uses the mode at 0x119818, so the module names the function player_steering_update<br>a non-zero angle gives the turn radius wheelbase/tan(angle) at +0x2f8, the yaw step +0x50c * 0x118e04 / radius at +0x2f4, adds it to the heading at +0x4cc, and stores the rotated position difference at +0x1c and +0x24 | inferred |
| `0x16f34` | `player_car_view_update` | the original at 0x16f34 holds 318 instructions and 1236 bytes up to the ret at 0x17407, so the record keeps the 1236-byte extent and sets no function size<br>the five callers at 0x2003c, 0x201d0, 0x202c0, 0x20360, and 0x203c4 run before the player physics group at 0x15a80 on the player object at object-3 offset 0xf91c; the callers are the five view groups that the frame update at 0x1ff70 selects, so the module takes the section The vehicle state chain<br>the function normalizes the angles at +0x4c8, +0x4cc, and +0x4d0, filters +0x300 from +0x688 / DAT_0011d254, projects the motion into +0x4b0 and +0x4b8, applies the rates at +0x548 and +0x54c and the shake terms at +0x4fc, +0x500, and +0x508, and writes the render flags at +0x7b4, so the module names the function player_car_view_update<br>the state at +0x148 selects the full pitch projection or the flat yaw projection, and the flag at +0x108 selects the pitch path or the roll angle path | inferred |
| `0x17d14` | `player_car_skid_update` | the original at 0x17d14 holds 290 instructions and 1078 bytes up to the ret at 0x18149, so the record keeps the 1080-byte extent and sets no function size<br>the only caller is the effect group at 0x15bac, and the five view groups at 0x2003c, 0x201d0, 0x202c0, 0x20360, and 0x203c4 call that group on the player object at object-3 offset 0xf91c, so the module takes the section The vehicle state chain<br>the function forms the slip from the speed +0x50c, the yaw step +0x2f4, the frame delta DAT_00118e04, and the table indices +0x1c0, +0x268, +0x214, and +0x2bc; it stores the shake terms +0x4f0, +0x4f4, +0x500, and +0x508, which player_car_view_update applies to the view<br>the filtered slip +0x128 gates the effect: above 0.23 it allocates through table_lookup at 0x1f718 and starts the four slots through 0x19828; at or below 0.23 it releases through 0x1f880 when the state +0x148 is below 3; the shake terms and the effect gate support the name player_car_skid_update<br>the harness covers the deceleration path, the large and small slip, the two amplitude clamps, the gate above the limit, the two early flags, and the input bit 0x40; the behavior check passes | inferred |
| `0x18db0` | `engine_rpm_update` | the original body at `0x18db0` holds 448 instructions and 1734 bytes up to the `ret` at `0x19475`; the record keeps the 1736-byte extent and sets no function size<br>the only direct caller is the player update group at `0x15a80`, which runs this function at `0x15a94` after the player input step at `0x18a14` and before `0x160a4`, `0x16150`, and `player_steering_update` at `0x1629c`; the group runs on the player object, so the module takes the section The vehicle state chain<br>the function reads the input flags `0x18e1c` and `0x18e1d` and the joystick axes `0x19880` through `0x19894`; the flag bit `0x40` selects the automatic axis store and the filter index `0x3d`, the flag bit `0` gates the manual and automatic paths, and the manual path compares the axes 4 and 5 against `0x1985c` and `0x19878`<br>the function stores the gear at `+0x7ac` and the shift flag at `+0x140`; the automatic path uses the thresholds `0x2134` and `0xfa0` against the engine speed at `+0x7a8`, the gear count at `+0x494`, and the state `+0x148`<br>the function forms the speed limit `+0x510` as `+0x480 - (sin(+0x4c8) * 4.0 + 1.0) * 1000.0`; it forms the engine-speed target from the throttle `+0x144`, the maximum `+0x484`, the wheel factor `+0x174`, and the gear ratio at `+0x494 + gear * 4`; it updates the road speed `+0x50c`, sets `+0x110` on a strong deceleration, and calls `0x19478`, `filter_update` at `0x22b80`, `sin` at `0x4da72`, `sqrt` at `0x22bec`, and `0x1f7e4` with `+0x7cc` and `14 * +0x7a8 + 32768`<br>the engine-speed store at `+0x7a8`, the gear store at `+0x7ac`, the two shift thresholds, the gear-ratio table, and the sound value support the name `engine_rpm_update`; the module records the name with the confidence inferred | inferred |
| `0x1cee0` | `object_drive_step` | computes the drive force from `+0x2e4`, `+0x2f0`, and the throttle `+0x3fc`; sets the brake flag `+0x2c0` when the force is below the speed `+0x418` and reduces the speed by `+0x2ec` times the frame delta<br>the original at 0x1cee0 holds 83 instructions and 297 bytes: it compares the mode at +0x410 with DAT_0000ffa4, forms the target (+0x2e4 + +0x2f0) * +0x3fc and the bound +0x2e4 * +0x3fc, or the single target +0x2e4 * +0x3fc; below the target it clears +0x2c0 and adds +0x2e8 times DAT_00118e04; above the target it sets +0x2c0 when the speed is above the bound, subtracts +0x2ec times DAT_00118e04, and clamps the speed to the target<br>the harness covers the accelerate branch, both clamps, the brake set and no-set paths, the equal-speed path, the inactive mode, and the negative-throttle bound; the behavior check passes | derived |
| `0x1d30c` | `object_revolution_step` | the original at `0x1d30c` holds 151 instructions and 589 bytes: it accumulates the revolution fraction at `+0x250` from the speed at `+0x418` and the frame delta `DAT_00118e04` divided by the axis call at `0x222e0`, wraps the fraction at 1.0, toggles the gear at `+0x254`, advances the table index at `+0x24c` from the entry at `+0x10`, builds the matrix through `0x1d55c`, truncates the positions at `+0x258` and `+0x260` with the helper at `0x4d9f4`, shifts the five state fields, and stores the angle from `0x226bc` at `+0x27c`; the record keeps the 592-byte extent<br>the harness holds the object record, the two globals, the gear table, and the four callee stubs; it covers the no-wrap path, the wrap with gear 0 and gear 1, the negative truncation, the exact 1.0 boundary, and the distance argument; the behavior check passes, and the module uses a local truncating FPU stub because the shared stub rounds to nearest | derived |
| `0x1e30c` | `object_proximity_check` | measures the distance from the reference `DAT_00113ccc` and `DAT_00113cd0`; inside 35000 units it sets the gate `+0x414` and the value `+0x4e4`, outside it clears the gate<br>the original at 0x1e30c holds 74 instructions and 241 bytes up to the first `ret`: the two absolute-value guards at 35000 units on `+0x498` and `+0x4a0`, the deltas scaled by 0.1 and truncated with the helper at 0x4d9f4, the squared sum, the `sqrt` call at 0x22bec, the scale by 10.0, and the gate store from the float at `+0x4e4` against 35000.0 at 0x100b98; the rebuild keeps the extent 264 and uses a module-local FPU stub with a truncating `__CHP` | derived |
| `0x22744` | `spline_interpolate` | the original at 0x22744 holds 360 instructions and 1081 bytes up to the ret 4 at 0x22b7a; the record keeps the 1084-byte extent and sets no function size<br>the only direct caller at 0x231e0 sits in the path playback at 0x23038; that function walks the 0x28-byte keyframe records that 0x231f0 appends from the view fields of the player object at 0xf91c; the frame update at 0x1ff70 selects the view group at 0x202c0, which calls 0x231f0 and 0x23038<br>the caller passes the previous, the first, the second, and the next keyframe plus the fraction between the two timestamps; the function reads the four points from the pointer frame and writes three interpolated integers to the output<br>the function measures the chords AB, BD, and AC with the square-root wrapper at 0x22bec; the chord AB divides by the double 3.0 at object-3 offset 0xda4; the tangents are A plus (B - D) times (|AB| / (3 * |BD|)) and B plus (A - C) times (|AB| / (3 * |AC|)), and a zero distance keeps the matching tangent<br>the result is the cubic Hermite form A + t * 3(Aprime - A) + t^2 * (3Bprime - 6Aprime + 3A) + t^3 * (3Aprime + B - 3Bprime - A), where Aprime and Bprime are the two tangents; the fraction, its square, and its cube stay in single precision, the polynomial evaluates in double precision, and each axis truncates through 0x4d9f4<br>the caller, the keyframe append, and the cubic form support the name spline_interpolate; the module records the name with the confidence inferred<br>the harness holds the four keyframes, the pointer frame, the output triple, and the chord scale 3.0; it covers the collinear fraction 0.5, the two endpoints, the zero |BD|, the zero |AC|, the three-dimensional point, the two three-dimensional endpoints, and the negative coordinates; the behavior check passes | inferred |
<!-- end generated: The vehicle state chain -->

`object_proximity_check` sets the gate at `+0x414`.  `object_physics` and
`object_prepare` run only when that gate is set, so the chain closes: the
proximity check enables the physics of the objects near the reference point.

### The timing and filter layer

<!-- begin generated: The timing and filter layer -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x128e0` | `race_session_start` | the original at `0x128e0` holds 42 instructions and 136 bytes up to the second error return and extends to `0x12c0b` (812 bytes): it sets `_DAT_00118e1c` to 0x102, calls `0x131a8`, sets `DAT_0011521c` to 0x1234 and `_DAT_0011cb14` to 1, clears five state globals, calls `0x13580` and `0x13540`, dispatches the game mode `DAT_0011d260` to the handlers `0x12c0c` for the modes 1 and 2, `0x12e18` for 4, `0x12d0c` for 0x10, `0x13120` for 0x20, `0x130a0` for 0x40, and `0x12f50` for 0x100, takes the default path `view_move(0, 0, 0)` at `0x1f94c` and `0x21da8(value ^ 1)` for every other mode including 3, then runs the frame setup and clears the state words | derived |
| `0x22df8` | `timing_update` | advances the timing when the replay flag is set: the timing subtraction, the replay read, and the filter entry from the frame rate | derived |
<!-- end generated: The timing and filter layer -->

`timing_update` divides 1 by the frame delta `DAT_00118e04` and selects a
400-byte block of the table `DAT_001080e8`.  The block index is the frame rate
in frames per second, so the table holds one filter coefficient set per frame
rate.

The filter table has two writers: `frame_timer` at `0x1fc38` and `timing_update`
at `0x22df8`.  The object functions read it: `0x18db0`, `0x19980`, `0x22f54`,
`0x1d834`, `0x18528`, and `0x1c84c`.  Evidence: the Ghidra references to
`0x10cf08`.

The frame delta `DAT_00118e04` has three writers: `frame_timer` at `0x1fc38`,
`race_session_start` at `0x128e0`, and `timing_update` at `0x22df8`.
Evidence: the Ghidra references to `0x118e04`.

### The runtime memory manager

<!-- begin generated: The runtime memory manager -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0xd1ec0` | `memset` | replicates the value byte over the dword and calls the run-time core at `0xe0860` | observed |
| `0xd1f25` | `malloc` | rejects the size 0 and the sizes above 0xffffffd4, rounds the size up to 4 bytes, and walks the free list at `DAT_001031d0`<br>the original at `0xd1f25` holds 84 instructions and 227 bytes: the nine register saves, the frame, the four-byte local area, the size checks at `0xd1f35` and `0xd1f39`, the round-up at `0xd1f4a`, the free-list walk from `0x31d0`, the helper calls at `0xe09dc`, `0xe0f73`, and `0xe112e`, the largest-free update in `0x31d8`, the cursor in `0x31d4`, the heap flag clear at `0x27de5`, and the epilogue | observed |
| `0xd2008` | `free` | the original at `0xd2008` holds 35 instructions and 94 bytes: it walks the free list from `DAT_001031d0`, stops before the next free block, releases the block through the heap routine at `0xe0a84`, keeps the largest free size in `DAT_001031d8`, and clears the heap flag at `0x27de5` | observed |
| `0xd2066` | `memmove` | copies `count` bytes from the source to the destination and returns the destination | observed |
| `0xd429f` | `__fprtf` | the original at `0xd429f` holds 52 instructions and 133 bytes: it saves the file flag bits 0x30, clears them, allocates the buffer through `0xddda9` when the buffer pointer at `+8` is zero, changes the `+0xd` bit 2 to bit 0, formats through `0xd68c0` with the table `DAT_001c428e`, restores the flags and calls `0xddfa8`, returns -1 when the `+0xc` bit 0x20 is set, and restores the saved 0x30 bits; the call sites are `printf` at `0x3009b` and `fprintf` at `0x4bf64`, so this is the run-time `__fprtf` | observed |
| `0xd4410` | `dpmi_int31_entry` | writes the returned registers, the flags, and the segment values back into the block after the dispatcher at 0xd4448 runs the interrupt | inferred |
<!-- end generated: The runtime memory manager -->

`list_create` allocates the record array through `malloc`, so the engine lists
and the `.3dx` object records come from the game heap at `0xd1f25`.  The DPMI
services allocate the separate buffers for the real-mode drivers.

## Fourth level: the gear step, the slide step, and the resource layer

### object_gear_step and object_slide_step callees

<!-- begin generated: object_gear_step and object_slide_step callees -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x22b80` | `filter_update` | applies the one-pole filter `*p = (*p - f) * table[i] + f` with the table pointer `DAT_0010cf08` | derived |
<!-- end generated: object_gear_step and object_slide_step callees -->

`object_gear_step` holds the vehicle states 1 and 2 at `+0x2f8`.  State 2 sets
the float fields `+0x2fc` and `+0x300` to 3000.0 and the three velocities
`+0x304`, `+0x308`, and `+0x30c` to 12.0, then falls through to state 1.
State 1 integrates the position `+0x49c` and the three velocities with the
damping `DAT_0010cf08 + 0x184`, decrements the value at `+0x2fc` by the frame
delta, and counts the value at `+0x300` when the position passes `+0x238`.
Evidence: the decompiled `FUN_0001d834`.

`object_slide_step` computes the slide from the speed `+0x418`, the steering
`+0x2f4`, and the float pair `+0x27c` and `+0x280`, clamps the slide between
-2.0 and 2.0, and filters the three parts at `+0x4c8`, `+0x4b8`, and `+0x4b0`.
Evidence: the decompiled `FUN_0001df34`.

### The resource layer

The resource layer holds the list, the file records, and the DPMI buffer
services.

<!-- begin generated: The resource layer -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x2edad` | `memory_error_report` | the six bodies print the memory format strings at 0x1394, 0x13c4, 0x13f9, 0x1415, 0x1444, 0x1475, and 0x14a2; the dispatch compares the status at [arg+0xc] at 0x2ee84<br>the original at 0x2edad holds 97 instructions and 291 bytes: it reads the status at [arg+0xc], prints the six memory format strings at 0x1394, 0x13c4, 0x13f9, 0x1415, 0x1444, 0x1475, and 0x14a2 through the printer at 0x4bf64 with the stream 0x2fd2, passes the name at +0x10 and the slot count at +0x08, and calls exit(-1) at 0x300be | observed |
| `0x2efc2` | `resource_read` | reads the current record and caches it through `0x2f062`<br>the original at `0x2efc2` holds 56 instructions and 160 bytes: it compares the count at resource + 0x04 with the limit at resource + 0x08, sets the status at + 0x0c to 2 and copies the name to resource + 0x10 through `sprintf` at `0x4d32c` when the count equals the limit, selects the record at `*resource + count * 0x1c`, calls `memory_alloc_small` at `0x2f172` with the size and the record cell at + 0x10, clears the flag at + 0x18, and calls the cache routine at `0x2f062` with the resource, the record, the allocation result, the name, and the size | observed |
| `0x2f0fc` | `dos_memory_alloc` | the original at `0x2f0fc` holds 41 instructions and 118 bytes: it sets `0x0100` at result word 0, sets the paragraph count `(size >> 4) + 1` at word 1, fills the segment set through `dpmi_fill_regs` at `0x4bf23`, runs the interrupt 0x31 call through `dpmi_call` at `0x4bf4b`, returns 0 on a non-zero status, and otherwise returns the segment word times 16 and stores the selector word at the third argument | derived |
| `0x2f172` | `memory_alloc` | the original at `0x2f172` holds 49 instructions and 147 bytes: it sets `0x0501` at block offset 0, splits the size into the words at offsets 4 and 8, fills the segment set through `dpmi_fill_regs` at `0x4bf23`, runs the interrupt 0x31 call through `dpmi_call` at `0x4bf4b`, returns 0 on a non-zero status at block offset `0x18`, and otherwise stores the address from block offsets `0x10` and `0x14` at the third argument and returns the size words | derived |
| `0x2f205` | `memory_free` | the original at `0x2f205` holds 33 instructions and 90 bytes: it builds the local register block with `0x0502` at offset 0 and the address words at offsets `0x10` and `0x14`, fills the segment set through `dpmi_fill_regs` at `0x4bf23`, runs the interrupt 0x31 call through `dpmi_call` at `0x4bf4b`, and returns the status at block offset `0x18` | derived |
| `0x2f25f` | `resource_find` | the original at `0x2f25f` holds 69 instructions and 199 bytes: it walks the 0x1c-byte records up to the count at `+0x04`, matches the key at `+0xc`, calls `resource_free` at `0x2f470`, shifts the records after the match with the memmove at `0xd2066`, decrements the count, and sets the status at `+0x0c` to 3 on a failure and 0 on a success | derived |
| `0x2f470` | `resource_free` | the original at `0x2f470` holds 47 instructions and 126 bytes: it selects the 0x1c-byte record at `*resource + index * 0x1c`, reads the address at `+0x10`, calls `0x2f40b` when the flag bit 0 at `+0x18` is set and `memory_free` at `0x2f205` otherwise, and returns -1 on a non-zero free result and 0 on success | derived |
| `0x2f904` | `pool_init` | the original at `0x2f904` holds 44 instructions and 129 bytes: it sets `0x0500` at result word 0, sets the result buffer to `pool + 0x1c` at word 5, fills the segment set through `dpmi_fill_regs` at `0x4bf23`, runs the interrupt 0x31 call through `dpmi_call` at `0x4bf4b`, returns 1 on a non-zero status, multiplies the field at `pool + 0x24` by 4096, multiplies the field at `pool + 0x3c` by 4096 when it is positive and sets it to 0 otherwise, and returns 0 | inferred |
| `0x4bf23` | `dpmi_fill_regs` | fills the 12-byte real-mode segment set with es, cs, ss, ds, fs, and gs | observed |
| `0x4bf4b` | `dpmi_call` | calls the DPMI INT 31h entry at `0xd4410` with the interrupt number, the register block, the result block, and the segment set, and returns the value from the result block | inferred |
| `0x4bf88` | `file_error_report` | the six bodies print the engine file strings at 0x1608, 0x1622, 0x1644, 0x1653, and 0x1685; the dispatch compares the status at [arg+0xac] at 0x4c060<br>the original at 0x4bf88 holds 88 instructions and 261 bytes: it reads the status at [arg+0xac], dispatches through the five-entry jump table at 0x4c04c, prints the five file strings and the default interface string through the printer at 0x4bf64 with the stream 0x2fd2, passes the name at +0x78, and calls exit(-2) at 0x300be; the string immediates 0x1608, 0x1622, 0x1644, 0x1653, 0x1667, and 0x1685 are object-3 offsets | observed |
<!-- end generated: The resource layer -->

`list_create` allocates `count * 0x1c` bytes.  This confirms the 28-byte record
size of the engine lists and of the object records.

`dos_memory_alloc` computes the paragraph count as `(size >> 4) + 1` and
converts
the returned segment to a linear address with a shift of 4.  `memory_alloc`
splits
the size into the high and the low word for EBX and ECX and reassembles the
returned address.  `memory_free` splits the address the same way.

## Resource loader

`object_load` at module address `0x10bfc` loads the `.3dx` object files.  It
walks the object list and loads every record with the load flag at `+0x14` set.
For each record it:

1. builds the file name from the record name with `sprintf`,
2. opens the file with `fopen`,
3. looks the `3D_HEADER` chunk up through `resource_get`,
4. looks the `3D_BLKOBJ` chunk up,
5. binds the file to the object record with `object_init_from_file`.

It returns `2` when the open fails, `3` when the `3D_HEADER` chunk is missing,
and `5` when the `3D_BLKOBJ` chunk is missing.

<!-- begin generated: Resource loader -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x10bfc` | `object_load` | reads `3D_HEADER` and `3D_BLKOBJ`; the flag at `+0x14` selects the records<br>the original at 0x10bfc holds 69 instructions and 232 bytes in the first block; the full 728-byte extent continues past the early return with the free-slot walk in the handle table at `DAT_00021540`, the 0x1c-byte record walk, the `%s.3DX` name build at `0x1003ac`, the name copy to `DAT_00007fe8`, the member open at `0x4c08d` with the base path `DAT_00007f80` and the mode `rb` at `0x1003b3`, the `3D_HEADER` look-up at `0x2eed0`, the stream read at `0x4c4e5`, the stream close at `0x4c38d`, the object count at `+0x14`, the `3D_BLKOBJ` look-up with the size `count * 0x20 + 0x60`, the six cleared block words at `+0x24` through `+0x38`, the handle store, the `object_init_from_file` call at `0x4cc10`, and the chunk link at `+0x40`; the behavior check passes; the identity check stays open because the early return and the frame differ<br>the plan row records 232 bytes and 69 instructions for the first block; the merged record keeps the 728-byte extent | observed |
| `0x2eed0` | `resource_get` | looks a resource up by name in the engine resource list and loads it from its file when missing<br>the original at 0x2eed0 holds 78 instructions and 242 bytes: the full-table test on the count at +0x04 and the limit at +0x08, the status write at +0x0c, the sprintf name copy at 0x4d32c, the record select at *resource + count * 0x1c, the size limit 0x96000, the DOS allocation at 0x2f0fc with the fallback at 0x2f172, the flag writes at +0x18, and the cache call at 0x2f062 | derived |
| `0x4bf64` | `fprintf` | formats into a stream through the run-time `__fprtf` at `0xd429f` | observed |
| `0x4c08d` | `fopen` | opens the path through the run-time file layer and fills the stream record; returns `-1` on failure<br>the original at `0x4c08d` holds 48 instructions and 155 bytes: it opens the path with the mode `rb` through `0xc5fe5`, copies the path to `stream + 0x78` and sets the state at `stream + 0xac` to 1 when the open fails, copies the category to `stream + 0x20` and the path to `stream + 0x24`, calls the member open `0x4c128` with the member name, and returns -1 on either failure and 0 on success | observed |
| `0x4c38d` | `stream_close` | the original at `0x4c38d` holds 34 instructions and 89 bytes: it frees the buffer at `stream + 0x1c` through the run-time `free` at `0xd2008`, frees the buffer at `stream + 0xb0` through the wrapper `free` at `0x300ec` when the flag at `stream + 0x10` is set, closes the handle at `stream + 0x00` through `0xc6245`, and returns 0 | derived |
| `0x4c4e5` | `stream_read` | buffered read; advances the position and clamps it to the stream size<br>the original at `0x4c4e5` holds 80 instructions and 226 bytes: the register saves, the frame, the three argument slots, the flag test at `stream + 0x10`, the two clamp branches, the expander call at `0x4d3fc`, the raw `fread` call at `0xc607b`, and the return | derived |
| `0x4c7e0` | `stream_status` | returns the word at `stream + 0x70` | inferred |
| `0x4cc10` | `object_init_from_file` | bounds the file index, initializes the object record, and copies the chunk table from the file record<br>the original at 0x4cc10 holds 120 instructions and 396 bytes: the index bound at 0x3e8, the message call at 0xc656e, the handle table read at 0x21540, the object field stores at +0x00 through +0x38, the 0x20-byte chunk records at object + 0x40 from the file count at +0x14 and the offset table at +0x18, and the end marker -1 | observed |
| `0x4d32c` | `sprintf` | formats into a buffer through the run-time `vsprintf` and writes the terminator | derived |
<!-- end generated: Resource loader -->

The chunk table in the object record holds eight words per chunk: the file
record, the chunk count, and six words of the chunk header.  Evidence: the
decompiled `FUN_0004cc10`.

The loader returns codes that the error reporter at `0x10978` prints.  The
reporter dispatches through a jump table to one message stub per code, and the
stubs print the `3dx` messages.  Evidence: the decompiled `FUN_00010978` and
the Ghidra references to `0x100074`, `0x1000a3`, `0x1000d1`, and `0x1000ff`.

## Subsystem initialisation

`engine_init` at `0x127a0` runs the subsystem setup in order.  The callees:

<!-- begin generated: Subsystem initialisation -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x10810` | `engine_interface_init` | allocates the engine object and creates its list<br>the original at 0x10810 holds 51 instructions and 183 bytes: it allocates 0x4c bytes through memory_alloc at 0x30050, creates the 0xb4-record list through list_create at 0x2ecf8, stores the list pointer at DAT_0000801c, clears the fields +0x0c, +0x10, and +0x14 and sets the field +0x18 to -1 of the 0x78 records of 0x1c bytes at the first argument, clears the name buffer DAT_00007fe8, and clears the state word at the second argument<br>the caller at 0x1286b in engine_init passes the record table 0x6530 in eax and the state word 0x6350 in edx | derived |
| `0x131a8` | `race_data_load` | the original at 0x131a8 holds 236 instructions and 919 bytes up to the ret at 0x1353e, so the record keeps the 920-byte extent and sets no function size<br>the caller race_session_start at 0x128e0 calls this function at 0x128fb as its first step on the race level, so the module names the function race_data_load<br>the function builds %s\gfx\%s%c.dat from DAT_00007f44, DAT_0011d294, and the letter H or L from DAT_0011d2b8, loads the five lists OSTACLES, TRACK_DATA, OPPONENTS, ANIMATIONS, and CARS_DATA, sets the screen mode 0x140 by 0x190 with the value 0x104, copies the TRACK_DATA color to PTR_DAT_0008a844 + 0x10, and starts the music through 0x2fcec behind DAT_0011d2cc | inferred |
| `0x14e20` | `keyboard_hook` | hooks interrupt 9 and drives the keyboard LEDs<br>the original at 0x14e20 holds 100 instructions and 334 bytes up to the early-exit block at 0x14f6e: the register saves, the EBP frame, the keyboard controller command 0xed and the 0xfa acknowledge with the 0xffff timeout and the three retries, the BIOS keyboard flag clear at 0040:0017, the INT 21h AX=3509h get-vector call, the INT 21h AX=2509h set-vector call with CS:0xe4, and the second set-vector call that restores the saved vector at 0x14f6e<br>the register block at 0xcf18 holds the seven dwords eax, ebx, ecx, edx, esi, edi, and flags, and the segment set at 0xcf34 holds es, cs, ss, ds, fs, and gs; the old vector is saved at the word 0xcf0c and the dword 0xcf10<br>the harness simulates the keyboard controller and the two DPMI calls; it prints the markers 0 through 4 and D for the first-acknowledge install, the one-retry install, the four-attempt install, the uninstall, and the BIOS flag mask | derived |
| `0x15460` | `list_file_load` | the original at `0x15460` holds 161 instructions and 581 bytes and calls 15 targets: the two `sprintf` calls at `0x4d32c` build `%s\data\%s.dat` from `DAT_00007edc` and the type, and `%s.dtr` from the name; the member open at `0x4c08d` uses the category `rt`; a failed open runs `file_error_report` at `0x4bf88` and `exit(1)` at `0x300be`; the four text reads at `0x4c695` take the line mode 1 and the token mode 0; the three `stricmp` calls at `0x4d9ba` match the list name, `END_LIST`, and `*`; the `sscanf` call at `0xc2024` with `%x`, the `atoi` call at `0xc2045`, the `atof` call at `0xc209c`, and the inline string copy fill the record at `row * stride + column`; the stream close at `0x4c38d` returns the row count<br>the plan row records 581 bytes, 161 instructions, 15 calls, and 1 FPU instruction; the merged record keeps the 584-byte extent; DOS/4G 2.01a under DOSBox-X leaves the initialized data pages zero, so the harness build defines `LIST_FILE_LOAD_HARNESS` and the module builds the constant text in local buffers, while the default build keeps the literals of the original | observed |
| `0x15720` | `palette_build` | fills a 50-entry table with floating-point ramp values<br>the body at `0x15720` holds 55 instructions and 152 bytes: the frame, the outer loop over 50 rows, the inner loop over 100 columns, the row base `0x80e8 + 400 * i`, the ramp value `j * 0.005f + 0.5`, the exponent `50.0 / (i + 1)`, the call at `0xc21e2`, the float store, and the epilogue. The `fadd qword ptr [0x66e]` reads the double 0.5 and the `fdivr qword ptr [0x676]` reads the double 50.0. The call at `0xc21e2` reaches the power routine at `0xc20a6`, which uses `fyl2x` and the `f2xm1` loop at `0xc21bf`; the rebuild calls `pow` and keeps the identity open because the shared toolchain differs | derived |
| `0x19ae8` | `player_view_setup` | the original at 0x19ae8 holds 223 instructions and 869 bytes up to the ret at 0x19e4c, so the record keeps the 912-byte extent and sets no function size<br>the player update at 0x19ab0 calls this function as its first step on DAT_00110a20, and the car camera at 0x1a930 calls it at 0x1ae8b on the view block at +0x4f8 of the 0x590-byte car records at DAT_00110ab8<br>the function selects a slot from the seven 0x14-byte slots at DAT_00101da4 by the kind flag byte DAT_000f003d, the input bytes DAT_00118e1c and DAT_00118e1d, and the chosen flag, sets the screen mode through 0x4c850, starts the renderer at 0x19e78 or the car camera at 0x1a930, and copies the car position and angle into the view, so the module names the function player_view_setup | inferred |
| `0x1a930` | `car_camera` | the original at 0x1a930 holds 396 instructions and 1425 bytes up to the ret at 0x1aec0, so the record keeps the 1428-byte extent and sets no function size<br>the only direct callers are player_view_setup at 0x19ae8, which calls this function at 0x19c76 and 0x19e3f, and the camera group at 0x1aec4, which calls it at 0x1aefc on every 0x590-byte car record at DAT_00110ab8 and then copies the selected view block at +0x4f8; the function moves the view along the route toward the nearest node, so the module names it car_camera<br>the function walks the 0x38-byte route records below the pointer DAT_0010f90c, with the count at +0x1c and the node position at +0x04 (x), +0x08 (z), and +0x0c (y); it picks the nearest nonzero horizontal distance, forms the yaw at +0x52c and the pitch at +0x528 with the constants 2pi at DAT_0010092c, -pi/2 at DAT_00100930, and -1500.0 at DAT_00100934, and sets the view flag at +0x568 from the absolute height modulo 10 and the saved distance at +0x55c<br>the calls are asin at 0xc5e42, sqrt at 0x22bec, sin at 0x4da72, cos at 0x4da66, player_view_setup at 0x19ae8, and the render path at 0x19f00 | inferred |
| `0x1b304` | `controls_load` | the original at `0x1b304` holds 40 instructions and 144 bytes: it builds `%s\config\controls.cfg` with the base path at `DAT_00007edc` through `sprintf` at `0x4d32c`, opens the path with the mode `rb` through `fopen` at `0xc5fe5`, reads six 28-byte records into `0x197d8`, forty 28-byte records into `0x192a4`, and ten 4-byte records into `0x19704` through `fread` at `0xc607b`, closes the handle through `0xc6245`, and has no open-failure test<br>the primary caller is `engine_init` at `0x127d4`; the other caller at `0x1fe17` runs when `DAT_0018e30` is non-zero; the plan row records 144 bytes, 40 instructions, 6 calls, and 0 FPU | observed |
| `0x1bb58` | `setup_load` | the original at `0x1bb58` holds 35 instructions and 111 bytes: it builds `%s\config\setup.cfg` with the base path at `DAT_00007edc` through `sprintf` at `0x4d32c`, opens the path with the mode `rb` through `fopen` at `0xc5fe5`, reports `Run SETUP before.` through `0xc656e` and exits with -1 on failure, reads `0x90` bytes into the buffer at `DAT_00008058` through `fread` at `0xc607b`, and closes the handle through `0xc6245` | observed |
| `0x1bf38` | `config_load` | the original at `0x1bf38` holds 46 instructions and 155 bytes: it opens `screamer.cfg` with the mode `rt` through `fopen` at `0xc5fe5`, reports `Files damaged. Re-install Screamer.` through `0xc656e` and exits with -1 on failure, reads `%s%s%s` into `DAT_00007edc`, `DAT_00007f10`, and `DAT_00007f44` and two `%c` values through `fscanf` at `0xc6618`, sets `DAT_00007f78` to 1 when the second character is `n`, and closes the handle through `0xc6245` | observed |
| `0x1db94` | `track_progress_update` | the original at 0x1db94 holds 259 instructions and 926 bytes up to the ret at 0x1df31, so the record keeps the 928-byte extent and sets no function size<br>the only caller is object_prepare at 0x1c79c, which calls this function first at 0x1c7a8 on each object, so the module takes the section object_prepare callees<br>the function reads the 0x20-byte node table at global 0x7250 with the count at 0x7ed0; 0x150f4 loads that table from the level list and race_start builds the MAP%d name into 0x1d294, so the table holds the track nodes<br>the function advances the current node at +0x320 and the target node at +0x3f0, copies the node value and data to +0x3f4 and +0x3fc, marks the visited chain at +0x324 and +0x328, counts a lap in +0x40c when every node is marked, and stores the segment ratio at +0x404 and 100 * laps + index + ratio at +0x408, so the module names the function track_progress_update | inferred |
| `0x1f1a0` | `mouse_init` | the original at 0x1f1a0 holds 67 instructions and 233 bytes: it sets the timer rate at 0xff00 through 0xc8245, installs the 0x64-rate timer with the code handler CS:0x20d and its id at DAT_00018d8c through 0xc835b, returns when DAT_00008058 is zero, starts the mouse system through 0xc8f81, stores 0x2b11 at DAT_00001e50, installs the mouse handler through 0xc903a with the handler DAT_00008060, the state DAT_00001e40, and the id DAT_00018d94, installs the 0x1e-rate timer from the far pointer DAT_00001e5c through 0xc835b with its id at DAT_00018d90, and shuts the started parts down through 0xc8321, 0xc8ffa, and 0xc98f9 before it exits through 0x300be | observed |
| `0x25db0` | `dpmi_real_int` | sets the real-mode state, runs the DOS multiplexer call INT 2Fh AX=1500h, and drives the real-mode interrupt path | derived |
| `0x262a8` | `sound_driver_set` | builds the HMI request block at `DAT_001166a4` from the argument and the state words, and calls the sound driver when the driver state is set | inferred |
| `0x4c8f0` | `video_buf_load` | loads the `VIDEO_BUF` resource through `resource_get`, stores the pointer in `DAT_00021504` and `DAT_00121528`, and releases the stream on failure | observed |
| `0x8bdec` | `vesa_set_mode` | allocates a DOS block, builds the real-mode register set for the VBE mode info call, and sets the mode through INT 10h | derived |
<!-- end generated: Subsystem initialisation -->

The init order is: the configuration, the list files, the setup, the controls,
the mouse, the keyboard, the palette, the video buffer, the VESA mode, the sound
banks, and the engine interface.

## Engine strings

The decompiler shows the engine interface.  The strings sit in object 3:

| String | Meaning |
| --- | --- |
| `%s.3dx` | the 3D object file name |
| `3D_HEADER`, `3D_BLKOBJ`, `GOBJ_LIST`, `OBJ_LIST` | the 3D object chunk names |
| `OSTACLES`, `TRACK_DATA`, `OPPONENTS`, `ANIMATIONS`, `CARS_DATA` | the track resources |
| `Cannot create sprite: object not loaded.` | the sprite interface |
| `Object not present in visual-list, cannot get out it.` | the visual list interface |
| `%s\DATA\MUSIC.DAT`, `%s\gfx\%s%c.dat`, `%s\gfx\graf.dat`, `%s\gfx\game.dat` | the data files |
| `%s.smg`, `attr%c.fnt`, `logo%c.spr`, `game%c.fnt`, `game%c.spr` | the asset files |
| `MELODIC_BANK`, `DRUM_BANK` | the HMI sound banks |

Evidence: `r2 -q -c "iz"` in the Nix dev shell and the Ghidra decompilation.

## Method

Follow the call path with the `le.py` subcommands:

```sh
python3 tools/le.py calls STARTH.EXE 0x300f8       # entry body and its calls
python3 tools/le.py tree  STARTH.EXE 0x300f8       # call tree
python3 tools/le.py funcs STARTH.EXE                # candidate function starts
python3 tools/le.py near  STARTH.EXE 0x10020        # function below an address
```

Confirm a run-time address in DOSBox-X:

```sh
python3 tools/dosboxx_control.py script \
  --file tools/scripts/break_at_main.dbg \
  --port 9876 --cwd work/entry/run --timeout 180 \
  --launch "dosbox-x -nogui -break-start -conf ../probe.conf -conf game/dosboxScreamer_single.conf"
```

## Limits

- The helper follows a linear sweep.  It stops at the first `ret`, `retf`,
  `iret`, `hlt`, or `int3`.  A function that ends with a tail call has no
  terminator, so the sweep continues into the next function.
- The candidate function starts come from the Watcom prologue `push ebp; mov
  ebp, esp`.  A function without that prologue is missing from the list.
- A call through a register or a memory operand has no static target.
- The run-time addresses depend on the module load base.  The base is `0x1c2000`
  in the tested configuration.
