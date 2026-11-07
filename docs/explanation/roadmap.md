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
  `main` and its 13 callees carry names, the 32 `unknown` records reachable
  from the ten verified and rebuilt roots carry names, and the new
  candidates await mapping).
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
  progress: two hundred fourteen named modules are rebuilt with the harness and the
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
  `engine_rpm_update` at `0x18db0`, and the Phase H modules `options_screen`
  at `0x21338`, `car_camera` at `0x1a930`, `menu_objects_init` at
  `0x2aab8`, `player_car_link` at `0x15c1c`, `map_object_update` at
  `0x2b9cc`, `spline_interpolate` at `0x22744`,
  `player_car_skid_update` at `0x17d14`, `name_entry_screen` at `0x2c430`,
  `race_radar` at `0x23a94`, `car_sound_update` at `0x242bc`,
  `name_entry_letter_update` at `0x2b660`, `race_records_screen` at
  `0x2d4b8`, the Phase I module `player_car_reset_update` at `0x18528`, and
  the Phase J modules `player_route_camera` at `0x1a630`,
  `player_track_node_update` at `0x1821c`,
  `player_car_suspension_update` at `0x168b4`,
  `map_object_place` at `0x2b044`, and `car_collision_response` at `0x17828`,
  and `track_nearest_node` at `0x120c0`, and `car_select_screen` at
  `0x2bedc`, and `track_select_screen` at `0x2c188`, and `choice_load` at
  `0x1bbc8`, and `menu_text_update` at `0x29918`, and
  `player_car_cornering_update` at `0x16ca0`, and `race_hud_draw` at
  `0x23420`, and the Phase K modules `player_view_orbit` at `0x1a3b8`,
  `opponent_avoid_player` at `0x1e414`, `traffic_collision_pass` at
  `0x175dc`, and `view_transition_update` at `0x19f00`, and `opponent_start` at
  `0x124c0`, and `spline_load` at `0x1af70`, and `sound_emitter_update` at
  `0x21b9c`, and `dos_datetime_set` at `0x1fa2c`, and
  `player_car_contact_update` at `0x166a8`, and `opponent_path_update` at
  `0x20b1c`, and `member_list_load` at `0x1b394`, and `opponent_route_setup` at
  `0x1517c`, and the Phase L modules `car_collision_pass` at `0x1d00c`,
  `track_select_update` at `0x2cc98`, `palette_shade_build` at `0x258d0`,
  `hud_panel_show` at `0x27a5c`, `credits_scroll` at `0x27c34`,
  `car_proximity_pass` at `0x17408`, `player_state_snapshot` at `0x1ed04`,
  `wheel_contact_update` at `0x11a30`, and `map_object_finish` at `0x2b494`,
  and `func_1e914` at `0x1e914`, and `path_playback_update` at `0x23038`, and
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
  at `0x836ac`, and `func_1e7b4` at `0x1e7b4`, and
  `object_revolution_matrix` at `0x1d55c`;
  thirty modules are verified, one is matching, and one hundred and
  eighty-four rebuilds hold the recorded difference; the `video_fill`,
  `frame_step`, `hud_update`, `music_bank_reset`, `stream_status`, and
  `engine_shutdown` modules use hand-written assembly oracles that match
  exactly).
- Keep assembly implementations to the necessary minimum.  Use C first; add
  assembly only when C cannot reproduce the original bytes, and record why.  Do
  not convert a module to assembly only to raise the verified count.
