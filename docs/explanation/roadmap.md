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
- Decode the HMI `ADLIB-` music bank (done: the bank decoder reports the
  header, the instrument table, and the instrument data).
- Decode the sample bank (`DATA/SOUND.DAT`) (done: the sample decoder joins
  the `SOUND_FX` records and exports WAV files).
- Verify each decoder against the original data (done: the pipeline check holds
  the bank header, the sample flags, and the song header).

### M3: Rebuild

- Rebuild the program in C or C++ for 32-bit DOS with OpenWatcom V2 (in
  progress: one hundred nineteen named modules are rebuilt with the harness and the
  store in `tools/rebuild`: the eleven modules of the earlier batches, the
  twelve Phase A modules, the twenty-four Phase B modules from
  `music_bank_reset` at `0x2f9b0` through `main invoker` at `0xd4324`,
  including `sound_update` at `0x81a70`, and the
  first thirty-two Phase C modules `graphics_load` at `0x10ba4`, `stream_close` at
  `0x4c38d`, `memory_free` at `0x2f205`, `panel_draw_centered` at `0x2e7cc`, the
  run-time `free` at `0xd2008`, `race_cleanup` at `0x2a1b0`, `vis_tab_load` at
  `0x4d030`, `object_state_save` at `0x1ccd8`, `input_poll` at `0x26448`,
  `race_loop` at `0x27700`, `setup_load` at `0x1bb58`, `music_play` at
  `0x2ff14`, `race_state_begin` at `0x294c8`, `net_race_screen` at `0x28ba4`,
  `dos_memory_alloc` at `0x2f0fc`, `video_present` at `0x27ec8`,
  `resource_free` at `0x2f470`, `list_create` at `0x2ecf8`, `pool_init` at
  `0x2f904`, `__fprtf` at `0xd429f`, `race_session_start` at `0x128e0`,
  `music_start` at `0x2ffc4`, `race_end` at `0x279a4`, `controls_load` at
  `0x1b304`, `memory_alloc` at `0x2f172`, `overlay_dispatch` at
  `0x28160`, `palette_build` at `0x15720`, `fade_update` at `0x29818`,
  `persptab_load` at `0x4cf94`, `config_load` at `0x1bf38`, `fopen` at
  `0x4c08d`, `resource_read` at `0x2efc2`, and the first twenty-six Phase D
  modules `race_panel_4` at `0x2e724`, `engine_interface_init` at `0x10810`,
  `race_frame` at `0x2807c`, `replay_read` at `0x22e94`, `main` at
  `0x10020`, `resource_find` at `0x2f25f`, `motion_update` at `0x21e6c`,
  `frame_update` at `0x27f94`, `object_slide_step` at `0x1df34`,
  `music_bank_load` at `0x2f9e0`, `camera_update` at `0x8ac78`,
  `stream_read` at `0x4c4e5`, `malloc` at `0xd1f25`, `replay_write` at
  `0x22f54`, `object_load` at `0x10bfc`, `mouse_init` at `0x1f1a0`,
  `sound_effects_update` at `0x81ab0`, `input_map` at `0x211e0`,
  `object_proximity_check` at `0x1e30c`, `resource_get` at `0x2eed0`,
  `object_transform` at `0x1cd44`, `race_start` at `0x27770`,
  `engine_init` at `0x127a0`, `memory_error_report` at `0x2edad`,
  `object_drive_step` at `0x1cee0`, `frame_timer` at `0x1fc38`, the first
  fifteen Phase E modules `object_update_all` at `0x2786c`, `keyboard_hook` at
  `0x14e20`, `object_integrate` at `0x1d6bc`, `race_panel_0` at `0x2e0b4`,
  `net_player_add` at `0x2dddc`, `object_init_from_file` at `0x4cc10`,
  `pcx_load` at `0x1b5a4`, `race_panel_3` at `0x2e58c`,
  `overlay_queue_update` at `0x289bc`, `opponent_update` at `0x20954`,
  `loading_screen` at `0x2a364`, `race_heading_update` at `0x29610`,
  `menu_frame` at `0x29f80`, `list_file_load` at `0x15460`, and
  `object_revolution_step` at `0x1d30c`;
  twenty-two modules are verified, one is matching, and ninety-seven rebuilds
  hold the recorded difference; the
  `video_fill` module holds an assembly
  alternative that matches exactly, for the later build with the original
  Watcom 10.x compiler).
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

