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
   - An assembly source is the build only when C cannot reproduce the original
     bytes, for example a library routine that the original compiler produced
     as assembly.  Record the reason.  Do not use assembly only to raise the
     verified count.
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
| `0x2e82c` | `race_record_save` (done) | 1185 | 347 | 5 | 6 |
| `0x16f34` | `player_car_view_update` (done) | 1236 | 318 | 22 | 157 |
| `0x2a558` | `race_results_screen` (done) | 1323 | 343 | 38 | 13 |
| `0x2cf68` | `championship_screen` (done) | 1357 | 370 | 33 | 11 |

### Phase G: next candidates

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x18db0` | `engine_rpm_update` (done) | 1734 | 448 | 10 | 162 |

### Phase H: next unnamed functions

These are the largest unnamed game functions in the object.  The rebuild infers
a name from the call graph and the data flow, as in Phase F.

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x21338` | `options_screen` (done) | 1715 | 464 | 38 | 47 |
| `0x1a930` | `car_camera` (done) | 1425 | 396 | 18 | 126 |
| `0x2aab8` | `menu_objects_init` (done) | 1420 | 395 | 32 | 0 |
| `0x15c1c` | `player_car_link` (done) | 1160 | 306 | 8 | 37 |
| `0x2b9cc` | `map_object_update` (done) | 1121 | 310 | 15 | 110 |
| `0x22744` | `spline_interpolate` (done) | 1081 | 360 | 12 | 85 |
| `0x17d14` | `player_car_skid_update` (done) | 1078 | 290 | 10 | 106 |
| `0x2c430` | `name_entry_screen` (done) | 885 | 230 | 19 | 60 |
| `0x23a94` | `race_radar` (done) | 870 | 208 | 16 | 37 |
| `0x242bc` | `car_sound_update` (done) | 829 | 215 | 14 | 23 |
| `0x2b660` | `name_entry_letter_update` (done) | 803 | 219 | 8 | 55 |
| `0x2d4b8` | `race_records_screen` (done) | 779 | 224 | 21 | 24 |

### Phase I: next unnamed functions

These are the next unnamed game functions in the object.  The rebuild infers a
name from the call graph and the data flow, as in Phase F.

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x18528` | `player_car_reset_update` (done) | 776 | 168 | 5 | 44 |

### Phase J: next unnamed functions

These are the next unnamed game functions in the object, in size order.

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x1a630` | `player_route_camera` (done) | 768 | 243 | 17 | 104 |
| `0x1821c` | `player_track_node_update` (done) | 756 | 220 | 6 | 27 |
| `0x168b4` | `player_car_suspension_update` (done) | 740 | 219 | 5 | 80 |
| `0x2b044` | `map_object_place` (done) | 708 | 222 | 4 | 25 |
| `0x17828` | `car_collision_response` (done) | 708 | 183 | 8 | 71 |
| `0x120c0` | `track_nearest_node` (done) | 695 | 201 | 0 | 9 |
| `0x2bedc` | `car_select_screen` (done) | 682 | 171 | 12 | 12 |
| `0x2c188` | `track_select_screen` (done) | 679 | 164 | 12 | 12 |
| `0x1bbc8` | `choice_load` (done) | 673 | 189 | 4 | 0 |
| `0x29918` | `menu_text_update` (done) | 657 | 174 | 5 | 6 |
| `0x16ca0` | `player_car_cornering_update` (done) | 657 | 182 | 6 | 66 |
| `0x23420` | `race_hud_draw` (done) | 633 | 173 | 22 | 14 |

### Phase K: next unnamed functions