- Rebuild the data pipeline that produces the shipped data files.
- Compare the rebuild with the original: same files, same behavior (in progress:
  `tools/rebuild/diff_module.py` compares the rebuilt code with the original
  code, `run.py --strict` accepts only an exact or relocation-only function
  match, and `tools/rebuild/image_compare.py` is the whole-image gate).

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
   level: `engine_init`, `race_loop`, and `frame_update` carry names, and the
   unknown records reachable from the verified and rebuilt roots are mapped,
   and the scan now matches both prologue encodings and seeds the call targets
   of those roots; the next step maps the new candidates).
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
    twenty-four Phase E modules `object_update_all` at `0x2786c`, `keyboard_hook` at
    `0x14e20`, `object_integrate` at `0x1d6bc`, `race_panel_0` at
    `0x2e0b4`, `net_player_add` at `0x2dddc`, `object_init_from_file` at
    `0x4cc10`, `pcx_load` at `0x1b5a4`, `race_panel_3` at `0x2e58c`,
    `overlay_queue_update` at `0x289bc`, `opponent_update` at `0x20954`,
    `loading_screen` at `0x2a364`, `race_heading_update` at `0x29610`,
    `menu_frame` at `0x29f80`, `list_file_load` at `0x15460`,
    `object_revolution_step` at `0x1d30c`, `startup` at `0x30170`,
    `joystick_update` at `0x24da0`, `menu_space_init` at `0x29d08`,
    `object_gear_step` at `0x1d834`, `object_collision` at `0x1e00c`,
    `object_car_link` at `0x1c84c`, `race_panel_2` at `0x2e21c`,
    `ai_update` at `0x205c4`, `sound_bank_load` at `0x1f2e0`, and the first
    twelve Phase F modules `car_setup` at `0x142ec`, `file_error_report` at
    `0x4bf88`, `player_car_init` at `0x136cc`, `net_race_lobby` at
    `0x2d7c4`, `player_view_setup` at `0x19ae8`, `race_data_load` at
    `0x131a8`, `track_progress_update` at `0x1db94`,
    `player_steering_update` at `0x1629c`, `race_record_save` at
    `0x2e82c`, `player_car_view_update` at `0x16f34`, `race_results_screen`
    at `0x2a558`, and `championship_screen` at `0x2cf68`, the Phase G module
    `engine_rpm_update` at `0x18db0`, and the Phase H modules `options_screen`
    at `0x21338`, `car_camera` at `0x1a930`, `menu_objects_init` at
    `0x2aab8`, `player_car_link` at `0x15c1c`, `map_object_update` at
    `0x2b9cc`, `spline_interpolate` at `0x22744`,
    `player_car_skid_update` at `0x17d14`, `name_entry_screen` at `0x2c430`,
    `race_radar` at `0x23a94`, `car_sound_update` at `0x242bc`,
    `name_entry_letter_update` at `0x2b660`, `race_records_screen` at
    `0x2d4b8`, the Phase I module `player_car_reset_update` at `0x18528`, and
    the Phase J modules `player_route_camera` at `0x1a630`,
    `player_track_node_update` at `0x1821c`,
    `player_car_suspension_update` at `0x168b4`,
    `map_object_place` at `0x2b044`, and `car_collision_response` at `0x17828`,
    and `track_nearest_node` at `0x120c0`, and `car_select_screen` at
    `0x2bedc`, and `track_select_screen` at `0x2c188`, and `choice_load` at
    `0x1bbc8`, and `menu_text_update` at `0x29918`, and
    `player_car_cornering_update` at `0x16ca0`, and `race_hud_draw` at
    `0x23420`, and the Phase K modules `player_view_orbit` at `0x1a3b8`,
    `opponent_avoid_player` at `0x1e414`, `traffic_collision_pass` at
    `0x175dc`, and `view_transition_update` at `0x19f00`, and
    `opponent_start` at `0x124c0`, and `spline_load` at `0x1af70`, and
    `sound_emitter_update` at `0x21b9c`, and `dos_datetime_set` at `0x1fa2c`,
    and `player_car_contact_update` at `0x166a8`, and `opponent_path_update` at
    `0x20b1c`
    and `member_list_load` at `0x1b394`, and `opponent_route_setup` at `0x1517c`,
    and the Phase L modules `car_collision_pass` at `0x1d00c`,
    `track_select_update` at `0x2cc98`, `palette_shade_build` at `0x258d0`,
    `hud_panel_show` at `0x27a5c`, `credits_scroll` at `0x27c34`,
    `car_proximity_pass` at `0x17408`, `player_state_snapshot` at `0x1ed04`,
    `wheel_contact_update` at `0x11a30`, and `map_object_finish` at `0x2b494`,
    and `func_1e914` at `0x1e914`, and `path_playback_update` at `0x23038`,
    and `quit_confirm_screen` at `0x219ec`, and `race_lap_complete` at
    `0x19678`, and `championship_screen_init` at `0x2c7a8`, and
    `race_intro_update` at `0x20d20`, and `race_session_run` at `0x1fd70`, and
    `traffic_object_link` at `0x1ef68`, and `collision_cell_test` at `0x11ccc`,
    and `race_simulation_update` at `0x2003c`, and `race_finish_update` at
    `0x21058`, and `music_load` at `0x2fcec`, and `lzw_bit_read` at `0x4d844`,
    and `car_select_update` at `0x2cb20`, and `matrix_compose` at `0x82258`,
    and `sound_range_update` at `0x830c8`, and `sound_mix_prepare` at
    `0x8aa60`, and `radar_world_draw` at `0x1057c`, and `video_copy` at
    `0x8beb8`, and `player_lap_progress_update` at `0x194a0`, and
    `text_measure` at `0x254a8`, and `traffic_step` at `0x1eae0`, and
    `opponent_curve_eval` at `0x22378`, and `sound_event_render` at
    `0x836ac`, and `func_1e7b4` at `0x1e7b4`, and
    `object_revolution_matrix` at `0x1d55c`
    are done;
    the result is thirty verified modules, one matching module, and one
    hundred and eighty-four open rebuilds).
13. Enforce the byte gate (in progress: `run.py --strict` accepts only the
    exact-bytes verdict and the relocation-only verdict, `store.py check`
    validates every record against the JSON Schema contracts, and stage 11
    compares the rebuilt whole image with `tools/rebuild/image_compare.py` when
    the image exists).
14. Compare function bodies with the relocation-aware oracle (done:
    `tools/rebuild/oracle.py` masks the fixup slots and the direct branch
    targets, `free` and `exit` pass `--strict`, and a reordered or shape-only
    sequence fails).