The ordered list of the remaining module functions is in `rebuild-plan.md`.
The plan follows the size order, starting with the smallest functions.

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
   (done: the bank header, the instrument table, and the instrument data carry a
   decoder and the rebuilt driver confirms the register order; the sample bank
   joins 87 members with the `SOUND_FX` lists and the loader flags, and the
   sample rate is 11025; the songs decode with the chunk table and the MIDI
   events, and the tool writes standard MIDI files; the five unlisted members
   stay unloaded).
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
12. Rebuild the game modules in size order (in progress: the plan in
    `rebuild-plan.md` orders the remaining functions by size; the twelve Phase
    A functions, the whole twenty-four-function Phase B batch including
    `sound_update` at `0x81a70`, and the first
    thirty-two Phase C modules `graphics_load` at `0x10ba4`, `stream_close` at
    `0x4c38d`, `memory_free` at `0x2f205`, `panel_draw_centered` at `0x2e7cc`,
    the run-time `free` at `0xd2008`, `race_cleanup` at `0x2a1b0`, `vis_tab_load`
    at `0x4d030`, `object_state_save` at `0x1ccd8`, `input_poll` at `0x26448`,
    `race_loop` at `0x27700`, `setup_load` at `0x1bb58`, `music_play` at
    `0x2ff14`, `race_state_begin` at `0x294c8`, `net_race_screen` at `0x28ba4`,
    `dos_memory_alloc` at `0x2f0fc`, `video_present` at `0x27ec8`,
    `resource_free` at `0x2f470`, `list_create` at `0x2ecf8`, `pool_init` at
    `0x2f904`, `__fprtf` at `0xd429f`, `race_session_start` at `0x128e0`,
    `music_start` at `0x2ffc4`, `race_end` at `0x279a4`, `controls_load` at
    `0x1b304`, `memory_alloc` at `0x2f172`, `overlay_dispatch` at `0x28160`,
    `palette_build` at `0x15720`, `fade_update` at `0x29818`,
    `persptab_load` at `0x4cf94`, `config_load` at `0x1bf38`, `fopen` at
    `0x4c08d`, `resource_read` at `0x2efc2`, and the first twenty-six Phase D
    modules `race_panel_4` at `0x2e724`, `engine_interface_init` at `0x10810`,
    `race_frame` at `0x2807c`, `replay_read` at `0x22e94`, `main` at
    `0x10020`, `resource_find` at `0x2f25f`, `motion_update` at `0x21e6c`,
    `frame_update` at `0x27f94`, `object_slide_step` at `0x1df34`,
    `music_bank_load` at `0x2f9e0`, `camera_update` at `0x8ac78`,
    `stream_read` at `0x4c4e5`, `malloc` at `0xd1f25`, `replay_write` at
    `0x22f54`, `object_load` at `0x10bfc`, `mouse_init` at `0x1f1a0`,
    `sound_effects_update` at `0x81ab0`, `input_map` at `0x211e0`,
    `object_proximity_check` at `0x1e30c`, `resource_get` at `0x2eed0`,
    `object_transform` at `0x1cd44`, `race_start` at `0x27770`,
    `engine_init` at `0x127a0`, `memory_error_report` at `0x2edad`,
    `object_drive_step` at `0x1cee0`, `frame_timer` at `0x1fc38`, the first
    fifteen Phase E modules `object_update_all` at `0x2786c`, `keyboard_hook` at
    `0x14e20`, `object_integrate` at `0x1d6bc`, `race_panel_0` at
    `0x2e0b4`, `net_player_add` at `0x2dddc`, `object_init_from_file` at
    `0x4cc10`, `pcx_load` at `0x1b5a4`, `race_panel_3` at `0x2e58c`,
    `overlay_queue_update` at `0x289bc`, `opponent_update` at `0x20954`,
    `loading_screen` at `0x2a364`, `race_heading_update` at `0x29610`,
    `menu_frame` at `0x29f80`, `list_file_load` at `0x15460`, and
    `object_revolution_step` at `0x1d30c` are done;
    the result is twenty-two verified modules, one matching module, and
    ninety-seven open rebuilds).
