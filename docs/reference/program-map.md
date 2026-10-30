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
| `0x10020` | `main` | called by the Watcom startup at `0xd4364` with `argc` in EAX and `argv` in EDX | derived |
| `0x108e4` | `bios_video` | builds a record with the argument and calls interrupt 10h through the run-time | inferred |
| `0x10978` | `error_report` | dispatches on the code through a jump table to one message stub per code | observed |
| `0x127a0` | `engine_init` | calls 17 setup functions, loads `MENU` from `%s\DATA\MUSIC.DAT`, and reads the video mode | derived |
| `0x128ac` | `engine_shutdown` | counterpart of `engine_init`: stops the music with `FUN_00014e20(0)` and tears the subsystems down | derived |
| `0x27700` | `race_loop` | the original at `0x27700` holds 34 instructions and 110 bytes: it calls `race_start` at `0x27770`, returns a non-zero start result, and otherwise runs the loop body `motion_update`, `default_data_copy`, `joystick_update`, the gated `input_poll`, `frame_timer`, `input_map`, `object_update_all`, and `video_fill` until `DAT_0011ce44` reaches 4, then calls `race_end` at `0x279a4` and returns 0 | derived |
| `0x27f94` | `frame_update` | clears the key state, reads the clock, advances a 6-frame counter, waits for the frame flag `DAT_0011d234`, and handles input | derived |
| `0x2ecf8` | `list_create` | allocates `count` records of `0x1c` bytes, stores the pointer, the cursor, and the count, and zeroes the block | derived |
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
| `0x1fc38` | `frame_timer` | waits for the frame counter, computes the frame delta, and clamps it between 0.2 and 0.02 | derived |
| `0x211e0` | `input_map` | reads the control bits `DAT_00118e1c` and dispatches the game action | inferred |
| `0x21e6c` | `motion_update` | scales the object position with the time base, calls `FUN_0001f94c`, and advances the time base with a wrap at 1.0 | derived |
| `0x24da0` | `joystick_update` | walks the 28-byte control records at `DAT_001197d8` with floating-point scaling | derived |
| `0x26448` | `input_poll` | the original at `0x26448` holds 27 instructions and 108 bytes up to the first return, plus the early-exit block at `0x264b4`: it tests the flags at `DAT_0016669e`, `DAT_00166684`, and `DAT_00166688`, counts `DAT_001666bc` to 8, reads through `0x260e0`, and applies the input through `0x263cc` when device bit 9 is clear and `DAT_001666ac` is set; it returns 0 on every early exit and `DAT_00166696` on the normal path | observed |
| `0x27770` | `race_start` | sets the 640x400 screen, copies the state globals, loads the resources, and calls `FUN_000128e0(1)` | derived |
| `0x2786c` | `object_update_all` | walks `DAT_0010f8ec` records of 0x164 bytes at `DAT_00110ab8` and runs four steps per record | derived |
| `0x279a4` | `race_end` | tears the race down through 13 calls | derived |
| `0x2aa98` | `video_fill` | calls the video copy at 0x8beb8 with 480, 400, and the video buffer pointer | derived |
<!-- end generated: race_loop callees -->

### frame_update callees