These are the next unnamed game functions in the object, in size order.

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x1a3b8` | `player_view_orbit` (done) | 631 | 179 | 14 | 58 |
| `0x1e414` | `opponent_avoid_player` (done) | 590 | 162 | 8 | 44 |
| `0x175dc` | `traffic_collision_pass` (done) | 587 | 165 | 5 | 11 |
| `0x19f00` | `view_transition_update` (done) | 577 | 178 | 11 | 54 |
| `0x124c0` | `opponent_start` (done) | 545 | 164 | 5 | 16 |
| `0x1af70` | `spline_load` (done) | 527 | 148 | 14 | 0 |
| `0x21b9c` | `sound_emitter_update` (done) | 524 | 153 | 13 | 39 |
| `0x1fa2c` | `dos_datetime_set` (done) | 524 | 166 | 11 | 0 |
| `0x166a8` | `player_car_contact_update` (done) | 524 | 159 | 4 | 26 |
| `0x20b1c` | `opponent_path_update` (done) | 516 | 166 | 10 | 60 |
| `0x1b394` | `member_list_load` (done) | 502 | 166 | 10 | 0 |
| `0x1517c` | `opponent_route_setup` (done) | 499 | 137 | 4 | 13 |

### Phase L: next unnamed functions

These functions use the C build.  Assembly is not added unless C cannot
reproduce the original bytes.

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x1d00c` | `car_collision_pass` (done) | 497 | 144 | 3 | 7 |
| `0x2cc98` | `track_select_update` (done) | 491 | 124 | 5 | 19 |
| `0x258d0` | `palette_shade_build` (done) | 475 | 151 | 0 | 44 |
| `0x27a5c` | `hud_panel_show` (done) | 471 | 135 | 8 | 15 |
| `0x27c34` | `credits_scroll` (done) | 466 | 133 | 3 | 7 |
| `0x17408` | `car_proximity_pass` (done) | 466 | 133 | 4 | 5 |
| `0x1ed04` | `player_state_snapshot` (done) | 465 | 115 | 0 | 17 |
| `0x11a30` | `wheel_contact_update` (done) | 461 | 141 | 8 | 21 |
| `0x2b494` | `map_object_finish` (done) | 457 | 118 | 5 | 14 |
| `0x1e914` | `func_1e914` (done) | 452 | 160 | 1 | 123 |
| `0x23038` | `path_playback_update` (done) | 437 | 148 | 1 | 19 |
| `0x219ec` | `quit_confirm_screen` (done) | 431 | 122 | 14 | 0 |

### Phase M: next unnamed functions

These functions use the C build.  Assembly is not added unless C cannot
reproduce the original bytes.

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x19678` | `race_lap_complete` (done) | 431 | 124 | 7 | 16 |
| `0x2c7a8` | `championship_screen_init` (done) | 425 | 125 | 9 | 0 |
| `0x20d20` | `race_intro_update` (done) | 421 | 106 | 5 | 15 |
| `0x1fd70` | `race_session_run` (done) | 419 | 112 | 12 | 0 |
| `0x1ef68` | `traffic_object_link` (done) | 415 | 117 | 5 | 20 |
| `0x11ccc` | `collision_cell_test` (done) | 413 | 127 | 4 | 9 |
| `0x2003c` | `race_simulation_update` (done) | 404 | 104 | 14 | 0 |
| `0x21058` | `race_finish_update` (done) | 391 | 114 | 10 | 5 |
| `0x2fcec` | `music_load` (done) | 379 | 99 | 13 | 0 |
| `0x4d844` | `lzw_bit_read` (done) | 374 | 119 | 1 | 0 |
| `0x2cb20` | `car_select_update` (done) | 373 | 103 | 3 | 19 |
| `0x82258` | `matrix_compose` (done) | 365 | 127 | 0 | 0 |

### Phase N: next unnamed functions

These functions use the C build.  Assembly is not added unless C cannot
reproduce the original bytes.

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x830c8` | `sound_range_update` (done) | 649 | 207 | 0 | 0 |
| `0x8aa60` | `sound_mix_prepare` (done) | 535 | 155 | 1 | 5 |
| `0x1057c` | `radar_world_draw` (done) | 483 | 154 | 1 | 0 |
| `0x8beb8` | `video_copy` (done) | 468 | 143 | 5 | 0 |
| `0x194a0` | `player_lap_progress_update` (done) | 363 | 103 | 4 | 12 |
| `0x254a8` | `text_measure` (done) | 360 | 104 | 0 | 0 |
| `0x1eae0` | `traffic_step` (done) | 358 | 112 | 6 | 0 |
| `0x22378` | `opponent_curve_eval` (done) | 353 | 121 | 1 | 31 |
| `0x836ac` | `sound_event_render` (done) | 351 | 100 | 2 | 13 |
| `0x1e7b4` | `func_1e7b4` (done) | 350 | 124 | 1 | 96 |
| `0x1d55c` | `object_revolution_matrix` (done) | 350 | 103 | 1 | 23 |
| `0x29bac` | `championship_result_apply` (done) | 348 | 90 | 0 | 0 |

