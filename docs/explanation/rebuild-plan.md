# Rebuild plan

## Purpose

This document holds the ordered plan for the function modules that still have
no module record.  The store holds the live status of every record.  The status
page at `work/rebuild/status.html` shows the current position.

The plan follows the M3 milestone in `roadmap.md`: rebuild the game modules in
size order.  The order starts with the smallest function.  A small function
gives a small module, a small harness, and a quick verification, so the plan
makes early progress and finds the recurring differences early.

## Method

One module is one function record.  For each function the method is:

1. Disassemble the function with `tools/le.py`.
2. Identify the call graph and the data references.
3. Write the rebuilt source under `tools/rebuild/`:
   - The C source is the primary build, with the stack-frame option `-of+`
     when the original game file uses a frame.
   - The assembly source is the build for a library routine that the original
     compiler produced as assembly, for example `memcpy`.
   - The harness source holds the callee stubs and prints the markers.
4. Add the module record under `tools/rebuild/modules/`.
5. Build the module with the harness and run it under DOSBox-X.
6. Run the identity check with `diff_module.py`.
7. Run the checks with `run.py` and record the result.
8. Record a new finding in `findings.md` and update the roadmap and the
   tooling reference.

The original game files use the Watcom compiler 10.x.  OpenWatcom V2 is the
pinned build compiler.  The two compilers differ in the frame code generation:
V2 saves the registers after the frame and always emits an ESP restore, while
the original compiler saves the registers before the frame and omits the
restore.  A C module that holds no such difference passes the identity check.
A C module that differs stays `rebuilt` and the finding records the
difference.  The project returns to the original Watcom 10.x compiler later,
so the C source stays the primary build and the exact-match assembly build
stays alongside where it exists.

## The order

The size is the byte length of the original function body.  The instruction
count and the call count come from the same body.  The FPU column counts the
floating-point instructions.

### Phase A: up to 40 bytes

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x8bdec` | `vesa_set_mode` (done) | 22 | 8 | 1 | 0 |
| `0xd1ec0` | `memset` (done) | 24 | 12 | 1 | 0 |
| `0x8c160` | `memset` (done) | 25 | 12 | 0 | 0 |
| `0x4bf4b` | `dpmi_call` (done) | 25 | 14 | 1 | 0 |
| `0x4d9f4` | `fp_helper` (done) | 29 | 12 | 0 | 4 |
| `0x4bf64` | `fprintf` (done) | 33 | 13 | 1 | 0 |
| `0x10193` | `default_data_copy` (done) | 33 | 16 | 0 | 0 |
| `0xd2066` | `memmove` (done) | 37 | 23 | 0 | 0 |
| `0x27a34` | `hud_update` (done) | 38 | 11 | 3 | 0 |
| `0x4c7e0` | `stream_status` (done) | 38 | 21 | 0 | 0 |
| `0x29564` | `race_state_update` (done) | 39 | 13 | 1 | 0 |
| `0x4bf23` | `dpmi_fill_regs` (done) | 40 | 16 | 0 | 0 |

### Phase B: 41 to 80 bytes

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x2f9b0` | `music_bank_reset` (done) | 46 | 14 | 0 | 0 |
| `0x1db38` | `object_gear_state` (done) | 47 | 16 | 0 | 0 |
| `0x4d32c` | `sprintf` (done) | 48 | 18 | 1 | 0 |
| `0x1c79c` | `object_prepare` (done) | 49 | 16 | 3 | 0 |
| `0x22b80` | `filter_update` (done) | 49 | 20 | 0 | 5 |
| `0x107a1` | `video_block_swap_b` (done) | 52 | 18 | 0 | 0 |
| `0x4c8f0` | `video_buf_load` (done) | 53 | 18 | 2 | 0 |
| `0x28b5c` | `menu_enter` (done) | 54 | 19 | 1 | 0 |
| `0x19ab0` | `player_update` (done) | 55 | 14 | 5 | 0 |
| `0x27f44` | `race_timeout` (done) | 56 | 16 | 1 | 0 |
| `0xd4410` | `dpmi_int31_entry` (done) | 56 | 28 | 1 | 0 |
| `0x1c810` | `object_physics` (done) | 57 | 18 | 4 | 0 |
| `0x25db0` | `dpmi_real_int` (done) | 58 | 17 | 0 | 0 |
| `0x1c760` | `object_player_input` (done) | 58 | 18 | 3 | 0 |
| `0x4c810` | `stream_layer_init` (done) | 61 | 19 | 6 | 0 |
| `0x81a70` | `sound_update` (done) | 62 | 16 | 1 | 0 |
| `0x1f94c` | `view_move` (done) | 65 | 22 | 3 | 0 |
| `0x1075f` | `video_block_swap_a` (done) | 66 | 25 | 0 | 0 |
| `0x22df8` | `timing_update` (done) | 71 | 20 | 1 | 3 |
| `0x2dfac` | `panel_show` (done) | 71 | 22 | 1 | 0 |
| `0x1f718` | `table_lookup` (done) | 74 | 28 | 1 | 0 |
| `0x30050` | `memory_alloc` (done) | 75 | 31 | 3 | 0 |
| `0x262a8` | `sound_driver_set` (done) | 79 | 24 | 1 | 0 |
| `0xd4324` | `main invoker` (done) | 79 | 26 | 4 | 0 |