<!-- begin generated: frame_update callees -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x2807c` | `race_frame` | blits the video buffer, reads the input bits, and dispatches on the game state `DAT_001203b8` | derived |
| `0x29d08` | `menu_space_init` | loads the `NET_SPACE` resource, builds the menu graphics and the menu model, sets the 640x400 screen, and starts the menu music | observed |
| `0x29f80` | `menu_frame` | loads `gfx\graf.dat` entry `menu1h.pcx`, blits it, and reads the menu input bits | observed |
| `0x2a1b0` | `race_cleanup` | the original at `0x2a1b0` holds 24 instructions and 97 bytes: the frame, the twelve cleanup calls at `0x264ec`, `0x1be6c`, `0x25640`, `0x258b8`, `0x115f8`, `0x11304`, `0x11a08`, `0x4cecc`, `0x1b58c`, `0x2f326`, `0x1f6f8`, and `0x2fe68`, and the epilogue; the two argument call pairs receive the imported data addresses and the last two calls receive the pointer at `DAT_000062ac` and the address `DAT_000062b0` | observed |
| `0x2aa84` | `frame_step` | calls `default_data_copy`, `joystick_update`, and `frame_timer` | derived |
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

`object_update_all` at `0x2786c` walks `DAT_0010f8ec` records of 0x164 bytes at
`DAT_00110ab8` and runs four passes over every record.

<!-- begin generated: object_update_all callees -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x19ab0` | `player_update` | runs the five steps at `0x19ae8`, `0x19e78`, `0x19f00`, `0x1a144`, and `0x212d0` on the player structure `DAT_00110a20` | inferred |
| `0x1c760` | `object_player_input` | runs the proximity check, the drive step gated on the state at `+0x2f8` and the input bits `DAT_00118e1c`, and the revolution step | derived |
| `0x1c79c` | `object_prepare` | runs the preparation steps, gated on the state at `+0x414` | derived |
| `0x1c810` | `object_physics` | runs the physics and collision steps, gated on the state at `+0x414` | derived |
| `0x1c84c` | `object_car_link` | indexes the 28-byte record table `DAT_0010653c` and `DAT_00106540` with `+0x4dc` and writes the position | derived |
| `0x1cd44` | `object_transform` | writes the world position at `+0x498`, `+0x49c`, and `+0x4a0` and the float fields at `+0x4a4` to `+0x4ac` from the position and the velocity | derived |
| `0x205c4` | `ai_update` | float computation through `0x11c00` and `0x2218c` | inferred |
| `0x20954` | `opponent_update` | walks `DAT_0010f178` records of 0xbc bytes at `DAT_0010f180` | inferred |
| `0x27a34` | `hud_update` | gated on the session state `DAT_0011ce44`: the overlays 0 and 1 run when the state is not 3, and the overlay 2 runs when bit 1 is set | inferred |
| `0x27f44` | `race_timeout` | computes the frame delta between `DAT_000107fd` and `DAT_0011d228`, and sets the timeout flag `DAT_00101e04` when the delta is greater than 1000 | derived |
| `0x81ab0` | `sound_effects_update` | a sound library update in the `0x8xxxx` region | inferred |
<!-- end generated: object_update_all callees -->

### motion_update callees

<!-- begin generated: motion_update callees -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x1f94c` | `view_move` | calls the view helper with the second argument, the position setter with the constant `0x62b0` and the shifted third argument, and the position setter with the doubled and shifted first argument | derived |
| `0x27ec8` | `video_present` | called after every draw step | inferred |
| `0x4d9f4` | `fp_helper` | rounds the value on the FPU stack to an integer | observed |
<!-- end generated: motion_update callees -->

### race_frame callees

`race_frame` at `0x2807c` runs every frame of a race.

<!-- begin generated: race_frame callees -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x1075f` | `video_block_swap_a` | byte-swaps and swaps dwords between the front of the video buffer and the region `0x9f` dwords ahead, 80 passes of 400 dwords | derived |
| `0x107a1` | `video_block_swap_b` | swaps 200 rows of 160 dwords between the front of the video buffer and the region `0xf960` dwords ahead | derived |
| `0x28160` | `overlay_dispatch` | reads the option flags in `DAT_000f0002` and calls the overlay for the set flag | derived |
| `0x29564` | `race_state_update` | switches on `DAT_0011fac4`; case 1 advances `DAT_0002038c` by the frame timing and copies `DAT_0011facc` into `DAT_0011fac4` and `DAT_000203b8` | derived |
| `0x29610` | `race_heading_update` | angle arithmetic on `_DAT_00120388` and `_DAT_00120394` with a 0x7ff wrap | inferred |
| `0x29818` | `fade_update` | ramps `DAT_0011d224` toward 1.0 or down to 0.0 by the frame delta and calls `video_present` | derived |
| `0x2dfac` | `panel_show` | stores the doubled position pair and the size, sets the visible flag, clears the count, and updates the panel | inferred |
| `0x2e0b4` | `race_panel_0` | the overlay for the race state 0 | inferred |
| `0x2e21c` | `race_panel_2` | the overlay for the race state 2 | inferred |
| `0x2e58c` | `race_panel_3` | the overlay for the race state 3 | inferred |
| `0x2e724` | `race_panel_4` | the overlay for the race state 4 | inferred |
| `0x2e7cc` | `panel_draw_centered` | the original at `0x2e7cc` holds 34 instructions and 93 bytes: it measures the text through `0x254a8`, halves the measured width with a signed division, centers the half width on the `0x140` screen width, and draws through `0x25020` with the doubled position | derived |
| `0x81a70` | `sound_update` | a sound library update with the driver block `DAT_00106350` | inferred |
| `0x8ac78` | `camera_update` | sets the position vector at `+0x3c`, `+0x40`, and `+0x44` and clears the velocity at `+0x6c`, `+0x70`, and `+0x74` | inferred |
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
| `0x1d6bc` | `object_integrate` | integrates the position at `+0x454` and `+0x45c` from the float velocity at `+0x460` and `+0x468` and the factor at `+0x450` with the frame delta `DAT_00118e04`, damps the factor with `DAT_0010cf08 + 0x13c`, and clamps it at 0 | derived |
<!-- end generated: object_transform callees -->

