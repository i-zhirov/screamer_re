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
| `0x108e4` | `sound_select` | builds a request record with the index and calls the sound runtime | inferred |
| `0x10978` | `sound_play` | dispatches on the index through the sound runtime | inferred |
| `0x30050` | `memory_alloc` | allocates the size argument and retries through the out-of-memory handler pointers `DAT_001274ac` and `DAT_001274a8` | derived |
| `0x3009b` | `printf` | passes the argument list and the output table `DAT_00102fd2` to the run-time print engine | inferred |
| `0x300be` | `exit` | calls the run-time pair `PTR_FUN_00102f84` and `PTR_FUN_00102f88`; `main` calls it with the error code | inferred |
| `0x300ec` | `free` | walks the heap block list `DAT_001031d0` and releases the block | inferred |

`main` calls the run-time helpers because the Watcom run-time is linked into the
same object.  The four run-time helpers sit at the end of the object, after the
program code.

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