### Phase C: 81 to 160 bytes

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x10ba4` | `graphics_load` (done) | 88 | 40 | 0 | 0 |
| `0x4c38d` | `stream_close` (done) | 89 | 34 | 3 | 0 |
| `0x2f205` | `memory_free` (done) | 90 | 33 | 2 | 0 |
| `0x2e7cc` | `panel_draw_centered` (done) | 93 | 34 | 2 | 0 |
| `0xd2008` | `free` (done) | 94 | 35 | 1 | 0 |
| `0x2a1b0` | `race_cleanup` (done) | 97 | 24 | 12 | 0 |
| `0x4d030` | `vis_tab_load` (done) | 101 | 31 | 6 | 0 |
| `0x1ccd8` | `object_state_save` (done) | 105 | 29 | 0 | 2 |
| `0x26448` | `input_poll` (done) | 108 | 27 | 2 | 0 |
| `0x27700` | `race_loop` (done) | 110 | 34 | 10 | 0 |
| `0x1bb58` | `setup_load` (done) | 111 | 35 | 6 | 0 |
| `0x2ff14` | `music_play` (done) | 111 | 39 | 3 | 0 |
| `0x294c8` | `race_state_begin` (done) | 117 | 35 | 0 | 0 |
| `0x28ba4` | `net_race_screen` (done) | 118 | 34 | 0 | 0 |
| `0x2f0fc` | `dos_memory_alloc` (done) | 118 | 41 | 2 | 0 |
| `0x27ec8` | `video_present` (done) | 122 | 40 | 2 | 5 |
| `0x2f470` | `resource_free` (done) | 126 | 47 | 2 | 0 |
| `0x2ecf8` | `list_create` (done) | 127 | 46 | 3 | 0 |
| `0x2f904` | `pool_init` (done) | 129 | 44 | 2 | 0 |
| `0xd429f` | `__fprtf` (done) | 133 | 52 | 3 | 0 |
| `0x128e0` | `race_session_start` (done) | 136 | 42 | 3 | 0 |
| `0x2ffc4` | `music_start` (done) | 140 | 41 | 7 | 0 |
| `0x279a4` | `race_end` (done) | 141 | 36 | 13 | 0 |
| `0x1b304` | `controls_load` (done) | 144 | 40 | 6 | 0 |
| `0x2f172` | `memory_alloc` (done) | 147 | 49 | 2 | 0 |
| `0x28160` | `overlay_dispatch` (done) | 152 | 42 | 1 | 0 |
| `0x15720` | `palette_build` (done) | 152 | 55 | 1 | 12 |
| `0x29818` | `fade_update` (done) | 154 | 48 | 1 | 14 |
| `0x4cf94` | `persptab_load` (done) | 155 | 43 | 8 | 0 |
| `0x1bf38` | `config_load` (done) | 155 | 46 | 7 | 0 |
| `0x4c08d` | `fopen` (done) | 155 | 48 | 5 | 0 |
| `0x2efc2` | `resource_read` (done) | 160 | 56 | 3 | 0 |

### Phase D: 161 to 300 bytes

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x2e724` | `race_panel_4` (done) | 167 | 50 | 4 | 0 |
| `0x10810` | `engine_interface_init` (done) | 183 | 51 | 2 | 0 |
| `0x2807c` | `race_frame` (done) | 189 | 48 | 13 | 3 |
| `0x22e94` | `replay_read` (done) | 190 | 58 | 0 | 0 |
| `0x10020` | `main` (done) | 194 | 56 | 15 | 0 |
| `0x2f25f` | `resource_find` (done) | 199 | 69 | 2 | 0 |
| `0x21e6c` | `motion_update` (done) | 202 | 53 | 7 | 19 |
| `0x27f94` | `frame_update` (done) | 212 | 63 | 11 | 0 |
| `0x1df34` | `object_slide_step` (done) | 215 | 65 | 3 | 22 |
| `0x2f9e0` | `music_bank_load` (done) | 220 | 72 | 4 | 0 |
| `0x8ac78` | `camera_update` (done) | 225 | 64 | 2 | 27 |
| `0x4c4e5` | `stream_read` (done) | 226 | 80 | 2 | 0 |
| `0xd1f25` | `malloc` (done) | 227 | 84 | 3 | 0 |
| `0x22f54` | `replay_write` (done) | 228 | 64 | 0 | 4 |
| `0x10bfc` | `object_load` (done) | 232 | 69 | 5 | 0 |
| `0x1f1a0` | `mouse_init` (done) | 233 | 67 | 12 | 0 |
| `0x81ab0` | `sound_effects_update` (done) | 235 | 51 | 10 | 0 |
| `0x211e0` | `input_map` (done) | 239 | 62 | 2 | 0 |
| `0x1e30c` | `object_proximity_check` (done) | 241 | 74 | 3 | 13 |
| `0x2eed0` | `resource_get` (done) | 242 | 78 | 5 | 0 |
| `0x1cd44` | `object_transform` (done) | 244 | 61 | 2 | 12 |
| `0x27770` | `race_start` (done) | 251 | 66 | 4 | 0 |
| `0x127a0` | `engine_init` (done) | 266 | 71 | 16 | 0 |
| `0x2edad` | `memory_error_report` (done) | 291 | 97 | 8 | 0 |
| `0x1cee0` | `object_drive_step` (done) | 297 | 83 | 0 | 33 |
| `0x1fc38` | `frame_timer` (done) | 299 | 76 | 2 | 34 |