### object_physics callees

<!-- begin generated: object_physics callees -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x1d834` | `object_gear_step` | switches on the state at `+0x2f8`; case 2 sets the float fields `+0x2fc`, `+0x300`, `+0x304`, `+0x308`, and `+0x30c` to 3000.0 and 12.0 and moves to case 1; case 1 integrates the position `+0x49c` and the three velocities with the damping `DAT_0010cf08 + 0x184` | derived |
| `0x1db38` | `object_gear_state` | sets `+0x4e8` to 0, `0xff00`, or `0x8000` from the value at `+0x3f4` | derived |
| `0x1df34` | `object_slide_step` | computes the slide from `+0x418` and `+0x2f4`, clamps it between -2.0 and 2.0, and calls `0x22b80` for the parts at `+0x4c8`, `+0x4b8`, and `+0x4b0` | derived |
| `0x1e00c` | `object_collision` | calls `0xc667d` and `0xc6920` in object 3 plus `0x1f718`, `0x1f7e4`, `0x1f880`, `0x2218c`, and `0x22bec` | inferred |
<!-- end generated: object_physics callees -->

### overlay_dispatch callees

<!-- begin generated: overlay_dispatch callees -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x1f718` | `table_lookup` | returns the table entry for the index when the table flag is set, and `-1` otherwise | inferred |
| `0x289bc` | `overlay_queue_update` | walks `DAT_00102108`, moves the entries into `DAT_0012082c` and `DAT_00120830` with the index `DAT_0012087c` bounded at 4 | inferred |
| `0x28b5c` | `menu_enter` | saves `DAT_0011fb58`, clears the fade `DAT_0011d224`, sets `DAT_0011fb64` to -1, calls `menu_frame`, and restores the saved value | derived |
| `0x28ba4` | `net_race_screen` | the state 2 overlay: calls `0x2a364` with the 0x2c-byte record table `DAT_001022c4` indexed by `DAT_001203dc`, then `menu_space_init`, `0x2e82c`, and `0x29bac` | inferred |
| `0x294c8` | `race_state_begin` | sets the constants 2.0, 0.6, 1.0, 20000, and -12.566, sets `DAT_0011fac8` to -1, copies `DAT_0011fac4` into `DAT_001203b8`, stores the parameter at `DAT_0011facc`, and sets `DAT_0011fac4` to 1 | derived |
| `0x2dddc` | `net_player_add` | takes a 0x38-byte record from `0xc6fbc` and copies the name `DAT_001203cc` into it | inferred |
<!-- end generated: overlay_dispatch callees -->

## The session start, the replay records, and the graphics decoders

### race_session_start

`race_session_start` at `0x128e0` sets the flags `_DAT_00118e1c = 0x102`, the
signature `DAT_0011521c = 0x1234`, and `_DAT_0011cb14 = 1`, clears the state
globals, and then dispatches on the game mode `DAT_0011d260`.