### Phase O: next unnamed functions

These functions use the C build.  Assembly is not added unless C cannot
reproduce the original bytes.

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x1c13c` | `quad_point_test` (done) | 338 | 107 | 0 | 0 |
| `0x188b8` | `player_car_spin_update` (done) | 347 | 99 | 1 | 41 |
| `0x4c98c` | `unr_file_load` (done) | 344 | 96 | 13 | 0 |
| `0x2392c` | `gauge_marker_draw` (done) | 344 | 110 | 15 | 43 |
| `0x19828` | `player_car_wheel_effect_start` (done) | 344 | 99 | 2 | 6 |
| `0x12f50` | `game_mode_100_start` (done) | 335 | 85 | 3 | 0 |
| `0x16150` | `player_brake_update` (done) | 332 | 95 | 1 | 22 |
| `0x4c695` | `stream_read_text` (done) | 331 | 106 | 3 | 0 |
| `0x13580` | `race_view_reset` (done) | 330 | 91 | 0 | 0 |
| `0x18a14` | `race_clock_update` (done) | 329 | 92 | 0 | 17 |
| `0x25658` | `font_load` (done) | 324 | 93 | 11 | 0 |
| `0x83ca8` | `background_scale` (done) | 323 | 111 | 0 | 0 |

### Phase P: next unnamed functions

These functions use the C build.  Assembly is not added unless C cannot
reproduce the original bytes.

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x25020` | `text_draw` (done) | 1044 | 292 | 0 | 0 |
| `0x82e0c` | `sound_range_reset` (done) | 699 | 216 | 0 | 0 |
| `0x14fb8` | `spline_table_load` (done) | 316 | 80 | 12 | 0 |
| `0x231f0` | `path_keyframe_append` (done) | 309 | 89 | 0 | 13 |
| `0x20428` | `race_position_update` (done) | 308 | 86 | 0 | 10 |
| `0x1b840` | `replay_load` (done) | 306 | 84 | 8 | 0 |
| `0x4cd9c` | `material_load` (done) | 303 | 81 | 12 | 0 |
| `0x1cbac` | `terrain_contact_update` (done) | 297 | 81 | 2 | 25 |
| `0x19980` | `player_car_wheel_effect_update` (done) | 297 | 90 | 4 | 7 |
| `0x11f98` | `player_segment_update` (done) | 295 | 96 | 3 | 5 |
| `0x8b1d4` | `sound_source_frame_select` (done) | 293 | 103 | 0 | 11 |
| `0x11e74` | `body_point_collision_detect` (done) | 292 | 85 | 6 | 17 |

### Phase Q: next unnamed functions