### Phase E: over 300 bytes

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x2786c` | `object_update_all` (done) | 309 | 84 | 12 | 0 |
| `0x14e20` | `keyboard_hook` (done) | 334 | 100 | 2 | 0 |
| `0x1d6bc` | `object_integrate` (done) | 339 | 82 | 4 | 34 |
| `0x2e0b4` | `race_panel_0` (done) | 357 | 103 | 5 | 29 |
| `0x2dddc` | `net_player_add` (done) | 394 | 111 | 3 | 0 |
| `0x4cc10` | `object_init_from_file` (done) | 396 | 120 | 1 | 0 |
| `0x1b5a4` | `pcx_load` (done) | 401 | 108 | 10 | 0 |
| `0x2e58c` | `race_panel_3` (done) | 408 | 120 | 6 | 6 |
| `0x289bc` | `overlay_queue_update` (done) | 415 | 117 | 1 | 0 |
| `0x20954` | `opponent_update` (done) | 454 | 134 | 6 | 17 |
| `0x2a364` | `loading_screen` (done) | 498 | 127 | 21 | 0 |
| `0x29610` | `race_heading_update` (done) | 519 | 136 | 9 | 48 |
| `0x29f80` | `menu_frame` (done) | 560 | 151 | 10 | 4 |
| `0x15460` | `list_file_load` (done) | 581 | 161 | 15 | 1 |
| `0x1d30c` | `object_revolution_step` (done) | 589 | 151 | 9 | 38 |
| `0x30170` | `startup` (done) | 605 | 196 | 3 | 0 |
| `0x300f8` | `entry jump` (not a module: the two-byte jump covers the banner; `startup` holds the body) | 607 | 197 | 3 | 0 |
| `0x24da0` | `joystick_update` (done) | 630 | 201 | 3 | 5 |
| `0x29d08` | `menu_space_init` (done) | 632 | 156 | 34 | 0 |
| `0x1d834` | `object_gear_step` (done) | 635 | 133 | 1 | 44 |
| `0x1e00c` | `object_collision` (done) | 765 | 193 | 13 | 33 |
| `0x1c84c` | `object_car_link` (done) | 863 | 238 | 6 | 28 |
| `0x2e21c` | `race_panel_2` (done) | 879 | 265 | 15 | 6 |
| `0x205c4` | `ai_update` (done) | 911 | 260 | 8 | 77 |
| `0x1f2e0` | `sound_bank_load` (done) | 1046 | 268 | 12 | 0 |

### Phase F: unnamed functions

The named plan is complete.  These records hold a function body but no name.
The rebuild infers a descriptive name from the call graph and the data flow.
The size is the function body, not the store extent.

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x142ec` | `car_setup` (done) | 226 | 65 | 2 | 0 |
| `0x4bf88` | `file_error_report` (done) | 261 | 88 | 7 | 0 |
| `0x136cc` | `player_car_init` (done) | 277 | 87 | 6 | 0 |
| `0x2d7c4` | `net_race_lobby` (done) | 690 | 198 | 28 | 0 |
| `0x19ae8` | `player_view_setup` (done) | 869 | 223 | 6 | 9 |
| `0x131a8` | `race_data_load` (done) | 919 | 236 | 38 | 0 |
| `0x1db94` | `track_progress_update` (done) | 926 | 259 | 6 | 37 |
| `0x1629c` | `player_steering_update` (done) | 997 | 267 | 9 | 90 |
| `0x2e82c` | unnamed | 1185 | 347 | 5 | 6 |
| `0x16f34` | unnamed | 1236 | 318 | 22 | 157 |
| `0x2a558` | unnamed | 1323 | 343 | 38 | 13 |
| `0x2cf68` | unnamed | 1357 | 370 | 33 | 11 |