| Mode | Handler | Name | Confidence |
| --- | --- | --- | --- |
| 1, 2, 3 | `0x12c0c` | `game_mode_1_start` | inferred |
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
| `0x22e94` | `replay_read` | reads the 20-byte record at `DAT_00118e5c + index * 0x14` and returns 1 at the end markers | derived |
| `0x22f54` | `replay_write` | writes the 20-byte record and advances the index with a wrap | derived |
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
| `0x1b5a4` | `pcx_load` | seeks 0x300 bytes before the end, reads the palette, shifts every palette byte right by 2, reads the 128-byte PCX header, and decodes the RLE runs | observed |
| `0x2a364` | `loading_screen` | clears the screen, sets the palette, builds `load%d%c.pcx` with the letter H, and loads it from `gfx\graf.dat` | derived |
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
| `0x4cf94` | `persptab_load` | opens `TABS\persp01.tab` with the mode `rb`, reads `fread(buffer, 1, 0x40740, stream)` into the `PERSPTAB` cell, and advances the cell by `0x40000` | derived |
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
| `0x1f2e0` | `sound_bank_load` | builds `%s\data\sound.dat`, walks the 36-byte sample records at `DAT_0015224`, builds `%s.RAW` for each name, and opens the member through the member open `0x4c08d`; a missing member calls the exit | derived |
| `0x2f9b0` | `music_bank_reset` | clears the fields `+0x04`, `+0x10`, and `+0x14` of the music bank struct | derived |
| `0x2f9e0` | `music_bank_load` | builds `%s\DATA\MUSIC.DAT`, copies the name, and opens the member `melodic.bnk` (or `drum.bnk`) through the member open `0x4c08d` | derived |
| `0x2ffc4` | `music_start` | starts the music from the bank | derived |
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
| `0x1cee0` | `object_drive_step` | computes the drive force from `+0x2e4`, `+0x2f0`, and the throttle `+0x3fc`; sets the brake flag `+0x2c0` when the force is below the speed `+0x418` and reduces the speed by `+0x2ec` times the frame delta | derived |
| `0x1d30c` | `object_revolution_step` | accumulates `+0x250` from the speed `+0x418` and the frame delta and wraps at 1.0; reads the gear `+0x254` | derived |
| `0x1e30c` | `object_proximity_check` | measures the distance from the reference `DAT_00113ccc` and `DAT_00113cd0`; inside 35000 units it sets the gate `+0x414` and the value `+0x4e4`, outside it clears the gate | derived |
<!-- end generated: The vehicle state chain -->

`object_proximity_check` sets the gate at `+0x414`.  `object_physics` and
`object_prepare` run only when that gate is set, so the chain closes: the
proximity check enables the physics of the objects near the reference point.

### The timing and filter layer

