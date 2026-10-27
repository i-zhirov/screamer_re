# Roadmap

## Goal

The goal is rebuildable source code for Screamer.  The first target is the
original DOS platform.  Later targets are modern platforms, starting with the
current host.

## Constraints

- The original build uses Watcom C/C++ 32-bit with the DOS/4GW extender.
- The project can rebuild the program with OpenWatcom V2.  OpenWatcom V2
  builds on macOS and targets 32-bit DOS.
- The project can run and test the original and the rebuild in DOSBox.
- The shipped artifacts are the reference.  Every format claim needs evidence
  from those artifacts.

## Milestones

### M0: Reconnaissance (done)

- Inventory the distribution and the installed game.
- Parse the LE headers and the object tables.
- Confirm the toolchain, the runtime, and the extender.
- Record the open questions.

### M1: Program map

- Resolve the entry alignment with a DOSBox debugger session (done).
- Find `main` and the subsystem entry points (done: `main` is at object
  offset `0x10020`).
- Map the code object into functions with names and roles (in progress:
  `main` and its 13 callees carry names).
- Map the data references: video, sound, input, file I/O, network.
- Record the call graph for the main loop and the menu system.

### M2: Format decoders

- Decode the graphics format (`GFX/GRAF.DAT`, `GFX/MENU.DAT`, `GFX/GAME.DAT`)
  (done: the container, the LZW members, the sprites, the fonts, and the PCX
  images have decoders, and the review confirms the exported assets).
- Decode the map and index formats (`DATA/MAP*`, `DATA/GEN.DAT`,
  `DATA/LOCAL.DAT`, `TABS/*`) (in progress: the `DATA` files are LZW text with
  the track and the car lists, and the loaders are known).
- Decode the HMI `ADLIB-` music bank.
- Decode the sample bank (`DATA/SOUND.DAT`).
- Verify each decoder against the original data.

### M3: Rebuild

- Rebuild the program in C or C++ for 32-bit DOS with OpenWatcom V2 (in
  progress: ten named modules are rebuilt with the harness and the store in
  `tools/rebuild`: `free` at `0x300ec`, `exit` at `0x300be` with its helper at
  `0x300d6`, `printf` at `0x3009b`, `memcpy` at `0x8c150`, and
  `engine_shutdown` at `0x128ac`, `error_report` at `0x10978`, `bios_video` at
  `0x108e4`, `list_destroy` at `0x2ed77`, `memory_available` at `0x2f8ca`, and
  `frame_step` at `0x2aa84`; the four run-time modules match the original, the
  six game modules hold the recorded difference, and the driver module matches
  by behavior).
- Rebuild the data pipeline that produces the shipped data files.
- Compare the rebuild with the original: same files, same behavior (in progress:
  `tools/rebuild/diff_module.py` compares the rebuilt code with the original
  code).

### M4: Modern platform

- Port the rebuild to a modern platform.  Start with macOS on arm64.
- Replace the DOS/4GW and DOS services with host libraries.
- Replace the CD audio path with a modern audio path.

### M5: Verification

- Run differential tests between the original and the rebuild.
- Record every test with its evidence and its limits.

## Next steps

1. Follow the calls from `main` into the game subsystems (done for the first
   level: `engine_init`, `race_loop`, and `frame_update` carry names).
2. Decode the compression of `GAME.DAT` and `MENU.DAT` (done: the container
   and the LZW members have decoders).
3. Decode the 3D object format.  The loader is `object_load` at `0x10bfc`, and
   the chunk names are `3D_HEADER`, `3D_BLKOBJ`, `GOBJ_LIST`, and `OBJ_LIST`
   (in progress: the chunk layout is decoded).
4. Decode the graphics format (done: `GFX/MENU.DAT`, `GFX/GRAF.DAT`, and
   `GFX/GAME.DAT`).
5. Decode the map and index formats: `DATA/MAP*.DAT`, `DATA/MAP*I.DAT`,
   `DATA/GEN.DAT`, `DATA/LOCAL.DAT`, and `TABS/*.TAB` (in progress: the
   `DATA` files expand to text lists, the loaders are known, and the two `TABS`
   loaders carry a caller chain from `engine_init` with the run-time order
   `PERSPTAB` then `VIS.TAB`).
6. Decode the sound formats: `DATA/SOUND.DAT` and the HMI `ADLIB-` music bank
   (in progress: `SOUND.DAT` is a member container of 92 `<name>.RAW` samples
   with a loader; `MUSIC.DAT` is an archive of the two `ADLIB-` banks and the
   seven `MUSICn.HMP` songs, and the bank header, the instrument table, and the
   instrument data are mapped).
7. Set up the OpenWatcom V2 build harness under `work/` (done: the harness is
   at `tools/rebuild`, and the first module rebuild and diff run).
8. Rebuild the next modules from the program map in size order (done: `free` at
   `0x300ec` and `exit` at `0x300be` with its helper at `0x300d6`).
9. Answer the frame question of the simple game functions (done: the original
   game files passed the Watcom stack-frame option `-of+` and the run-time files
   did not, and OpenWatcom V2 adds `mov esp,ebp` to the epilogue of a function
   that does not change ESP).
10. Rebuild `engine_shutdown` at `0x128ac` with `-of+` (done: the frame and the
    eight calls match, and the rebuild holds one extra `mov esp,ebp` restore that no
    option removes, so the module stays open as a byte-level match).
11. Rebuild the self-contained `memcpy` at `0x8c150` (done: the module matches the
    original byte for byte, the first exact match; the behavior run needs more work).
12. Rebuild the game modules in size order (in progress: the ten modules
    under `tools/rebuild/modules` carry a recipe and a result, and `frame_step`
    at `0x2aa84` is the latest: 7 instructions and 20 bytes, the call order
    `default_data_copy`, `joystick_update`, `frame_timer`, and the same extra
    `mov esp,ebp` restore as `engine_shutdown`).