## Position

One hundred thirty-seven modules carry a recipe and a result: the twelve Phase A modules,
the eleven modules of the earlier batches, the twenty-four Phase B modules,
including `sound_update` at `0x81a70`, and the first thirty-two Phase C modules
`graphics_load` at `0x10ba4`,
`stream_close` at `0x4c38d`, `memory_free` at `0x2f205`,
`panel_draw_centered` at `0x2e7cc`, the run-time `free` at `0xd2008`,
`race_cleanup` at `0x2a1b0`, `vis_tab_load` at `0x4d030`,
`object_state_save` at `0x1ccd8`, `input_poll` at `0x26448`,
`race_loop` at `0x27700`, `setup_load` at `0x1bb58`, `music_play` at `0x2ff14`,
`race_state_begin` at `0x294c8`, `net_race_screen` at `0x28ba4`,
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
twenty-four Phase E modules `object_update_all` at `0x2786c`, `keyboard_hook` at
`0x14e20`, `object_integrate` at `0x1d6bc`, `race_panel_0` at `0x2e0b4`,
`net_player_add` at `0x2dddc`, `object_init_from_file` at `0x4cc10`,
`pcx_load` at `0x1b5a4`, `race_panel_3` at `0x2e58c`,
`overlay_queue_update` at `0x289bc`, `opponent_update` at `0x20954`,
`loading_screen` at `0x2a364`, `race_heading_update` at `0x29610`,
`menu_frame` at `0x29f80`, `list_file_load` at `0x15460`,
`object_revolution_step` at `0x1d30c`, `startup` at `0x30170`,
`joystick_update` at `0x24da0`, `menu_space_init` at `0x29d08`,
`object_gear_step` at `0x1d834`, `object_collision` at `0x1e00c`,
`object_car_link` at `0x1c84c`, `race_panel_2` at `0x2e21c`, `ai_update`
at `0x205c4`, `sound_bank_load` at `0x1f2e0`, and the first eight Phase F modules
`car_setup` at `0x142ec`, `file_error_report` at `0x4bf88`, `player_car_init`
at `0x136cc`, `net_race_lobby` at `0x2d7c4`, `player_view_setup` at
`0x19ae8`, `race_data_load` at `0x131a8`, `track_progress_update` at
`0x1db94`, and `player_steering_update` at `0x1629c`.
The next unnamed function is `0x2e82c`.