These functions use the C build.  Assembly is not added unless C cannot
reproduce the original bytes.

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x293a8` | `championship_record_select` (done) | 288 | 78 | 0 | 0 |
| `0x18ba8` | `sla_marker_check` (done) | 291 | 90 | 2 | 8 |
| `0x4d4c4` | `lzw_block` (done) | 290 | 73 | 2 | 0 |
| `0x8adac` | `sound_voice_setup` (done) | 279 | 71 | 1 | 0 |
| `0x17b54` | `traffic_push_response` (done) | 279 | 70 | 3 | 26 |
| `0x82c14` | `camera_project` (done) | 277 | 72 | 0 | 0 |
| `0x15874` | `cd_check_screen` (done) | 269 | 69 | 18 | 0 |
| `0x1d200` | `car_collision_push` (done) | 268 | 70 | 2 | 32 |
| `0x1ba4c` | `replay_load_atk` (done) | 267 | 82 | 5 | 0 |
| `0x1b738` | `replay_save` (done) | 263 | 69 | 8 | 0 |
| `0x16b98` | `player_segment_apply` (done) | 262 | 95 | 1 | 4 |
| `0x257b0` | `sprite_load` (done) | 261 | 67 | 12 | 0 |

### Phase R: next unnamed functions

These functions use the C build.  Assembly is not added unless C cannot
reproduce the original bytes.

| Address | Name | Size | Instructions | Calls | FPU |
| --- | --- | --- | --- | --- | --- |
| `0x22018` | `angle_approach` (done) | 257 | 93 | 0 | 43 |
| `0x1a2bc` | `player_cockpit_camera` (done) | 250 | 71 | 2 | 10 |
| `0x12d0c` | `game_mode_10_start` (done) | 249 | 76 | 6 | 10 |
| `0x23328` | `path_playback_apply` (done) | 248 | 71 | 3 | 12 |
| `0x1c38a` | unnamed | 243 | 73 | 0 | 0 |
| `0x833a4` | unnamed | 242 | 98 | 0 | 0 |
| `0x15984` | unnamed | 240 | 70 | 7 | 0 |
| `0x201d0` | unnamed | 237 | 66 | 11 | 0 |
| `0x2c954` | unnamed | 235 | 65 | 9 | 0 |
| `0x8affc` | unnamed | 234 | 74 | 9 | 0 |
| `0x15378` | unnamed | 229 | 70 | 2 | 0 |
| `0x18ccc` | unnamed | 226 | 69 | 3 | 19 |

## Position

Two hundred fifty-five modules carry a recipe and a result: the twelve Phase A modules,
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
at `0x205c4`, `sound_bank_load` at `0x1f2e0`, and the first twelve Phase F modules
`car_setup` at `0x142ec`, `file_error_report` at `0x4bf88`, `player_car_init`
at `0x136cc`, `net_race_lobby` at `0x2d7c4`, `player_view_setup` at
`0x19ae8`, `race_data_load` at `0x131a8`, `track_progress_update` at
`0x1db94`, `player_steering_update` at `0x1629c`, `race_record_save` at
`0x2e82c`, `player_car_view_update` at `0x16f34`, `race_results_screen`
at `0x2a558`, and `championship_screen` at `0x2cf68`, the Phase G module
`engine_rpm_update` at `0x18db0`, and the Phase H modules `options_screen` at
`0x21338`, `car_camera` at `0x1a930`, `menu_objects_init` at `0x2aab8`,
and `player_car_link` at `0x15c1c`, `map_object_update` at `0x2b9cc`,
`spline_interpolate` at `0x22744`, `player_car_skid_update` at `0x17d14`,
`name_entry_screen` at `0x2c430`, `race_radar` at `0x23a94`, and
`car_sound_update` at `0x242bc`, `name_entry_letter_update` at `0x2b660`,
and `race_records_screen` at `0x2d4b8`, and the Phase I module
`player_car_reset_update` at `0x18528`, and the Phase J modules
`player_route_camera` at `0x1a630`, `player_track_node_update` at `0x1821c`,
`player_car_suspension_update` at `0x168b4`, `map_object_place` at `0x2b044`,
and `car_collision_response` at `0x17828`, and `track_nearest_node` at
`0x120c0`, and `car_select_screen` at `0x2bedc`, and `track_select_screen` at
`0x2c188`, and `choice_load` at `0x1bbc8`, and `menu_text_update` at `0x29918`,
and `player_car_cornering_update` at `0x16ca0`, and `race_hud_draw` at
`0x23420`, and `player_view_orbit` at `0x1a3b8`, and `opponent_avoid_player` at `0x1e414`,
and `traffic_collision_pass` at `0x175dc`, and `view_transition_update` at
`0x19f00`, and `opponent_start` at `0x124c0`, and `spline_load` at `0x1af70`,
and `sound_emitter_update` at `0x21b9c`, and `dos_datetime_set` at `0x1fa2c`,
and `player_car_contact_update` at `0x166a8`, and `opponent_path_update` at `0x20b1c`,
and `member_list_load` at `0x1b394`, and `opponent_route_setup` at `0x1517c`,
and the Phase L modules `car_collision_pass` at `0x1d00c`,
`track_select_update` at `0x2cc98`, `palette_shade_build` at `0x258d0`,
`hud_panel_show` at `0x27a5c`, `credits_scroll` at `0x27c34`,
`car_proximity_pass` at `0x17408`, `player_state_snapshot` at `0x1ed04`,
and `wheel_contact_update` at `0x11a30`, `map_object_finish` at `0x2b494`, and
`func_1e914` at `0x1e914`, and `path_playback_update` at `0x23038`, and
`quit_confirm_screen` at `0x219ec`, and `race_lap_complete` at `0x19678`, and
`championship_screen_init` at `0x2c7a8`, and `race_intro_update` at `0x20d20`,
and `race_session_run` at `0x1fd70`, and `traffic_object_link` at `0x1ef68`,
and `collision_cell_test` at `0x11ccc`, and `race_simulation_update` at
`0x2003c`, and `race_finish_update` at `0x21058`, and `music_load` at
`0x2fcec`, and `lzw_bit_read` at `0x4d844`, and `car_select_update` at
`0x2cb20`, and `matrix_compose` at `0x82258`, and `sound_range_update` at
`0x830c8`, and `sound_mix_prepare` at `0x8aa60`, and `radar_world_draw` at
`0x1057c`, and `video_copy` at `0x8beb8`, and `player_lap_progress_update`
at `0x194a0`, and `text_measure` at `0x254a8`, and `traffic_step` at
`0x1eae0`, and `opponent_curve_eval` at `0x22378`, and `sound_event_render`
at `0x836ac`, and `func_1e7b4` at `0x1e7b4`, and `object_revolution_matrix`
at `0x1d55c`, and `championship_result_apply` at `0x29bac`, and the Phase O
modules `quad_point_test` at `0x1c13c`, `player_car_spin_update` at
`0x188b8`, `unr_file_load` at `0x4c98c`, `gauge_marker_draw` at `0x2392c`,
`player_car_wheel_effect_start` at `0x19828`, and `game_mode_100_start` at
`0x12f50`, and `player_brake_update` at `0x16150`, and `stream_read_text` at
`0x4c695`, and `race_view_reset` at `0x13580`, and `race_clock_update` at `0x18a14`,
and `font_load` at `0x25658`, and `background_scale` at `0x83ca8`, and the Phase P
modules `text_draw` at `0x25020`, `sound_range_reset` at `0x82e0c`,
`spline_table_load` at `0x14fb8`, and `path_keyframe_append` at `0x231f0`,
and `race_position_update` at `0x20428`, and `replay_load` at `0x1b840`,
and `material_load` at `0x4cd9c`, and `terrain_contact_update` at `0x1cbac`,
and `player_car_wheel_effect_update` at `0x19980`, and
`player_segment_update` at `0x11f98`, and
`sound_source_frame_select` at `0x8b1d4`, and
`body_point_collision_detect` at `0x11e74`, and the Phase Q modules
`championship_record_select` at `0x293a8`, `sla_marker_check` at `0x18ba8`,
`lzw_block` at `0x4d4c4`, and `sound_voice_setup` at `0x8adac`,
`traffic_push_response` at `0x17b54`, and `camera_project` at `0x82c14`,
and `cd_check_screen` at `0x15874`, and
`car_collision_push` at `0x1d200`, and `replay_load_atk` at `0x1ba4c`,
and `replay_save` at `0x1b738`, and
`player_segment_apply` at `0x16b98`, and
`sprite_load` at `0x257b0`, and
`angle_approach` at `0x22018`, and
`player_cockpit_camera` at `0x1a2bc`, and
`game_mode_10_start` at `0x12d0c`, and
`path_playback_apply` at `0x23328`.
The next unnamed function is `0x1c38a`.
