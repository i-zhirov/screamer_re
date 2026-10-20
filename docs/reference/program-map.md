# Program map

This page records the recovered program structure.  It grows with the
investigation.

## Startup path

The path from the DOS stub to the program `main`:

| Step | Module address | Object offset | Run-time address | Evidence |
| --- | ---: | ---: | --- | --- |
| entry jump | `0x300f8` | `0x200f8` | `0150:1f20f8` | `jmp short +0x76` over the banner string |
| startup | `0x30170` | `0x20170` | `0150:1f2170` | `sti`, stack setup, DOS version check |
| main invoker | `0xd4324` | `0xc4324` | `0150:296324` | loads `argc` and `argv`, calls `main` |
| program `main` | `0x10020` | `0x20` | `0150:1d2020` | 56 instructions, 15 call sites |

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

| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x10020` | `main` | called by the Watcom startup at `0xd4364` with `argc` in EAX and `argv` in EDX | derived |
| `0x2ecf8` | `list_create` | allocates `count` records of `0x1c` bytes, stores the pointer, the cursor, and the count, and zeroes the block | derived |
| `0x2ed77` | `list_destroy` | releases the block when the pointer is set; counterpart of `list_create` | observed |
| `0x2f8ca` | `memory_available` | probes the heap, frees the probe blocks, and returns the total; `main` compares it with `6400000` | derived |
| `0x127a0` | `engine_init` | calls 17 setup functions, loads `MENU` from `%s\DATA\MUSIC.DAT`, and reads the video mode | derived |
| `0x128ac` | `engine_shutdown` | counterpart of `engine_init`: stops the music with `FUN_00014e20(0)` and tears the subsystems down | derived |
| `0x27700` | `race_loop` | starts a race through `FUN_00027770` and loops until the session state `DAT_0011ce44` reaches 4 | derived |
| `0x27f94` | `frame_update` | clears the key state, reads the clock, advances a 6-frame counter, waits for the frame flag `DAT_0011d234`, and handles input | derived |
| `0x108e4` | `bios_video` | builds a record with the argument and calls interrupt 10h through the run-time | inferred |
| `0x10978` | `error_report` | dispatches on the code through a jump table to one message stub per code | observed |
| `0x30050` | `memory_alloc` | allocates the size argument and retries through the out-of-memory handler pointers `DAT_001274ac` and `DAT_001274a8` | derived |
| `0x3009b` | `printf` | passes the argument list and the output table `DAT_00102fd2` to the run-time print engine | inferred |
| `0x300be` | `exit` | calls the run-time pair `PTR_FUN_00102f84` and `PTR_FUN_00102f88`; `main` calls it with the error code | inferred |
| `0x300ec` | `free` | walks the heap block list `DAT_001031d0` and releases the block | inferred |

`main` calls the run-time helpers because the Watcom run-time is linked into the
same object.  The four run-time helpers sit at the end of the object, after the
program code.

## Race loop and frame update

`main` alternates `race_loop` at `0x27700` and `frame_update` at `0x27f94`
until the quit flag `DAT_00107ed8` is 1.

### race_loop callees

| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x27770` | `race_start` | sets the 640x400 screen, copies the state globals, loads the resources, and calls `FUN_000128e0(1)` | derived |
| `0x21e6c` | `motion_update` | scales the object position with the time base, calls `FUN_0001f94c`, and advances the time base with a wrap at 1.0 | derived |
| `0x10193` | `default_data_copy` | copies 32 dwords from `DAT_00010113` in object 1 into `DAT_000f0002` in object 2 | observed |
| `0x24da0` | `joystick_update` | walks the 28-byte control records at `DAT_001197d8` with floating-point scaling | derived |
| `0x26448` | `input_poll` | counts to 8 and reads the mouse and keyboard state; the shared input poll | observed |
| `0x1fc38` | `frame_timer` | waits for the frame counter, computes the frame delta, and clamps it between 0.2 and 0.02 | derived |
| `0x211e0` | `input_map` | reads the control bits `DAT_00118e1c` and dispatches the game action | inferred |
| `0x2786c` | `object_update_all` | walks `DAT_0010f8ec` records of 0x164 bytes at `DAT_00110ab8` and runs four steps per record | derived |
| `0x2aa98` | `video_fill` | fills 480 by 400 | inferred |
| `0x279a4` | `race_end` | tears the race down through 13 calls | derived |

### frame_update callees

| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x2f8ca` | `memory_available` | the memory probe, named earlier | derived |
| `0x29d08` | `net_space_init` | loads the `NET_SPACE` resource and sets the 640x400 screen | observed |
| `0x29f80` | `menu_frame` | loads `gfx\graf.dat` entry `menu1h.pcx`, blits it, and reads the menu input bits | observed |
| `0x2aa84` | `frame_step` | calls `default_data_copy`, `frame_timer`, and `joystick_update` | derived |
| `0x2807c` | `race_frame` | blits the video buffer, reads the input bits, and dispatches on the game state `DAT_001203b8` | derived |
| `0x26448` | `input_poll` | the shared input poll | observed |
| `0x2ff14` | `music_play` | processes a music request record and sets a state from 0 to 6 | derived |
| `0x2aa98` | `video_fill` | fills 480 by 400 | inferred |
| `0x8c160` | `memset` | fills `count >> 2` dwords with one value | observed |
| `0x2a1b0` | `race_cleanup` | tears the race down and releases the `OPT_PCX` resource | observed |

`frame_update` clears the screen with `memset(DAT_00121528, 0, 0x3e800)`.
The count `0x3e800` is 256000, which is 640 times 400: the screen buffer is
640 by 400 bytes.  Evidence: the decompiled `FUN_00027f94` and `FUN_0008c160`.

`input_poll` is shared by both loops.  `default_data_copy` and
`motion_update` are shared by `race_loop` and `frame_step`.

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

| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x10bfc` | `object_load` | reads `3D_HEADER` and `3D_BLKOBJ`; the flag at `+0x14` selects the records | observed |
| `0x4d32c` | `sprintf` | formats into a buffer through the run-time `vsprintf` and writes the terminator | derived |
| `0x4c08d` | `fopen` | opens the path through the run-time file layer and fills the stream record; returns `-1` on failure | derived |
| `0x4c7e0` | `stream_status` | returns the word at `stream + 0x70` | inferred |
| `0x2eed0` | `resource_get` | looks a resource up by name in the engine resource list and loads it from its file when missing | observed |
| `0x4c4e5` | `stream_read` | buffered read; advances the position and clamps it to the stream size | derived |
| `0x4c38d` | `stream_close` | frees the buffers and closes the handle; the counterpart of `fopen` | derived |
| `0x4cc10` | `object_init_from_file` | bounds the file index, initializes the object record, and copies the chunk table from the file record | observed |

The chunk table in the object record holds eight words per chunk: the file
record, the chunk count, and six words of the chunk header.  Evidence: the
decompiled `FUN_0004cc10`.

The loader returns codes that the error reporter at `0x10978` prints.  The
reporter dispatches through a jump table to one message stub per code, and the
stubs print the `3dx` messages.  Evidence: the decompiled `FUN_00010978` and
the Ghidra references to `0x100074`, `0x1000a3`, `0x1000d1`, and `0x1000ff`.

## Subsystem initialisation

`engine_init` at `0x127a0` runs the subsystem setup in order.  The callees:

| Module address | Name | Evidence | Confidence |
| --- | --- | --- | --- |
| `0x1bf38` | `config_load` | reads `screamer.cfg` and sets a flag from a character | observed |
| `0x15460` | `list_file_load` | builds a file name, opens it, and parses records until `END_LIST` | observed |
| `0x1bb58` | `setup_load` | reads `CONFIG/SETUP.CFG` (144 bytes) and reports `Run SETUP before.` on failure | observed |
| `0x1b304` | `controls_load` | reads three 28-byte records from a `cfg` file | derived |
| `0x1f1a0` | `mouse_init` | calls interrupt 33h and delays | observed |
| `0x25db0` | `dpmi_real_int` | calls the DOS/4GW real-mode interrupt dispatcher with `0x1500` | derived |
| `0x262a8` | `sound_driver_set` | fills an HMI request block and calls the sound driver | inferred |
| `0x15720` | `palette_build` | fills a 50-entry table with floating-point ramp values | derived |
| `0x14e20` | `keyboard_hook` | hooks interrupt 9 and drives the keyboard LEDs | observed |
| `0x10810` | `engine_interface_init` | allocates the engine object and creates its list | derived |
| `0x4c810` | `stream_layer_init` | binds the engine pointer to the stream layer | derived |
| `0x4c8f0` | `video_buf_load` | loads the `VIDEO_BUF` resource | observed |
| `0x8bdec` | `vesa_set_mode` | calls the real-mode interrupt dispatcher with `0x4f01`, the VBE mode info call | derived |

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