<!-- begin generated: The timing and filter layer -->
| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x128e0` | `race_session_start` | sets `_DAT_00118e1c` to 0x102 and `DAT_0011521c` to 0x1234, clears the state globals, and calls `0x131a8` and `0x13540` | inferred |
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
| `0xd1f25` | `malloc` | rejects the size 0 and the sizes above 0xffffffd4, rounds the size up to 4 bytes, and walks the free list at `DAT_001031d0` | observed |
| `0xd2008` | `free` | the original at `0xd2008` holds 35 instructions and 94 bytes: it walks the free list from `DAT_001031d0`, stops before the next free block, releases the block through the heap routine at `0xe0a84`, keeps the largest free size in `DAT_001031d8`, and clears the heap flag at `0x27de5` | observed |
| `0xd2066` | `memmove` | copies `count` bytes from the source to the destination and returns the destination | observed |
| `0xd429f` | `dpmi_issue_int` | clears the flags at `+0xc` and `+0xd`, calls `0xddda9` and `0xd68c0` | inferred |
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
| `0x2edad` | `stream_release` | dispatches on the status at `+0xc` and calls `fprintf` at `0x4bf64` for each case | inferred |
| `0x2efc2` | `resource_read` | reads the current record and caches it through `0x2f062` | inferred |
| `0x2f0fc` | `dos_memory_alloc` | calls INT 31h AX=0x0100 with the size in 16-byte paragraphs; the segment times 16 is the linear address | derived |
| `0x2f172` | `memory_alloc` | calls INT 31h AX=0x0501 with the size in EBX:ECX and returns the block address | derived |
| `0x2f205` | `memory_free` | the original at `0x2f205` holds 33 instructions and 90 bytes: it builds the local register block with `0x0502` at offset 0 and the address words at offsets `0x10` and `0x14`, fills the segment set through `dpmi_fill_regs` at `0x4bf23`, runs the interrupt 0x31 call through `dpmi_call` at `0x4bf4b`, and returns the status at block offset `0x18` | derived |
| `0x2f25f` | `resource_find` | walks the records, matches the key at `+0xc`, and removes the match with a memmove at `0xd2066` | derived |
| `0x2f470` | `resource_free` | frees the record through `memory_free` when the flag at `+0x18` bit 0 is clear and through `0x2f40b` otherwise | derived |
| `0x2f904` | `pool_init` | queries the free memory through INT 31h AX=0x0500 and keeps the pool size at `+0x24` in 4 KB pages | inferred |
| `0x4bf23` | `dpmi_fill_regs` | fills the 12-byte real-mode segment set with es, cs, ss, ds, fs, and gs | observed |
| `0x4bf4b` | `dpmi_call` | calls the DPMI INT 31h entry at `0xd4410` with the interrupt number, the register block, the result block, and the segment set, and returns the value from the result block | inferred |
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
| `0x10bfc` | `object_load` | reads `3D_HEADER` and `3D_BLKOBJ`; the flag at `+0x14` selects the records | observed |
| `0x2eed0` | `resource_get` | looks a resource up by name in the engine resource list and loads it from its file when missing | observed |
| `0x4bf64` | `fprintf` | formats into a stream through the run-time `__fprtf` at `0xd429f` | observed |
| `0x4c08d` | `fopen` | opens the path through the run-time file layer and fills the stream record; returns `-1` on failure | derived |
| `0x4c38d` | `stream_close` | the original at `0x4c38d` holds 34 instructions and 89 bytes: it frees the buffer at `stream + 0x1c` through the run-time `free` at `0xd2008`, frees the buffer at `stream + 0xb0` through the wrapper `free` at `0x300ec` when the flag at `stream + 0x10` is set, closes the handle at `stream + 0x00` through `0xc6245`, and returns 0 | derived |
| `0x4c4e5` | `stream_read` | buffered read; advances the position and clamps it to the stream size | derived |
| `0x4c7e0` | `stream_status` | returns the word at `stream + 0x70` | inferred |
| `0x4cc10` | `object_init_from_file` | bounds the file index, initializes the object record, and copies the chunk table from the file record | observed |
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
| `0x10810` | `engine_interface_init` | allocates the engine object and creates its list | derived |
| `0x14e20` | `keyboard_hook` | hooks interrupt 9 and drives the keyboard LEDs | observed |
| `0x15460` | `list_file_load` | builds a file name, opens it, and parses records until `END_LIST` | observed |
| `0x15720` | `palette_build` | fills a 50-entry table with floating-point ramp values | derived |
| `0x1b304` | `controls_load` | reads three 28-byte records from a `cfg` file | derived |
| `0x1bb58` | `setup_load` | the original at `0x1bb58` holds 35 instructions and 111 bytes: it builds `%s\config\setup.cfg` with the base path at `DAT_00007edc` through `sprintf` at `0x4d32c`, opens the path with the mode `rb` through `fopen` at `0xc5fe5`, reports `Run SETUP before.` through `0xc656e` and exits with -1 on failure, reads `0x90` bytes into the buffer at `DAT_00008058` through `fread` at `0xc607b`, and closes the handle through `0xc6245` | observed |
| `0x1bf38` | `config_load` | reads `screamer.cfg` and sets a flag from a character | observed |
| `0x1f1a0` | `mouse_init` | calls interrupt 33h and delays | observed |
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
