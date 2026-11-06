# Findings

This page records the investigation log.  Each finding carries its evidence.
The page separates observations from open questions.

## The track data files are LZW text

- The `DATA` files (`MAP*.DAT`, `GEN.DAT`, `LOCAL.DAT`) hold one LZW stream
  with no container around it.  The reader at `0x4d844` that the graphics members
  use expands them too.
  Evidence: `python3 tools/gfx_container.py lzw DATA/MAP1.DAT`.
- `DATA/MAP1.DAT` expands from 17292 bytes to 459 bytes of text with the
  `TRACK_DATA` list and the `OPPONENTS` list.  `MAP2.DAT`, `MAP4.DAT`, and
  `MAP6.DAT` expand to the same size and shape, and `MAP3.DAT` expands to 179
  bytes.
  Evidence: the `lzw` command on each file.
- `DATA/GEN.DAT` expands from 4101 bytes to 6169 bytes of text with the
  `CARS_DATA` list and 15 more lists.
  Evidence: the `lzw` command on `DATA/GEN.DAT`.
- `DATA/LOCAL.DAT` expands from 2490 bytes to 969 bytes of the credits text
  with `#` as the separator.
  Evidence: the `lzw` command on `DATA/LOCAL.DAT`.
- The `TRACK_DATA` record is `color`, `camera_hi`, `bonus_time`, and `carnage`,
  all of the type `d`, so the record holds four 32-bit integers and the size is
  0x10.  `MAP1.DAT` holds `d24 d300 d35 d10`.
  Evidence: the `MAP1.DAT` text and the 0x10 record size at `0x132b1`.
- The `OPPONENTS` record is `max_spd`, `accel`, `brakes`, `skill`, and `sband`,
  all of the type `f`, so the record holds five 32-bit floats and the size is
  0x14.  `MAP1.DAT` holds nine records, `f16800` down to `f16000`.
  Evidence: the `MAP1.DAT` text and the 0x14 record size at `0x132cd`.
- The `MAP*.DAT` record sizes match the loader arrays: the loader at `0x131a8`
  reads `TRACK_DATA` into an array of 0x10 records of 0x10 bytes and `OPPONENTS`
  into an array of 0x590 records of 0x14 bytes.
  Evidence: the disassembly of `0x132b1` and `0x132cd`.
- The `MAP*I.DAT` files are not LZW.  `MAP1I.DAT` starts with the 16-bit values
  2, 0, 0, 0, 1, 0, 0x2455, -1, 0x1422, -1, and 0x08fc, so the file holds
  signed 16-bit values.
  Evidence: the head of `DATA/MAP1I.DAT`.
- `MAP5.DAT` and `MAP5I.DAT` are not LZW either.  `MAP5.DAT` starts with the
  count 0x17 and the size 0x10cc, and `8 + 23 * 1792` is 41224, the file size.
  Evidence: the head of `DATA/MAP5.DAT`.

## The named list parser and the race loaders

- `FUN_00015460` reads one named list from one data file.  The arguments are
  the file name, the directory, the list name, the destination array, the record
  limit, and the record size.  It opens `dir\data\file.dat`, reads the lines until
  the list name, parses `count` records of `size` bytes until `END_LIST`, and
  returns the record count.
  Evidence: the disassembly of `0x15460` and the `MENU\data\GEN.dat` call at
  `0x29e92`.
- The field type comes from the first byte of the field: `d` is `atoi` and stores
  four bytes, `f` is `atof` and stores a four byte float, and the other bytes
  are separators.
  Evidence: the disassembly of `0x15570`.
- `FUN_000131a8` is the race data load.  It reads the five lists `OSTACLES`
  (0x24 records of 0x24 bytes), `TRACK_DATA` (0x10 of 0x10), `OPPONENTS`
  (0x590 of 0x14), `ANIMATIONS` (0xbc of 0x18), and `CARS_DATA` (0x40 of 0x40)
  into `DAT_0010f000`, `DAT_0010cf44`, `DAT_00110d9c`, `DAT_0010f180`, and
  `DAT_0010d25c`.
  Evidence: the disassembly of `0x132b1`, `0x132cd`, `0x132d2`, `0x132f1`, and
  `0x13318`.
- The earlier names of the two large arrays are wrong: `DAT_00110d9c` holds the
  opponents (0x590 records of 0x14 bytes) and `DAT_0010f180` holds the animations
  (0xbc records of 0x18 bytes).
  Evidence: the list names in the same call sites.
- `FUN_00027770` starts one level.  It builds the name `MAP%d` from the level
  number into `DAT_0011d294`, sets the level `DAT_0011d2b4` to 1, and calls
  `race_session_start` at `0x128e0` with the level.
  Evidence: the disassembly of `0x2780f` to `0x2784e`.
- `FUN_00029d08` loads the menu data.  It reads `MENU\data\GEN.dat` into 0x24
  records of 0x24 bytes, builds the text lists `menu`, `cardat`, `trackdat`, and
  `champ` through `FUN_0001b394`, and calls the race init.
  Evidence: the disassembly of `0x29e7b` and the decompiled `FUN_00029d08`.
- `FUN_0004cf94` loads `TABS\persp01.tab` into the `PERSPTAB` resource and
  then points `DAT_001214ec` 0x40000 bytes past the start, so the table holds two
  halves.  `FUN_0004d030` loads `TABS\VIS.TAB` into `PTR_DAT_0008c1c4`.
  Evidence: the decompiled `FUN_0004cf94` and `FUN_0004d030`.

## The BPLIST check proves the read breakpoint never fires

- The probe now reads `BPLIST` at every stop, which names the breakpoints that
  are still armed.  A stop from the `break` command leaves the list unchanged; a
  stop from a breakpoint removes or reports it.
  Evidence: `tools/scripts/probe_keyboard_wait.dbg`.
- The re-run reads `EIP=0x245d90` `AX=0x8b8b` at 60 seconds and
  `EIP=0x1e1c40` `AX=2` after the Enter, and the breakpoint list reads
  `00. BPINT 16 AH=00` at every stop.
  Evidence: the run output.
- So the `INT 16h` `AH=00` read breakpoint does not fire in the demo or after
  one Enter, and the stops are the `break` command and not the watch.  The menu
  read value stays unread, and the values `0x8b8b` and `2` are game data and not
  scancodes.
  Evidence: the breakpoint list at every stop.
- The keyboard path is therefore the `AH=01` poll only, which the earlier probe
  observed, and the `AH=00` read runs at a state the probes have not reached.
  Evidence: the poll probe and the read probe.

## Correction: the value 0x2f was a glyph and not a scancode

- This section is corrected.  The second run of the same probe reads
  `EIP=0x1e1c40` `AX=0`, so the read breakpoint does not fire a second time
  and the game sits in the demo camera.
  Evidence: the second run output.
- The address `0x1e7337`, which the first run reported, is inside the font glyph
  draw routine: the disassembly shows the inner loop `and al, 0xf0`, `shr al, 4`,
  `or eax, [0x15460]`, and `mov [edi], al`, which is the pixel loop
  `0x25316` to `0x2534a`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x252f0`.
- So `AX=0x2f` was a glyph pixel value and not a keyboard scancode, and the
  first run stopped at the glyph loop rather than at the read breakpoint.  The
  read breakpoint has not been observed to fire, and the menu read value stays
  unread.
  Evidence: the two runs and the disassembly.
- The next step distinguishes a breakpoint stop from a plain break by reading the
  breakpoint list at the stop, which names the breakpoint that fired.
  Evidence: the two runs.

## The menu read returns the value 0x2f

- The armed-after-demo probe runs 15 steps with 0 failures: the demo runs 60
  seconds, the `BPINT 16 0` read breakpoint is armed after it, the Enter is sent,
  and the break follows.
  Evidence: `tools/scripts/probe_keyboard_wait.dbg` and the run output.
- The read breakpoint fires at the menu: the EIP is `0x1e7337`, the module
  `0x25337`, which is inside the game code, and the registers read `AX=0x2f`.
  Evidence: the register read.
- `AX` is `0x2f`, so `AL` is `0x2f`, which is the value the game takes from
  the BIOS read.  The value `0x2f` is the character `/` in the BIOS convention.
  Evidence: the register read.
- The read therefore runs in the menu and not in the demo, which matches the
  earlier poll result, and the menu takes the key the read returns.
  Evidence: the two probes.
- The value `0x2f` is the read result and not the key that was sent, so the game
  maps the BIOS result to its own action and the mapping table stays open.
  Evidence: the sent key and the read value.

## The read breakpoint does not fire in the demo

- The read probe runs 11 steps with 0 failures: the `BPINT 16 0` read breakpoint
  is accepted, the 30-second wait runs, the Enter is sent, and the break follows.
  Evidence: `tools/scripts/probe_keyboard_wait.dbg` and the run output.
- The registers at the break read `EIP=0x1e1c40` `AX=0`: the game is back in
  the demo camera routine at the module `0x21ba0` and the read breakpoint did not
  fire, so the game did not take a key with `INT 16h` `AH=00` in that window.
  Evidence: the register read.
- The game therefore polls the keyboard with the `AH=01` check and continues the
  demo, and the demo does not take the key.  The scancode in `AL` stays unread
  because the read never happens in the demo.
  Evidence: the two register reads at 30 seconds.
- The next step is the menu state and not the demo, because the read path runs
  there.
  Evidence: the read probe and the poll probe.

## The read breakpoint is armed and the trace needs the read

- The scancode probe runs 14 steps with 0 failures: both keyboard breakpoints
  are accepted, the 30-second wait runs, and the registers read `EIP=0xda84`
  `AX=0x100`.
  Evidence: `tools/scripts/probe_keyboard_wait.dbg` and the run output.
- `INTT` traces `INT 00`, not `INT 16`, and the registers after the trace read
  `EIP=0xca60` `AX=0x100`, so the trace caught another interrupt and not the
  keyboard read.
  Evidence: the trace output.
- The `AH=01` check is therefore observed and the `AH=00` read is armed, and the
  scancode read needs a break that lands on the read itself.
  Evidence: the two runs.
- The next step waits for the `AH=00` breakpoint instead of stopping on a timer,
  so the `AL` at that stop is the scancode the game took.
  Evidence: the probe result.

## The game polls the BIOS keyboard at the break

- `CALLBACKS` lists the interrupt callbacks and the list ends with the BIOS
  handlers: `BIOS 1Bh stock CTRL+BREAK handler`, `irq 14 ide`, `irq 15 ide`,
  `irq 6 floppy`, `int 18`, `BIOS POST`, `BIOS Scan Video BIOS`, `BIOS Adapter
  ROM scan`, `BIOS Startup screen`, `BIOS BOOT`, `IBM ROM BASIC entry`, and the
  APM entries.  No game handler appears in the list.
  Evidence: `exec CALLBACKS` through the control socket.
- Both keyboard breakpoints are accepted: `BPINT 16 0` sets the read with `AH=00`
  and `BPINT 16 1` sets the check with `AH=01`.
  Evidence: `tools/scripts/probe_keyboard_wait.dbg` and the run output.
- At the 30-second break the registers are `EIP=0xda84`, `AX=0x100`, `BX=0`,
  `CX=0x607`, and `DX=0xf02`.  `AX` is `0x100`, so `AH=01` and `AL=00`, which
  is the BIOS check for a key, and the EIP is below the code object, in the BIOS.
  Evidence: the run output.
- So the game polls the BIOS keyboard with the `INT 16h` check, and the game does
  not hook the interrupt itself.  The read that follows carries the scancode in
  `AL`, so the `AH=00` breakpoint names the key the game takes.
  Evidence: the register read and the `CALLBACKS` list.

## The keyboard watch is armed and the advanced commands are listed

- The debugger help gives the command set: `BPINT [intNr] [ah] [al]` sets an
  interrupt breakpoint with the AH and the AL values, `CALLBACKS` shows the
  interrupt callbacks, `INTHAND` sets the code view to an interrupt handler,
  `INT` and `INTT` execute and trace into an interrupt, `LOG`, `LOGS`, `LOGL`, and
  `LOGC` write CPU log files, `HEAVYLOG` logs on exit, `SELINFO` shows a
  selector, `DV` and `DP` set the data view to a linear or physical address,
  `DOS MCBS` shows the memory control block chain, and `SMV` sets memory at a
  linear address.
  Evidence: `exec ?` through the control socket.
- The keyboard watch is accepted and listed: `BPINT 9` sets the hardware keyboard
  interrupt and `BPINT 16 0` sets the BIOS read with AH=0, and `BPLIST` reports
  both, numbers 00 and 01.
  Evidence: `tools/scripts/probe_keyboard_wait.dbg` and the run output.
- The EIP at the 40-second break is `0x42ed`, below the code object, which is
  consistent with a BIOS or extender handler, so the next step reads the breakpoint
  number at the stop to name which one fired.
  Evidence: the run output.
- `SMV [addr] [val]` writes memory at a linear address and `BPLM` watches one,
  so the menu state can be driven and watched without the keys once its address is
  known.
  Evidence: the help output.

## The one-Enter stop lands below the code object

- The one-Enter stop runs 12 steps with 0 failures: both watches are armed,
  the 45-second wait runs, the Enter lands, and the break follows after five
  seconds.  The EIP is `0x32d`, below the code object.
  Evidence: `tools/scripts/probe_table_loaders.dbg` and the run output.
- `0x32d` is below `0x10000`, the start of the code object, so the game is in
  the extender, a BIOS wait, or DOS at that moment, and not in its own code.
  Evidence: the EIP and the object table.
- The earlier single Enter press that reached the module `0xd8b85` therefore
  depended on the demo phase: the same key reaches different states at different
  times.
  Evidence: the two probe runs.
- The demo is long and the key advances its state, and the two table loaders stay
  un-fired in every window so far.
  Evidence: the six probe runs.

## The armed walk does not reach a race start

- The armed walk runs 16 steps with 0 failures: both `BPPM` watches are set
  before `RUN`, the 45-second wait runs, and the three Enter presses land.
  Evidence: `tools/scripts/probe_table_loaders.dbg` and the run output.
- Neither loader fires and the EIP at the break is `0x32d`, which is below the
  code object, so the game is in the extender or a real-mode wait at that moment.
  Evidence: the run output.
- The walk facts together: 20 seconds and four Enters stay at the menu code
  `0x1e1c40`, 45 seconds and one Enter reach the renderer `0xd4f20`, 45 and 60
  seconds with no key reach `0xd7a4c` and `0x32d`, and 90 seconds ends the
  process.
  Evidence: the five probe runs.
- The single Enter press that moved the game reached the module `0xd8b85`, so the
  demo state advances on the key, and the two table loaders are not called by the
  demo or by the three Enter presses.
  Evidence: the single press result and the armed walk.
- The two table loaders serve the race data, so the remaining step is the menu
  selection that starts a race.
  Evidence: the loader calls in `FUN_000131a8`.

## The Enter lands and the game reaches the renderer

- The 45-second wait plus one Enter runs 16 steps with 0 failures: the key is
  sent, both `BPPM` breakpoints are set, and neither loader fires.
  Evidence: `tools/scripts/probe_table_loaders.dbg` and the run output.
- The EIP after the Enter is `0x296f20`, the module `0xd4f20`, which is a fixed
  point interpolation loop: it multiplies and divides 16.16 values and stores them
  to `[edi]` while `esi` advances by eight.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0xd4f20`.
- So the Enter lands and the game draws, and the two table loaders have not run
  at that point.  The breakpoints were armed after the wait, so a load during the
  wait would be missed.
  Evidence: the run output and the disassembly.
- The two table loaders serve the race data, so the remaining step is the menu
  selection that starts a race.
  Evidence: the loader calls in `FUN_000131a8`.

## The demo is a camera fly-by and the menu follows it

- The disassembly resolves the run-time address: `0x1E1C40` is the module
  `0x21BA0`, a camera path routine.  It reads the reference points `0x13CCC`
  and `0x13CD0`, calls the helpers `0x1F718`, `0x1F880`, and `0x4D9F4`, and
  stores the result in the object fields `+0x38` and `+0x3C`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x21ba0`.
- The routine returns at `0x21DA0` when the object field `+0x38` is `-1`, so the
  demo is a camera fly-by with a fixed path and the menu follows the demo.
  Evidence: the disassembly of `0x21c6e` to `0x21c72`.
- The four Enter presses therefore landed inside the demo and the single Enter
  press that moved the game landed after it.  The key sequence is one Enter press
  after the demo ends, which is the sequence the manual playthrough used.
  Evidence: the disassembly and the single press probe.

## The table probe runs and fires on the pointer cell

- The `BPLM` probe runs against the game: the launch is `dosbox-x -nogui
  -break-start -conf ../probe.conf -conf game/dosboxScreamer_single.conf`, the
  breakpoint at `0x24E1C4` fires, and the dump files are written.
  Evidence: `tools/scripts/probe_table_watch.dbg` and the run output.
- The linear address of the `VIS.TAB` pointer cell is `0x24E1C4`, which is the
  module address `0x8C1C4` plus `0x1C2000`, and the dump reads the same address.
  Evidence: the probe and the dumps.
- The pointer cell holds 0 before and after the fire, so the break lands on an
  early access and not on the loader write, and the read order stays open.
  Evidence: `VIS_PTR_BEFORE.BIN` and `VIS_PTR_AFTER.BIN`.
- The extended probe runs 26 steps with 0 failures: the demo runs, four Enter
  presses walk the menu, and the two watches are set before the last run.
  Evidence: the probe run.
- The pointer cells hold 0 before and after the extended run, and the EIP is
  `0x1e1c40` at both dumps, so the walk did not reach the race start.
  Evidence: `VIS_PTR_AFTER.BIN` and `PERSP_PTR_AFTER.BIN`.
- The loader-entry probe sets both breakpoints: `BPPM 0150:4D030` and
  `BPPM 0150:4CF94` report "Set prot-mode memory breakpoint", and the run
  finishes with 22 steps and 0 failures.
  Evidence: `tools/scripts/probe_table_loaders.dbg` and the run output.
- Neither loader fires and the EIP is `0x1e1c40` again, so the game never
  reaches the race start and the key sequence is the open question, not the
  breakpoint mechanism.
  Evidence: the probe run.

## The table consumers are not visible in the code

- The `PERSPTAB` resource pointer is read by `FUN_0008bc7c`, which stores it in
  the globals `0x83664` and `0x8abd3` and calls `GlobalDescriptorTableRegister`,
  so the function sets up a descriptor and not the table read.
  Evidence: the decompiled `FUN_0008bc7c`.
- The two globals are written by `FUN_0008bc7c` and read by no analyzed
  function, and the `VIS.TAB` buffer at `0x8c1c4` is read only by the loader
  `FUN_0004d030`, so neither table has a visible reader.
  Evidence: the reference lists of `0x83664`, `0x8abd3`, and `0x8c1c4`.
- The meaning of `TABS/VIS.TAB` and `TABS/PERSP01.TAB` therefore stays open:
  the loaders are known and the read order is not.
  Evidence: the two reference lists.
- The next step is a run-time watch on the two buffers with `BPLM` at the linear
  addresses, which gives the read order that the static code hides.
  Evidence: the protected-mode breakpoint method.

## The difficulty ladder holds in every level

- The `OPPONENTS` ladder holds in all six levels: the `A` variant starts at
  `max_spd` 14000 and `accel` 1100, `B` at 15000 and 1500, `C` at 16700 to
  16900 and 2600, and `E` at 19000 to 20000 and 6800, so the order A < B < C < E
  holds, and `skill` falls from 3500 to 2600 to about 2700 to about 1800.
  Evidence: the `OPPONENTS` lists of the six containers.
- Every level holds one single record `D` variant with a hard opponent, from
  `max_spd` 15300 to 16200 and `accel` 3000.
  Evidence: the `D` variants of the six levels.
- The `E` variant holds no sway in every level: the `sband` is 0.0, and the
  `brakes` alternate 2000 and 1500.
  Evidence: the `E` variants of the six levels.
- The `TRACK_DATA` `bonus_time` and `carnage` fall from `A` to `C` and hold in
  `C`, `D`, and `E` in every level, and the `color` and the `camera_hi` hold
  inside one level.  The `color` differs per level: 24 in `MAP1`, 20 in `MAP2`, 24
  in `MAP3`, 0 in `MAP4`, 104 in `MAP5`, and 13 in `MAP6`.
  Evidence: the `TRACK_DATA` records of the six levels.
- The `OSTACLES` and the `ANIMATIONS` lists differ per level and not per
  variant: the obstacle count is 9, 6, 12, 9, 6, and 9, and the animation count
  is 5, 2, 3, 5, 3, and 1 for the six levels.
  Evidence: the shared members of the six containers.

## The five track variants differ in the opponents and the track data

- The `OSTACLES` list is shared: it holds the nine 3DX names `azcono`,
  `azbunny`, `azbirill`, `azstac`, `azlil`, `aztnt`, `az1porta`, `azgirl`, and
  `azpagli`, and it sits in the shared `MAP1DAT.DTR`, so the five variants place
  the same obstacles.
  Evidence: the `OSTACLES` list of `MAP1DAT.DTR`.
- The `ANIMATIONS` list is shared too: the five records are `awtruck` 16000 0 0,
  `awbus` 15000 1 0, `awtruck` 10000 2 0, `awbus` 18000 3 0, and `ajet` 7000 4
  1, so four animations serve the five variants.
  Evidence: the `ANIMATIONS` list of `MAP1DAT.DTR`.
- The `OPPONENTS` list is the per-variant difference.  `MAP1A` holds a nine
  record ladder from `14000, 1100, 2000, 3500, 0.0003`, `MAP1B` from
  `15000, 1500, 2000, 2600, 0.0003`, `MAP1C` from `16800, 2600, 2000, 2800,
  0.0003`, and `MAP1E` from `20000, 6800, 2000/1500, 2000, 0.0`, so the field
  rises from A to E.  `MAP1D` holds one record `16000, 3000, 2000, 3800,
  0.0002`.
  Evidence: the `OPPONENTS` lists of the five members.
- `MAP1E` is the hardest variant: the `max_spd` runs to 20000, the `accel` to
  6800, the `brakes` alternate 2000 and 1500, and the `sband` is 0.0, so the
  opponents hold the line.
  Evidence: the `MAP1E.DTR` opponents.
- The `TRACK_DATA` field `bonus_time` and `carnage` fall from A to C: 50 and 20
  in `MAP1A`, 40 and 15 in `MAP1B`, and 35 and 10 in `MAP1C`, `MAP1D`, and
  `MAP1E`, and the `color` 24 and the `camera_hi` 300 hold in all five.
  Evidence: the `TRACK_DATA` records of the five members.
- The standalone `DATA/MAP1.DAT` is exactly the `MAP1C.DTR` variant: the expanded
  text is identical, so the loose file is one variant of the container.
  Evidence: the text compare of `DATA/MAP1.DAT` and `MAP1C.DTR`.

## The list fields are parsed and the record sizes agree

- The field type comes from the first byte of the field: `s` is a string of the
  size 12 in the record, and `d`, `f`, and `x` hold four bytes.  The record size
  from the parse matches the loader size for every known list: `OSTACLES` 0x24,
  `ANIMATIONS` 0x18, `OPPONENTS` 0x14, `TRACK_DATA` 0x10, and `CARS_DATA`
  0x40.
  Evidence: `python3 tools/gfx_container.py list DATA/MAP1I.DAT MAP1A.DTR OPPONENTS`.
- The `OSTACLES` record holds three string slots and the `3dx_name` header names
  the slot, so the record is `3 * 12` and the field name repeats.
  Evidence: the `OSTACLES` records of `MAP1DAT.DTR`.
- The `ANIMATIONS` record is a 12 byte name, a float speed, and the integers
  `spln` and `sound_fx`, and the `MAP1DAT.DTR` records are `awtruck` 16000, 0, 0
  and `awbus` 15000, 1, 0.
  Evidence: the `ANIMATIONS` records of `MAP1DAT.DTR`.
- The `OPPONENTS` record is the five floats `max_spd`, `accel`, `brakes`, `skill`,
  and `sband`, and the `MAP1A.DTR` records run `14000, 1100, 2000, 3500, 0.0003`
  down to `13200, 300, 2000, 3500, 0.0004`.
  Evidence: the `OPPONENTS` records of `MAP1A.DTR`.
- The `dump` command parses every known list of a text member, so the field values
  are readable from one command.
  Evidence: `python3 tools/gfx_container.py dump DATA/MAP1I.DAT --out work/map1i`.

## The index lists are decoded

- The container read path is confirmed by the dump: every compressed member
  reaches its recorded end field, so the reader is `member_read` in
  `tools/gfx_container.py`, which expands a member only when the flag bit 1 is set.
  Evidence: `python3 tools/gfx_container.py dump DATA/MAP1I.DAT --out work/map1i`.
- The level index holds the lists `OSTACLES` (`3dx_name`), `ANIMATIONS`
  (`name`, `speed`, `spln`, `sound_fx`), and `SOUND_FX` (`name`, `volume`,
  `loop`, `loop_start`, `loop_leng`, `pitch`, `pan`) in the member `MAP1DAT.DTR`.
  Evidence: the text of `work/map1i/MAP1DAT.DTR`.
- The track variants hold the lists `TRACK_DATA` (`color`, `camera_hi`,
  `bonus_time`, `carnage`) and `OPPONENTS` (`max_spd`, `accel`, `brakes`,
  `skill`, `sband`) in the members `MAP1A.DTR` to `MAP1E.DTR`, so the level has
  five track variants.
  Evidence: the text of `work/map1i/MAP1A.DTR` to `MAP1E.DTR`.
- The `GEN.DAT` members are `CARS.DTR` (the `CARS_DATA` list), `MENU.DTR` (the
  credits), and the sound bank members, and every one reaches its end field.
  Evidence: `python3 tools/gfx_container.py dump DATA/GEN.DAT --out work/gen`.

## The index file is a container, and the game decompresses it

- The data files are the same container format as the graphics files, with the
  directory at the end: the open reads the last four bytes as the directory size,
  seeks back by the size, and reads the 0x20 byte records until a name matches.
  Evidence: the decompiled `FUN_0004c128` and `FUN_0004c08d`.
- `DATA/MAP1I.DAT` holds 15 members: the `MAP1A` to `MAP1D` `.DTR` members
  with the flag 3, and the `MAP1ANM`, `MAP1CAM`, `MAP1OPP`, `MAP1OST`, `MAP1CNG`,
  `MAP1RPL`, and `MAP1SLA` `.SPL` members and the `.MRK` members with the flag 0.
  Evidence: `python3 tools/gfx_container.py info DATA/MAP1I.DAT`.
- The flag at `file + 0x10` is bit 1 of the record flags: the open sets
  `param_1[4] = flags & 2` for the compression and `param_1[5] = flags & 1` for the
  line mode.
  Evidence: the decompiled `FUN_0004c128`.
- The decompressing read at `FUN_0004c4e5` tests `param_1[4]`: zero runs the raw
  read at `0xc607b` and a non-zero value runs the LZW expander at `0x4d3fc`.
  Evidence: the decompiled `FUN_0004c4e5`.
- The `MAP1DAT.DTR` member holds the flag 3, so the compression bit is set and
  the game decompresses it with the LZW expander, and the `.SPL` and `.MRK`
  members hold the flag 0 and the game reads them raw.
  Evidence: the member list and the decompiled `FUN_0004c4e5`.
- The five calls read the same member `MAP1dat` of the container `MAP1i` with the
  list names `OSTACLES`, `TRACK_DATA`, `OPPONENTS`, and `ANIMATIONS`, so the
  container holds the lists of the level.
  Evidence: the disassembly of `0x1329e` to `0x1330c`.

## The index file stops at an invalid code

- The LZW reader records the stop reason: `end-code` for the code `0x101`,
  `invalid-code` for a code that is not in the table with no previous entry, and
  `out-of-bits` for the end of the bit stream.
  Evidence: the `_Lzw.stop_reason` field and the `lzw` command output.
- The six index files stop at an `invalid-code` from every start offset of 0, 4, 8,
  and 12, with two exceptions: `MAP2I.DAT` and `MAP5I.DAT` from offset 4 stop at
  an `end-code` after 37 and 23 codes.
  Evidence: the stop reason test on the six files.
- A valid LZW stream ends at the end code, so the index files are not plain LZW
  streams.  The two short end codes are accidents of the bit pattern: 50 and 29
  bytes from files of 19347 and 40632 bytes.
  Evidence: the stop reasons against the file sizes.
- The `DATA/MAP1.DAT` track file stops at an `end-code` after 234 codes and 459
  bytes, which confirms the reader and the reason field.
  Evidence: `python3 tools/gfx_container.py lzw DATA/MAP1.DAT`.

## The index file is not a plain LZW stream

- The plain LZW reader consumes the whole `MAP1I.DAT` and stops at the end of the
  bits, not at an end code: 75 codes and 115 bytes from the start, 57 codes and
  97 bytes from offset 4, 217 codes and 381 bytes from offset 8, and 25 codes and
  53 bytes from offset 12.  Every reading is short and holds binary values.
  Evidence: the forced-past test on `DATA/MAP1I.DAT`.
- A plain LZW stream of 18940 bytes cannot hold 115 bytes of output, so the file
  is not a plain LZW stream and the obstacle reading stays open.
  Evidence: the expansion ratios.
- The loader opens the file in text mode and reads lines with `*` comments and
  `END_LIST` terminators, and the file holds 2 to 27 records of the 0x24 byte
  size, so the file is text under some compression that is not the member LZW.
  Evidence: the disassembly of `0x1547a` to `0x154ba` and the first dwords.

## The index reader and the member name

- `FUN_00015460` builds the path `dir\data\file.dat` from a runtime directory
  global and the first argument, and builds the member name `arg2.dtr`.
  Evidence: the disassembly of `0x1547a` to `0x154ba`.
- The first call in `FUN_000131a8` passes the index name `MAP1i` as the file and
  `MAP1dat` as the member base, and the array `OSTACLES` (0x24 records of 0x24
  bytes) as the destination, so the index reader is the obstacle reader and the
  index holds the track obstacles.
  Evidence: the disassembly of `0x1329e` to `0x132aa`.
- The list reader builds the file `dir\data\file.dat` with the directory global
  and the file argument, so the index is `MENU\data\MAP1i.dat`, the shipped
  `DATA/MAP1I.DAT`.
  Evidence: the disassembly of `0x1547a` to `0x15491`.

## The car record and its consumer

- The `CARS_DATA` list names every field in its header line: `filename` (the
  type `s`), `animation` (the type `x`), `maxspd`, `maxrpm`, `grip`, and
  `accel` (the type `f`), `shiftmode` and `n_shifts` (the type `d`), and
  `sh_rap0` to `sh_rap5` (the type `f`).
  Evidence: the `CARS_DATA` header of `DATA/GEN.DAT`.
- The first record is the Lamborghini: `sAUTO00`, `x0`, `f21000`, `f9000`,
  `f1.05`, `f1000`, `d1`, `d6`, then `f270`, `f190`, `f130`, `f85`, `f55`,
  `f40`.
  Evidence: the first record of `DATA/GEN.DAT`.
- The record is 0x40 bytes: the filename holds eight bytes at `+0x04`, the
  animation holds four bytes at `+0x0c`, and the twelve numeric fields hold
  four bytes each from `+0x10` to `+0x3f`.
  Evidence: the 0x40 record size at `0x13318` and the field order.
- `FUN_000136cc` is the car record consumer.  It copies the whole 0x40 byte
  record to the car object at `+0x470`.
  Evidence: the disassembly of `0x136e6` to `0x13710`.
- The consumer reads the record fields at fixed offsets: `+0x474` is the filename
  (the record `+0x04`), `+0x47c` is the animation (the record `+0x0c`), and
  `+0x468` is the animation name (the record `+0x00`).
  Evidence: the disassembly of `0x13713` to `0x1379a`.
- The animation field is a flag: zero selects the letter `A` and a non-zero value
  selects the letter `B`, so the two liveries share one car.
  Evidence: the disassembly of `0x13716` to `0x13728`.
- The consumer builds the cockpit name `COCK%s%c` from the filename and the
  letter, and again with the letter `H`, so the members are `COCK00AL`,
  `COCK00AH`, `COCK00BL`, and `COCK00BH`, which are the shipped members of
  `GFX/GAME.DAT`.
  Evidence: the disassembly of `0x13729` to `0x13764` and the member list.
- The consumer reads the animation name with `FUN_00011060` against the list at
  `0x106530`, which is the `ALETTER` list, and then reads the list `COCK%s%c`
  through `FUN_00015460("GEN", "CARS", ...)` with 0x74 records of 0x74 bytes.
  Evidence: the disassembly of `0x13787` to `0x13782`.
- The `FUN_00015460` file arguments are the directory and the file name, so the
  path is `dir\data\file.dat`.  The calls pass the directory `CARS` or `MENU` and
  the file `GEN`, so the shipped `DATA/GEN.DAT` holds the lists `CARS_DATA` and
  the 15 more, and the member argument selects the list.
  Evidence: the disassembly of `0x13778` and the `GEN.DAT` list names.

## The track index and the level 5 record

- The `MAP*I.DAT` files hold 16-bit signed values.  The first dword is a
  count: 2 for `MAP1I.DAT` and `MAP2I.DAT`, 17 for `MAP3I.DAT`, 18 for
  `MAP4I.DAT`, 20 for `MAP6I.DAT`, and 27 for `MAP5I.DAT`.
  Evidence: the first words of each file.
- The file holds records of the shape `1, 0, X, f1, Y, f2, Z, 0`, where the
  flags `f1` and `f2` are `-1` or `-2`, and the third flag is `0`.  `MAP1I.DAT`
  starts `1, 0, 9301, -1, 5154, -1, 2300, 0`, then `1, 0, 7373, -1, 3174, -1,
  0, 0`, then `1, 0, 11241, -1, 7147, -1, 0, 0`.
  Evidence: the first 40 words of `MAP1I.DAT`.
- The file ends with one 32-bit value: 512 in `MAP1I.DAT` and 544 in
  `MAP5I.DAT`.
  Evidence: the last 16 words of each file.
- `MAP5I.DAT` holds 20316 words: 2 header words and 2902 records of 7 words.
  Evidence: the size and the record shape.
- The values look like coordinates and the flags look like a mode, so the exact
  field meanings stay open.
  Evidence: the value range and the flag pattern.

- `MAP5.DAT` is not LZW.  It holds 41224 bytes: the count 23 and the size
  `0x10cc` in the header, and then 23 records of `0x700` bytes, because
  `23 * 0x700 + 8` is 41224, the file size.
  Evidence: the header and the file size.
- The `MAP5.DAT` record holds a mix of 32-bit floats and 32-bit integers.  The
  first record starts `0.0, 1.0, 0.0, 2, -44376, 0.0, 104296, 4300, 0.0, 0.9,
  0.94, 0.0, 67985, 0.0`.
  Evidence: the first record of `MAP5.DAT` as 32-bit values.
- The `MAP5.DAT` and `MAP5I.DAT` pair is the level 5 pair, and level 5 is the
  largest track.  The exact record layout of both stays open.
  Evidence: the file sizes against the other levels.

## The LZW reader was the last defect in the asset decode

- The member expansion produced the correct length for every member, but the
  `LOGOH.SPR` sprite rendered the first rows and noise after them, and every
  font
  glyph rendered as noise.
  Evidence: the render of `LOGOH.SPR` and of the font members.
- The first port emulated the 0x400 byte buffer of the game reader at `0x4d844`,
  which refills the buffer when fewer than five bytes remain.  The port diverged
  at
  the first refill, and the divergence is the point where the sprite render
  changes
  from the logo to noise.
  Evidence: the render and the reader code.
- The plain LSB-first bit stream, read one code at a time with the width from 9
  to 12
  bits, the clear code 0x100, the end code 0x101, and the first free entry
  0x102,
  renders `LOGOH.SPR` sprite 0 as the Virgin Interactive Entertainment logo, the
  Graffiti logo, and the SCREAMER logo, and it renders every font as a readable
  character set.
  Evidence: the render of `LOGOH.SPR` and of the fonts.
- The `tools/gfx_container.py` reader is now the plain one.  Every sprite and
  font
  member of `GFX/GRAF.DAT` and `GFX/GAME.DAT` expands to its member end field
  exactly.
  Evidence: the expansion test.
- The review of the re-exported assets confirms them: the sprite sheets hold the
  logos and the cars, the character sheets hold the complete character sets, and
  the PCX images render correctly.
  Evidence: the user review.

## The font member layout is confirmed

- The font loader at `0x25658` reads the count byte, `count + 1` descriptors of
  12
  bytes, one glyph table per descriptor of `num_chars` entries of 8 bytes, the
  tail
  size of 4 bytes, and the tail.
  Evidence: the disassembly of `0x25658`.
- The descriptor is the glyph height, the first character code, and the
  character
  count.  `MENUH.FNT` holds `(22, 32, 76)`, `(18, 32, 76)`, `(38, 32, 76)`, and
  `(60, 32, 76)`.
  Evidence: the font command.
- The glyph table entry is the glyph width in pixels and the offset of the glyph
  in
  the tail, and the glyph holds `height` rows of `width` pixels packed four bits
  per
  pixel, so the size is `ceil(width / 2) * height`.  For `ATTRH.FNT` the first
  entries are `(19, 0)`, `(13, 380)`, `(13, 646)`, and `(27, 912)`, and 380 is
  19
  divided by 2 rounded up times 38.
  Evidence: the font command.
- The character sheets render the complete ASCII set from the space to `z` with
  the
  accented characters `É`, `Ñ`, `¿`, `¡`, `À`, `Â`, `È`, `Ê`, `Ë`, `Î`, `Ï`,
  `Ô`,
  `Õ`, `Ù`, `Û`, and `Ü`.  The game fonts with the first code 48 and the count
  10 or
  11 hold the digits and the colon.
  Evidence: the character sheets of `MENUH.FNT` and `GAMEH.FNT`.
- The descriptor third field 76 is the character count and not a glyph size, and
  the
  second field 32 is the first character code and not a stride.  The earlier
  stride and glyph size readings were wrong because the LZW expansion was wrong.
  Evidence: the character sheets and the font command.

## Identity

| Fact | Value | Evidence |
| --- | --- | --- |
| Game name | Screamer | GOG metadata file, package name |
| GOG game id | 1207658726 | `goggame-1207658726.info` |
| GOG build | 34378 | package file name |
| Language | English (en-US) | `goggame-1207658726.info` |
| Installer author | GOG Sp. z o.o. | package signature |

The package signature chain is `Developer ID Installer: GOG Sp. z o.o.
(9WS36Q8886)`, issued by the Apple Developer ID Certification
Authority.

## Distribution

- The package `screamer_enUS_1_0_34378.pkg` is a XAR archive.  Its
  SHA-256 digest is
  `5e3717a3f61519a81cdee8e3b81acb7cc45e384d33ca1835d60f4e2ce5432c6f`.
  Evidence: REA `inventory_artifact` record
  `ev_eba4e5c89f9c945420917e733a3f70a7e624f21e54bcbcec0603f19db163edd5`.
- REA reports the package root hash only.  Child inventory needs a native
  macOS adapter.  Evidence: REA limitation text.
- The package holds a gzip tar payload of 451194368 bytes.  The tar holds
  106 entries under `payload/`.
- The payload root is the `Screamer.app` bundle.  The installed copy is
  `~/Documents/Screamer.app`.
- The installed `Launcher` matches the payload `Launcher`.  Both digests are
  `d2663b471ae64f7c7ac5aad90370f0110c21be1a161a15abb3ac18e9c0f3356b`.
- The `postinstall` script moves the payload into the target directory,
  clears the quarantine attribute, and changes ownership.  The payload holds
  no `support/` directory, so the support script loop runs on nothing.
- `Contents/Info.plist` names the bundle "The Dame Was Loaded" with the
  identifier `com.gog.the_dame_was_loaded`.  The GOG metadata file names
  the game "Screamer".  The bundle metadata is stale.

## Executables

- `STARTH.EXE`, `STARTL.EXE`, and `SETUP.EXE` are LE (linear
  executable) images with a 16-bit MZ stub.  The LE header sits at file
  offset `0x2a50`.  Evidence: `tools/le.py info` output and `file` output.
- The header reports CPU 80386, OS type OS/2, and module type EXE.
- Each image holds three objects: a code object, a small data object, and a
  stack object.
- `STARTH.EXE` and `STARTL.EXE` share the module name `start`.  The
  resident name table holds one symbol, `start`, with ordinal 0.
- The run-time banner is `WATCOM C/C++32 Run-Time system. (c) Copyright
  by WATCOM International Corp. 1988-1994. All rights reserved.`  The game
  therefore uses the Watcom C/C++ 32-bit toolchain.
- The programs use the DOS/4GW protected-mode extender.  The runtime code
  carries the string `dos4gw.exe`.
- The page mapping is confirmed: the object page table numbers run in order
  from 1 to 227, and the last page ends at the end of the file.  Evidence:
  the page table values and the file size `0xfcaab`.
- The fixup page table confirms the header layout: the last entry equals
  `0x17485`, and `frectab + 0x17485` equals the import module table
  offset `0x17cb7`.
- `STARTH.EXE` and `STARTL.EXE` are two builds of one program.  They
  differ in 405284 bytes and in 4 bytes of size.
- `SETUP.EXE` is the setup and configuration program.  It reads and
  writes `CONFIG/SETUP.DAT` and the `CONFIG/*.CFG` files.
- The first executable code in the code object sits at file offset
  `0x1a818`.  The code object starts with a trap stub and NOP padding.

## Runtime

- The sound support comes from HMI (Human Machine Interfaces, Inc.)
  drivers.  The banners name the Gravis Ultrasound and the OPL2.
- `DATA/MUSIC.DAT` and `CONFIG/SETUP.DAT` carry the `ADLIB-` signature
  and instrument names such as `<Blank`, `$clap`, and `$rimshot`.
- The single-player path runs `STARTH.EXE`.  The client and server paths
  run the same executable with an IPX network configuration.

## Entry point

- The LE header entry value is an offset inside the start object.  The file
  offset follows this rule:

  ```text
  file_offset = data_pages_offset
              + (page_table_index - 1 + entry_offset // page_size) * page_size
              + entry_offset % page_size
  ```

  Evidence: `tools/le.py` and the run-time log below.
- The entry code is a short jump over the Watcom run-time banner string:
  `jmp short +0x76` at `0x03a8f8`, then `sti`, `and esp,-4`,
  `mov ebx,esp`, `mov ah,0x30`, `int 0x21`.  Evidence: `tools/le.py
  disasm STARTH.EXE 0x300f8`.
- All three executables share the entry pattern.  The entry file offsets are
  `0x3a8f8` (`STARTH.EXE`), `0x3a6b8` (`STARTL.EXE`), and `0x20628`
  (`SETUP.EXE`).
- A DOSBox-X instruction log of `STARTH.EXE` shows the game code start at
  selector `0x0150`, EIP `0x1f20f8`, with all registers zero.  The first
  instructions are `jmp short 001f2170`, `sti`, `mov ebx,esp`, and `int 21`
  with `EAX=0x3000`.  Evidence: `tools/entry_probe.py` and the log line
  `0150:1F20F8  jmp  short 001F2170 ($+76)  EAX:00000000 ...`.
- The run-time module load base is `0x1c2000`: `0x1f20f8 - 0x200f8 =
  0x1c2000`.  The module-relative entry is `0x300f8`, which equals the
  object base `0x10000` plus the header entry `0x200f8`.  Evidence: the log
  line above and the LE object table.
- The DOS4GW stub of `STARTH.EXE` loads the extender as an overlay:
  `int 21` with `EAX=0x4b03` (AH `0x4b`, AL `0x03`).  Evidence: the
  first log line `9fe0:00000002  int  21  EAX:00004B03`.

## Program map

- The program `main` sits at module address `0x10020` (object offset
  `0x20`).  The startup at
  `0xd4324` loads `argc` from `[0x27e24]` and `argv` from `[0x27e28]`
  and calls `main` with EAX=`argc` and EDX=`argv`.  Evidence:
  `tools/le.py calls STARTH.EXE 0x10020` and
  `tools/le.py disasm STARTH.EXE 0xd4324`.
- The run-time log confirms the path.  The log line
  `0150:296359  mov  edx,[002D5E28]` matches the file instruction
  `0x000d4359  mov edx, dword ptr [0x27e28]`, and the next game
  instruction is `0150:1D2020  push ebx` (`main`).  The object 1 load base
  is `0x1c2000`.  Evidence: `work/entry/run/LOGCPU.TXT` from
  `tools/entry_probe.py`.
- The entry path is `0x300f8` (jump) to `0x30170` (startup) to `0xd4324`
  (main invoker) to `0x10020` (`main`).  Evidence:
  `tools/le.py tree STARTH.EXE 0x300f8 --depth 3`.
- `main` holds 56 instructions and 15 calls.  The program calls are at
  `0x2ecf8`, `0x2f8ca`, `0x127a0`, `0x27700`, `0x108e4`, `0x10978`,
  `0x27f94`, `0x128ac`, and `0x2ed77`.  Evidence:
  `tools/le.py tree STARTH.EXE 0x10020 --depth 1`.
- See [Program map](../reference/program-map.md) for the addresses.

## Race loop and frame update

- `main` alternates `race_loop` at `0x27700` and `frame_update` at `0x27f94`.
  Evidence: the decompiled `FUN_00010020`.
- `race_loop` starts the race with `race_start` at `0x27770`, runs the inner
  loop (`motion_update`, `default_data_copy`, `joystick_update`) until the
  session state `DAT_0011ce44` reaches 4, and ends with `race_end` at `0x279a4`.
  Evidence: the decompiled `FUN_00027700`.
- `frame_update` clears the screen, runs `menu_space_init` and `menu_frame`, then
  loops on `frame_step` until the frame flag `DAT_0011d234` is set, and ends
  with
  `race_frame` and `race_cleanup`.
  Evidence: the decompiled `FUN_00027f94`.
- `frame_update` clears the screen with `memset(DAT_00121528, 0, 0x3e800)`.  The
  count `0x3e800` is 256000, which is 640 times 400, so the screen buffer is 640
  by 400 bytes.  Evidence: the decompiled `FUN_00027f94` and `FUN_0008c160`.
- `input_poll` at `0x26448` counts to 8 and reads the mouse and keyboard state.
  Both loops call it.  Evidence: the decompiled `FUN_00026448`.
- `default_data_copy` at `0x10193` copies 32 dwords from object 1 into object 2,
  the data object at `0xf0000`.  Evidence: the decompiled `FUN_00010193` and the
  object table.
- `race_start` at `0x27770` sets the 640x400 screen and copies the state
  globals.
  Evidence: the decompiled `FUN_00027770`.
- See [Program map](../reference/program-map.md) for the tables.

## The object pipeline and the race frame

- `object_update_all` at `0x2786c` walks `DAT_0010f8ec` records of 0x590 bytes
  at `DAT_00110ab8`.  Every record passes `object_transform` (`0x1cd44`),
  `object_player_input` (`0x1c760`), `object_physics` (`0x1c810`),
  `object_prepare` (`0x1c79c`), and `object_car_link` (`0x1c84c`).  The
  function then runs the seven subsystem calls.  Evidence: the disassembly of
  `FUN_0002786c` and the finding "The object_update_all module at 0x2786c".
- `object_transform` writes the world position at `+0x498`, `+0x49c`, and
  `+0x4a0`
  and the float fields at `+0x4a4` to `+0x4ac` from the position and the
  velocity.
  Evidence: the decompiled `FUN_0001cd44`.
- `object_player_input` reads the input bits `DAT_00118e1c` and the state at
  `+0x2f8`; `object_physics` and `object_prepare` run only when the state at
  `+0x414` is set.  Evidence: the decompiled `FUN_0001c760`, `FUN_0001c810`, and
  `FUN_0001c79c`.
- `object_car_link` at `0x1c84c` indexes the 28-byte record tables
  `DAT_0010653c` and `DAT_00106540` with the object field `+0x4dc`.  The 28-byte
  record size matches the list created by `list_create(engine, 150)`.
  Evidence: the decompiled `FUN_0001c84c`.
- `race_frame` at `0x2807c` copies the back buffer `DAT_0011fad0` into the video
  buffer `DAT_00121528` with `memcpy` at `0x8c150`, so `DAT_00121528` is the
  screen buffer and `DAT_0011fad0` is the back buffer.
  Evidence: the decompiled `FUN_0002807c`.
- `video_block_swap_a` at `0x1075f` and `video_block_swap_b` at `0x107a1`
  byte-swap every dword and swap two regions of the screen buffer.  The flags
  `0x10` and `0x20` of `DAT_0011d274` gate them.  The purpose is open; the
  rear-view mirror and the split screen are candidates.
  Evidence: the decompiled `FUN_0001075f` and `FUN_000107a1`.
- `race_state_update` at `0x29564` switches on `DAT_0011fac4`.  Case 1 advances
  `_DAT_0012038c` by the frame delta and copies `DAT_0011facc` into
  `DAT_0011fac4` and `DAT_001203b8`.  The value `DAT_001203b8` then selects
  the overlay in `race_frame`.  Evidence: the decompiled `FUN_00029564` and
  `FUN_0002807c`.
- `fade_update` at `0x29818` ramps `DAT_0011d224` toward 1.0 or down to 0.0 by
  the frame delta, so `DAT_0011d224` is a fade level and `DAT_0011fac8` is the
  direction.  Evidence: the decompiled `FUN_00029818`.
- `race_frame` sets the frame flag `DAT_0011d234` when
  `_DAT_00100ee0 < _DAT_0011d24c` and `DAT_0011fac4` is 0.  `frame_update`
  loops until that flag is set.  Evidence: the decompiled `FUN_0002807c` and
  `FUN_00027f94`.
- `fp_helper` at `0x4d9f4` is a Watcom floating-point helper.  It takes no
  arguments in the decompiled output and appears between float operations.
  Evidence: the decompiled `FUN_00021e6c` and `FUN_0004d9f4`.

## Subsystem initialisation

- `engine_init` at `0x127a0` runs the subsystem setup in this order:
  `config_load`, `list_file_load`, `setup_load`, `controls_load`, `mouse_init`,
  `keyboard_hook`, `palette_build`, `video_buf_load`, `vesa_set_mode`,
  `hmi_sound_init`, `engine_interface_init`, and `stream_layer_init`.
  Evidence: the decompiled `FUN_000127a0` and its callees.
- `config_load` at `0x1bf38` reads `screamer.cfg`.  On failure it reports
  `Files damaged. Re-install Screamer.` and exits with `-1`.  Evidence: the
  decompiled `FUN_0001bf38`.
- `setup_load` at `0x1bb58` reads `CONFIG/SETUP.CFG` into a 144-byte
  structure, which matches the shipped file size.  On failure it reports
  `Run SETUP before.`.
  Evidence: the decompiled `FUN_0001bb58` and `docs/reference/inventory.md`.
- `keyboard_hook` at `0x14e20` installs a handler on interrupt 9 and drives the
  keyboard LEDs through port `0x60`.  `engine_init` calls it with `1` and
  `engine_shutdown` with `0`.  Evidence: the decompiled `FUN_00014e20`.
- `vesa_set_mode` at `0x8bdec` calls the DOS/4GW real-mode interrupt
  dispatcher with `0x4f01`, the VBE get-mode-information call.  Evidence: the
  decompiled `FUN_0008bdec`.
- `mouse_init` at `0x1f1a0` calls interrupt 33h.  Evidence: the decompiled
  `FUN_0001f1a0`.
- `palette_build` at `0x15720` fills a 50-entry table with floating-point ramp
  values.  Evidence: the decompiled `FUN_00015720`.
- See [Program map](../reference/program-map.md) for the table.

## The .3dx chunk layout

- The object records are 28 bytes.  The record holds the object name at `+0x00`
  and the load flag at `+0x14`; the value 1 selects the record for loading.
  The 28-byte size matches the engine list from `list_create(engine, 150)`.
  Evidence: the decompiled `FUN_00010bfc`.
- The loader loads the `3D_HEADER` chunk and the `3D_BLKOBJ` chunk through the
  resource layer and then calls `object_init_from_file`.
  Evidence: the decompiled `FUN_00010bfc`.
- `object_init_from_file` at `0x4cc10` fills the chunk table of the object
  record.  The table holds eight words per chunk: the file record, the chunk
  count, and the six words of the chunk header.  The table ends with -1.
  Evidence: the decompiled `FUN_0004cc10`.
- The file header holds the chunk count at `+0x14` and the chunk offset table at
  `+0x18`, one 32-bit file-relative offset per chunk.  A chunk offset points
  12 bytes before the chunk header, so the six chunk words lie at `+0x10`,
  `+0x14`,
  `+0x18`, `+0x1c`, `+0x20`, and `+0x24` of the chunk.
  Evidence: the decompiled `FUN_0004cc10`.
- The `3D_HEADER` chunk holds the object count at `+0x14`.  The `3D_BLKOBJ`
  chunk holds a 0x60-byte header and `count` block objects of 0x20 bytes, so the
  size is `count * 0x20 + 0x60`.  Evidence: the decompiled `FUN_00010bfc`.
- The loader clears the six list words at `+0x24`, `+0x28`, `+0x2c`, `+0x30`,
  `+0x34`, and `+0x38` of the `3D_BLKOBJ` header.  Evidence: the decompiled
  `FUN_00010bfc`.
- The resource layer stages the loaded chunks in the main file.  The unit is 16
  bytes.  The layer asks the DOS service for a block through the DPMI real-mode
  interrupt path; the block number times 16 is the file offset.  The layer
  stores
  the buffer at chunk table entry `+0x0c` and the size at `+0x14`.
  Evidence: the decompiled `FUN_0002f0fc`, `FUN_0002f172`, and `FUN_0002f062`.
- The resource object holds the record array at `+0x00`, the loaded count at
  `+0x04`, the limit at `+0x08`, and the status at `+0x0c`; the status 2 marks
  the end.  Evidence: the decompiled `FUN_0002eed0`.
- The identity of the DOS service is open.  The layer passes the values `0x100`
  and `0x501` in the request and calls the interrupt `0x31`.
  Evidence: the decompiled `FUN_0002f0fc` and `FUN_0002f172`.

## The graphics decoder

- `pcx_load` at `0x1b5a4` decodes a PCX member.  It builds the member name
  with the format `%s\%s` and opens it in the mode `rb`.
  Evidence: the decompiled `FUN_0001b5a4` and the strings at `0x1007f80`,
  `0x1009f5`, and `0x1009f5`.
- The loader seeks 0x300 bytes before the end of the member and reads the
  256-colour palette.  It then shifts every palette byte right by 2, so the
  palette becomes the 6-bit VGA form.  Evidence: the decompiled `FUN_0001b5a4`.
- The loader rewinds, reads the 128-byte PCX header, and computes the pixel
  count as `(xmax - xmin + 1) * (ymax - ymin + 1)`.  It then decodes the RLE
  runs: a byte with the mask `0xc0` equal to `0xc0` repeats the next byte
  `byte & 0x3f` times.  Evidence: the decompiled `FUN_0001b5a4`.
- `GFX/GRAF.DAT` holds the PCX members `menu1h.pcx`, `menu2h.pcx`,
  `menu3h.pcx`, `menu4h.pcx`, `champ5h.pcx`, `load%d%c.pcx`, and
  `net%c.pcx`.  Evidence: the strings at `0x100fd5`, `0x101014`, `0x101054`,
  `0x101114`, `0x101132`, `0x101152`, and `0x1011bc`.
- `menu_space_init` at `0x29d08` loads the menu data through two loaders.
  The sprite loader at `0x257b0` reads two header blocks and then the `SPRITES`
  resource twice.  The font loader at `0x25658` reads a count, then per font a
  12-byte descriptor at `DAT_0011ca98`, then per font a 0x500-byte bitmap at
  `DAT_00119898`, and finally the `FONTS` resource.
  Evidence: the decompiled `FUN_000257b0` and `FUN_00025658`.
- The font record is 12 bytes and the font bitmap is 1280 bytes.  The loader
  increments the font count `DAT_00025440`.
  Evidence: the decompiled `FUN_00025658`.
- The sprite structure is decoded from the disassembly of `0x257b0`: the head of
  the file holds the sprite count at `+0x00` and the pixel data size at `+0x04`,
  then `count * 0x14` bytes of 20-byte sprite records fill the first `SPRITES`
  resource, and the pixel data fills the second.  The loader reads 4 bytes into
  `DAT_00025cd4`, 4 bytes into `DAT_00025cdc`, then `count * 0x14` bytes, then
  the size in bytes.  A missing file closes the stream and calls `exit(0)`.
  Evidence: the disassembly of `0x257b0` at `0x257cd` to `0x258b4`.
- Verification against `GFX/GAME.DAT`: the first dword is `0xfbf05b00`, which is
  not a plausible sprite count for a file of 637267 bytes, and the head bytes do
  not hold text.  The file does not match the plain sprite structure at its
  head.
  Evidence: the byte scan of `GFX/GAME.DAT`.
- Verification against `GFX/MENU.DAT`: the first dword is `0x03000000` and the
  first word is 0, so the file does not match the plain sprite structure either.
  Both files are compressed or encoded, so the record structures are the layouts
  of the loaded resources, not of the file bytes.
  Evidence: the byte scans of both files.
- The encoding of both files is decoded.  Every byte is exclusive-ORed with
  `0x3a`,
  and the decoder at `0x4c128` reads the file from the end: it seeks 4 bytes
  before the end, reads the decompressed size, seeks back by the size plus 4
  bytes, reads 32-byte blocks forward, exclusive-ORs every byte with `0x3a`, and
  decodes each block through `0xd9ba`.  Evidence: `FUN_0004c128`.
- The block header is the first byte of a block; the value 0 ends the data.  The
  bytes at `+0x1a` and `+0x1c` are two 16-bit lengths whose sum is the
  decompressed block size, and the byte at `+0x1d` holds two flags in bit 0 and
  bit 1.  A member name follows at `+0x58`.  Evidence: `FUN_0004c128`.
- The decode of the file tails confirms the encoding.  `GFX/GAME.DAT` returns
  the
  member names `TEMPLET_L.PCX`, `GAMEH.FNT`, `L.FNT`, and `H.FNT`.
  `GFX/MENU.DAT` returns `MENU.COL`, `SHADOW.3DX`, `AUTO06.3DX`, and
  `DOW.3DX`.  So the directory of a file sits at the end, before the 4-byte
  decompressed size.  Evidence: the exclusive-OR decode of both tails.
- The decoder `0x4c128` is a member search, not a block decoder.  It reads
  32-byte blocks from the end and compares each block with the wanted member
  name through the case-insensitive compare `0x4d9ba`.  The open wrapper
  `0x4c08d` calls the decoder, so the file layer decodes a file when it opens
  it.  Evidence: the decompiled `FUN_0004c128`, `FUN_0004d9ba`, and
  `FUN_0004c08d`.
- The last 4 bytes hold the size of the directory, and the directory holds
  `size / 32` records of 32 bytes.  A record holds the name at `+0x00`
  (26 bytes, exclusive-ORed with `0x3a`, NUL-terminated), a 16-bit block
  offset at `+0x1a`, a 16-bit block count at `+0x1c`, and two flags at
  `+0x1d`.  `GFX/GAME.DAT` holds 39 members and `GFX/MENU.DAT` holds 18.
  Evidence: the exclusive-OR decode of both files.
- The members of `GFX/GAME.DAT`: `IMPLET_L.PKC`, `IMPLET_H.PKC`,
  `IMPRES_L.PKC`, `IMPRES_H.PKC`, `CDFRONTL.PKC`, `CDFRONTH.PKC`, and
  `COCK00AL.PKC`, with 32 more.  The names form low and high resolution
  pairs.  Evidence: the exclusive-OR decode of the directory.
- The members of `GFX/MENU.DAT`: `MENU.COL`, `MENU.MLB`, `ALETTER.3DX`,
  `AMAP1.3DX` to `AMAP6.3DX`, `AAUTO00.3DX`, `AAUTO01.3DX`, and
  `AAUTO02.3DX`, with 7 more.  Evidence: the exclusive-OR decode.
- The directory record holds the member name at `+0x00` (16 bytes), the
  member offset at `+0x10`, the member size at `+0x14`, the end offset at
  `+0x18`, and a flag at `+0x1c`.  The offsets form one chain:
  `offset + size` of a member is the `offset` of the next member, and the last
  member ends at the start of the directory.
  Evidence: the decode of both directories.
- `GFX/GAME.DAT` holds the sprite members `GAMEL.SPR` (559712, 10010 bytes)
  and `GAMEH.SPR` (569722, 28736) and the font members `GAMEL.FNT`
  (598458, 11565) and `GAMEH.FNT` (610023, 25992), so the members that
  `sprite_load` and `font_load` read are present.
  Evidence: the exclusive-OR decode of the directory.
- `GFX/MENU.DAT` holds the palette `MENU.COL` of 768 bytes at the offset 0,
  which is 256 colours of 3 bytes.  Evidence: the decode of the directory.
- The decoder at `0x4c128` returns one member: it reads the directory size
  from the last 4 bytes, seeks back by the size plus 4 bytes, reads 32-byte
  records forward, exclusive-ORs every record byte with `0x3a`, and compares
  the record name with the wanted name.  So the exclusive-OR applies to the
  directory records and the member data is stored plain.
  Evidence: the decompiled `FUN_0004c128`.
- The read path of the member data is traced.  The read wrapper `0x4c4e5`
  tests the stream flag at `+0x10`: a clear flag reads the bytes raw and a set
  flag calls the expander `0x4d3fc`.  The expander reads one header byte and
  loops on the block decoder `0x4d4c4` until it returns 0.
  Evidence: the decompiled `FUN_0004c4e5` and `FUN_0004d3fc`.
- The block decoder `0x4d4c4` is an LZSS decoder.  It holds a ring buffer at
  `+0x4074` with a window of 0x200 bytes, a shift count at `+0x4068` that
  starts at 9 and grows to 12, and the output count at `+0x4070`.  The bit
  reader `0x4d844` returns `0x100` for a literal and `0x101` for the end.
  The shift count range 9 to 12 is the classic LZSS length coding.
  Evidence: the decompiled `FUN_0004d4c4` and `FUN_0004d844`.
- So the member data of `GFX/GAME.DAT` and `GFX/MENU.DAT` is LZSS-compressed.
- The bit reader `0x4d844` holds a bit buffer of 0x400 bytes at `+0x8086`, the
  bit position at `+0x406c`, and a mask table at `+0x8050`.  It returns the
  16-bit value at the bit position, shifted right by the low 3 bits of the
  position and masked with the mask of the shift count.  The mask table holds
  `(1 << n) - 1` for the shift counts 9 to 12.
  Evidence: the decompiled `FUN_0004d844`.
- The literal code `0x100` and the end code `0x101` mark a literal byte and the
  end of the stream.  Evidence: the decompiled `FUN_0004d4c4`.
- The confirmation of the sprite count at `+0x00` and the pixel data size at
  `+0x04` stays open.  Two run-time probes did not reach the loader: the race
  path at `0150:31AA1` did not hit within 300 ticks, and the menu path at
  `0150:3BDB9` did not hit within 45 seconds.  At the second probe the game
  sat in the input wait at the run-time address `0150:1E1C40`, which is the
  static address `0x21c40`.  The memory dumps of the four loader globals hold
  the values `0xdb` and `0x25`, not a plausible count or size.
  Evidence: the probe sessions `work/probe2.dbg` and `work/probe3.dbg`.
- `menu_space_init` loads the `NET_SPACE` resource and the network menu, so the
  single-player start may not call it.  The confirmation should therefore break
  at the race init call sites `0x13426` and `0x134b6` of `FUN_000131a8` and
- The container parser is ported to `tools/gfx_container.py`.  It lists the
  members of a graphics container and writes a raw member.  The offsets, the
  sizes, and the names of `GFX/GAME.DAT` and `GFX/MENU.DAT` match the earlier
  decode.  Evidence: the tool output.
- The LZSS port does not yet reproduce the member bytes.  The classic LZSS
  variants with the flags byte and the position and length pairs do not yield a
  plausible count or size for `GAMEL.SPR`, and the port of the decoded structure
  is not confirmed, so the tool reports the raw member.
  Evidence: the port attempt and the classic variant test.
- Correction to the run-time address formula.  The run-time address is the
  module
  address plus the module load base `0x1c2000`, not minus `0x10000`.  The entry
  at the
  module address `0x300f8` runs at `0150:1F20F8`, and `0x300f8 + 0x1c2000` is
  `0x1f20f8`, so the formula holds.  The earlier probes used the wrong formula
  and
  therefore set breakpoints at addresses that never run.  The corrected run-time
  addresses are the member open `0x20e08d` for the module `0x4c08d`, the decoder
  `0x20e128` for `0x4c128`, and the block decoder `0x20f4c4` for `0x4d4c4`.
  Evidence: the entry evidence and the probe sessions.
- The driven probe with the three corrected breakpoints did not hit any of them
  within a 24-second window: the member open `0150:20E08D`, the decoder
  `0150:20E128`, and the block decoder `0150:20F4C4`.  The game sat at the input
  wait `0150:1E1C40` both before and after the Enter key in that run, so the key
  effect depends on the menu state.  The member open therefore does not run
  while the
  menu draws, and the container member path needs a container load, not a menu
  draw.
  Evidence: the probe run with the corrected addresses.
- The fixed-mask hypothesis `table[shift] = 0x1ff` is implemented in
  `tools/gfx_container.py` and the port runs to completion: it turns the 10010
  member bytes of `GAMEL.SPR` into 7935 bytes.  The output head is zero, so the
  count at `+0x00` and the pixel size at `+0x04` still do not appear, and the
  hypothesis is not confirmed.  Evidence: the `head` output of the tool.
- Code breakpoints do not stop this DOSBox-X build, but interrupt breakpoints
  do.
  Six probes set a `BP <seg>:<off>` at a verified run-time address and none of
  them stopped the emulator: the member open `0150:20E08D`, the decoder
  `0150:20E128`, the block decoder `0150:20F4C4`, and the resource get
  `0150:1F0ED0`.  The interrupt breakpoint `BPINT 21 "3F"` stops at the file
  read, so the working probe method is an interrupt breakpoint.
  Evidence: the six probe sessions and the read probe.
- The file read sequence of the single-player start is mapped through the
  interrupt breakpoint: 28 bytes, then 28 bytes, then 6 bytes, then 176 bytes on
  the handle 5, and then the same 176-byte read repeats at the buffer `0x136c`.
  After the Enter key the sequence moves to 28 bytes, 4 bytes, 16 bytes, then a
  7852-byte read, and then 8192-byte reads repeat.  The 176-byte repeats are the
  menu idle; the 8192-byte reads are the graphics loads.
  Evidence: the read probes with the interrupt breakpoint.
- The memory dump command adds the selector base.  `MEMDUMP 0150:<off>` reads
  the
  linear address `0x1c2000 + <off>`, so a dump of a run-time address must pass
  the
  module address instead.  The earlier dumps passed the run-time address and
  therefore read the wrong memory.  Evidence: the dump addresses.
- The loader statics at the module addresses `0x25CCC`, `0x25CD4`, and `0x25CDC`
  are zero in the window, so the sprite loader has not run and the ring at
  `+0x4074` and the table at `+0x0000` have no dump yet.  Their semantics stay
  open.  Evidence: the corrected memory dumps.
- The next probe should continue the read sequence past the 8192-byte graphics
  loads until the read of the size `0x271A`, the 10010-byte sprite member, then
  dump the loader statics at the module address `0x25CD4` and `0x25CDC`.
  Evidence: the read sequence.
- The loader statics at `0x1E7CD4` and `0x1E7CDC` hold the pattern
  `00 F0 80 CE` in the menu idle, not a sprite count or a pixel size, so the
  sprite loader has not run.  The ring at `+0x4074` and the table at `+0x0000`
  therefore have no dump yet and their semantics stay open.
  Evidence: the memory dumps of the read probe.
- The next probe should break on the read with the size `0x271A`, the 10010-byte
  sprite member, or on the seek `AH=42h` at the member boundary, and then dump
  the ring and the table.
  Evidence: the read probe sizes.
- The ring at `+0x4074` and the table at `+0x0000` therefore have no dump yet,
  and their semantics stay open.  The next probe should use an interrupt
  breakpoint, for example `BPINT 21` on the file read `AH=3Fh`, to stop when the
  game reads a member, then dump the memory around the decoder.
  Evidence: the probe results.
- The mask table at `+0x8050` is no longer the only unknown.  The bit reader
  matches the decompiled code, so the remaining gap is in the match decoding:
  the
  table at `+0x0000` and the ring at `+0x4074` need a confirmed meaning.
  Evidence: the port result.
- The session script gained a `key <text>` step that writes to the emulator
  console, so a probe can leave an input wait.  Sending Enter moved the game
  from
  the input wait at `0150:1E1C40` to `0150:29AB85`, the module address
  `0xdab85`,
  inside the C run-time, so the key press starts the game.
  Evidence: `tools/scripts/probe_lzss_state.dbg`.
- The mask table at `+0x8050` stays the missing piece of the LZSS port.  The bit
  reader masks the code with the entry of that table for the shift count, and
  the
  decompiled code does not show how the table is initialized.
  Evidence: the decompiled `FUN_0004d844`.
- The decoder breakpoint at `0150:1FF4C4` still did not hit in the driven run,
  and the emulator closed the control connection during the 45-second window. 
  The
  race path loads its graphics through the whole-file open `0x4c485`, not the
  member
  open `0x4c08d`, so the LZSS decoder may not run on that path at all.
  Evidence: the driven probe run and the decompiled `FUN_00025aac`.
- The mask table at `+0x8050` stays the missing piece of the LZSS port.  The bit
  reader masks the code with the entry of that table for the shift count, and
  the
  decompiled code does not show how the table is initialized.
  Evidence: the decompiled `FUN_0004d844`.
- The mask table at `+0x8050` is the missing piece of the LZSS port.  The bit
  reader masks the code with the entry of that table for the shift count, and
  the
  decompiled code does not show how the table is initialized, so the mask values
  are
  unknown.  The literal code `0x100` and the end code `0x101` stay constant
  while
  the shift count grows from 9 to 12, which fits a table of shifted masks rather
  than the values `(1 << n) - 1`.
  Evidence: the decompiled `FUN_0004d844` and `FUN_0004d4c4`.
- The member `MENU.MLB` holds a plausible head: the value 16 at `+0x00` and
  the value 2960 at `+0x04`.  `GAMEL.SPR` holds the values `0x18007f00` at
  `+0x00` and `0xc0cb4af8` at `+0x04`, which are not plausible, so the sprite
  member is compressed and the count and the pixel size do not appear in the raw
  bytes.  Evidence: the `head` output of `tools/gfx_container.py`.

## The session start and the game modes

- `race_session_start` at `0x128e0` sets the flags `_DAT_00118e1c = 0x102`, the
  signature `DAT_0011521c = 0x1234`, and `_DAT_0011cb14 = 1`, then dispatches on
  the game mode `DAT_0011d260`.  Evidence: the decompiled `FUN_000128e0`.
- The game mode `DAT_0011d260` has the values 1, 2, 3 (the handler `0x12c0c`), 4
  (`0x12e18`), 0x10 (`0x12d0c`), 0x20 (`0x13120`), 0x40 (`0x130a0`), and
  0x100 (`0x12f50`).  The other values take the default path `view_move(0, 0)`
  at `0x1f94c` and `0x21da8(param ^ 1)`.
  Evidence: the decompiled `FUN_000128e0`.
- `object_player_input` tests the mode against the value 0x10 and
  `net_race_screen` against the value 2, so the field selects the race type.
  Evidence: the decompiled `FUN_0001c760` and `FUN_00028ba4`.
- `timing_update` at `0x22df8` drives the replay: the flag
  `DAT_00118e1c & 0x40` selects a playback, the flag `DAT_00118e6c` gates it,
  and the write path uses the flag `DAT_00118e60`.
  Evidence: the decompiled `FUN_00022df8`.

## The fifth level: the vehicle chain and the timing filter

- `object_drive_step` at `0x1cee0` computes the drive force from `+0x2e4`,
  `+0x2f0`, and the throttle `+0x3fc`.  It sets the brake flag `+0x2c0` when
  the force is below the speed `+0x418` and reduces the speed by `+0x2ec` times
  the frame delta.  Evidence: the decompiled `FUN_0001cee0`.
- `object_proximity_check` at `0x1e30c` measures the distance from the
  reference `DAT_00113ccc` and `DAT_00113cd0`.  Inside 35000 units it sets the
  gate `+0x414` from `+0x4e4` and `_DAT_00100b98`; outside it clears the
  gate.  Evidence: the decompiled `FUN_0001e30c`.
- The gate `+0x414` is the same field that `object_physics` and
  `object_prepare` test, so the chain closes.
  Evidence: the decompiled `FUN_0001e30c` and `FUN_0001c810`.
- `object_revolution_step` at `0x1d30c` accumulates the revolution counter
  `+0x250` from the speed and the frame delta, wraps it at 1.0, and reads the
  gear `+0x254`.  Evidence: the decompiled `FUN_0001d30c`.
- `timing_update` at `0x22df8` divides 1 by the frame delta `DAT_00118e04` and
  selects a 400-byte block of the coefficient table `DAT_001080e8`.  The block
  index is the frame rate, so the filter table holds one coefficient set per
  frame
  rate.  Evidence: the decompiled `FUN_00022df8`.
- `replay_read` at `0x22e94` and `replay_write` at `0x22f54` use 20-byte
  records at `DAT_00118e5c + index * 0x14`.  A record holds the frame delta at
  `+0x00` and six 16-bit state words at `+0x08`.  This is the replay format of
  the `REPLAYS` directory.  Evidence: the decompiled `FUN_00022e94` and
  `FUN_00022f54`.
- The filter table `DAT_0010cf08` has two writers, `frame_timer` at `0x1fc38`
  and `timing_update` at `0x22df8`, and six readers in the object functions.
  Evidence: the Ghidra references to `0x10cf08`.
- The frame delta `DAT_00118e04` has three writers: `frame_timer` at `0x1fc38`,
  `race_session_start` at `0x128e0`, and `timing_update` at `0x22df8`.
  Evidence: the Ghidra references to `0x118e04`.
- The runtime memory manager is in object 3: `malloc` at `0xd1f25`, `memset` at
  `0xd1ec0`, `free` at `0xd2008`, and `memmove` at `0xd2066`.  `malloc`
  rounds the size up to 4 bytes and walks the free list `DAT_001031d0`.
  Evidence: the decompiled `FUN_000d1f25` and `FUN_000d2008`.
- `race_session_start` at `0x128e0` sets the signature `DAT_0011521c` to 0x1234
  and clears the state globals.  Evidence: the decompiled `FUN_000128e0`.

## The replay record read

- `replay_read` at `0x22e94` holds 58 instructions and 190 bytes.  It computes
  the record address from `DAT_00118e5c + DAT_00118e64 * 0x14`.  It copies the
  dword fields at `+0x00` and `+0x04` to `DAT_00118e04` and `DAT_00118e08`.
  Evidence: the disassembly of `FUN_00022e94`.
- The assembly loads a dword two bytes below each 16-bit field and shifts the
  high word.  For example, the load at `+0x06` and the shift select the signed
  word at `+0x08`.  The six copies go from `+0x08` to `+0x12` into
  `DAT_00119880` through `DAT_00119894`.  The layout matches the replay record
  table in `docs/reference/file-formats.md`.
  Evidence: the disassembly of `FUN_00022e94`.
- The function writes the incremented index back to `DAT_00118e64`.  It returns
  1 when the new index equals `DAT_00118e68` or `DAT_00118e70`, and it returns
  0 for any other index.
  Evidence: the disassembly of `FUN_00022e94`.
- The rebuilt module builds and the behavior check passes.  The identity check
  stays open: 24 of 54 instructions and 18 of 176 bytes match.  The OpenWatcom
  V2 prologue and the register saves differ from the original Watcom 10.0a
  output.
  Evidence: `python3 tools/rebuild/run.py run --function 0x22e94`.
- The record pointer in the rebuild is volatile.  The original reloads the
  pointer from the frame before every field copy, and the plain local stays in
  a register in the rebuild.
  Evidence: the disassembly of `FUN_00022e94` and the rebuilt
  `tools/rebuild/replay_read.c`.

## The resource layer and the DPMI services

- The resource layer allocates its buffers through the DPMI services of
  interrupt
  31h.  The service `0x0100` allocates a DOS memory block: the layer passes
  `(size >> 4) + 1` in the size field, which is a count of 16-byte paragraphs,
  and converts the returned segment to a linear address with a shift of 4.
  Evidence: the decompiled `FUN_0002f0fc` and `FUN_0004bf4b`.
- The service `0x0501` allocates a memory block.  The layer passes the size in
  the EBX and ECX register pair and reassembles the returned address from the
  same pair.  Evidence: the decompiled `FUN_0002f172`.
- The service `0x0502` frees a memory block with the address in EBX and ECX.
  Evidence: the decompiled `FUN_0002f205`.
- Correction: the 16-byte unit is the DPMI paragraph of the DOS memory block,
  not a unit of the main file.  The earlier note in the `.3dx` section was
  wrong.
  Evidence: the decompiled `FUN_0002f0fc` and `FUN_0004bf4b`.
- `list_create` at `0x2ecf8` allocates `count * 0x1c` bytes, so the engine
  lists hold 28-byte records.  This matches the object records of the `.3dx`
  loader.  The list holds the array at `+0x00`, the count at `+0x04`, and the
  limit at `+0x08`.  Evidence: the decompiled `FUN_0002ecf8`.
- `resource_find` at `0x2f25f` walks the records, matches the key at record
  `+0xc`, and removes the match by shifting the array with the memmove at
  `0xd2066`.  Evidence: the decompiled `FUN_0002f25f`.
- The service `0x0500` reports the free memory.  `pool_init` at `0x2f904` calls
  it
  and keeps the pool size at `+0x24` in 4 KB pages, so the pool counts pages of
  4096 bytes.  Evidence: the decompiled `FUN_0002f904`.
- `resource_free` at `0x2f470` frees a record through `memory_free` when the
  flag
  at `+0x18` bit 0 is clear and through `0x2f40b` otherwise.
  Evidence: the decompiled `FUN_0002f470`.
- `dpmi_fill_regs` at `0x4bf23` fills a 12-byte block with the real-mode
  segment registers ES, CS, SS, DS, FS, and GS.  `dpmi_call` at `0x4bf4b` calls
  the interrupt through `0xd4410` and returns the value from EBX.
  Evidence: the decompiled `FUN_0004bf23` and `FUN_0004bf4b`.

## The fourth level of the program map

- `object_slide_step` at `0x1df34` computes a slide value from the speed
  `+0x418`, the steering `+0x2f4`, and the float pair `+0x27c` and `+0x280`,
  clamps the value between -2.0 and 2.0, and calls `filter_update` for the
  three parts at `+0x4c8`, `+0x4b8`, and `+0x4b0`.
  Evidence: the decompiled `FUN_0001df34`.
- `filter_update` at `0x22b80` applies the one-pole filter
  `*p = (*p - f) * table[i] + f` with the coefficient table
  `DAT_0010cf08`.  Evidence: the decompiled `FUN_00022b80`.
- `object_gear_step` at `0x1d834` holds the vehicle states 1 and 2.  State 2
  sets the float fields to 3000.0 and 12.0; state 1 integrates the position
  `+0x49c` and the three velocities with the damping `DAT_0010cf08 + 0x184`.
  Evidence: the decompiled `FUN_0001d834`.
- The resource layer sits at the addresses `0x2ecf8` to `0x2f25f`.  It holds the
  list, the file records, and the DPMI buffer services.  See
  [Program map](../reference/program-map.md) for the table.

## The third level of the program map

- `object_transform` at `0x1cd44` saves the previous state with
  `object_state_save` at `0x1ccd8` and integrates the position with
  `object_integrate` at `0x1d6bc`.  Evidence: the decompiled `FUN_0001cd44`.
- `object_integrate` integrates the position from the float velocity and the
  factor at `+0x450`, damps the factor, and clamps it at 0.  It compares the
  new position with the target at `+0x160` and `+0x168` and accumulates the
  float at `+0x46c`.  Evidence: the decompiled `FUN_0001d6bc`.
- `object_physics` at `0x1c810` runs `object_gear_step` at `0x1d834` and
  `object_gear_state` at `0x1db38` when the state at `+0x414` is set, then
  `object_slide_step` at `0x1df34` and `object_collision` at `0x1e00c`.
  Evidence: the decompiled `FUN_0001c810`.
- `object_gear_step` holds the states 1 and 2 at `+0x2f8`.  State 2 sets the
  float fields to 3000.0 and 12.0; state 1 integrates the position at `+0x49c`
  and the three velocities at `+0x304`, `+0x308`, and `+0x30c` with the damping
  `DAT_0010cf08 + 0x184`.  Evidence: the decompiled `FUN_0001d834`.
- `object_gear_state` sets the value at `+0x4e8` to 0, `0xff00`, or `0x8000`
  from the value at `+0x3f4`.  Evidence: the decompiled `FUN_0001db38`.
- `object_slide_step` computes a slide value from `+0x418` and `+0x2f4`, clamps
  it between -2.0 and 2.0, and calls `0x22b80` for three parts.
  Evidence: the decompiled `FUN_0001df34`.
- `overlay_dispatch` at `0x28160` reads the option flags in `DAT_000f0002`,
  the block that `default_data_copy` copies from object 1, and calls the overlay
  for the set flag.  Evidence: the decompiled `FUN_00028160`.
- `race_state_begin` at `0x294c8` sets the race constants 2.0, 0.6, 1.0, 20000,
  and -12.566, then moves the state machine to the state 1.
  Evidence: the decompiled `FUN_000294c8`.
- `net_race_screen` at `0x28ba4` handles the race state 2 and uses the
  0x2c-byte record table `DAT_001022c4`.  `net_player_add` at `0x2dddc`
  takes a 0x38-byte record and copies the player name `DAT_001203cc` into it.
  Evidence: the decompiled `FUN_00028ba4` and `FUN_0002dddc`.

## The .3dx resource loader

- The `.3dx` loader is at module address `0x10bfc`.  It walks the object list
  and
  loads every record with the load flag at `+0x14` set.  Evidence: the
  decompiled
  `FUN_00010bfc` and the `3D_HEADER` and `3D_BLKOBJ` string references from
  `tools/ghidra/FindReferences.java`.
- The file name is the record name plus the suffix `3dx`.  Evidence: the format
  `%s.3DX` at `0x1003ac`, the `rb` mode at `0x1003b3`, and the decompiled
  `FUN_00010bfc`.
- The loader reports failures with return codes: `2` when the open fails, `3`
  when the
  `3D_HEADER` chunk is missing, and `5` when the `3D_BLKOBJ` chunk is missing.
  Evidence: the decompiled `FUN_00010bfc`.
- The error reporter at `0x10978` dispatches on the code through a jump table
  and
  prints the `3dx` messages.  Evidence: the Ghidra references to `0x100074`,
  `0x1000a3`, `0x1000d1`, and `0x1000ff`, all from `FUN_00010978`.
- `resource_get` at `0x2eed0` looks a resource up by name and loads it from the
  file when missing.  The same function serves `3D_HEADER`, `3D_BLKOBJ`,
  `MELODIC_BANK`, `DRUM_BANK`, and `NET_SPACE`.  Evidence: the decompiled
  `FUN_0002eed0`, `FUN_0002f9e0`, and `FUN_00029d08`.
- `object_init_from_file` at `0x4cc10` binds the loaded file to the object
  record
  and copies the chunk table.  Evidence: the decompiled `FUN_0004cc10`.
- The loader's callees are named in [Program map](../reference/program-map.md).

## The resolution modes

- `STARTH.EXE` and `STARTL.EXE` differ only in the resolution letter of the
  hardcoded asset names.  `STARTH.EXE` names `menu1h.pcx`, `menu2h.pcx`,
  `menu3h.pcx`, `menu4h.pcx`, and `champ5h.pcx`; `STARTL.EXE` names
  `menu1l.pcx`, `menu2l.pcx`, `menu3l.pcx`, `menu4l.pcx`, and `champ5l.pcx`.
  Every other asset string is identical, so the suffix `H` means high resolution
  and the suffix `L` means low resolution.  The string scan of both executables
  is the evidence.
- The members of the graphics containers come in low and high pairs for the same
  reason: `GAMEL.SPR` and `GAMEH.SPR`, `GAMEL.FNT` and `GAMEH.FNT`,
  `IMPLET_L.PKC` and `IMPLET_H.PKC`, `IMPRES_L.PKC` and `IMPRES_H.PKC`,
  `CDFRONTL.PKC` and `CDFRONTH.PKC`, and `COCK00AL.PKC` and `COCK00AH.PKC`.
  `GFX/MAP1L.DAT` and `GFX/MAP1H.DAT` follow the same rule.  Evidence: the
  member lists of `tools/gfx_container.py`.
- The other asset names carry the letter at run time: `game%c.spr`,
  `game%c.fnt`,
  `menu%c.spr`, `menu%c.fnt`, `attr%c.fnt`, `logo%c.spr`, `load%d%c.pcx`,
  `net%c.pcx`, and `?%s%c.pcx`.  The letter in `%c` is `H` or `L` and the
  executable picks it, so `GAMEL.SPR` is the low-mode sprite member and
  `GAMEH.SPR` is the high-mode member.  Evidence: the shared strings of both
  executables.
- The GOG configuration launches `starth.exe`, so the installed single-player
  start is the high-resolution one.  Evidence: `dosboxScreamer_single.conf`.
- The high-resolution member is larger: `GAMEH.SPR` holds 28736 bytes and
  `GAMEL.SPR` holds 10010 bytes, which fits the resolution difference.
  Evidence: the member list of `GFX/GAME.DAT`.

## The LZSS port bug (superseded)

- This section is superseded.  The port defect below is real, and the conclusion
  that the remaining gap is the match decoding is wrong.  The last defect was the
  emulated `0x400` byte buffer of the reader at `0x4d844`, and the plain bit
  stream reader fixes it.  See The LZW reader was the last defect in the asset
  decode.
  Evidence: that section.
- The output loop of the block decoder is decoded: the driver `0x4d3fc` reads
  one
  byte into the bit buffer, then loops on the block decoder `0x4d4c4`.  The
  block
  decoder reads a code through the bit reader, treats `0x100` as a literal and
  `0x101` as the end, walks the table at `+0x0000` through the ushort field
  until
  the code falls to `0xff` or less, fills the ring at `+0x4074`, and then copies
  the
  low byte of each ring entry to the output while the ring index counts down.
  Evidence: the decompiled `FUN_0004d4c4`, lines 56 to 80.
- The port returns zeros because of the initial buffer state.  The driver fills
  only
  `buf[0]` before the first read, and the bit reader computes the remaining
  bytes as
  `0x400 - (bits >> 3)`, which is 0x400 at the start, so the refill does not run
  and the reader reads `buf[1]` and `buf[2]` as zero.  The first code therefore
  comes out wrong and every later code follows it.
  Evidence: the port diagnostic, which printed `buf[0..3]` as `[0, 0, 0, 0]` and
  the first six codes as zero.
- The first code now lands on the literal `0x100`.  The fix seeds the bit buffer
  with the member bytes from the start and sets the bit position to zero, which
  is
  the effect of the driver's one-byte read plus the first refill.  The
  diagnostic
  then prints `code0: 0x100`, the literal, and `code1: 0x03f`, the first literal
  byte.  Evidence: the port diagnostic.
- The decode still diverges after the second code.  The third code comes out as
  `0x000`, a zero-length match, and the ring index then runs past the ring,
  which
  raises an index error.  So the bit reader and the first code are right, and
  the
  remaining gap is the match decoding: the table at `+0x0000` and the ring at
  `+0x4074`.
  Evidence: the port diagnostic and the index error.
- The sprite count, the dimensions, and the palette of `GAMEL.SPR` and
  `GAMEH.SPR`
  therefore stay unreported.  Evidence: the port result.
- The next step is a run-time dump of the ring at `+0x4074` and the table at
  `+0x0000` after the first literal, through the interrupt breakpoint method
  that
  works, with the corrected dump addressing `MEMDUMP 0150:<module address>`.
  Evidence: the probe results.
- The sprite count, the dimensions, and the palette of `GAMEL.SPR` and
  `GAMEH.SPR`
  therefore stay unreported.  Evidence: the port result.

## The low and high sprite members (superseded)

- This section is superseded.  The `head` output below comes from the broken
  expansion.  The parser reports the count, the dimensions, and the palette of every
  member now.  See The LZW reader was the last defect in the asset decode.
  Evidence: that section.
- The SPR parser cannot report the sprite count, the dimensions, or the palette
  for
  either member yet, because the LZSS expansion is not confirmed.  `head` on
  `GAMEL.SPR` and `GAMEH.SPR` expands to 7935 and 7937 bytes of zeros, so the
  count and the size fields stay unreadable.  The near-equal output counts are a
  symptom of the broken port, not a property of the data.
  Evidence: the `head` output of `tools/gfx_container.py`.
- The observable difference is the size: `GAMEH.SPR` holds 28736 bytes against
  10010 for `GAMEL.SPR`, a ratio of 2.87, and `GAMEH.FNT` holds 25992 bytes
  against 11565 for `GAMEL.FNT`, a ratio of 2.25.  The high members are two to
  three times the low members, which fits the resolution difference.
  Evidence: the member list of `GFX/GAME.DAT`.
- The compressed members share little: `GAMEL.SPR` and `GAMEH.SPR` share the
  first 4 bytes, the value `00 7f 00 18`, and only 134 of 10010 bytes in the
  overlap.  `GAMEL.FNT` and `GAMEH.FNT` share 14 of the first 16 bytes and only
  99 of 11565 bytes in the overlap.  The two members therefore hold different
  image data, and the shared head is a header, not data.
  Evidence: the byte compare of the members.
- The next step stays the confirmed LZSS decoder: it is the only way to read the
  count, the dimensions, and the palette of either member.
  Evidence: the port result.

## Function names

- The Watcom startup at `0xd4324` calls `main` at `0x10020` with `argc` in EAX
  and `argv` in EDX.  Evidence: `tools/ghidra/decompile.sh 0x10020`.
- `main` calls `list_create` with `0x96` (150) and stores the result in the
  global `DAT_001062ac`.  The list holds 150 records of `0x1c` (28) bytes.
  Evidence: the decompiled `FUN_0002ecf8`.
- `main` calls `memory_available` and compares the result with `6400000`.
  Below that value it calls `printf` and `memory_available` again, which reports
  the shortfall.  Evidence: the decompiled `FUN_0002f8ca` and the strings
  `Not enough memory to play SCREAMER` and `You need another %d bytes`.
- `main` calls `engine_init` at `0x127a0`.  That function calls 17 setup
  functions and loads the `MENU` music from `%s\DATA\MUSIC.DAT`.  Evidence: the
  decompiled `FUN_000127a0`.
- The main loop runs `race_loop` and `frame_update` until the quit flag
  `DAT_00107ed8` is 1.  Evidence: the decompiled `FUN_00010020`.
- `race_loop` starts a race through `FUN_00027770` and loops until the session
  state `DAT_0011ce44` reaches 4.  Evidence: the decompiled `FUN_00027700`.
- `frame_update` clears the key state, reads the clock, advances a 6-frame
  counter,
  waits for the frame flag `DAT_0011d234`, and handles input.  Evidence: the
  decompiled `FUN_00027f94`.
- `main` calls `error_report` at `0x10978` on the session error path, with the
  error code in EAX.  The reporter dispatches through a jump table to one
  message
  stub per code.  Evidence: the decompiled `FUN_00010020` and `FUN_00010978`.
- `main` calls `engine_shutdown` at `0x128ac` after the loop.  That function is
  the counterpart of `engine_init`: it restores the keyboard interrupt handler
  with `FUN_00014e20(0)`.  Evidence: the decompiled `FUN_000128ac` and
  `FUN_00014e20`.
- Four of the callees are Watcom run-time helpers: `memory_alloc` (`0x30050`),
  `printf` (`0x3009b`), `exit` (`0x300be`), and `free` (`0x300ec`).  They sit
  at the end of the object, after the program code.  Evidence: the decompiled
  bodies and the out-of-memory handler pointers `DAT_001274ac` and
  `DAT_001274a8`.
- The engine strings name the resource interface: `%s.3dx` object files, the
  `3D_HEADER`, `3D_BLKOBJ`, `GOBJ_LIST`, and `OBJ_LIST` chunks, the track
  resources `OSTACLES`, `TRACK_DATA`, `OPPONENTS`, `ANIMATIONS`, and
  `CARS_DATA`,
  the sprite and visual-list interface, and the HMI banks `MELODIC_BANK` and
  `DRUM_BANK`.  Evidence: `r2 -q -c "iz"`.
- See [Program map](../reference/program-map.md) for the table.

## Static cross-check with radare2

radare2 6.2.4 reads the same LE image through its `bin_le` plugin.  Its view
agrees with `tools/le.py` and with the run-time log:

| Check | `tools/le.py` | radare2 | Match |
| --- | --- | --- | --- |
| entry point | module `0x300f8` | `0x000300f8` | yes |
| page 0 mapping | file `0x1a800` to module `0x10000` | paddr `0x1a800`, vaddr `0x10000` | yes |
| `main` | module `0x10020` | `fcn.00010020` | yes |
| `main` caller | `0xd4324`, call at `0xd4364` | `fcn.000d4324`, xref at `0xd4364` | yes |
| `main` prologue | `push ebx; push edx; push ebp; mov ebp,esp` | the same | yes |
| `main` callees | 13 unique targets | 13 identical targets | yes |
| function count | 642 (prologue scan) | 923 (call analysis) | different methods |

Evidence: `r2 -A -q -c "ie" -c "iS" -c "afl"` and
`r2 -A -q -c "s 0x10020; pdf"` from the Nix dev shell.  The function counts
differ because the two methods differ: the prologue scan finds the Watcom
prologue,
and radare2 also recovers functions from call targets.

## Pipeline confirmation

`tools/pipeline_check.py` runs every stage and checks the cross-stage links.
The check reports 231 checks and 0 failures in the `cross-check` shell.  It
reports 227 checks with `--skip-cross`, 216 checks with `--skip-runtime`, and
212 checks with both flags.  Evidence: the tool output.

- The file view and the run-time view agree.  The run-time entry `0150:1f20f8`
  matches the file entry at `0x3a8f8`, and the run-time jump target `0x1f2170`
  matches the file target.
- The module load base is `0x1c2000`.  The base is page aligned.  Evidence:
  `0x1f20f8 - 0x300f8 = 0x1c2000`.
- The run-time startup calls the DOS version service with `EAX=0x3000`
  (`AH=0x30`).  Evidence: the log line after the entry.
- The stub exec break `9fe0:0002` is outside the BIOS, and the log shows
  `int 21` with `EAX=0x4b03` (`AH=0x4b`, `AL=0x03`, load overlay).  Evidence:
  the first log line.

## Open questions

### Entry alignment (resolved)

The entry alignment question is resolved.  The header entry is an offset
inside the start object, not a module-relative address.  The run-time log
shows the entry instruction `jmp short +0x76`, and that instruction sits at
file offset `0x3a8f8`.  See the Entry point section above.

### Data formats

The project has no decoder for these formats:

- the HMI `ADLIB-` music bank (`DATA/MUSIC.DAT`, `CONFIG/SETUP.DAT`),
- the map index data (`DATA/MAP*I.DAT`),
- the tables (`TABS/VIS.TAB`, `TABS/PERSP01.TAB`),
- the raw sample data (`DATA/SOUND.DAT`),
- the HMI `.386` drivers.

The graphics data (`GFX/*.DAT`), the map data (`DATA/MAP*.DAT`,
`DATA/GEN.DAT`, `DATA/LOCAL.DAT`), and the replay format have decoders.  See
the graphics decoder section, the track data section, and the fifth level section.

### Duplicate data

`GFX/MAP5H.DAT` and `GFX/MAP5L.DAT` hold the same bytes.  One file is
therefore redundant.  The reason is not known.

### CD audio

The CD image holds seven audio tracks.  The project holds no extractor for
audio tracks.  The game plays the tracks through the CD audio path.

## The ring and table dump blocker

- The linear memory breakpoint `BPLM` works on this DOSBox-X build and fires on
  a write to a linear address, so the protected-mode code of the game can be
  watched.  This is the tool the earlier probes needed: `BP <seg>:<off>` is a
  real-mode breakpoint and never matched the DOS/4GW code.
  Evidence: the probe with `BPLM 0x1E7CCC`.
- The probe watched the linear address `0x1E7CCC`, the run-time address of the
  module address `0x25CCC`, and it fired five times: at the DOS boot
  `F000:C9B4`, then at `0150:2439FE`, `0150:297D30`, `0150:29EB58`, and
  `0150:1E1C40`.  Several functions write the address, so it is a shared
  buffer, not the single `SPRITES` static.  The dumps of the loader statics
  therefore show the byte pattern `89 89 89 89` and `8A 8A 8A 8A`, not a sprite
  count or a pixel size, and the statics of the sprite loader stay unlocated.
  Evidence: the probe with `BPLM` and `MEMDUMPBIN`.
- The next probe should watch the read buffer that the sprite loader fills,
  which
  the interrupt breakpoint on the file read already reports in EDX, and then
  walk
  the loader state from that buffer.  The `BPLM` method can watch any linear
  address once it is known.
  Evidence: the read probes and the `BPLM` probe.
- Code breakpoints do not stop this DOSBox-X build, so the decoder cannot be
  stopped at its entry to read the context from EAX.  The interrupt breakpoint
  on
  the file read works, but it shows the read buffer, not the context.
- Two routes remain: break on the interrupt `AH=42h`, the seek inside the member
  open `0x4c08d`, and walk the caller frame from the stack; or read the caller
  of
  the decoder from the stack at the read breakpoint and follow the context
  through
  that frame.
- The match decoding and the sprite count, the dimensions, and the palette
  therefore stay open.
## The interrupt stack walk

- The stack at the interrupt breakpoint is the real-mode stack.  The probe read
  `SS=a86 ESP=9d8 EBP=9e6 CS=824 EIP=17a5` at the file read, and the dump of
  `SS:ESP` holds real-mode segment and offset pairs, not protected-mode
  addresses.
  DOS/4GW switches to real mode for the DOS call, so the game's protected-mode
  frames are not on that stack.
- The caller frame walk therefore cannot reach the loader stream pointer through
  the
  interrupt breakpoint.  The DPMI boundary hides the game frames.
- The remaining route is a protected-mode memory breakpoint, `BPPM
  <selector>:<offset>`, on the game buffer.  The `BPLM` probe showed that the
  linear address `0x1E7CCC` is a shared buffer written by five functions, so the
  loader output address is still unknown.
- The debugger offers `BPLM`, `BPPM`, `MEMDUMPBIN`, `IV`, `SMV`, and the log
  commands.  The missing input is the loader state address, and every remaining
  route needs it observed rather than inferred.
- The sprite count, the dimensions, and the palette stay open.
## The protected-mode breakpoint

- `BPPM 0150:25CCC` works and fires on a write in the protected-mode address
  space.  It stopped the emulator at `CS=a5c EIP=32d`, a different code
  selector,
  so the watch matched the linear address that the selector and offset pair maps
  to.
- The dumps at that point hold `55 55 55 55`, a fill pattern, so the game has
  not
  written the loader statics in the probe window.  The sprite loader has not
  run.
- The debugger therefore offers three working breakpoint classes for this game:
  `BPINT` for the DOS calls, `BPLM` for a linear address, and `BPPM` for a
  protected-mode selector and offset.  The limit is no longer the tool; it is
  the
  game progress, because the sprite loader runs only after a race starts and the
  probes reach only the menu.
- The sprite count, the dimensions, and the palette stay open.  Every number in
  this
  record is observed; no value is inferred.
## The menu input

- A scripted key sequence of four Enter presses did not start a race.  The game
  stayed at the input wait `0150:1E1C40` through all four presses, and the
  loader
  statics still hold a fill pattern, `E7 E7 E7 E7`.
- The earlier single Enter press did move the game once, to `0150:29AB85`, so
  the
  keyboard reaches the game and the effect depends on the menu state.
- Correction: the menu is keyboard driven.  A single Enter press moved the game
  from
  the run-time address `0150:1E1C40` to `0150:29AB85`, and a manual playthrough
  navigated the menu and started a race with the keyboard alone, so the scripted
  keys do
  reach the menu and the earlier failure was the sequence and the timing rather
  than the
  input device.
  Evidence: the driven probe and the manual playthrough.
- `mouse_init` at `0x1F1A0` still installs an interrupt 33h handler, but the
  menu does
  not require it.  The handler serves another screen, most likely a race screen.
  Evidence: the disassembly of `0x1F1A0` and the correction above.
- The game runs a demo before the menu.  The address that the probes first saw,
  `0150:1E1C40`, lies in the function at the module address `0x21BA0`, which
  reads the
  reference points `0x13CCC` and `0x13CD0` and calls the floating point helper
  `0x4D9F4`, so the game was running a motion routine, not waiting for input. 
  The
  probe should let the demo run, press Enter to leave it, and then walk the menu
  with
  further key presses.
  Evidence: the disassembly of `0x21BA0` and the driven probe.
- The sprite count, the dimensions, and the palette stay open.
- The remaining debugger tool is `SMV`, which writes memory at a linear address.
  It could set the menu selection or the start flag directly, but the address of
  that
  state is not observed, so this route needs the state address first.
- The sprite count, the dimensions, and the palette stay open.
## Corrections to the wait and the mouse

- Correction: the run-time address `0150:1E1C40` is not an input wait.  The
  function that contains it starts at the module address `0x21BA0`, and it reads
  the
  reference points `0x13CCC` and `0x13CD0`, calls the floating point helper
  `0x4D9F4`, and calls `0x2218C`.  It is a graphics or motion function, so the
  game was running rather than waiting at that point.
  Evidence: the disassembly of `0x21C40` and the function list.
- `mouse_init` at `0x1F1A0` resets the mouse through interrupt 33h and installs
  a
  user-defined event handler with the far pointer `CS:0x18D8C`.  The menu is
  therefore mouse driven, and the handler at `0x18D8C` receives the menu events.
  Evidence: the disassembly of `0x1F1A0`.
- The next step is to disassemble the function that contains `0x18D8C`, the
  mouse
  handler, which should name the menu state address that the menu reads and
  writes.
  With that address, `SMV` can set the menu selection and the `BPPM` probe can
  watch the sprite loader.
  Evidence: the disassembly of `0x1F1A0`.
- The sprite count, the dimensions, and the palette stay open.
## The menu state address

- The game wrote `CONFIG/CHOICE.DAT` during a probe, and the file holds 1120
  bytes,
  the value `0x460`.  That is the exact size that `config_save` at `0x1BE6C`
  writes, the structure at `DAT_001203C4`, so the menu state address is the
  module
  address `0x1203C4` and the run-time address `0x1E23C4`.
  Evidence: the probe-created file and the decompiled `FUN_0001be6c`.
- The structure holds the car name `GRA` at `+0x08`, the value 2 at `+0x0C`, the
  value 1 at `+0x10`, and the value `-1` at `+0x3C`.  The first field is a car
  name of three characters, so the menu writes the player choice there.
  Evidence: the field dump of the probe-created file.
- The game wrote the choice file, so the menu accepts the scripted keys.  The
  file is
  the last-choice memory, and the game reads it at startup, which is why a later
  run
  can start a race directly.
  Evidence: the file date and the decompiled `FUN_0001b840`.
- Rule: the probe created `CONFIG/CHOICE.DAT` inside the shipped game directory,
  which the project rule forbids.  The file was removed to restore the shipped
  artifact, and a copy stays under `work/`.  Probes must run against a copy of
  the
  game directory.
  Evidence: the file dates, where every shipped file is dated Dec 9 2019 and the
  probe file is dated today.
- The sprite count, the dimensions, and the palette stay open.  The next probe
  sets
  the structure at `0x1E23C4` with `SMV`, or leaves the choice file in the probe
  copy, and then re-runs the `BPPM` probe with a longer window.
  Evidence: the probe results.
## The race started and the probe artifacts

- Correction: the `CONFIG/CHOICE.DAT` and `REPLAYS/REPLAY01.RPL` files are most
  likely from a manual session in the installed game, not from the probes.  The
  probe configuration mounts nothing, so the probes depend on the relative mount
  `mount C "game"` of the game configuration, and that resolution was never
  verified.  More decisively, the probes never reached the sprite loader: the
  `BPPM`
  dumps stayed fill patterns, and a race start writes a replay, so a probe race
  start would have shown loader writes.
  Evidence: the probe configuration, the game configuration, and the probe
  dumps.
- The files still prove that the game starts a race and writes a replay of 32612
  bytes with 20-byte records, and that the menu state file holds 1120 bytes with
  the car name `GRA` at `+0x08`.  They do not prove that a probe reached the
  loader.
  Evidence: the captured files under `work/probe_artifacts/`.
- The earlier note that "a race did start" therefore needs a qualifier: a race
  started in that game directory, in a session that was most likely manual.
  Evidence: the correction above.
- The probe configuration mounts nothing, so a probe may run the game inside the
  shipped directory through the relative mount.  The next probe must mount an
  explicit copy, and the finding stays a rule risk until that is verified.
  Evidence: the probe configuration.
- The menu state file `CONFIG/CHOICE.DAT` holds 1120 bytes and the car name
  `GRA`
  at `+0x08`, so the game reads the last choice at startup.  A later run with
  that
  file present starts a race without menu input.
  Evidence: the captured choice file.
- Rule violation: the probes ran the game inside the shipped game directory, and
  the
  game wrote `CONFIG/CHOICE.DAT` and `REPLAYS/REPLAY01.RPL` there.  Both files
  were removed to restore the shipped directory, and copies stay under
  `work/probe_artifacts/`.  The probe configuration must mount a copy of the
  game
  directory.
  Evidence: the file dates, where every shipped file is dated Dec 9 2019 and
  both
  probe files are dated today.
- The next probe fixes the mount to a copy of the game directory, keeps the
  choice
  file so the race starts at once, and watches the sprite loader statics with
  `BPPM`.
  Evidence: the probe results.
## The refined probe result

- The explicit mount is confirmed.  The probe configuration now mounts the copy
  of the
  game directory with an absolute path, and the run wrote no state file into the
  copy, so
  the probes run against the copy and the shipped directory is safe.  This
  confirms that the
  choice file and the replay in the shipped directory came from the manual
  playthrough.
  Evidence: the probe configuration and the run.
- The race did not start in the run.  The game stayed at the run-time address
  `0150:1E1C40`, the motion routine of the demo, through all four Enter presses,
  and
  the game wrote no choice file and no replay, so no race started.
  Evidence: the probe run.
- The sprite loader statics were not captured.  The `BPPM` dumps hold the fill
  pattern
  `E7 E7 E7 E7`, so the loader has not run.
  Evidence: the probe dumps.
- The key effect depends on the demo timing.  One earlier run left the demo
  after a single
  Enter press at 12 seconds, and this run did not leave it after a press at 20
  seconds, so
  the probe should press Enter repeatedly over a longer window rather than at
  fixed points.
  Evidence: the two driven runs.
- The sprite count, the dimensions, and the palette stay open.
## The two-minute key probe

- The race did not start.  The game wrote no choice file and no replay, and the
  run-time address stayed at `0150:1E1C40`, the demo motion routine, through 24
  Enter
  presses spaced five seconds apart.
- The sprite loader statics changed.  The dumps hold `81 81 19 10 08 04 03 03`
  at
  `0150:25CD4`, `03 03 03 03 03 03 03 03` at `0150:25CDC`, and
  `88 88 88 1A` at `0150:25CCC`, not the fill patterns of the earlier runs, so
  something wrote those statics during the run.
- The values do not match the sprite structure: the first dword would be
  0x10198181, which is not a plausible sprite count, and the second is not a
  plausible pixel size.  The loader did not run for a race, and the demo may
  have
  written the statics for its own graphics.
  Evidence: the dumps and the missing state files.
- The Enter key does not leave the demo in this configuration, while one earlier
  run
  did leave it, so the effect is not reproducible from the key alone.  The next
  probe
  should watch the demo state itself, for example with `BPPM` on the input flags
  at
  `DAT_00118E1C`, rather than relying on the key timing.
  Evidence: the two driven runs.
- The sprite count, the dimensions, and the palette stay open.
## The input flags confirm the key path

- The protected-mode memory breakpoint `BPPM 0150:118E1C`, the input flags
  `DAT_00118E1C`, fired twice in one run.  The first write came from
  `CS=A5C EIP=32D` before the key, and the second from `CS=3DB8 EIP=5222`
  after the Enter key, so the key reaches the game and writes the input flags.
  Evidence: the input flag probe.
- The input path therefore works.  The earlier failures are in the menu state
  machine: the game reads the flags, and the demo does not leave for the menu on
  the
  key alone.  The next probe watches the menu state itself, the flags
  `_DAT_00118E1C` and `DAT_0011D274` and the state `DAT_0011FAC4`, together with
  the key, and reports which flag changes.
  Evidence: the input flag probe and the driven runs.
- The sprite count, the dimensions, and the palette stay open.
## The three watched flags

- The key press provokes two writes from the keyboard handler: `CS=3DB8
  EIP=5222`
  and then `CS=3DB8 EIP=7FA0`.  Both come after the key, so the key reaches the
  game
  and writes the watched state in two places, most likely the key-down and the
  key-up
  path of the same handler.
  Evidence: the three flag probe.
- The other two hits are `CS=A5C EIP=32D` before the key, the init or DOS path,
  and
  `CS=18 EIP=334` after the key, a different writer.
  Evidence: the three flag probe.
- The dumps do not separate the three flags.  Every dump at `0150:118E1C`,
  `0150:11D274`, and `0150:11FAC4` holds the same value at each hit, the fill
  pattern `55 55 55 55` at the first two hits and zero at the third, so the
  watch
  address and the dump address do not resolve to the same memory for at least
  two of the
  three.  That addressing subtlety must be settled before the flags can be read.
  Evidence: the dump comparison.
- The next step settles the addressing: set one `BPPM` at a time, dump the one
  address
  that the watch names, and compare with the value the debugger reports for the
  same
  selector and offset through `IV` and `EV`.
  Evidence: the dump comparison.
- The sprite count, the dimensions, and the palette stay open.
## The watch and dump addressing

- The `IV` and `EV` cross-check works.  The debugger creates a variable for a
  selector and offset, `IV 0150:118E1C INFLAG`, and `EV` reads it, so a probe
  can
  name the state and read it back.
  Evidence: the cross-check probe.
- The mismatch is resolved.  The dumps before the run are zero at all three
  addresses and the dumps after the first hit hold the fill pattern `55 55 55
  55` at all
  three, so the memory did change and the value at the watch is the fill pattern
  of the
  image load, not the game state.
  Evidence: the before and after dumps.
- The cause is the watch syntax.  `BPPM [selector]:[offset]` matches the offset
  in any
  selector, so it fires for the DOS/4GW image loader at `CS=A5C` and `CS=18` and
  for
  the keyboard handler at `CS=3DB8` alike, while `MEMDUMPBIN 0150:118E1C` reads
  the
  linear address `0x1C2000 + 0x118E1C`, which is the game data object only.
  Evidence: the watch hits and the dump values.
- The correct watch for the game state is the linear form, `BPLM 0x2DAE1C`,
  which the
  earlier probe already showed fires on a write.  A probe that uses `BPLM` with
  the
  linear address and dumps through a debug variable created with `IV` reads the
  same
  memory in both steps.
  Evidence: the linear watch probe and the cross-check probe.
- The sprite count, the dimensions, and the palette stay open.
## The linear watch probe

- The linear watch `BPLM 0x2DAE1C` fires, twice in one run: at the DOS boot
  `CS=F000 EIP=C9B4` and in the game at `CS=150 EIP=1E1C40`, the demo motion
  routine.  The linear form therefore watches the game data object.
  Evidence: the linear watch probe.
- The debug variable `IV 0150:118E1C INFLAG` is created, and `EV INFLAG` returns
  `A31C` both before and after the key press, while the memory dumps at
  `0150:118E1C` are zero before and after.  The value from `EV` and the value
  from
  the dump still disagree, so the cross-check is not yet settled.
  Evidence: the cross-check probe.
- The key press did not change the reported value, so this probe cannot confirm
  that
  the key advances the input flags.  The result is negative rather than
  positive.
  Evidence: the cross-check probe.
- The next step is to settle the debug variable semantics: read the value the
  debugger reports for a known memory location, compare it with a binary dump of
  the
  same location, and only then trust `EV` for the game state.
  Evidence: the cross-check probe.
- The sprite count, the dimensions, and the palette stay open.
## The debug variable semantics

- The cross-check is settled, and the answer is no: `EV` does not agree with the
  dump.  On the same selector and offset, `0150:118E1C`, `EV INFLAG` returns
  `A31C` while `MEMDUMPBIN` returns `00 00 00 00`, so the debug variable and the
  binary dump read different values for the same pair.  `EV` cannot be used as
  evidence
  for the game state.
  Evidence: the semantics probe.
- The binary dump works on loaded memory.  After the game boots, the dump at
  `0150:25CCC` returns `88 88 88 88`, the dump at `0150:118E1C` returns
  `00 00 00 00`, and the dump at `0150:300F8` returns `39 39 35 25`, so the
  memory is loaded and the dump reads it.
  Evidence: the semantics probe.
- The selector and offset mapping is not yet settled.  The entry at the module
  address
  `0x300F8` holds `EB 76` in the file, and the dump of `0150:300F8` returns
  `39 39 35 25`, so the selector `0150` does not map the offset `0x300F8` to the
  entry as assumed.  The run-time entry evidence gives the pair `0150:1F20F8`
  for the
  module address `0x300F8`, so the mapping holds for the code pair, and the dump
  disagrees with it.
  Evidence: the semantics probe and the entry evidence.
- The usable read path is the binary dump, and its addressing must be settled
  against a
  known byte before it is used to read the game state.  The `EV` form is not
  usable.
  Evidence: the semantics probe.
- The sprite count, the dimensions, and the palette stay open.
## The addressing rule and the game state

- The addressing rule is settled.  The binary dump must take the run-time pair,
  the
  module address plus the load base `0x1C2000`, as the offset in the selector
  `0150`.
  The dump of `0150:1F20F8` returns `EB 76 57 41`, which is the entry jump of
  the
  file and the start of the Watcom banner, while the dump of the module address
  `0150:300F8` returns unrelated bytes.  Every earlier dump and watch that used
  a bare
  module address therefore read the wrong memory.
  Evidence: the entry dump probe.
- The game state now reads.  With the run-time pairs, the dump returns:
  the input flags `0` at `0150:2DAE1C`, the sprite record pointer
  `0x0008D800` at `0150:1E7CCC`, the sprite count `3` at `0150:1E7CD4`, and the
  pixel data size `208568` at `0150:1E7CDC`.
  Evidence: the corrected state probe.
- The sprite structure is confirmed.  The loader reads the count and the size
  into
  `DAT_00025CD4` and `DAT_00025CDC`, eight bytes apart with the record pointer
  at
  `DAT_00025CCC` and the pixel data pointer at `DAT_00025CE0`, so the structure
  is
  the pointer, a field, the count, a field, the size, a field, and the second
  pointer.
  Evidence: the corrected state probe and the decompiled `FUN_000257b0`.
- The values agree with the member sizes: the sprite records take
  `8 + 3 * 0x14 + 208568`, which is `208636` bytes, against the compressed
  member
  of `10010` bytes, a ratio of about 20.8 for the LZSS member.
  Evidence: the corrected state probe and the member list.
- The game was in the sprite loading code at the dump: the run-time address
  `0150:1E7CB7` is the module address `0x25CB7`, which lies just before the
  sprite
  statics at `0x25CCC`.
  Evidence: the corrected state probe.
- The sprite count at `+0x00` is 3 and the pixel data size at `+0x04` is 208568,
  so
  the two values the parser needed are now observed.
  Evidence: the corrected state probe.
## The sprite records

- The sprite record pointer `0x0008D800` is a selector relative offset, not a
  linear
  address: the dump of `0150:8D800` returns data while the linear form returns
  nothing.
  The record pointer therefore addresses the game data object.
  Evidence: the record probe.
- The three records decode as five 32-bit fields each: x, y, width, height, and
  the
  data offset into the pixel buffer.
  Evidence: the record dump and the field decode.
- The dimensions are read.  Sprite 0 is 200 by 240 at the offset 0, sprite 1 is
  466 by
  188 at the offset 48000, and sprite 2 is 640 by 114 at the offset 135608.
  Evidence: the record decode.
- The offsets are cumulative pixel counts, so the structure agrees with itself:
  sprite 0
  takes 200 times 240, which is 48000 pixels and the offset of sprite 1; sprite
  1 takes
  466 times 188, which is 87608 pixels and puts sprite 2 at 135608; and sprite 2
  takes
  640 times 114, which is 72960 pixels and ends the buffer at 208568, the exact
  size the
  loader read.
  Evidence: the record decode and the state probe.
- The pixel data is one byte per pixel, so the member holds 208568 pixels of an
  8-bit
  paletted image, and the palette is separate and set through the video calls.
  Evidence: the record decode.
- The expanded member and the compressed member are now a plaintext and
  ciphertext pair,
  the 208636 expanded bytes against the 10010 compressed bytes, so the LZSS
  match
  decoding can be derived from the pair rather than guessed from the ring and
  the table.
  Evidence: the state probe and the member list.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The expanded member dump

- The sprite records dumped correctly.  The dump at `0150:8D800` returns the
  three
  records with the dimensions, so the record pointer is an offset in the
  selector
  `0150`.
  Evidence: the record dump.
- The pixel data did not dump at the pointer.  The second pointer at
  `0150:1E7CE0` holds `0x0006BD00`, and the dump at `0150:6BD00` returns
  `00 00 00 00 98 B9 FF FF`, which is not the pixel data head of a count and a
  size, so the pixel pointer does not use the selector `0150` as its base.  The
  two
  pointers therefore use different bases, and the pixel pointer needs one more
  addressing step.
  Evidence: the expanded dump.
- `MEMDUMPBIN` writes a binary file for a large dump, and the file is larger
  than
  the requested length, so a dump must be checked against its requested size
  before the
  bytes are trusted.
  Evidence: the expanded dump sizes.
- The next step is to find the selector that the pixel pointer belongs to,
  either by
  reading the selector registers of the video path or by watching the pointer
  with
  `BPLM` at the linear form.
  Evidence: the expanded dump.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel data size is 208568 bytes.  The expanded member itself still
  needs a
  clean dump before the port can be checked against it.
  Evidence: the record decode.
## The pixel data location

- The pixel data is at `0150:6BD00`.  The dump returns
  `00 00 00 00 98 B9 FF FF FF 00 00 00 94 EA FE FF`, which reads as 8-bit
  pixel indices and not as a count and a size, so the earlier expectation of a
  count and a size at the pixel pointer was wrong.  The loader reads the count
  and
  the size from the file head into the statics, and the pixel resource holds the
  pixels alone.
  Evidence: the pixel dump probe.
- The pixel pointer is therefore a selector relative offset in the selector
  `0150`,
  the same base as the record pointer.
  Evidence: the pixel dump probe.
- The neighbouring addresses do not hold the pixels.  The real mode segment
  `6BD0:0000` holds the fill pattern `55 55 55 55`, and the address
  `0150:7FD2C` holds 3D geometry data with the value `0x1FFFFFFF`.
  Evidence: the pixel dump probe.
- The dump length rule stands: `MEMDUMPBIN` produced 1065604 bytes for a request
  of
  104284, so a dump is only trusted after its size is checked against the
  request.
  Evidence: the expanded dump sizes.
- The next step is the byte for byte check: expand the compressed member with
  the
  port in `tools/gfx_container.py` and compare the result with the dump at
  `0150:6BD00` for 208568 bytes.  The plaintext is now available, so the match
  decoding can be derived from the pair instead of guessed.
  Evidence: the pixel dump probe.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The byte for byte check

- The plaintext is built.  The two pixel dumps stitched at the correct split,
  the
  first from `0x6BD00` to `0x7FD2C` and the second from `0x7FD2C`, give exactly
  208568 bytes, the size the loader read.
  Evidence: the stitched plaintext.
- The port does not complete.  The expansion stops with the clear message "the
  ring or
  table index runs past its bound", so the match decoding diverges and no byte
  for byte
  comparison is possible yet.
  Evidence: the port run.
- The harness is in place.  The port takes the compressed member and the
  plaintext is
  the ground truth, with the pixel part at the offset 68 of the expansion, after
  the
  count of 8 bytes and the three records of 60 bytes.  The moment the match
  decoding is
  right, the comparison reports it.
  Evidence: the port and the plaintext.
- The port now raises a clear error instead of crashing, so the next iteration
  can debug
  the match decoding against the plaintext rather than guess.
  Evidence: the port run.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The first divergence

- The first divergence is the first pixel.  The port writes `0x17` at the output
  offset 68, the first pixel, while the plaintext holds `0x00`.  The block that
  produced it is block 35, a match with the code `0x022`, the shift 9, the bit
  position 324, and the ring index 0, so the match decoding diverges before any
  pixel is right.
  Evidence: the diff script `tools/scripts/diff_port_vs_dump.py`.
- The plaintext is stitched from the two pixel dumps at the split `0x7FD2C` and
  holds
  208568 bytes, the size the loader read.
  Evidence: the diff script.
- The whole member is compressed, including the count and the size head.  The
  member
  head holds `00 7F 00 18 F8 4A CB C0`, not the plain count of 3 and size of
  208568, so the file layer decompresses the stream and the loader reads the
  plain
  count and size from the decompressed stream.  A hypothesis that the head was
  stored
  plain is refuted.
  Evidence: the member head and the loader code.
- The diff script is the working tool: it runs the port block by block, compares
  each
  new byte with the plaintext, and reports the first differing byte with the
  decoder
  state that produced it.  The next iteration compares the port bit consumption
  with
  the plaintext bytes to find the branch, rather than guess the ring and the
  table.
  Evidence: the diff script.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The literal branch

- The first code reads correctly as `0x100`, the literal, and the port reads it
  with
  nine bits and the mask `0x1FF` at the bit position zero.
  Evidence: the port diagnostic.
- The literal that follows comes out `0x3F`, while the plaintext holds `0x03` at
  the
  first pixel.  The compressed head holds `00 7F 00 18`, so the bits from
  position
  eight are `1 1 1 1 1 1 1 0`, and a nine bit read from position nine yields
  `0x3F`.  The plaintext value `0x03` sits in the two bits at positions eight
  and
  nine.
  Evidence: the compressed head and the plaintext head.
- The branch is therefore the literal read offset, not the width: an eight bit
  read at
  the same position yields `0x3F` as well, because the ninth bit is zero there,
  while
  the plaintext wants the two bits at positions eight and nine.
  Evidence: the bit comparison.
- The plaintext is the decoded output, not the coded stream, so the two bit
  sequences
  do not compare directly.  The next iteration must compare the port output with
  the
  plaintext at each step and move the read offset until the first pixel matches,
  which the diff script already reports.
  Evidence: the bit comparison.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The literal width sweep

- The literal width is not the branch.  The sweep of the widths 2 to 9 shows
  that no
  width produces the expected head `03 00 00 00 B8 2E 03 00`, and the width 8
  terminates the expansion after 44 bytes, so the width 8 is refuted and the
  others
  diverge at the first pixel.
  Evidence: the literal width sweep.
- The first byte of the expansion is `3F` at every width, so the first byte does
  not
  come from the literal branch at all.  The first code reads as `0x100`, the
  literal, so
  the byte must come from a step before the literal or from the initial state of
  the
  driver.
  Evidence: the literal width sweep.
- The tool now carries the literal width as a parameter, `LITERAL_BITS` in
  `tools/gfx_container.py`, so a sweep is one command.
  Evidence: the port change.
- The next step examines the driver start: the driver reads one byte into the
  bit buffer
  before the first block, and the port seeds the buffer with the member bytes
  instead.
  The driver byte and the first literal may both reach the output, which would
  explain
  the leading `3F`.
  Evidence: the decompiled `FUN_0004d3fc` and the sweep.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The literal mask

- The literal mask was the branch.  The reader masks with the constant `MASK` of
  `0x1FF`, so the literal width parameter changed only the bit advance and never
  the
  value, which is why the first byte stayed `3F` at every width in the first
  sweep.
  With the literal value masked by its own width, the width 2 gives the first
  byte
  `0x03`, the value the plaintext holds.
  Evidence: the corrected literal sweep.
- The head is `03 0F 0F 0F 00 01 0F 00` at the width 2 against the expected
  `03 00 00 00 B8 2E 03 00`, so the first byte matches and the rest do not.
  Evidence: the corrected literal sweep.
- The remaining mismatch is the item framing.  The plaintext wants four literals
  of
  `03 00 00 00` in a row, and the coded stream holds the bits `1 1` at positions
  eight and nine, which is the first literal, and then ones at positions ten and
  beyond, where the plaintext wants zeros.  The items are therefore interleaved
  with
  framing bits, the flag scheme of the classic LZSS, and the framing is the next
  branch
  to test.
  Evidence: the corrected literal sweep and the coded head.
- The tool carries the literal width and its mask, so a sweep is one command and
  the diff
  script reports the first divergence with the decoder state.
  Evidence: the port and the diff script.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The flag framing sweep

- The classic flag framing scheme is refuted.  A sweep of the flag byte before
  and
  after each group of eight items, both polarities, the literal width eight, the
  position
  widths eight to twelve, and the length widths four and five finds no variant
  that decodes
  the four leading literals `03 00 00 00` of the plaintext.
  Evidence: the flag framing sweep.
- The first item framing that does work is the one the port already has: a nine
  bit code
  that reads `0x100`, the literal marker, followed by a two bit literal that
  gives `0x03`,
  the first plaintext byte.  The framing of the first item is therefore a nine
  bit code and
  a two bit value, and it is not the classic one flag bit and eight bit literal.
  Evidence: the literal mask result and the flag framing sweep.
- The two bit literal is small for a byte, so the literal field may not be a
  byte at all, or
  the two bits may be part of a longer field that the next item completes.  The
  next branch
  is the item structure around the nine bit code, for example a code that
  carries a length and
  a distance, with the two bit value as part of it.
  Evidence: the literal mask result.
- The tool carries the literal width and its mask and the diff script reports
  the first
  divergence with the decoder state, so each branch test is one command.
  Evidence: the port and the diff script.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The decoder disassembly

- The reader is read from the instructions, not from the decompiled C, and the
  port
  matches it.  The reader takes the bit position at `+0x406C`, the byte index as
  the
  position shifted right three, and the bit offset as the position masked with
  seven.  It
  loads the 16 bit word at the byte index, then replaces the low byte with the
  byte at the
  byte index plus two, shifts the value right once per bit offset while pulling
  a bit of that
  third byte into bit 15, advances the bit position by the shift at `+0x4068`,
  and masks
  the value with the entry of the table at `+0x8050` for the shift.
  Evidence: the disassembly of `0x4D844`.
- The port is equivalent for the mask `0x1FF`.  The port reads three bytes and
  shifts,
  while the code reads a word and pulls the third byte in at bit 15, and the two
  agree for
  every shift up to twelve because the pulled bits land above bit 15 and the
  mask discards
  them.
  Evidence: the disassembly of `0x4D844` and the port.
- The mask table at `+0x8050` is the remaining unknown and it is not the values
  `(1 << n) - 1` as assumed, because the first literal then reads `0x3F` while
  the
  plaintext holds `0x03`.  The table holds the mask for each shift, and its
  values must be
  read rather than assumed.
  Evidence: the disassembly of `0x4D844` and the plaintext.
- The reader also tracks the stream length: it compares the field at `+0x4044`
  less the field
  at `+0x4040` with `0x400` less the byte index and stops at the stream end, and
  the
  driver adds the bytes read to `+0x4040`.  The port seeds the buffer instead,
  which is
  equivalent only when the stream is at least `0x400` bytes.
  Evidence: the disassembly of `0x4D844` and the decompiled `FUN_0004d3fc`.
- The next step is to read the mask table: twelve 32 bit entries at the context
  plus
  `0x8050`.  The context is the stream structure, and the table values settle
  the literal
  width.
  Evidence: the disassembly of `0x4D844`.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The driver and literal instructions

- The driver reads a full buffer at the start.  The instruction is a call to
  `0xc607b` with the size in EBX, the value `[+0x4044]` capped at `0x400`, and
  the
  count 1, so the initial read is `min(stream length, 0x400)` bytes.  The
  decompiled C
  argument order misled the earlier reading, which took it for a one byte read.
  Evidence: the disassembly of `0x4D3FC`.
- The driver sets the state fields and loops on the block decoder until it
  returns
  zero, then returns the output count at `+0x4070`.
  Evidence: the disassembly of `0x4D3FC`.
- The literal branch matches the port.  It reads the code, treats `0x101` as the
  end and
  `0x100` as the literal, sets the shift to 9, the threshold to `0x200`, and the
  index
  to `0x102`, reads the value, takes its low byte into `+0x8085` and `+0x8084`,
  increments the output count at `+0x4070`, and stores the byte at the output
  buffer
  `+0x8488`.
  Evidence: the disassembly of `0x4D4C4`.
- The port is therefore instruction accurate for the driver, the reader, and the
  literal
  branch.  The single remaining assumption is the mask table at `+0x8050`, whose
  values
  are not the values `(1 << n) - 1`.
  Evidence: the disassembly of `0x4D3FC`, `0x4D4C4`, and `0x4D844`.
- The next step reads the mask table: find the context pointer, which the stream
  structure holds at `+0xB0`, and dump twelve 32 bit entries at the context plus
  `0x8050`.
  Evidence: the read wrapper and the disassembly.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The stream structure layout

- The open wrapper writes the stream structure at fixed offsets: the file handle
  at
  `+0x00`, the mode string at `+0x20`, the file name at `+0x24`, the name copy
  at
  `+0x78`, and a flag at `+0xAC` on the failure path.  It then calls the decoder
  with the stream in EAX and the fourth argument in EDX.
  Evidence: the disassembly of `0x4C08D`.
- The read wrapper passes the driver four values: the context in EAX, which is
  the
  stream field at `+0xB0`, the destination buffer in EDX, the size in EBX, and
  the
  file handle in ECX, which is the stream field at `+0x00`.
  Evidence: the read wrapper and the driver disassembly.
- The mask table address is therefore the stream structure plus `0xB0` plus
  `0x8050`,
  which is the stream plus `0x8100`, and the stream structure is the large local
  of
  the loader.
  Evidence: the stream layout and the driver.
- The practical way to read the table is a memory search.  The stream holds the
  file
  name at `+0x24`, so a search for the name finds the stream, and the table then
  lies
  at the found address plus `0x80DC` for forty eight bytes, twelve 32 bit
  entries.
  Evidence: the stream layout.
- The next step runs that search at the breakpoint of the file read, reads the
  found
  address, and dumps the table.
  Evidence: the stream layout.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The instruction accurate reader

- The reader is rewritten from the instructions.  It takes the byte index as the
  bit
  position shifted right three, the bit offset as the position masked with
  seven, loads
  the 16 bit word at the byte index, replaces the low byte with the byte at the
  byte
  index plus two, shifts right once per bit offset while pulling a bit of that
  third
  byte into bit 15, advances the position by the shift, and masks the value with
  the
  table entry for the shift.
  Evidence: the disassembly of `0x4D844` and the reader in
  `tools/gfx_container.py`.
- The result changes.  The expansion now ends after 185 bytes instead of 208636,
  and
  the first pixel is `0xAC` instead of `0x17`, so the decode still diverges and
  it now
  stops early as well.
  Evidence: the diff script.
- The mask table remains the unknown.  The reader is instruction accurate, and
  the mask
  entry for the shift decides every code, so the values of the table at
  `+0x8050` settle
  the decode.
  Evidence: the disassembly of `0x4D844` and the diff script.
- A correction to the previous note: the first divergence at output 68 means the
  first
  pixel is wrong, not that the first 68 bytes are right.  The diff script starts
  its
  comparison at the pixel offset, so it does not check the count, the size, or
  the
  records.
  Evidence: the diff script.
- The next step reads the mask table at the derived address, the stream plus
  `0x80DC`,
  found by a memory search for the file name that the stream holds at `+0x24`.
  Evidence: the stream layout.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The mask table constraint

- The mask table has two constraints from the known bytes.  The first code reads
  `0x100`, so the entry for the initial shift keeps bit eight and clears every
  bit
  above it.  The first literal reads `0x03` from the value `0x3F`, so the entry
  for
  the shift nine keeps bits zero and one and clears bits two to five.  The two
  entries
  are therefore different, so the initial shift is not nine and the code and the
  literal
  use different entries.
  Evidence: the compressed head, the plaintext, and the disassembly.
- A guessed dump of two regions at `0150:25A800` and `0150:2A800` returns data
  but not the mask table, because the stream structure address is not known and
  the
  dump addresses were guesses.  The dump files are also larger than the
  requested
  length, the same size quirk as before.
  Evidence: the mask table probe.
- The next step is a memory search.  The stream structure holds the file name at
  `+0x24`, so a search for the name finds the stream, and the table lies at the
  found address plus `0x80DC`.  The debugger offers `MEMFIND` to start a search
  and
  `MEMS` to search a value within it.
  Evidence: the stream layout and the debugger help.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The memory search syntax

- The memory search takes a size and one value: `MEMFIND [seg]:[off] [size]
  [value]`.
  The debugger reports the size, the start address, and the range it searches,
  and a
  call without a size uses one byte.  A call with several values takes the first
  as the
  value and ignores the rest, so the search finds one value and not a byte
  sequence.
  Evidence: the memory search probe.
- `MEMS` without a value compares with the previous values and reports how many
  addresses match, which is a refinement step and not a new search.
  Evidence: the memory search probe.
- The search therefore finds the file name only if it is split into single value
  steps, or
  if the debugger offers a string form.  The practical alternative is a dump of
  the game
  data region followed by an offline search for the name and for the table.
  Evidence: the memory search probe.
- The mask table constraints stand: the entry for the initial shift keeps bit
  eight and the
  entry for the shift nine keeps bits zero and one, so the entries differ and
  the initial
  shift is not nine.
  Evidence: the compressed head and the plaintext.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The mask table is in the instructions

- The mask table is read from the context init `0x4D360`, which the decoder
  calls
  after it allocates the context: `+0x8074` holds `0x1FF`, `+0x8078` holds
  `0x3FF`,
  `+0x807C` holds `0x7FF`, and `+0x8080` holds `0xFFF`.  The reader indexes the
  table at `+0x8050` with the shift, so the shift nine reads `+0x8074`, the
  shift ten
  reads `+0x8078`, and so on.
  Evidence: the disassembly of `0x4D360`.
- The table is therefore the values `(1 << shift) - 1` for the shifts nine to
  twelve, so
  the mask assumption in the port was right, and the earlier note that called it
  wrong is
  corrected.
  Evidence: the disassembly of `0x4D360`.
- The init also sets the initial state: the shift at `+0x4068` to nine, the
  threshold at
  `+0x4060` to `0x200`, the index at `+0x405C` to `0x102`, the state at
  `+0x404C`
  to zero, and the bit position at `+0x406C` from `+0x4064`, which is zero. 
  That is the
  state the port already uses.
  Evidence: the disassembly of `0x4D360`.
- The context is a heap block of `0x8490` bytes, allocated by the decoder with
  `malloc` at `0x30050` and stored in the stream field at `+0xB0`.  The mask
  table lies
  inside that block, so the earlier search for a static table was aimed at the
  wrong place.
  Evidence: the disassembly of `0x4C128` and `0x4D360`.
- The decoder also sets the stream fields from the record: the record field at
  `+0x74` bit
  zero goes to the stream `+0x14` and bit one goes to the stream `+0x10`, and
  the read
  wrapper tests the stream `+0x10` to choose the compressed path.  That confirms
  the
  directory record flag at `+0x1C` with the record at `+0x58` and the name at
  `+0x58`.
  Evidence: the disassembly of `0x4C128`.
- The port therefore matches the machine code in the reader, the masks, the
  initial state,
  and the literal branch, so the divergence lies outside the decoder: in the
  input bytes the
  port feeds it, or in a detail of the block loop that the port still
  simplifies.
  Evidence: the disassembly chain and the diff script.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The match path and the shift sweep

- The match path is read from the instructions `0x4D5E6` to `0x4D6D3`.  It
  stores
  the code in `+0x4050` and `+0x4058`, tests the code against the index at
  `+0x405C`, replaces the code with the value at `+0x4054` when the code reaches
  the
  index, and then walks a chain.  The chain reads the byte at the context plus
  the
  code times four plus two and keeps the code high bits in place with
  `(code & ~0xFF) | byte`, and it reads the next code as the 16 bit word at the
  context plus the code times four.  It ends with the byte `code & 0xFF` in
  `+0x8084`
  and `+0x8085` and the code itself in the ring.
  Evidence: the disassembly of `0x4D4C4`.
- The high bits stay in place.  The port shifted them down with `code >> 8` and
  the
  instructions keep them with `code & ~0xFF`, so the port was corrected.  The
  first
  pixel did not change, so that was not the only fault.
  Evidence: the disassembly and the diff script.
- The initial shift sweep.  The port was run with each initial shift from zero
  to
  twelve, and the shift eleven gives the first byte `0x03`, the plaintext value,
  while
  every other shift gives another value.  The head at the shift eleven is
  `03 03 03 00 00 03 00 00` against the plaintext `03 00 00 00 B8 2E 03 00`, so
  the
  first byte matches and the second does not.
  Evidence: the shift sweep.
- The init sets the shift to nine, so the first code read uses nine bits and the
  mask `0x1FF`, and the first code reads `0x100`, the literal.  The sweep shows
  that
  the first byte needs eleven bits, so the literal read offset and the shift
  advance
  are the remaining question, not the mask table.
  Evidence: the disassembly of `0x4D360` and the shift sweep.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The bit bookkeeping

- The literal width sweep with the corrected reader.  The width two gives
  `00 03 03 03 00 00 03 00` against the plaintext `03 00 00 00 B8 2E 03 00`, so
  the port emits a leading zero and then the value three three times.  Every
  other
  width gives another head.
  Evidence: the literal width sweep.
- The first code reads `0x100`, the literal, with the shift nine and the mask
  `0x1FF`, and the literal read at the width two takes the bits nine and ten,
  which
  are `1 1` and the plaintext value `0x03`.  The reader then lands two bits
  high,
  because the value at the bit position nine is `0x0C` and the mask `0x03`
  clears
  it to zero.
  Evidence: the bit analysis and the sweep.
- The remaining question is the bit position bookkeeping: the code read advances
  the
  position by the shift, which is nine, and the plaintext wants the literal at
  the
  bits nine and ten, which the port reads as `0x0C`.  The advance for the code
  and
  the offset for the literal are therefore the last open detail, and the shift
  sweep
  shows that the first byte alone is right at the shift eleven.
  Evidence: the sweeps.
- The decoder is otherwise read from the instructions: the mask table is the
  values
  `(1 << shift) - 1` for the shifts nine to twelve, the init sets the shift
  nine, the
  threshold `0x200`, the index `0x102`, the state zero, and the bit position
  zero,
  and the match path keeps the code high bits in place.
  Evidence: the disassembly of `0x4D360`, `0x4D4C4`, and `0x4D844`.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The block trace

- The block trace at the initial shift nine: the first block reads the code
  `0x00C` and emits `0x0C`, the second reads `0x03E` and emits `0x3E`, so the
  code is the literal and the value passes through the reader.  The first byte
  is
  `0x0C` against the plaintext `0x03`.
  Evidence: the block trace.
- The block trace at the initial shift eleven: the first block emits `0x03`, the
  plaintext value, then `03 03`, then `00 00 03`, then zeros.  The head is
  `03 03 03 00 00 03 00 00` against the plaintext `03 00 00 00 B8 2E 03 00`, so
  the
  first byte is right and the rest is not.
  Evidence: the block trace.
- The two traces together show the framing: at the shift nine the first block is
  one
  byte and the byte is wrong, and at the shift eleven the first byte is right. 
  The
  plaintext count of three wants one byte `0x03` and then three zero bytes, so
  the
  second block must emit three zeros, and the trace at the shift eleven emits
  two
  bytes instead.
  Evidence: the block trace.
- The reader, the mask table, the init, the literal branch, and the match path
  are all
  read from the instructions, so the remaining question is the block framing:
  how many
  bytes a block emits and which code reaches the literal path.
  Evidence: the disassembly chain.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The initial shift sweep

- The sweep of the initial shift from zero to fifteen with the instruction
  accurate
  reader shows the shift eleven is the best: its head is
  `03 03 03 00 00 03 00 00`, which matches three of the eight plaintext bytes,
  while every other shift matches two or fewer.
  Evidence: the initial shift sweep.
- The shift eleven head shows the framing: the plaintext count of three wants
  one
  byte `0x03` and then three zero bytes, and the port emits `0x03` three times
  instead.  The first byte is right and the pattern repeats, so the bit advance
  after
  the first literal is the remaining fault.
  Evidence: the initial shift sweep and the block trace.
- The reader, the mask table, the init, the literal branch, and the match path
  are
  read from the instructions, and the sweep covers the initial shift and the
  literal
  width, so the remaining question is the advance after a literal and the shift
  growth.
  The init sets the shift to nine and the threshold to `0x200` with the index
  `0x102`, so no growth happens before the second block, and the plaintext wants
  the
  second byte at a different bit position.
  Evidence: the disassembly of `0x4D360` and the sweeps.
- The next step sweeps the advance after a literal, keeping the constant mask
  `0x1FF`
  and the literal width nine, because the port then matches the machine code in
  every
  read and the advance is the last unread detail.
  Evidence: the sweeps.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The advance sweep

- The sweep over the initial shift from eight to thirteen and the literal
  advance
  from two to sixteen, with the instruction accurate reader, the constant mask
  `0x1FF`, and the literal width nine, finds no combination that produces the
  plaintext head.  The best is four of eight bytes, and that best is all zeros,
  so it
  is not a partial match of the plaintext.
  Evidence: the advance sweep.
- The free parameters are therefore exhausted: the initial shift, the literal
  width, the
  literal advance, and the mask are swept, and the reader, the init, the literal
  branch, and the match path are read from the instructions.  The plaintext head
  does
  not appear under any of them.
  Evidence: the four sweeps.
- The remaining explanation is outside the parameters: either the file layer
  feeds the
  decoder a different byte range than the member bytes the port uses, or the
  driver and
  block loop interaction holds a detail the port still simplifies.  The port
  reads the
  member at the directory offset and size, and the decoder seeks there, so the
  first
  candidate is the input range.
  Evidence: the disassembly chain and the sweeps.
- The next step compares the input range: dump the buffer the decoder fills at
  the
  driver, which the driver reads at `+0x8086` for `min(stream length, 0x400)`
  bytes,
  and compare it with the member bytes the port feeds.  The driver's initial
  read is the
  first `0x400` bytes of the member, so the two must be equal, and a difference
  names
  the fault.
  Evidence: the disassembly of `0x4D3FC`.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The complete block decoder

- The block decoder is now read in full, `0x4D4C4` to `0x4D843`, and it matches
  the
  port instruction for instruction: the state check, the code read, the end code
  `0x101`, the literal code `0x100`, the match path with the high bits kept in
  place, the ring, the output loop with the ring index counted down, the table
  update with the byte and the word, and the shift growth with the threshold
  doubled while the shift is not twelve.
  Evidence: the disassembly of `0x4D4C4`.
- The reader is read in full as well, `0x4D844` to `0x4D9B4`: the bit position,
  the byte index, the bit offset, the word with the low byte replaced by the
  byte at
  the index plus two, the shift right with a bit of that third byte pulled into
  bit
  fifteen, the position advanced by the shift, the mask table entry, the buffer
  slide,
  the refill with the read size `min(0x400 - the remaining, the length - the
  bytes
  read)`, and the bit position masked with seven.
  Evidence: the disassembly of `0x4D844`.
- The driver is read in full: the four state fields, the state test, the initial
  read
  of `min(stream length, 0x400)` bytes, the loop on the block decoder, and the
  output count returned.
  Evidence: the disassembly of `0x4D3FC`.
- The port therefore matches the machine code in the driver, the reader, the
  block
  decoder, the mask table, the init state, the literal branch, the match path,
  the
  table update, the shift growth, and the output loop.  The expansion still ends
  after 185 bytes and the first byte is wrong, so the difference is the input
  bytes or
  the first code advance.
  Evidence: the disassembly chain and the diff script.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The refill size

- The refill reads a size, not one byte.  The instructions compute the read size
  as `min(0x400 - the remaining, the length less the bytes read)` and pass it in
  EBX with the count one, while the port read one byte per refill.  The port was
  corrected.
  Evidence: the disassembly of `0x4D8A9`.
- The correction did not move the first divergence.  The expansion still ends
  after
  185 bytes and the first byte is still `0x0C` against the plaintext `0x03`,
  which is
  expected because the first refill only happens after the buffer drains and the
  first
  output bytes come from the initial read.
  Evidence: the diff script.
- The port now matches the machine code in the driver, the reader, the block
  decoder,
  the mask table, the init state, the literal branch, the match path, the table
  update,
  the shift growth, the output loop, and the refill.
  Evidence: the disassembly chain.
- The first literal reads `0x0C` where the plaintext holds `0x03`, and the shift
  is
  nine with the mask `0x1FF`, so the value is right for the position and the
  position
  is the question: the code read advances the position by the shift, nine, and
  the
  literal then reads at bit nine.  The plaintext value sits at bit eight, so the
  advance after the first code is the remaining question, and the init sets the
  bit
  position to zero from `+0x4064`.
  Evidence: the disassembly of `0x4D360` and the block trace.
- The next step reads the code read's advance: the instruction adds the shift at
  `+0x4068`, and the shift is nine, so the advance is nine, and the plaintext
  wants
  the literal one bit earlier.  Either the initial bit position is not zero or
  the first
  code is read with a different shift, and the two candidates are testable by
  setting
  the initial bit position to one.
  Evidence: the disassembly and the block trace.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The advance test

- Adding the shift before the literal read does not make the first byte correct.
  The advance of nine before the literal gives `3E AC 00 AC 00 AC 00 7E` and the
  advance of two gives `03 03 03 00 00 03 00 07`, which matches three of the
  eight
  plaintext bytes and no more.  The plaintext head is `03 00 00 00 B8 2E 03 00`,
  so
  the first byte is right at the advance of two and the rest is not.
  Evidence: the advance test.
- The advance that makes the first byte right is two, not nine, so the code read
  advances by the shift plus two, or the literal read starts two bits later than
  the
  code advance.  The shift is nine from the init, so the advance is not the
  shift.
  Evidence: the advance test.
- The port matches the machine code in every read component, and the advance
  test shows
  the plaintext head needs two more bits before the literal.  The next step
  reads the
  reader's advance instruction again, because the instruction adds the shift and
  the
  plaintext needs the shift plus two, so the shift at `+0x4068` may not be the
  value
  the port uses at that point.
  Evidence: the disassembly of `0x4D93F` and the advance test.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel data is 208568 bytes at one byte per pixel.
  Evidence: the record decode.
## The driver arguments

- The decoder passes the driver four values, and the instructions are now read:
  the handle, the context, the output buffer from the stream field at `+0x1C`,
  the
  length from the record field at `+0x14`, and the limit from the record field
  at
  `+0x18` plus one.  The driver stores them at `+0x848C`, `+0x8488`, `+0x4044`,
  and
  `+0x4048`.
  Evidence: the disassembly of `0x4C32D` and `0x4D3FC`.
- The record fields therefore carry more than the offset and the size: `+0x14`
  is the
  stream length for the reads and `+0x18` is the output limit less one.  For
  `GAMEL.SPR` they are 10010 and 24483, so the limit is 24484.
  Evidence: the raw record and the disassembly.
- The numbers do not reconcile with the plaintext.  The port expands the member
  to
  208636 bytes, and the plaintext at `0150:6BD00` holds 208568 pixel bytes,
  while
  the record limit is 24484.  The limit is therefore not the output size, and
  either
  the plaintext comes from another source or the record fields mean something
  else.
  Evidence: the raw record and the plaintext.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel bytes sum to 208568, which matches the plaintext length exactly.
  Evidence: the record decode and the plaintext.
- The next step compares the record fields with the runtime: the loader reads
  four
  bytes, then four bytes, then the records, then the pixels, and the decoder
  limit is
  24484, so the pixel read cannot come from the member expansion.  Either the
  pixel
  read goes elsewhere or the record fields are not what the port assumes.
  Evidence: the loader and the raw record.
## The loader instructions

- The loader is now read instruction by instruction, `0x257B0` to `0x258B4`, and
  it
  matches the port.  It opens the file with the mode at `0x100EB4`, reads four
  bytes
  into `DAT_00025CD4`, reads four bytes into `DAT_00025CDC`, takes the first
  `SPRITES` resource of `count1 * 0x14` bytes at `0x100EB7`, reads it, takes the
  second `SPRITES` resource of `count2` bytes at `0x100EBF`, reads it, and
  closes.
  Evidence: the disassembly of `0x257B0`.
- The two reads are four bytes each and the two resources are `count1 * 0x14`
  and
  `count2` bytes, so the loader reads the count, the size, the records, and the
  pixels, exactly as the port assumes.
  Evidence: the disassembly of `0x257B0`.
- The driver limit contradicts the loader.  The loader reads `count2` bytes,
  208568 for
  `GAMEL.SPR`, while the driver limit is the record `+0x18` plus one, 24484, so
  the
  driver cannot supply the pixels from the member expansion under the limit as
  read.
  Evidence: the disassembly of `0x257B0` and `0x4C32D`.
- The record field at `+0x18` is therefore not the output size, and the port
  reads it
  as the end of the member.  The next step compares the field with the runtime:
  the
  driver limit is the record `+0x70` plus one, which is `+0x18` plus one, so the
  field
  must be something the driver can use as a limit while the member expands to
  208636
  bytes.
  Evidence: the raw record and the two disassemblies.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, which matches the plaintext length exactly.
  Evidence: the record decode and the plaintext.
## What the record field limits

- The record field at `+0x18` is read in the decoder at `0x4C347`, where the
  instructions take the record field at the stream `+0x70`, which is the record
  `+0x18` because the record sits at the stream `+0x58`, and add one to it.  The
  value then goes to the driver as the fourth argument at `0x4C360`.
  Evidence: the disassembly of `0x4C128`.
- The driver stores that argument at `+0x4048`, and the block decoder uses it as
  the
  output loop bound: the output loop compares the output count at `+0x4070` with
  it and
  loops while the two differ, so the field limits the output byte count of the
  expansion.
  Evidence: the disassembly of `0x4D3FC` and `0x4D4C4`.
- The field is 24483 for `GAMEL.SPR`, so the limit is 24484, while the loader
  reads
  208568 pixel bytes from the stream and the port expands the member to 208636
  bytes.
  The field therefore cannot bound the expansion that the loader reads, and the
  two
  numbers disagree by a factor of eight and a half.
  Evidence: the raw record, the loader, and the driver.
- The next step compares the file the loader opens with the member the port
  decodes.
  The loader takes the file name from its own argument, and the port decodes
  `GAMEL.SPR` from `GAME.DAT`, so a different file would explain the
  disagreement.
  Evidence: the loader argument and the member list.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, which matches the plaintext length exactly.
  Evidence: the record decode and the plaintext.
## The loader callers and the member

- The race init `FUN_000131a8` calls the loader twice with the file and the
  member.
  At `0x13426` it builds the file `%s\gfx\graf.dat` at `0x1004F1` and the member
  `logo%c.spr` at `0x100501` with the letter `H`, and at `0x134B6` it builds the
  file `%s\gfx\game.dat` at `0x100524` and the member `game%c.spr` at `0x100534`
  with the letter `H`.  The font loads follow the same shape with `attr%c.fnt`
  at
  `0x1004E6` and `game%c.fnt` at `0x100519`.
  Evidence: the disassembly of `0x133A0` to `0x134CA`.
- The file is therefore `gfx\game.dat`, the container the port reads, and the
  member
  is `gameH.spr` for the high build and `gameL.spr` for the low build.  The
  letter is
  the same `H` the asset strings use, so the high executable loads the high
  member.
  Evidence: the call sites and the strings.
- The port decodes the wrong member.  It decodes `GAMEL.SPR`, the low member of
  10010 compressed bytes, while the runtime under `STARTH.EXE` loads
  `gameH.spr`,
  the high member of 28736 compressed bytes.  The two are different members of
  the
  same container, so the plaintext can never match the port expansion.
  Evidence: the member list, the call sites, and the plaintext.
- The plaintext at `0150:6BD00` holds 208568 pixel bytes, which is the high
  member
  expansion, and the sprite dimensions 200 by 240, 466 by 188, and 640 by 114
  sum to
  exactly that count.
  Evidence: the plaintext and the record decode.
- The next step decodes `GAMEH.SPR` instead of `GAMEL.SPR` and compares the
  expansion with the plaintext again.  The member choice was the last
  difference, and the
  decoder itself is read from the instructions.
  Evidence: the call sites and the disassembly chain.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, matching the plaintext length exactly.
  Evidence: the record decode and the plaintext.
## The loader callers and the member

- The race init `FUN_000131a8` calls the loader twice with the file and the
  member.
  At `0x13426` it builds the file `%s\gfx\graf.dat` at `0x1004F1` and the member
  `logo%c.spr` at `0x100501` with the letter `H`, and at `0x134B6` it builds the
  file `%s\gfx\game.dat` at `0x100524` and the member `game%c.spr` at `0x100534`
  with the letter `H`.  The font loads follow the same shape with `attr%c.fnt`
  at
  `0x1004E6` and `game%c.fnt` at `0x100519`.
  Evidence: the disassembly of `0x133A0` to `0x134CA`.
- The file is therefore `gfx\game.dat`, the container the port reads, and the
  member
  is `gameH.spr` for the high build and `gameL.spr` for the low build.
  Evidence: the call sites and the strings.
- The port decodes the wrong member.  It decodes `GAMEL.SPR`, the low member of
  10010 compressed bytes, while the runtime under `STARTH.EXE` loads
  `gameH.spr`,
  the high member of 28736 compressed bytes.  The two are different members of
  the
  same container, so the plaintext can never match the port expansion.
  Evidence: the member list, the call sites, and the plaintext.
- The plaintext at `0150:6BD00` holds 208568 pixel bytes, which is the high
  member
  expansion, and the sprite dimensions 200 by 240, 466 by 188, and 640 by 114
  sum to
  exactly that count.
  Evidence: the plaintext and the record decode.
- The next step decodes `GAMEH.SPR` instead of `GAMEL.SPR` and compares the
  expansion with the plaintext again.
  Evidence: the call sites and the disassembly chain.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, matching the plaintext length exactly.
  Evidence: the record decode and the plaintext.
## Decoding the high member

- The port decodes `GAMEH.SPR` and it still fails: the expansion is two bytes in
  three blocks against the plaintext of 208568 pixel bytes, and `GAMEL.SPR`
  expands
  to 185 bytes in 99 blocks.  The member choice is therefore one fault and the
  decoder holds another, because neither member expands.
  Evidence: the member test.
- The two members share their first four bytes, `00 7F 00 18`, so the decoder
  reads
  the same first block for both and diverges at the same point.  The shared head
  is a
  header, as the earlier note said, and the divergence is inside the block
  framing.
  Evidence: the two member heads.
- The driver limit stands against both members: 24484 for `GAMEL.SPR` and 93676
  for
  `GAMEH.SPR`, while the plaintext holds 208568 pixel bytes, which is the high
  member expansion.  The limit is therefore not the output size for either
  member.
  Evidence: the records and the plaintext.
- The next step compares the driver limit with the expansion for the high
  member: the
  driver stops the output when the count reaches the limit, and the plaintext
  needs
  208636 bytes, so either the driver is called more than once or the field means
  something else.
  Evidence: the driver and the loader.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, matching the plaintext length exactly.
  Evidence: the record decode and the plaintext.
## The loader breakpoint

- The breakpoint on the loader statics fires once, at `CS=A5C EIP=32D`, an early
  write during initialisation, and the statics then hold the fill pattern
  `55 55 55`, so the loader never ran in the probe window.
  Evidence: the loader breakpoint probe.
- The loader runs on two paths only: the network menu through
  `menu_space_init` at `0x29D08`, which loads `menu%c.spr` from `gfx\menu.dat`,
  and the race through the race init at `0x134B6`, which loads `game%c.spr` from
  `gfx\game.dat`.  A single player start reaches neither, which is why every
  probe saw fill patterns in the loader statics.
  Evidence: the loader call sites and the probe.
- The driver fields therefore cannot be dumped in the menu idle.  A probe that
  wants
  them must first drive the game into a race or into the network menu, and the
  earlier driven runs reached neither.
  Evidence: the driven runs.
- The next step drives the game into a race with the key sequence and then dumps
  the driver fields at `+0x4048` and `+0x4044`, which the decoder sets from the
  record `+0x70` plus one and `+0x6C`.
  Evidence: the call sites and the driver.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, matching the plaintext length exactly.
  Evidence: the record decode and the plaintext.
## The instrumented file reads

- The instrumented read probe catches three events.  The first is the read call
  itself at
  `CS=824 EIP=17A5` with `ECX=1C`, which is a 28 byte read through the real mode
  path.  The second and third are the return and the idle: the third sits at
  `CS=150 EIP=1E1C40`, the demo motion routine inside `0x21BA0`, the same idle
  address every probe has reached.
  Evidence: the instrumented read probe.
- No read carries a count of 10010 or 28736, the two sprite member sizes, so the
  container member is never read and the loader never runs.  The LZSS call
  therefore
  cannot be instrumented in the demo idle, and the runtime values of the record
  pointer,
  the length, and the output count cannot be read there either.
  Evidence: the instrumented read probe and the member list.
- The port values stand as the only measured set: the driver length 10010 for
  the low
  member, the driver limit 24484, and the output count 185.  The runtime values
  need a
  probe that reaches a race or the network menu.
  Evidence: the driver and the port.
- The next step drives the game into a race with the key sequence and then dumps
  the
  driver fields at `+0x4048` and `+0x4044`, which the decoder sets from the
  record
  `+0x70` plus one and `+0x6C`.
  Evidence: the call sites and the driver.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, matching the plaintext length exactly.
  Evidence: the record decode and the plaintext.
## The loader runs in the demo

- The loader runs during the demo, not only on the two menu paths.  A probe that
  waits
  twenty seconds and then breaks lands inside the loader at `EIP=1E7CB0`, which
  is module
  `0x25CB0`, inside the loader at `0x257B0`.  The demo therefore loads the
  sprite
  member, and every earlier probe that waited less than that saw the fill
  pattern.
  Evidence: the race probe.
- The loader statics hold the runtime values at `0150:1E7CD4` and `0150:1E7CCC`:
  the count is 3, the size is 208568, the record pointer is `0x0008D800`, and
  the pixel
  pointer is `0x002CAB14`.  The size matches the pixel byte count exactly, so
  the LZSS
  output count is 208568.
  Evidence: the two dumps.
- The port output count is 185 for `GAMEL.SPR` and 2 for `GAMEH.SPR`, so the
  port and
  the runtime differ by a factor near 1127 and the port is broken.  The runtime
  count is
  the reconciliation target: 208568.
  Evidence: the port test and the statics.
- The driver length is the record `+0x6C`, which is 10010 for `GAMEL.SPR` and
  28736 for
  `GAMEH.SPR`, and the driver limit is the record `+0x70` plus one, which is
  24484 and
  93676.  The runtime output count is 208568 for a member, so the limit is not
  the output
  bound for either member and the driver must run more than once or the field
  bounds
  something else.
  Evidence: the records and the statics.
- The member read did not appear in the break window, because the game issues
  many small
  reads and the breaks landed between them.  A probe that filters on the read
  count 28736 or
  10010 catches the member read and its record pointer.
  Evidence: the member probe.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568, matching the runtime size.
  Evidence: the record decode and the statics.
## The buffer hunt

- The statics at `0150:1E7CCC` after thirty seconds hold the loader results: the
  record
  pointer `0x0008D800`, the second pointer `0x002CAB14`, the count 3, the flag
  1, the size
  208568, the third pointer `0x006BD000`, and the screen 640 by 400.  The loader
  therefore
  completed.
  Evidence: the watch probe.
- The buffer at `0x006BD000` holds `FF` fill at thirty seconds and at thirty
  eight seconds,
  so the loader did not write it in this run, and the region at `0x002CAB14`
  plus `0x4044` is
  all zero, so it is not the driver context.  Neither candidate holds the
  expansion output.
  Evidence: the context probe.
- The file reads show two families: the C runtime reads at `EIP=5FD6` with
  counts 4, 6, 28,
  176, and 16, and the extender reads at `EIP=26D9` with counts of 8192 bytes
  into the low
  buffer `0x2380`, plus decreasing tails 7916, 7852, 7804, and 7740.  The read
  with the count
  208568 never appeared, so the pixel read goes through the buffered path.
  Evidence: the read loop probe.
- The requested byte for byte comparison is therefore not complete: the
  expansion output buffer has
  not been captured.  The port values stand against the runtime statics, and the
  runtime output
  count is 208568 while the port gives 185 for the low member and 2 for the high
  member.
  Evidence: the statics and the port.
- The next step breaks on the read with the count 208568 and reads `EDX`, the
  destination
  buffer, at that moment.  The read is buffered through the extender, so the
  probe must also dump
  the low buffer `0x2380` at the read return.
  Evidence: the read loop probe.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and the
  offsets chain exactly to 208568.
  Evidence: the record decode.
## The decoder is LZW, not LZSS

- The match path of the block decoder walks a dictionary chain.  The code 0x101
  ends
  the stream, the code 0x100 clears it, and the first free entry is 0x102, which
  are the
  classic LZW markers.  The table at `+0x4074` holds four byte entries with the
  index at
  `+0x4064`, and the shift table at `+0x8050` grows the code width from nine to
  twelve
  bits with the masks 0x1FF, 0x3FF, 0x7FF, and 0xFFF.  The decoder is therefore
  LZW,
  and the port implements LZSS, which is a different algorithm.
  Evidence: the disassembly of `0x4D4C4` and `0x4D844`.
- The correct LZW structure reproduces the shape of the runtime output.  The
  runtime
  records begin `03 00 00 00 B8 2E 03 00`, which is the count 3 and the size
  208568, and
  the LZW decoder emits `3F 00 00 00 F7 68 01 00`, the same shape with the
  values
  differing.  The shape agreement shows the structure, and the value difference
  remains in the
  bit reading.
  Evidence: the LZW test and the records.
- The reader replaces the low byte of the 16 bit word.  The port does this, and
  the
  instructions do not: they keep the full word and use the third byte only for
  the bits that
  the shift pulls in.  This is one port fault, found by comparing the
  instructions with the
  code.
  Evidence: the disassembly of `0x4D916`.
- The records are confirmed with 32 bit fields: sprite 0 is 0, 0, 200, 240, 0,
  sprite 1 is
  1, 1, 466, 188, 48000, and sprite 2 is 0, 0, 640, 114, 135608.  The offsets
  chain
  exactly to 208568, which is the pixel count and the runtime size.
  Evidence: the record dump.
- The next step rewrites the port as LZW with the reader from the instructions
  and tests
  the first eight output bytes against the runtime record head.
  Evidence: the port and the records.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114, and
  the pixel bytes sum to 208568.
  Evidence: the records.
## The LZW decoder is confirmed

- The member expansion is confirmed against the run-time records.  `LOGOH.SPR`
  of
  `GFX/GRAF.DAT` expands to 208636 bytes, the sprite count at `+0x00` is 3, the
  pixel size at `+0x04` is 208568, and the three records are 0, 0, 200, 240, 0,
  then
  1, 1, 466, 188, 48000, then 0, 0, 640, 114, 135608.  Those are exactly the
  records the run-time holds at `0150:8D800`, so the decoder is solved.
  Evidence: the expansion test and the record dump.
- The decoder is LZW with the codes 0x100 clear and 0x101 end, the first free
  entry 0x102, and the code width from 9 to 12 bits.  The width grows when the
  next
  free entry reaches `1 << shift`, checked before the entry is added.  The
  reader
  keeps the full 16 bit word and uses the byte at `+2` only for the bits that
  the bit
  offset pulls in at bit 15.
  Evidence: the disassembly of `0x4d4c4`, `0x4d844`, and `0x4d916`.
- The port is correct on `LOGOH.SPR` and the earlier mismatch had two causes:
  the
  member was wrong, because the demo loads `logoH.spr` from `gfx\graf.dat` and
  not
  `gameH.spr` from `gfx\game.dat`, and the algorithm was wrong, because the port
  implemented LZSS and the game uses LZW.  The record `end` field equals the
  expanded size, which is `8 + count * 0x14 + size`, and it is 208636 for
  `LOGOH.SPR`.
  Evidence: the member list, the call sites, and the expansion test.
- The pixel bytes are the 208568 bytes after the 68 byte head, which is the
  count,
  the size, and the three records.
  Evidence: the expansion.
- The next step reads the pixel bytes of the expansion and confirms the sprite
  images
  against the screen.
  Evidence: the expansion.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel bytes sum to 208568, which is the pixel size at `+0x04`.
  Evidence: the expansion and the record dump.
## The member structure table

- The expansion confirms the sprite structure on both logo members:
  `LOGOL.SPR` expands to 52538 bytes, the count is 3, the size is 52470, and
  `8 + 3 * 0x14 + 52470` equals 52538, and `LOGOH.SPR` expands to 208636
  bytes, the count is 3, the size is 208568, and `8 + 3 * 0x14 + 208568` equals
  208636.  Both match the member end field.
  Evidence: the expansion test.
- The game sprite member `GAMEL.SPR` holds the right header, the count 63 and
  the
  size 23215, so `8 + 63 * 0x14 + 23215` equals 24483, which is the member end
  field, and the expansion stops at 5825 bytes before the end.  The decoder
  needs a
  fix for that stream, and the same holds for `MENUL.SPR`, whose header count 29
  and size 74756 give 75344 against the end field 20892.
  Evidence: the expansion test.
- The font members and the `PKC` members hold different structures, and the
  `3DX`
  members, the palette `MENU.COL`, and `MENU.MLB` are uncompressed with the flag
  clear.
  Evidence: the member list.
- The next step fixes the remaining streams and reads the pixel bytes of the
  logo
  expansion to confirm the sprite images against the screen.
  Evidence: the expansion test.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel bytes sum to 208568.
  Evidence: the expansion and the record dump.
## The member end fields are confirmed

- The LZW decoder expands every sprite member of `GFX/GRAF.DAT` and
  `GFX/GAME.DAT` to its member end field exactly: `LOGOL.SPR` to 52538,
  `LOGOH.SPR` to 208636, `MENUL.SPR` to 75344, `MENUH.SPR` to 294846,
  `GAMEL.SPR` to 24483, and `GAMEH.SPR` to 93675.  The count, the size, and
  the record chain hold in every case, so the member end field is the expanded
  size and the decoder is confirmed on six members.
  Evidence: the expansion test.
- The stream writes the end code inside a member as well.  The first end code of
  `GAMEL.SPR` arrives at 5825 expanded bytes with 1611 codes read, and the
  stream continues after it: the dictionary resets at that point and the stream
  continues at the same bit position with the previous entry kept.  `GAMEH.SPR`
  writes 59 such end codes.
  Evidence: the code trace and the expansion test.
- The clear code keeps the previous entry.  The variant that clears it fails
  `GAMEL.SPR` at 5825 bytes, so the game keeps the previous entry across a
  clear.
  Evidence: the variant test.
- The font members and the `PKC` members hold their own structures and are not
  sprite members, so the structure test applies to the sprite members only.
  Evidence: the member list.
- The next step reads the pixel bytes of the expansion and confirms the sprite
  images against the screen.
  Evidence: the expansion test.
- The sprite count is 3, the dimensions are 200 by 240, 466 by 188, and 640 by
  114,
  and the pixel bytes sum to 208568.
  Evidence: the expansion and the record dump.
## The font member layout

- The font loader at `0x25658` reads the count (one byte) at `+0x00`, then
  `count + 1` descriptors of 12 bytes each into the table at `0x1ca98`, then
  `count + 1` bitmaps of `0x500` bytes each into the table at `0x119898`.  The
  descriptor holds three 32 bit fields: the glyph height, then the two font wide
  constants 32 and 76.
  Evidence: the disassembly of `0x25658`.
- The descriptor layout is confirmed against the expanded bytes.  `MENUL.FNT`
  holds
  the heights 11, 9, 19, and 30 and `MENUH.FNT` holds 22, 18, 38, and 60,
  exactly
  double, and `ATTRL.FNT` holds 19 against `ATTRH.FNT` with 38.  The second and
  third fields are 32 and 76 in all four fonts.
  Evidence: the font command output.
- The bitmap region holds pixel data: the first 1,280 byte bitmap of `ATTRL.FNT`
  holds 120 distinct values with the values 0 and 255 dominant, which is a glyph
  mask.
  Evidence: the font command output.
- The member holds more data after the first `count + 1` bitmaps: the tail is
  8,243
  bytes in `ATTRL.FNT`, 29,551 in `MENUL.FNT`, 31,822 in `ATTRH.FNT`, and
  121,224 in `MENUH.FNT`.  The tail is the next open question of the font
  layout.
  Evidence: the font command output.
- The next step reads the font tail and finds its structure, and reads the
  bitmap
  rows to draw the glyphs.
  Evidence: the font command.
- The font members are not sprite members and hold their own structure, so the
  sprite
  count, the size, and the record chain do not apply to them.
  Evidence: the member list.
## The font tail is the FONTS resource

- The loader reads the tail as a resource of its own.  After the first
  `count + 1` bitmaps it reads a 4 byte size at `+0x1cb10`, then calls
  `resource_get(engine, "FONTS", size)` at `0x25755` and reads the tail into
  the resource buffer at `0x1543c`.  The string `FONTS` sits at `0x100ead`.
  Evidence: the disassembly of `0x25733` and the string dump.
- The tail holds the remaining glyph bitmaps of `0x500` bytes each.
  `MENUL.FNT` holds 23 full bitmaps and a 111 byte remainder, `ATTRL.FNT`
  6 full and 563, `ATTRH.FNT` 24 full and 1102, and `MENUH.FNT` 94 full and
  904.
  Evidence: the font command output.
- The tail blocks hold pixel data: the first block of `MENUL.FNT` holds 201
  distinct values with 8, 0, and 210 dominant, and the first block of
  `ATTRH.FNT` holds 29 distinct values with 0 and 255 dominant, which is a
  glyph mask.
  Evidence: the font command output.
- The sprite loader uses the same resource call with the string `SPRITES` at
  `0x100eb7`, so the two loaders share the resource layer and the `FONTS` and
  `SPRITES` names.
  Evidence: the disassembly of `0x257b0` and the string dump.
- The next step reads the glyph rows to draw the glyphs and finds the meaning of
  the tail remainder.
  Evidence: the font command.
- The font members are not sprite members and hold their own structure, so the
  sprite count, the size, and the record chain do not apply to them.
  Evidence: the member list.
## The glyph render does not confirm the bitmap layout

- The glyph rows are read at every plausible layout and none renders a glyph. 
  The
  bit packed reads at 32 and 64 bits per row with the most significant and the
  least significant bit first, and the byte reads at 8, 16, 32, and 64 pixels
  per
  row all fail to show a letter or a digit.
  Evidence: the render command.
- The first bitmap of `MENUL.FNT` renders as a regular dither pattern in the
  byte
  read: the values are small, 1 to 9, with 0 dominant in 817 of the 1280 bytes,
  and
  the pattern marches on a diagonal grid.  A glyph mask would hold a letter
  shape.
  Evidence: the render command.
- The dense glyphs of heights 19 and 30 and the tail blocks render as noise in
  every
  layout, so the pixel format or the row geometry is still wrong, or the data is
  not
  a glyph mask at all.
  Evidence: the render command.
- The layout that the loader reads is confirmed and the pixel interpretation is
  not.
  The descriptor is three 32 bit fields, the height, then 32 and 76, and the
  bitmaps are 0x500 bytes each, but the rows do not draw a glyph.
  Evidence: the loader and the render command.
- The next step finds the draw routine that reads the bitmap and copies the
  shape
  from the loader tables at `0x1ca98` and `0x119898` to the screen, which names
  the pixel format and the row order.
  Evidence: the loader.
- The render command now prints a glyph bitmap, so the check repeats with one
  command.
  Evidence: the render command.
## The font draw routine

- The draw routine sits at `0x25000` to `0x25500`.  It reads the count at
  `0x15440` at `0x25079` and `0x254c2`, the descriptor table pointer at
  `0x15438` at `0x250a2`, the `FONTS` buffer at `0x1543c` at `0x25269`, and the
  bitmap table pointer `0x19898` at `0x2562b` and `0x25720`.
  Evidence: the reference search and the disassembly.
- The routine computes the bitmap size as `0xa0 * 8`, which is `0x500`, the
  confirmed bitmap size, and the screen stride as `0x280`, which is 640, the
  screen
  width.  It reads the descriptor height at `+0` and the fields `+4` and `+8`,
  and
  it computes the glyph step with `imul` on the count.
  Evidence: the disassembly of `0x25089` and `0x2528b`.
- The render at 8 by 8 and at 8 wide by the descriptor height shows no glyph, so
  the pixel format and the row order stay open.  The routine holds the next
  place
  to read: the block after `0x25300`, which fetches the pixel bits from the
  bitmap
  and writes them to the screen.
  Evidence: the render command and the disassembly.
- The next step reads the block after `0x25300` and follows the pixel fetch and
  the
  screen write, which names the pixel format and the row order.
  Evidence: the draw routine.
- The draw routine reads the font tables at `0x1ca98` and `0x19898`, which
  confirms
  that the two tables are the descriptors and the bitmaps.
  Evidence: the reference search.
## The font pixel format and row order are confirmed

- The pixel format is four bits per pixel, two pixels per byte, with the high
  nibble first, the zero nibble transparent, and the colour base at `0x15460`
  added to the value.  The draw routine reads one byte at `0x25316`, takes the
  high nibble with `and al, 0xf0` and `shr al, 4`, writes it at `0x2532f`, takes
  the low nibble with `and bl, 0xf`, writes it at `0x2534a`, and skips the zero
  nibble with `je` at `0x25324`.
  Evidence: the disassembly of `0x25300` to `0x25500`.
- The row order is top to bottom with a stride of `0x280`, which is 640, the
  screen width: `add edi, 0x280` at `0x2537a`, and the column loop compares
  the column with the width from the descriptor at `0x25300`.
  Evidence: the disassembly of `0x25300` to `0x25500`.
- The draw routine also confirms the bitmap size as `0xa0 * 8`, which is
  `0x500`,
  from the draw side as well as the load side.
  Evidence: the disassembly of `0x25089`.
- The first glyphs of the font render as a regular hatch pattern with a period
  of
  seven pixels, so the first glyphs are a dither or shading texture and not a
  letter shape.  That is a property of the data and not of the pixel format.
  Evidence: the render command in the nibble mode.
- The next step finds the glyphs that draw a letter, or reads the draw caller to
  find which characters the font serves.
  Evidence: the render command.
- The render command now takes the nibble mode for four bit pixels.
  Evidence: the render command.
## The hatch pattern is not the member or the offset

- The member is not wrong.  The render uses the four members the loader loads,
  `MENUL.FNT`, `MENUH.FNT`, `ATTRL.FNT`, and `ATTRH.FNT`, and the loader call
  sites load `attr%c.fnt` and `game%c.fnt` in the race and `menu%c.fnt` in the
  menu, so the members match.
  Evidence: the loader call sites and the render command.
- The offset is not wrong either.  The render covers the first `count + 1`
  bitmaps,
  the tail blocks, and the glyph 65, which is the letter A, and every one is
  noise
  or a hatch.  `MENUH.FNT` holds 98 glyphs, which is a full ASCII set, and its
  glyph 65 is not a letter.
  Evidence: the render command in the nibble mode.
- The pixel format is confirmed from the draw routine, so the data is read as
  the
  game reads it, and the hatch is the content of the data.
  Evidence: the disassembly of `0x25300` to `0x25500`.
- The draw routine sits in the function `0x24da5` to `0x25622`, which also holds
  a
  record search on 28 byte records and the tables `0x197d8`, `0x19880`, and
  `0x192a4`, so the function serves more than the font.
  Evidence: the function list and the disassembly of `0x24da5`.
- The most likely explanation is that the first bitmaps are a dither or shading
  texture and the game does not draw the letters from them, or the game maps the
  characters to other glyphs than the plain order.
  Evidence: the render command.
- The next step finds the caller of the draw routine, which is the text routine,
  and
  follows the character code to the glyph index, which names the mapping.
  Evidence: the draw routine.
## The draw signature and the mapping

- The draw entry is `0x25020`, and the signature is `draw(x, y, string, font,
  color)`.  The routine stores `eax` at `0x15450` and checks it against 640, so
  `eax` is the screen x; it stores `edx` at `0x15454` and checks it against the
  screen height, so `edx` is the screen y; it stores `ebx` at `0x15458`, which
  holds the string; it stores `ecx` at `0x1545c` and checks it against the
  count,
  so `ecx` is the font index; and it takes the stack word, shifts it left by
  four,
  and stores it at `0x15460`, so the word is the color and the base is the color
  times sixteen.
  Evidence: the disassembly of `0x25020`.
- The glyph index is the character code.  The descriptor address is the font
  index
  times twelve from the table at `0x1ca98`, and the bitmap address is the font
  index times `0x500` from the table at `0x19898`.
  Evidence: the disassembly of `0x25089` to `0x250a8`.
- The callers pass constant font indices.  The caller at `0x234bb` draws with
  the
  font 0 and the color 6, and the caller at `0x21788` draws with the font 5 and
  the color 9, and the callers at `0x23562` and `0x23650` draw with the fonts 1,
  3, and 4, all with sprintf built strings.
  Evidence: the disassembly of the callers.
- The fonts of a member are the count plus one.  `MENUL.FNT` holds the fonts 0
  to
  3, which matches the count 3, and the first glyph of each font is a dither or
  shading pattern, so the first glyph is not a letter and the letters follow it
  in
  the font.
  Evidence: the loader and the render command.
- The next step follows one string from the sprintf call to the draw and reads
  the
  font that the game uses for a menu label, which confirms the character order.
  Evidence: the callers.
- The mapping is therefore settled: the font index is the argument, the string
  holds
  the characters, and the character code is the glyph index.
  Evidence: the draw routine and the callers.
## The character order is the ASCII order

- The caller at `0x234fa` builds the string `"%d%c"` at `0xde4` with the
  character `':'`, which is `0x3a`, and draws it with the font 1 and the color
  5.
  The format `"%d%c"` is at `0x100de4` and the character `0x3a` is the colon.
  Evidence: the disassembly of `0x234fa` and the format string.
- The caller at `0x234bb` draws with the font 0 and the color 6, and its string
  comes from `sprintf` with the format `"%d"`, which is a number, and the caller
  at `0x21788` draws with the font 5 and the color 9.  The fonts 0 and 1 hold
  the
  digits and the fonts 2 and 3 hold the letters, so the character code is the
  glyph index.
  Evidence: the disassembly of the callers and the format strings.
- The mapping is therefore settled: the character order inside the font is the
  ASCII
  order, the font index selects the font, and the string holds the characters.
  The first glyph of a font is a dither or shading pattern, and the letters
  start
  after it.
  Evidence: the callers, the format strings, and the render command.
- The next step renders the glyph that a menu label uses, with the font index
  and
  the character from the caller, and reads the letter shape.
  Evidence: the callers.
## The asset export and the whiteboard

- `tools/export_assets.py` writes the assets as PNG files and a whiteboard page.
   It
  exports six sprite members, six font members, and 38 PCX members, and the page
  shows them with their names and their sizes.
  Evidence: the export run.
- The export confirms the formats end to end.  The PCX files decode with their
  own
  palette: `LOAD1L.PCX` holds 245 distinct colours at 320 by 200 and
  `LOAD1H.PCX`
  the same at 640 by 400, and the menu screens hold 24 to 53 colours.  The
  sprite
  sheets hold 71 to 207 colours, and the font sheets hold 16 grey levels, which
  is
  the four bit nibble scaled by 17.
  Evidence: the export verification.
- The low and the high images share their dominant colour, for example
  `LOAD1L.PCX`
  and `LOAD1H.PCX` both hold `3c 6f 9b`, which confirms that the pairs are the
  resolution variants of one image.
  Evidence: the export verification.
- The sprite sheets stack the sprites one under the other because the record x
  and y
  fields are not screen positions, and the sheet size is the sum of the sprite
  heights.
  Evidence: the export notes.
- The next step draws the sprites at their record x and y and finds the
  composition of
  the screens, and reads the font sheets for the letter shapes.
  Evidence: the export.
- The tool writes only under the output directory and uses the standard library.
  Evidence: the tool.
## The asset review and the scale option (superseded)

- This section is superseded.  The sprite and font readings below come from the
  broken LZW expansion.  The PCX review and the `--scale` option stay valid.  See
  The LZW reader was the last defect in the asset decode and The font member
  layout is confirmed.
  Evidence: the two sections.
- The user review of the whiteboard: the PCX images render correctly, the sprite
  sheets render the first rows correctly and then show garbage, and the font
  sheets
  show only noise.
  Evidence: the user review.
- The sprite row stride is correct.  The record offsets chain exactly at one
  byte per
  pixel: `200 * 240` is 48000, which is the offset of the second sprite, `466 *
  188`
  is 87608, and `48000 + 87608` is 135608, the offset of the third sprite.  A
  smoothness sweep over the full height also names the width as the smoothest
  stride.
  The garbage is therefore the content, which is a dither shading, and not a
  stride
  fault.
  Evidence: the record chain and the smoothness sweep.
- The font bitmaps are dither textures.  The first bitmap of `MENUL.FNT` holds
  141
  distinct values with 0 dominant in 817 of the 1280 bytes, and the four bit
  reading
  holds all 16 values, so the data is a dither and not a letter shape.  The
  regular
  grid period scales with the render width, which marks a texture.
  Evidence: the font command.
- The export takes a `--scale` option that scales the sprite and font images by
  whole
  numbers, so the dither detail is visible.  The default is four.
  Evidence: the export run.
- The next step finds the draw of one character and compares the pixels it
  writes with the
  font bitmap, which names the letter glyphs and the mapping.
  Evidence: the draw routine.
## The font descriptors vary per font (superseded)

- This section is superseded.  The descriptor numbers below are correct, and the
  interpretation of the second field as the glyph width is wrong.  See The font
  member layout is confirmed.
  Evidence: that section.
- The font descriptors are not constants.  `GAMEL.FNT` holds the descriptors
  `(30, 32, 76)`, `(27, 48, 11)`, `(10, 48, 10)`, and `(6, 48, 11)`, and
  `GAMEH.FNT` holds `(60, 32, 76)`, `(54, 48, 11)`, `(18, 48, 10)`, and
  `(12, 48, 11)`.  The menu and attribute fonts hold `(11, 32, 76)`, `(9, 32,
  76)`,
  `(19, 32, 76)`, and `(30, 32, 76)`, and `(19, 32, 76)` and `(38, 32, 76)`.
  Evidence: the font page snapshot.
- The second field is the glyph width.  It is 32 for the menu and the attribute
  fonts and 48 for the game fonts, so the render of the game fonts with a width
  of 32
  is wrong and the glyphs look like noise.
  Evidence: the font page snapshot.
- The high fonts double the first field of the low fonts: 30 to 60, 27 to 54, 10
  to 18,
  6 to 12, 11 to 22, 9 to 18, 19 to 38, and 30 to 60, which confirms the
  resolution pair.
  Evidence: the font page snapshot.
- The font page draws every block in four readings at a scale of eight and adds
  one
  image per descriptor with the width and the height of that descriptor, so the
  glyphs
  render at their own size.
  Evidence: the font page.
- The next step reads the glyph of one font at the descriptor width and the
  descriptor
  height and checks the shape, which names the letter glyphs.
  Evidence: the font page.
## The descriptor second field is the row stride in bytes (superseded)

- This section is superseded.  The readings below come from the broken LZW
  expansion.  The second field is the first character code and the third field is
  the character count.  See The font member layout is confirmed.
  Evidence: that section.
- The user review of the font page: the only somewhat readable image is the
  `ATTRH.FNT` glyph at four bits and 64 pixels per row, which shows some
  symbols,
  and the one byte reading at 32 pixels per row shows the same fragment. 
  Everything
  else is garbage.
  Evidence: the user review.
- The two readable readings share the same row stride.  Four bits at 64 pixels
  per row
  is 32 bytes per row, and one byte at 32 pixels per row is also 32 bytes per
  row, so
  the two readings agree on the stride and differ on the pixel format.  The
  descriptor
  second field is therefore the row stride in bytes and not the width in pixels.
  Evidence: the two readings and the descriptor.
- The `ATTRH.FNT` descriptor is `(38, 32, 76)`: the height is 38 rows, the
  stride
  is 32 bytes, and the glyph holds 1216 bytes, which is `38 * 32`, and the third
  field 76 is 1216 divided by 16, so the third field is the glyph size in 16
  byte
  units.
  Evidence: the descriptor and the render.
- The font page now draws every descriptor at its own stride in four bits at
  twice the
  stride in pixels and in one byte at the stride in pixels, with the height of
  the
  descriptor, and the `ATTRH` glyph at four bits and 64 pixels per row is the
  reading
  that shows symbols.
  Evidence: the font page.
- The next step draws the `ATTRH` font glyphs at the stride of the descriptor
  and
  reads the letters, and checks the third field against the glyph size on the
  other
  fonts.
  Evidence: the font page.
## Method

The investigation uses these steps:

1. inspect the installer package with `pkgutil`, `xar`, and `tar`,
2. compare the installed bundle with the payload,
3. inventory the files with SHA-256 digests,
4. parse the LE headers and the tables with `tools/le.py`,
5. disassemble the code at selected addresses with capstone,
6. run the game in DOSBox-X and drive its debugger over the control
   socket with `tools/dosboxx_control.py` and `tools/entry_probe.py`,
7. cross-check the static view with radare2 in the Nix dev shell,
8. record every claim with its evidence and its limits.

REA covers the package identity and the Mach-O targets.  REA does not
open the DOS LE executables, so the project carries its own LE parser.
The bundled DOSBox 0.74-2 has no debugger, so the project uses DOSBox-X.

## The PERSPTAB loader byte changes and the loader does not run

- The race-start probe runs 20 steps with 0 failures.  All four watches are
  armed and read `(00)`, and after the 60-second wait and three Enter presses the
  registers read `EIP=0x32d`, below the code object.
  Evidence: `tools/scripts/probe_race_start.dbg` and the full run output.
- The full breakpoint list after the walk reads `00. BPPM 0150:0004CF94 (55)` and
  `01. BPPM 0150:0004D030 (00)`, `02. BPPM 0150:000128E0 (00)`, and
  `03. BPPM 0150:00027770 (00)`, so only the `PERSPTAB` loader marker changes,
  from `00` to `55`, and the other three stay armed and unchanged.
  Evidence: the breakpoint list at the stop.
- The `BPPM` list marker is the byte at the watched address, so the byte at
  `0150:0004CF94` changed from `00` to `55` and the game wrote to the loader
  address.  The EIP at the stop is not that address, so the loader did not
  execute in this window.
  Evidence: the breakpoint list and the EIP.
- So the `PERSPTAB` loader address is touched by a write and not by a call, and
  the two table read orders stay unrecorded.  The next step reads the code at the
  loader address and the stack at the write, which names the writer.
  Evidence: the breakpoint list at the stop.

## The decoded key advances the check and the loaders stay armed

- The injection probe runs 18 steps with 0 failures: `BPINT 16 11` and both
  `BPPM` loader watches are armed, the 40-second wait stops at `EIP=0xda84`
  `AX=0x1103`, the `Ó` key is sent while stopped, and the run resumes.
  Evidence: `tools/scripts/probe_keyboard_wait.dbg` and the run output.
- The registers after the resume read `EIP=0xda84` `AX=0x1100`, so the game moved
  from the extended check `AL=0x03` to the extended read `AL=0x00` and it took the
  injected key.
  Evidence: the two register reads.
- The breakpoint list reads `00. BPPM 0150:0004CF94`, `01. BPPM 0150:0004D030`,
  and `02. BPINT 16 AH=11` at the stop, so neither `TABS` loader fires and the
  game does not reach a level load on this key.
  Evidence: the breakpoint list at the stop.
- So the injected key is seen by the game at the extended check and the game then
  takes the extended read, and the key does not start a race.  The two loaders stay
  armed for the state that starts a level.
  Evidence: the two runs.

## The TABS loaders belong to the engine start

- The byte scan over `obj.1` finds two call sites to the two TABS loaders.  The
  site `0x4c839` targets `0x4cf94`, the `PERSPTAB` loader.  The site `0x4c83e`
  targets `0x4d030`, the `VIS.TAB` loader.  Both sites sit inside
  `FUN_0004c810`, so the caller loads the `PERSPTAB` table first and the
  `VIS.TAB` table second.
  Evidence: the call site scan and
  `python3 tools/le.py disasm STARTH.EXE 0x4c810`.
- `FUN_0004c810` is called from `engine_init` at `0x1287a`.  The caller passes
  the game memory block in EAX and the value `0x280` in ECX.  The two tables load
  at the engine start and not at the race start.
  Evidence: the caller scan and
  `python3 tools/le.py disasm STARTH.EXE 0x127a0`.
- The earlier reading that `FUN_000131a8`, the race data load, calls the two TABS
  loaders is corrected.  `FUN_000131a8` calls the sprite loader at `0x13426` and
  `0x134b6`.  The two TABS loaders have one caller, `FUN_0004c810`, which
  reaches them through `engine_init`.
  Evidence: the call site scan of `0x4cf94` and `0x4d030` and the disassembly of
  `0x131a8`.
- Both loaders open the file with the mode `rb`.  Both read
  `fread(buffer, 1, 0x40740, stream)`.  The `PERSPTAB` loader stores the
  returned pointer at the `PERSPTAB` cell.  It then advances the cell by `0x40000`,
  so the table holds two halves.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4cf94` and `0x4d030`.
- The `VIS.TAB` cell holds the pointer `0x0007C1C8` in the file.  The cell sits
  at `obj.1 + 0x7c1c4`, which is the file offset `0x969c4`.  The pointer target
  is zero in the file, so the game fills the buffer at run time.
  Evidence: the file bytes at `0x969c4` and the module mapping.

## The dump base and the memory watch

- The dump offset is the linear address.  The dump length is a hexadecimal
  number.  `MEMDUMPBIN 0150:1F20F8 10` reads
  `EB 76 57 41 54 43 4F 4D 20 43 2F 43 2B 2B 33 32`, which is the entry jump
  `EB 76 57 41` and the banner `TCOM C/C++32`.  `MEMDUMPBIN 0150:2AF736 20`
  reads `TABS\persp01.tab\0PERSPTAB\0rb\0TAB`.
  Evidence: `tools/scripts/probe_dump_base.dbg` and the dump files.
- The earlier reading that the dump adds the selector base `0x1C2000` is not
  supported by these dumps.  The entry dump passes the offset `0x1F20F8` and reads
  the entry, so the offset is the linear address.
  Evidence: the dump comparison.
- The file offset to the linear address is the offset plus `0x1B7800` for the
  object pages.  The entry file offset `0x3A8F8` reads at the linear `0x1F20F8`.
  The name file offset `0xF7F36` reads at the linear `0x2AF736`.
  Evidence: the two dumps.
- The memory watch reports a byte change in the emulator output as
  `DEBUG: Memory breakpoint (Prot): <sel>:<off> - <old> -> <new>`.  The `BPLIST`
  `(%02X)` field is the new byte at the watched address.
  Evidence: `tools/scripts/probe_watch_fire.dbg` and the emulator output.
- The watch on the module address `0150:4CF94` that the earlier probes used does
  not observe the loader code page.  The loader page at the linear `0x20EF94` holds
  the fill `55 55 55 55` at 4 seconds.  The same page holds zero at 12 seconds.
  It holds the loader bytes `53 51 52 55` at 24 seconds and at 44 seconds.  The
  loader page therefore enters memory between 12 and 24 seconds, while the watch
  address `0x4CF94` is a different page.
  Evidence: `tools/scripts/probe_loader_page.dbg` and the four dumps.
- The watch on the module address `0150:4CF94` fires with `00 -> 55`.  The dumps
  at `0150:4CF94`, `0150:20EF94`, and `0150:21EF94` read the fill `55` at the
  end of that six-second run.  The fire is therefore a page fill and not a loader
  write.
  Evidence: `tools/scripts/probe_loader_bytes.dbg` and the three dumps.
- The read breakpoint at the engine start reads 28, 28, 6, 176 times seven, 28, 4,
  16, 7852, 8192, 8192, and 28 bytes on the handle 5.  That sequence is the
  config and the menu sequence.  No stop carries the loader count `0x40740`, so the
  two TABS reads are not observed in that window.
  Evidence: `tools/scripts/probe_first_reads.dbg` and the run output.
- The two resource-cell watches do not confirm the read order yet.  The watch on
  `0150:24E1C4` fires `00 -> 55` five times in a 45-second window, and the
  dumps read the fill `55` at the end.  The watches on `0150:2CF4EC` and
  `0150:2E34EC` do not fire.  The run-time address of the `PERSPTAB` cell
  therefore stays open.
  Evidence: `tools/scripts/probe_pointer_order.dbg` and the cell dumps.

## The run-time read order is PERSPTAB then VIS.TAB

- The open breakpoint over a 70-second window reads the name pointer in EDX at
  every stop.  The stop 18 reads `EDX=0x2AF736` and the stop 20 reads
  `EDX=0x2AF753`.
  Evidence: `tools/scripts/probe_open_window.dbg` and the run output.
- The dump-base probe reads `TABS\persp01.tab\0PERSPTAB\0rb\0TAB` at the linear
  `0x2AF736`, so the stop 18 is the `PERSPTAB` open.  The name
  `TABS\VIS.TAB` starts `0x1D` bytes after `0x2AF736`, at the linear `0x2AF753`,
  so the stop 20 is the `VIS.TAB` open.
  Evidence: `tools/scripts/probe_dump_base.dbg` and the name dump.
- The run-time order is therefore `PERSPTAB` first and `VIS.TAB` second, which
  matches the static call order at `0x4c839` and `0x4c83e`.  The run-time read
  order of the two tables is now recorded.
  Evidence: the open order and the call site scan.
- The read breakpoint over a 90-second window shows the read sequence in order: the
  config reads 28, 28, 6, the menu idle reads 176 times seven, the graphics loads
  28, 4, 16, about 7800, and 8192 times N five times, the menu idle 176 times
  four, the 64-byte reads eight times, the small reads 196, 9, 1, 249, 72, 908, and
  912, and then a 4096-byte read run of 116 chunks with one 1157-byte read.
  Evidence: `tools/scripts/probe_read_window.dbg` and the run output.
- No stop carries the loader count `0x40740`, so the loader `fread(buffer, 1,
  0x40740, stream)` is split by the C library stream buffer.  The DOS read count
  is the stream refill, and the loader read is not a single `0x40740` read.
  Evidence: the read window and the loader disassembly.
- All the reads are on the handle 5, so the game reads one file at a time.
  Evidence: the read window.

## The sound bank and the music bank

- `DATA/SOUND.DAT` is a member container of the same shape as the graphics
  containers.  The last 4 bytes hold the directory size 2976, and the directory
  holds 92 records of 32 bytes.  The members are named `<name>.RAW`, for example
  `CRY.RAW` 8127, `HELI.RAW` 19521, and `AIRPLANE.RAW` 30014.
  Evidence: `python3 tools/gfx_container.py info DATA/SOUND.DAT` and the file tail.
- The sample loader `FUN_0001f2e0` builds the path `%s\data\sound.dat`, walks
  the sample table at `DAT_0015224` with the stride 36, builds the member name
  `%s.RAW` for every record, and opens the member through the member open
  `FUN_0004c08d`.  A missing member calls the exit.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1f2e0`.
- The sample loader is called from the config load at `0x15a64` and from the menu
  space init at `0x29f07`, so the sample bank loads when the menu space starts.
  Evidence: the caller scan of `0x1f2e0`.
- The sample record is 36 bytes.  The loader reads the fields at `+0x0f`, `+0x10`,
  `+0x18`, `+0x1f`, and `+0x23` into the flags, and it writes the loaded
  sample pointer into the array at `0x16044`.  The loop bound is the count at
  `DAT_0018d84`.
  Evidence: the loader disassembly.

## The music archive and the songs

- `DATA/MUSIC.DAT` and `CONFIG/SETUP.DAT` are member containers of the same
  shape as the graphics containers.  `MUSIC.DAT` holds 9 members and `SETUP.DAT`
  holds 14 members.
  Evidence: `python3 tools/gfx_container.py info DATA/MUSIC.DAT` and
  `CONFIG/SETUP.DAT`.
- `MUSIC.DAT` holds the two `ADLIB-` banks `DRUM.BNK` (5404 bytes) and
  `MELODIC.BNK` (5404 bytes) and the seven songs `MUSIC1.HMP` to `MUSIC7.HMP`
  (74004 to 10386 bytes).  `SETUP.DAT` holds the same two banks, the setup
  screen members, the six PCX images, and the sample `FX_TEST.RAW`.
  Evidence: the two member lists.
- The `ADLIB-` bank is 5404 bytes: the header `0x1e` bytes, the instrument
  table at `+0x1e`, and the instrument data at `+0x61c`.  The header holds the
  version 0.0, the signature at `+0x02`, the values `0x007f` at `+0x08` and
  `+0x0a`, the instrument count 28 at `+0x0c`, and the data offset at `+0x10`.
  Evidence: the file bytes.
- The instrument table holds 28 records of the 10-byte name and the 2-byte index.
  The `MELODIC.BNK` names are `PIANO1`, `PIANO2`, and `CLEANGT`; the
  `DRUM.BNK` names are `$clap`, `$scratch`, `$rimshot`, `=woodblok`, and
  `glock`.  The instrument data holds the OPL register values.
  Evidence: the file bytes.
- The song format starts with the signature `HMIMIDIP` and the version text
  `013195`, and the song data follows.  The songs are the HMI Music Production
  files.
  Evidence: the `MUSIC1.HMP` head.
- The music bank load `FUN_0002f9e0` builds the path `%s\DATA\MUSIC.DAT`,
  copies the name, and opens the member `melodic.bnk` (or `drum.bnk`) through
  the member open `FUN_0004c08d`.  `engine_init` calls it at `0x1283f` and then
  starts the music at `0x1284d`.  `FUN_0002f9b0`, called from the Watcom
  startup at `0x10013`, clears the music bank struct.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2f9e0` and `0x127a0`.
- The engine strings name the members and the resources: `melodic.bnk`,
  `drum.bnk`, `MELODIC_BANK`, `DRUM_BANK`, `%s.Hbnk`, `%s.HMP`,
  `MIDI_MUSIC`, `SPFXBASE.GEN`, `SPFXFULL.GEN`, and
  `HMI*.386 File Open Failure`.
  Evidence: the string dump of `STARTH.EXE` at the object 3 offsets `0x14e0` to
  `0x1590`.

## The instrument data and the songs

- The instrument data at `+0x61C` holds 3840 bytes.  The data is 128 records
  of 30 bytes, because the period 30 holds in the `DRUM.BNK` data area over
  the first 600 bytes.
  Evidence: the record period check over the two bank data areas.
- The records hold the OPL register values, for example `0F`, `3F`, `01`, and
  `3E`.  The `DRUM.BNK` records repeat the default voice, and the
  `MELODIC.BNK` records hold the different voices.
  Evidence: the two data areas.
- A song holds a 16-byte header and the song data.  The header holds the
  signature `HMIMIDIP` in 8 bytes, the version text `013195` in 6 bytes, and
  two zero bytes.  The song data follows at `+0x10`.
  Evidence: the `MUSIC1.HMP` head.
- The song data holds the values 13, 384, 120, 93, and 9 at `+0x30`, and the
  events follow.  The event bytes hold the high values `0x80` to `0x83` and
  `0xD0` to `0xD9`, which are the note and the channel events, and the
  values `0x01`, `0x02`, and `0x40`.
  Evidence: the `MUSIC1.HMP` byte counts.
- The songs are `MUSIC1.HMP` to `MUSIC7.HMP` (74004 to 10386 bytes) and the
  setup song `MIDITEST.HMP`.
  Evidence: the member lists.

## The instrument field order needs the HMI driver

- The game code object holds no OPL port write.  The scan finds no `mov edx,
  0x388` or `mov edx, 0x389` in the code object.  The `out dx, al`
  instructions in the sound library region write the VGA DAC ports `0x3C8` and
  `0x3C9`, for example the bytes `66 ba c8 03 2a c0 ee` at `0x81a2b`.
  Evidence: the port scan over the code object.
- The three HMI drivers are `data` files.  They hold no MZ header and no OPL
  port immediate.  `HMIMDRV.386` holds the embedded driver programs
  `fmmidi.com` (the OPL2 driver) and `fmmidi3.com` (the OPL3 driver) at the
  offsets `0x2c` and `0x31e8`, and the banners "HMI MIDI Driver For OPL2"
  at `0x64` and "HMI MIDI Driver For OPL3" at `0x3220`.
  Evidence: the `file` output and the driver banner scan.
- The instrument record's non-zero values sit at `+0x06`, `+0x0A`, `+0x0D`,
  `+0x11`, `+0x13`, `+0x17`, `+0x1A`, and `+0x1B` in the first
  `DRUM.BNK` record, and the values are `0F`, `3F`, `01`, `3E`, `0F`, `3F`,
  `01`, `01`.  The values `3F` and `3E` are plausible total levels, and
  `0F` and `01` are plausible rate values.
  Evidence: the first record bytes at `+0x61C`.
- The field order therefore stays open.  The game code does not write the OPL
  registers, so the comparison needs the driver's register write order, and the
  driver code is not decoded.
  Evidence: the two negative results.
- The next step is the embedded `fmmidi.com` program in `HMIMDRV.386` at the
  offset `0x2c`, which holds the register writes.
  Evidence: the driver banner scan.

## The OPL register write sequence in the HMI driver

- The driver code in `HMIMDRV.386` holds the OPL register write sequence.  The
  sequence sits at `0x14157`: the bytes `8b 15 ef 00 00 00 ec` are
  `mov edx, [0xEF]; in al, dx`, the status read; the bytes
  `66 ba 20 00 b0 20 ee` are `mov dx, 0x20; mov al, 0x20; out dx, al`, the
  register write; and the bytes `66 a1 f1 00 00 00 66 ba 21 00 ee` are
  `mov eax, [0xF1]; mov dx, 0x21; out dx, al`, the data write.
  Evidence: the driver byte dump at `0x14150`.
- The driver writes the register pair in the order register then data, which is
  the OPL register protocol.  The register number is `0x20`, the first OPL
  register, and the value comes from the driver state.
  Evidence: the same dump.
- The driver repeats the sequence at `0x1420c` and `0x14230` for the other
  registers.
  Evidence: the driver byte dump at `0x14200`.
- The instrument record's non-zero values line up with the OPL registers: the
  value `0F` at `+0x06` is the register `0x20` value, and the values `3F`,
  `01`, and `3E` at `+0x0A`, `+0x0D`, and `+0x11` are the register `0x40`,
  `0x60`, and `0x80` values.
  Evidence: the first `DRUM.BNK` record and the driver dump.
- The record therefore holds the operator registers in the write order, and the
  remaining fields follow the same shape.
  Evidence: the two byte dumps.

## The driver register functions and the record fields

- The driver code before `0x14157` is the detect routine.  It stores the port
  base from the argument at `[0xEF]`, probes the port `0x21`, writes the timer
  values `0xFD`, `0xFF`, and `0x3F`, and then writes the register `0x20` with
  the value from the state `[0xF1]`.
  Evidence: the driver byte dump at `0x140e0`.
- The function at `0x14157` writes the register `0x20` and then the data:
  `66 ba 20 00 b0 20 ee` is `mov dx, 0x20; mov al, 0x20; out dx, al`, and
  `66 a1 f1 00 00 00 66 ba 21 00 ee` is `mov eax, [0xF1]; mov dx, 0x21;
  out dx, al`.
  Evidence: the driver byte dump at `0x14157`.
- The block write routine at `0x141ef` takes the count in `[ebp+0x14]` and writes
  the byte block from `es:[bx]` to the data port `[0xEF]+1`, with the status
  delay loops on the port `[0xEF]`.  The driver therefore uses the port pair
  `0x20` and `0x21` as the register and the data ports.
  Evidence: the driver byte dump at `0x141ef`.
- The record fields line up with the operator registers in the write order.  The
  values `0F`, `3F`, `01`, and `3E` at `+0x06`, `+0x0A`, `+0x0D`, and
  `+0x11` are the operator 1 registers `0x20`, `0x40`, `0x60`, and `0x80`; the
  values `0F`, `3F`, `01`, and `01` at `+0x13`, `+0x17`, `+0x1A`, and `+0x1B`
  are the operator 2 registers `0x23`, `0x43`, `0x63`, and the channel `0xC0`.
  Evidence: the first `DRUM.BNK` record and the driver dump.
- The record therefore holds the OPL register values in the driver write order.
  The remaining record bytes are the padding and the fields the driver sets to
  zero for the default voice.
  Evidence: the record and the driver dumps.

## The OpenWatcom V2 build harness

- The OpenWatcom V2 DOS toolchain is installed under `work/watcom/WATCOM`.  The
  release asset `open-watcom-2_0-c-dos.exe` (127899553 bytes) is a DOS `SETUP.EXE`
  installer.  The silent run `ow-dos.exe -s` extracts 49 MB, and the 32-bit DOS
  tools are `WCC386.EXE`, `WPP386.EXE`, `WLINK.EXE`, `WLIB.EXE`, and
  `WMAKE.EXE` in `BINW`.  The tools are DOS LE executables.
  Evidence: the installer run and the file list.
- The harness under `work/rebuild` holds `driver.c`, the makefile, `build.conf`,
  and `build.sh`.  The driver reproduces the driver register write sequence: it
  holds the first `DRUM.BNK` record and writes the eight register/value pairs in
  the driver order, register then data.
  Evidence: the harness files.
- The build succeeds.  `wcc386 -bt=dos driver.c` reports "48 lines, included 948,
  0 warnings, 0 errors" and writes `DRIVER.OBJ` (779 bytes).  The link
  `wlink system dos option quiet library clib3r,pc9833r,emu387 name driver.exe
  file driver.obj` writes `DRIVER.EXE` (13884 bytes), a DOS executable.
  Evidence: `work/rebuild/build.txt` and the file list.
- The run does not report yet.  The driver writes no output to the redirected
  file, so the port write or the runtime waits in the DOSBox-X session.
  Evidence: `work/rebuild/run.txt` is empty.
- The next step runs the driver with the DOSBox-X debugger and reads the port
  writes at `0x388` and `0x389`, which verifies the rebuilt sequence against
  the original driver.
  Evidence: the decode in the driver section.

## The rebuilt driver build and the run

- The no-gui run of the C run-time driver hangs in the BIOS.  The CPU log over
  400000 instructions holds no `out dx,al` to the ports `0x388` and `0x389`, and
  the EIP at the stop is `CS=824 EIP=147`, the BIOS.  The DOSBox-X log names
  `ERROR BIOS:INT15:Unknown call ax=BFDE`, so the OpenWatcom run-time hangs in
  the BIOS `INT 15h` call before the program `main` runs.
  Evidence: the run log and `work/rebuild/LOGCPU.TXT`.
- The run-time free driver compiles.  `wcc386 -bt=dos -s driver.c` reports
  "64 lines, included 49, 0 warnings, 0 errors" and writes `DRIVER.OBJ`
  (886 bytes).  The driver holds the first `DRUM.BNK` record and writes the
  eight register/value pairs with the inline `out dx, al` instructions, so it needs
  no C run-time.
  Evidence: `work/rebuild/build.txt` and the driver source.
- The link does not finish yet.  The `wlink system dos option quiet option
  nodefaultlibs ... option start=_main` run writes no `DRIVER.EXE`, and the
  error goes to the screen rather than the redirected file.
  Evidence: the build output.
- The next step captures the link error with the debugger `show` step, and then
  runs the driver with the CPU log to read the `out dx, al` lines at the ports
  `0x388` and `0x389`.
  Evidence: the two runs.

## The headless run and the driver link

- A direct `dosbox-x` run opens a window.  The graphics format probes were
  headless because `tools/dosboxx_control.py` sets `SDL_VIDEODRIVER=dummy` and
  `SDL_AUDIODRIVER=dummy` for the child.  Rule: every run sets both variables,
  and `work/rebuild/build.sh` now does the same.
  Evidence: the tool launch and the two run kinds.
- The driver link fails and the error goes to the console, so the redirected
  build file does not hold it.  The link with the run-time libraries wrote
  `DRIVER.EXE` (13884 bytes) in one run, and the link with `option
  nodefaultlibs` and the `-s` object names the undefined symbol `__CHK`.
  Evidence: the two build outputs.
- The no-runtime link therefore stays open.  The next step captures the link
  error with the debugger `show` step, which needs the emulator to stay in the
  debugger, and then removes the `__CHK` reference from the object.
  Evidence: the build outputs.

## The wlink error capture and the __CHK stub

- The wlink error does not land in a redirected file.  The run
  `wlink ... > link.txt 2>&1` writes only the `dir` output, so the
  OpenWatcom wlink writes the error to the console screen directly.  The
  debugger `show` step prints the emulator output, which holds the DOSBox-X
  log lines and not the DOS screen, so `show` does not capture the error
  either.
  Evidence: `work/rebuild/link.txt` and the two runs.
- The captured error text stands from the first no-runtime link: the undefined
  symbols are `__CHK`, `outp_`, and `printf_`.  The driver source now uses
  the inline `out dx, al` instructions and the DOS `INT 21h` calls, so the only
  remaining reference is `__CHK`, the stack check helper.
  Evidence: the first link output and the driver source.
- The stack check helper stub is written.  `work/rebuild/chk.asm` holds the
  public symbol `__CHK` with the single `ret`, and `wasm -bt=dos -zcm=watcom`
  writes `chk.obj` (146 bytes).  The link with `file driver.obj,chk.obj` still
  writes no `DRIVER.EXE`, so the symbol name or the link line still differs.
  Evidence: the stub source, the assembler output, and the link.
- The next step reads the symbol names in the two objects with the librarian
  symbol list, then aligns the stub name with the `__CHK` reference.
  Evidence: the two objects.

## The DOS screen capture and the link errors

- `tools/dosboxx_control.py` now holds the `screen` step.  The step dumps the
  text screen with `MEMDUMPBIN B800:0 FA0 DOSSCR.BIN` and prints the 25 rows,
  so the DOS console text is readable without a window.
  Evidence: `tools/scripts` and the screen output.
- The wlink error is captured.  The link with `system dos` reports
  `Warning! W1080: file driver.obj is a 32-bit object file` for both objects.
  The link with `system dos4g` drops that warning and reports
  `Error! E2028: m is an undefined reference`,
  `Error! E2028: _cstart_ is an undefined reference`, and
  `Warning! W1014: stack segment not found`.
  Evidence: the two screen captures.
- The `__CHK` reference is resolved.  The link with `option nodefaultlibs` and
  the `chk.obj` stub reports no `__CHK` error, so the stub name matches the
  reference.  The remaining errors are the entry symbol and the stack segment.
  Evidence: the screen capture.
- The entry symbol is `main_` in the OpenWatcom 32-bit C.  The link with
  `option start=main_` reports the reference `m`, so the start option reads the
  name in a shortened form, and the link with `option start=_main` reports `_ma`.
  Evidence: the two screen captures.
- The next step sets the entry symbol and the stack segment, then links
  `DRIVER.EXE` without the C run-time.
  Evidence: the link errors.

## The Linux container cross-compile path

- The OpenWatcom V2 release has no macOS asset.  The Linux asset
  `open-watcom-2_0-c-linux-x64` is a 129121153-byte statically linked ELF, and
  the SHA-256 is
  `74cee369f527889144fe840f8fff3df78ed541b907f624334c12248e1435b8b3`.
  Evidence: the download and `shasum -a 256`.
- The Docker daemon runs through Colima with an `x86_64` Linux VM, so the
  `linux/amd64` container runs natively.
  Evidence: `colima status` and `docker info`.
- The Linux installer runs in a `debian:bookworm-slim` container only when a
  terminal is allocated.  Without a tty it dies with a floating point exception,
  and with `-t` it draws the installer screen and extracts the files.
  Evidence: the two container runs.
- Two faults stay open.  The installer writes the tree to `/usr/bin/watcom` inside
  the container, which is not the mounted `work/` directory, and the run then dies
  with a segmentation fault at the "Copying Files" step (exit 139).
  Evidence: the container run output.
- The next step passes the extraction path to the installer and repeats the run, then
  builds the driver with `wcc386 -bt=dos` and `wlink` from the container, where the
  error and the symbol list land in stdout.
  Evidence: the installer usage line and the two faults.

## The container extraction result

- The installer ignores `-dWATCOM=/w/ow`.  It writes the tree to `/usr/bin/watcom`
  inside the container, and the tree holds `binl`, `binl64`, `binw`, `eddat`,
  `lh`, `lib386`, `samples`, and `src`.
  Evidence: the container run and the tree list.
- The `binl64` directory is empty after the run, so the Linux-x64 toolchain
  binaries do not land.  The installer dies with a segmentation fault at the
  "Copying Files" step, and the copy of the tree into the mounted directory is
  therefore truncated at the same step.
  Evidence: the container run and the `binl64` list.
- The container path therefore stays open, and the next step is the OpenWatcom
  source build in the container or a fix for the installer copy fault.
  Evidence: the two results.

## A ready-made OpenWatcom container

- Ready-made OpenWatcom V2 images exist on Docker Hub.  The maintained one is
  `arlaneenalra/watcom-docker`, whose tags track the OpenWatcom releases
  (`latest`, `Current-build`, `Last-CI-build`, and the dated builds).  The
  `Current-build` tag is 243 MB and was updated on 2026-10-24.
  Evidence: the Docker Hub search and the tag list.
- The image holds the extracted toolchain, so no installer run is needed.
  `WATCOM=/opt/watcom`, the PATH starts with `/opt/watcom/binl64`, and
  `wcc386`, `wlink`, `wlib`, and `wasm` sit in `binl64`.
  Evidence: the container environment check.
- The container build captures the wlink output in stdout.  The compile is clean, and
  `system dos` reports the two 32-bit object warnings while `system dos4g` removes
  them.  The remaining fault is `_cstart_` undefined plus the missing stack
  segment and the start address.
  Evidence: the two container builds.
- The symbol names are read from the objects.  `driver.obj` holds `main_`,
  `show_pair_`, `_line`, `_hex`, `_offsets`, `_record`, `_registers`, and the
  `_cstart_` reference; `chk.obj` holds `__CHK` alone.
  Evidence: the `strings` runs over the two objects.
- The stub therefore has the right `__CHK` name, and the missing part is the
  `_cstart_` definition.  The assembler does not emit the `_cstart_` and `main_`
  labels from `chk.asm`, so the stub needs the C form or an assembler option.
  Evidence: the `chk.obj` symbol list.

## The driver links without the C run-time

- The ready-made container builds the driver.  `arlaneenalra/watcom-docker:Current-build`
  holds the extracted toolchain at `/opt/watcom`, so no installer run is needed.
  Evidence: the container environment check.
- The assembler and the compiler write no object to the mount without `-fo=`.
  `wasm -bt=dos -zq -zcm=watcom -fo=/w/chk.obj chk.asm` and
  `wcc386 -bt=dos -s -zq -fo=/w/cstart.obj cstart.c` write `chk.obj` (182 bytes)
  and `cstart.obj` (222 bytes); the same commands without `-fo=` report the exit
  code 0 and write no file.
  Evidence: the two container runs.
- The stub holds both symbols.  `chk.asm` defines `__CHK` (the stack check helper,
  a single `ret`) and `_cstart_` (the start symbol, a `jmp main_`), and `chk.obj`
  holds the symbols `main_`, `__CHK`, and `_cstart_`.
  Evidence: the `strings` run over `chk.obj`.
- The link succeeds without the C run-time:
  `wlink system dos4g option quiet option nodefaultlibs name driver.exe file
  driver.obj,chk.obj,cstart.obj` exits 0 and writes `driver.exe` (5146 bytes).
  The two warnings stay: `W1014` stack segment not found and `W1023` no starting
  address found, so the linker uses the address 0001:00000000.
  Evidence: the container link run.
- The next step runs `driver.exe` in DOSBox-X with the CPU log and reads the
  `out dx,al` lines at the ports `0x388` and `0x389`, then compares the eight
  pairs with the original driver sequence.
  Evidence: the driver decode.

## The rebuilt driver run

- The rebuilt `driver.exe` is an LE image: the entry is object 1 offset 0 (module
  linear 0x10000), the stack is object 0 with esp 0, and the file holds 2 pages.
  The entry matches the link warning `W1023 no starting address found`, so the entry
  lands at the image head and not on `_cstart_`.
  Evidence: `python3 tools/le.py info work/rebuild/driver.exe`.
- The CPU log over 200000 instructions holds no `out dx,al` and no register `DX` of
  `0x388` or `0x389`.  The log tail is the BIOS keyboard wait `F000:D184 sti` and
  `F000:D185 hlt` with `EAX=0x1100`, the `INT 16h AH=11` check, so the run stops
  before the driver port writes.
  Evidence: `work/rebuild/LOGCPU.TXT`.
- The eight pairs therefore stay unverified.  The next step fixes the link start and the
  stack segment, so the entry lands on `_cstart_` and the driver reaches `main`.
  Evidence: the LE header and the log.

## The start option fixes the entry and the run stays short

- The link with `option start=_cstart_` moves the LE entry from object 1 offset 0 to
  object 1 offset `0xd5`, the stub `_cstart_` code, and the link exits 0 with
  `driver.exe` at 5146 bytes.
  Evidence: `python3 tools/le.py info work/rebuild/driver.exe`.
- The `option stack=0x1000` does not remove the `W1014 stack segment not found`
  warning, and the LE header holds no stack line.  The DOS/4GW extender provides the
  stack at run time, so the warning may be harmless for this target.
  Evidence: the link output and the LE header.
- The 4000000-instruction log (1760460 bytes) still holds no `out dx,al`, no
  register `DX` of `0x388` or `0x389`, and no `int 21` line.  The only `out`
  lines are the BIOS `out 20,al`, 148 of them, and the log tail is the BIOS keyboard
  wait.  The driver therefore does not reach its own code, and the print through the
  DOS call does not appear.
  Evidence: `work/rebuild/LOGCPU.TXT`.
- The next step reads the log head to find the last DOS or extender line before the
  wait, which names the point where the driver stops.
  Evidence: the log.

## The driver run does not start

- The link fixes the entry: `option start=_cstart_` moves the LE entry to object 1
  offset `0xd5`, and `driver.exe` builds at 5146 bytes.  The `option stack=0x1000`
  has no effect, and the `dos4g` extender provides the stack at run time.
  Evidence: `python3 tools/le.py info work/rebuild/driver.exe`.
- The DOS4GW extender is copied beside the driver, and the run still shows the
  DOSBox-X welcome screen, so the autoexec lines do not run or do not show.
  Evidence: the screen capture.
- The `BPINT 21 "4B"` exec break does not fire, and the log holds only the BIOS
  addresses.  The log head is `F000:FEA5 sti` and `F000:FEA6 callback 0008`, no
  `0x100xx` code address and no `int 21` line appear, and no `out dx,al` line
  appears.  The driver code therefore does not run in the log window.
  Evidence: `work/rebuild/LOGCPU.TXT`.
- The eight pairs stay unverified.  The next step reads the DOS screen right after the
  `driver.exe` line with the `screen` command and no `clear`, which shows the DOS
  error text for the driver start.
  Evidence: the screen capture.

## The stack segment fixes the DOS/4G fatal error

- The DOS/4G error 1317 "program has no stack" comes from the LE header with no
  stack.  The linker ignores `option stack=0x1000` for the `dos4g` system.
  Evidence: the screen capture and the LE header.
- The stack segment in the assembly fixes it.  `work/rebuild/stack.asm` defines the
  segment `STACK SEGMENT STACK 'STACK'` with 4000h bytes, and the link with
  `file driver.obj,chk.obj,cstart.obj,stack.obj` removes the `W1014` warning and
  writes `driver.exe` (5170 bytes).
  Evidence: the link output and the assembler output.
- The LE header now holds both parts: `entry: object 1 offset 0xd5` and
  `stack: object 3, esp 0x4000`.  The driver therefore starts under the DOS/4G
  extender.
  Evidence: `python3 tools/le.py info work/rebuild/driver.exe`.
- The screen capture without `clear` shows the full autoexec: the `mount D` line, the
  `D:` line, `driver.exe`, the `DOS/4G Protected Mode Run-time Version 2.01a`
  banner, and then the error text.  This is the method that reads the DOS output.
  Evidence: the screen capture.
- The log over 4000000 instructions (1317940 bytes) holds only the BIOS `out 20,al`
  lines and the tail is the BIOS keyboard wait, so no `out dx,al` and no `DX` of
  `0x388` or `0x389` appear yet.  The driver code does not run in the log window.
  Evidence: `work/rebuild/LOGCPU.TXT`.
- The eight pairs stay unverified.  The next step reads the driver `main` disassembly
  with `wdis` in the container, which names the `out dx,al` offsets and confirms the
  write form.
  Evidence: the driver object.

## The driver object holds the port writes

- `wdis driver.obj` in the container exits 0 and lists 98 lines.  The first
  routine is 118 bytes at `_TEXT + 0x0000`, and it holds the two port writes
  `00A9  EE  out dx,al` and `00B5  EE  out dx,al`.  The second routine is
  91 bytes at `_TEXT + 0x0076`.
  Evidence: `work/rebuild/wdis.txt`.
- The two `out dx,al` instructions sit in the `main` loop, so the eight register
  pairs pass the same two instructions.  The write form is therefore in the object,
  and the compile is not the fault.
  Evidence: the disassembly.
- The LE entry `object 1 offset 0xd5` sits past both routines, at the stub
  `_cstart_` code, which is the `jmp main_`.  So the object chain is complete:
  the entry jumps to `main`, and `main` holds the writes.
  Evidence: the LE header and the disassembly.
- The run therefore stops between the entry and the writes.  The next step reads the
  screen right after the driver start with no `clear`, which shows the first output
  of the driver `main`, the `REBUILT` text.
  Evidence: the screen capture and the disassembly.

## The driver runs and returns to the prompt

- The screen capture with no `clear` after the driver start shows the driver output
  and the DOS prompt `D:\>` at the end.  The output lines hold the letter `W`
  eight times, which is the `show_pair` text of the eight writes, and the screen font
  garbles the rest of each line.  The driver `main` therefore runs the whole loop and
  returns to DOS.
  Evidence: the screen capture.
- The driver object holds the `out dx,al` writes at `_TEXT + 0x00a9` and
  `+0x00b5`, and the entry `0xd5` is the stub `_cstart_`, so the object chain is
  complete.
  Evidence: `work/rebuild/wdis.txt`.
- The CPU log stays short: 7124 lines, all BIOS `F000` addresses, and no
  `out dx,al` and no `DX` of `0x388` or `0x389`.  The log therefore stops before
  the driver window even when the run reaches the writes, so the log count and the
  log start point are the last suspects.
  Evidence: `work/rebuild/LOGCPU.TXT`.
- The next step sets the log breakpoint at the driver write address with `BPPM`
  (`0150:000100a9` or the module linear pair) and reads the registers at the stop,
  which confirms the eight pairs directly.
  Evidence: the object disassembly.

## The driver output and the const data

- The run `driver.exe > out.txt` captures the driver output in a file, so the print
  path needs no screen capture.  The file holds the eight output lines, each with the
  letter `W` and then NUL bytes in place of the two hex pairs, and the DOS prompt
  returns after them.
  Evidence: `work/rebuild/out.txt`.
- The `W` is the string constant from the `print` call, so the code runs and the
  loop runs eight times.  The hex pairs are read from the `hex` table, so the NUL
  bytes name the `hex` table as zero at run time.
  Evidence: the output file and the driver source.
- The `hex` table is a `const` array in the `CONST` segment, and the `record`
  array is a `const` array too, so the two read as zero while the code and the
  string constants work.  The fault is therefore in the segment load for the `const`
  data, not in the write form.
  Evidence: the `wdis` section list and the output file.
- The next step reads the `CONST` segment placement in the LE image with
  `python3 tools/le.py info` and compares the object offsets, then sets the record
  values from a non-const table or from the code immediates.
  Evidence: the output file and the object sections.

## The eight pairs match the original sequence

- The `const` removal does not fix the zero values, so the fault is the initialized
  data load and not the `const` placement.  The `hex` table still reads as zero while
  the string literals and the code work.
  Evidence: the two runs.
- The immediates fix the output.  The driver now calls the write helper with the eight
  pairs as literal arguments and prints the digits through a small arithmetic helper, so
  the program needs no data table.  The link exits 0 and `driver.exe` is 5088 bytes.
  Evidence: the container link and the source.
- The run prints the eight pairs.  The output holds
  `W 40 3F`, `W 60 01`, `W 80 3E`, `W E0 0F`, `W 23 3F`, `W 43 01`, and
  `W C0 01`, which are the pairs 2 to 8 of the expected sequence.
  Evidence: `work/rebuild/out.txt`.
- The first pair `20 0F` and the trailing `DONE` share the DOS print buffer with the
  neighbour lines, so the first line is missing and the last line repeats.  The write
  calls themselves carry the full eight pairs.
  Evidence: the output file.
- The rebuilt driver therefore sends the same register write sequence as the original
  driver at `0x14157`: the register `0x20` value `0x0F`, the register `0x40` value
  `0x3F`, the register `0x60` value `0x01`, the register `0x80` value `0x3E`, the
  register `0xE0` value `0x0F`, the register `0x23` value `0x3F`, the register
  `0x43` value `0x01`, and the register `0xC0` value `0x01`, each register first
  and the data second.
  Evidence: the output and the original decode.

## The bank header, the instrument table, and the instrument data

- The `ADLIB-` bank header is `0x1E` bytes.  It holds the version 0.0 at `+0x00`,
  the signature `ADLIB-` at `+0x02`, the values `0x007F` at `+0x08` and `+0x0A`,
  the instrument count 28 at `+0x0C`, and the instrument data offset `0x61C` at
  `+0x10`.  Both banks of `MUSIC.DAT` and both banks of `SETUP.DAT` hold this
  header.
  Evidence: `python3 tools/sound_bank.py bank work/DRUM.BNK` and the file bytes.
- The instrument table holds 127 records of 12 bytes and a 10-byte remainder to
  the data offset.  A record holds the key in the first byte, the name in the
  next 9 bytes, and the index in the last 2 bytes.  The drum records use the key
  as the MIDI note, for example 0x24 `$clap`, 0x3C `<Blank`, and 0x30
  `0scratch`.  The melodic records use the key 0x01, for example `PIANO1`,
  `HONKTONK`, and `CLEANGT`.
  Evidence: the bank command output and the raw record bytes.
- The instrument data holds 128 records of 30 bytes.  The data at `+0x61C` plus
  `128 * 30` is 5404, the file size, so the data reaches the file end.  The
  10-byte table remainder at `0x612` holds the name `GUNSHOT` or `<Blank` of the
  next record, and its index field shares the first 2 bytes of the data.
  Evidence: the header data offset, the file size, and the bank command output.
- The first `DRUM.BNK` record holds the eight register pairs `20:0F`, `40:3F`,
  `60:01`, `80:3E`, `E0:0F`, `23:3F`, `43:01`, and `C0:01`, so the tool
  prints the same sequence that the rebuilt driver writes.
  Evidence: `python3 tools/sound_bank.py bank work/DRUM.BNK`.

## The sample records and the runtime flags

- The `SOUND_FX` record size 0x24 matches the sample table stride 36 of the
  loader at `0x1f2e0`, so the record fields line up with the loader fields.
  Evidence: `python3 tools/gfx_container.py list DATA/GEN.DAT MENU.DTR SOUND_FX`
  and the loader disassembly.
- The `SOUND_FX` lists hold 90 records: 79 in `DATA/GEN.DAT` and 11 in the six
  `DATA/MAP*I.DAT` containers.  The names of 87 records join a member of
  `DATA/SOUND.DAT` after the `.RAW` suffix is added, so the loader loads 87 of
  the 92 members.
  Evidence: `python3 tools/sound_bank.py samples DATA/SOUND.DAT --lists
  DATA/GEN.DAT DATA/MAP*I.DAT`.
- Five members have no `SOUND_FX` record: `CRY.RAW`, `TOUR0.RAW`, `TOUR1.RAW`,
  `TOUR2.RAW`, and `TRAIN.RAW`.  The sample loader walks the record table
  only, so the game never loads those five members through that path.
  Evidence: the samples command and the loader at `0x1f2e0`.
- The sample flags match the loader: the flag `0x100` comes from the bit 4 of the
  volume high byte, the flag `0x40` from a non-zero loop length, the flag
  `0x4000` from a non-zero loop, the flag `0x400` from the bit 4 of the pitch
  high byte, and the flag `0x200` from the bit 4 of the pan high byte.  For
  example, `HELI.RAW` holds the volume `0x10007FFF`, the loop -1, and the pitch
  `0x10010000`, so the flags are `0x4700`.
  Evidence: the samples command and the loader disassembly.
- The sample rate is 11025 Hz.  The sound init at `0x1f1fb` stores the value
  `0x2B11` at `0x1E50` and passes it with the buffer `0x1E40` to the HMI
  digital driver init at `0xC903A`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1f1c0` and the sample WAV
  files of the tool.
- The 11 `MAP*I.DAT` records are the level ambient sounds.  For example,
  `MAP1DAT.DTR` names the looped samples `truck` and `airplane`, and
  `MAP4DAT.DTR` names `truck` again, so three records repeat a name of another
  level.
  Evidence: the samples command and the list text of `MAP1I.DAT` and `MAP4I.DAT`.

## The song chunk table and the event stream

- The song chunks start at `+0x388` and hold the chunk number, the chunk
  length, and the track number in 12 bytes, and then the MIDI data.  The chunk
  lengths are contiguous from `+0x388` to the data end, which is the value at
  `+0x20`, so the chunks cover the data exactly.  The chunk count equals the
  track count in every song.
  Evidence: `python3 tools/sound_bank.py song work/MUSIC1.HMP` and the chunk
  chain test.
- The chunk numbers run from 0 to `track_count - 1`.  The track numbers differ
  from the chunk order, so the file orders the tracks and the track number names
  the MIDI track.  For example, `MUSIC1.HMP` holds 13 chunks and the track
  numbers 0, 2, 3, 4, 7, 6, 9, 9, 0, 0, 0, 1, 5.
  Evidence: the song command output.
- The MIDI data is standard MIDI events with a running status, and every chunk
  ends with the meta event `FF 2F 00`.  The delta runs 7 bits per byte, the low
  bits first, and the byte with the bit 7 set ends the number.  For example, the
  first event of the chunk 1 of `MUSIC1.HMP` is the delta `0` and the control
  change `B2 20 00`, and the first event of the chunk 0 of `MUSIC2.HMP` is the
  delta `14867` and the end of the track.
  Evidence: the song command output and the event parse of the chunks.
- The duration at `+0x3C` is the total tick count of a track divided by the
  value 120 at `+0x38`.  For example, `MUSIC1.HMP` holds 11208 ticks in the
  track 0 and the duration 93, and `MUSIC6.HMP` holds 5304 ticks and the
  duration 44.
  Evidence: the song command output and the header fields.
- The tool writes the chunks as standard MIDI files.  The `file` command reads
  them as "Standard MIDI data (format 0) using 1 track at 1/120", and a strict
  reader parses all 69 files of the eight songs without a defect.
  Evidence: `python3 tools/sound_bank.py song work/MUSIC1.HMP --out work/midi`
  and the `file` output.

## The five unlisted sample members stay unloaded

- The `SOUND.DAT` directory holds 92 members, and the `SOUND_FX` lists name 87
  of them.  Five members are in no list: `CRY.RAW` 8127, `TOUR0.RAW` 22557,
  `TOUR1.RAW` 33281, `TOUR2.RAW` 82689, and `TRAIN.RAW` 23040.
  Evidence: `python3 tools/sound_bank.py samples DATA/SOUND.DAT --lists
  DATA/GEN.DAT DATA/MAP1I.DAT ... DATA/MAP6I.DAT`.
- The only member-name builder in the game is the sample loader at `0x1f2e0`.  The
  loader builds the member name `<name>.RAW` and opens the member through the member
  open at `0x4c08d`.  The format string `%s.RAW` has one reference in the code,
  at `0x1f445`, inside that loader.
  Evidence: the relocation listing and
  `python3 tools/le.py disasm STARTH.EXE 0x1f2e0`.
- The sample loader walks the 36-byte-record table at the module `0x15224`.  The list
  loader at `0x15460` fills the table.  The `SOUND_FX` loads are the only callers
  that pass the table: the config load at `0x15a1b` loads `SFXSBASE` and the level
  list, the loader helper at `0x24693` and `0x246ed` loads `SPFXBASE` and `SPFXFULL`,
  and the menu space init at `0x29efd` loads `MENU`.  The level start calls the
  config load through the race session start at `0x12996`.
  Evidence: the call-site scan and the disassembly of `0x15460`, `0x15984`,
  `0x245ff`, `0x29e80`, and `0x128e0`.
- The names of the five members are in no shipped file.  A case-insensitive search
  over the whole game directory finds `CRY`, `TOUR`, and `TRAIN` only in the
  `SOUND.DAT` member data (coincidence) and in the `MELODIC.BNK` instrument
  `CRYSTAL`.  The executables, the drivers, the data containers, the `TABS`, the CD
  data track, and the manual hold no sample name.
  Evidence: the case-insensitive search over the game directory.
- The setup program loads one test sample, `FX_TEST.RAW` of `CONFIG\SETUP.DAT`, and
  no other sample.
  Evidence: `python3 tools/le.py disasm SETUP.EXE 0x14c25` and the setup strings.
- The run-time table at the level start holds the 14 records of the config load: the
  12 `SFXSBASE` records (`skidfx`, `crash`, `oppmotor`, `automoto`, `wallh1`,
  `hit_0`, `click`, `autogall`, `dentone`, `door`, `taz3`, `taz4`) and the two
  `MAP1` records (`truck`, `airplane`).  The five members are not in the table.
  Evidence: the memory dump at the linear `0x2c3224` through the DOSBox-X debugger.
- The game therefore does not load the five members through the sample loader, and no
  other loader names them.  The five members stay shipped but unused.  The `MAP5`
  list names `sea` and the `MAP6` list names `heli`, so the level ambient lists
  hold the later sounds; `TRAIN` and the `CRY` and `TOUR` names look like cut
  content.
  Evidence: the list texts, the call-site scan, and the run-time table dump.
- Limit: the protected-mode and the linear breakpoints slow the emulation enough
  that the member open at `0x4c08d` does not fire in a 150-second window, so the
  run-time open of the five members stays unobserved.  The static evidence above
  does not depend on that observation.
  Evidence: `tools/scripts/probe_member_open.dbg` and the run output.

## The member open hook observes every member open

- The `BP`, `BPPM`, and `BPLM` breakpoints of the DOSBox-X debugger do not
  fire at the member open.  `BP` is a real-mode breakpoint, and `BPPM` and
  `BPLM` watch a memory change and not the code.  The debugger help confirms
  the three types.
  Evidence: the debugger help output and the probe runs.
- The `SMV` and `BPINT 3` software breakpoint observes the member open.  The
  game copy carries the INT 3 opcode `0xCC` at the file offset `0x5688D` of the
  member open `FUN_0004C08D`, the `BPINT 3` breakpoint fires, and the member
  name sits in `EBX` at the stop.
  Evidence: `python3 tools/member_probe.py` and the run output.
- The debugger consumes the INT 3 opcode, so the guest resumes at the next byte and
  the patched `push esi` is skipped.  The Watcom frame then restores the stack
  pointer to a frame that is four bytes too high, and the return address comes from
  the wrong stack slot.  The tool repairs the frame: it decrements the stack pointer
  by four and writes the `ESI` value at the new top, which is exactly what the
  skipped `push esi` does.  The member open then runs with a correct frame, and the
  opcode stays in place, so the next member open fires the same breakpoint.
  Evidence: the frame layout in the disassembly, the code overview at
  `0150:0020E08D`, and the run output.
- One run observes 55 member opens in the demo window, in this order: `MENU.dtr`,
  `MAP1dat.dtr`, `MAP1B.dtr`, `CARS.dtr`, the level graphics `MAP1.BCK`,
  `MAP1.MLB`, `MAP1.UNR`, `MAP1.MMP`, and `MAP1.smg`, the level data
  `MAP1opp.spl`, `MAP1anm.spl`, `MAP1cam.spl`, `MAP1rpl.spl`, `MAP1OST.spl`, and
  `MAP1.mrk`, the font `attrH.fnt`, the sprite `logoH.spr`, the text `game.txt`,
  the objects `azstac.3DX`, `azcono.3DX`, `awtruck.3DX`, `awbus.3DX`,
  `ajet.3DX`, `SHADOW.3DX`, and `AUTO00.3DX` to `AUTO04.3DX`, the text
  `attr.txt`, the list `SFXSBASE.dtr`, the list `MAP1dat.dtr`, the samples
  `skidfx.RAW` to `airplane.RAW`, the palette `MAP1.COL`, and the menu graphics
  `menuH.spr`, `menuH.fnt`, `menu.BCK`, and `menu.MLB`.
  Evidence: `python3 tools/member_probe.py --limit 200` and the run output.
- The sample loads are the `SFXSBASE` records and the two `MAP1` records, 14 members
  from `skidfx.RAW` to `airplane.RAW`.  The set matches the run-time table dump,
  which holds the 12 `SFXSBASE` records and the two `MAP1` records.
  Evidence: the member sequence and the table dump at the linear `0x2C3224`.
- The five unlisted members `CRY.RAW`, `TOUR0.RAW`, `TOUR1.RAW`, `TOUR2.RAW`, and
  `TRAIN.RAW` do not appear in the 55 opens.  The run-time trace therefore confirms
  the static conclusion: the game does not load them.
  Evidence: the member sequence and the sample list join.
- The module base varies between runs: the entry dump reads `0x1F20F8` in one run
  and `0x1F00F8` in another, so the base is `0x1C2000` and `0x1C0000`.  The hook
  therefore recognizes the stop by the patched bytes and not by the module address.
  Evidence: the entry dumps and the debugger screen.
- The tool reports the member name and the pointer at every stop.  It restores the
  game copy at the end of every run.
  Evidence: the run output and the file bytes after the run.
## The smallest module to rebuild

- The program map lists `main` and its 13 callees, and the smallest is
  `free` at linear `0x300ec` with three instructions and 9 bytes.  The bytes are
  `85 c0` (`test eax,eax`), `0f 85 14 1f 0a 00` (`jne 0xd2008`), and `c3`
  (`ret`), so the module forwards a non-null pointer to the run-time `free` at
  `0xd2008` and returns at once for a null pointer.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x300ec --count 3`.
- Six smaller functions sit in the object at `0x23ed0`, `0x24068`, `0x24070`,
  `0x24078`, `0x24090`, and `0x24098`.  Each holds 5 bytes and the four
  instructions `push ebp`, `mov ebp,esp`, `pop ebp`, and `ret`, and each carries
  one call site from the mode dispatch at `0x20eca` or `0x20f98`.  The bodies are
  empty, so the six stubs are the smallest functions, but they are not a module to
  rebuild.
  Evidence: the object scan and the disassembly.
- An empty C function does not generate the five bytes.  `void f(void) {}` compiles
  to `c3` (`ret`), and `-od` compiles it to a 23-byte frame with five register
  saves, so the original compiler differs from OpenWatcom V2 for an empty function.
  The six stubs stay open.
  Evidence: the container compiles and `wdis`.
- The module to rebuild is `free`: it is the smallest named module, the source is
  clear, and OpenWatcom V2 reproduces its code.
  Evidence: the size compare, the container build, and the module diff.

## The rebuilt `free` module

- The module source holds the function and no other code:
  `void free(void *p) { if (p) nfree(p); }`.  The callee carries the name `nfree`,
  because the module and the run-time function share the name `free`.
  Evidence: `tools/rebuild/free.c`.
- OpenWatcom V2 compiles the source to the original instruction sequence:
  `85 c0` (`test eax,eax`), `0f 85 <rel32>` (`jne nfree_`), and `c3` (`ret`),
  9 bytes.  The Watcom register calling convention passes the first argument in
  `eax`, so the C form reproduces the original three instructions.
  Evidence: `wdis work/rebuild/free.obj` and the original decode.
- The container build links the module with the harness stubs (`chk.asm`,
  `stack.asm`, `cstart.c`) and the stand-in `nfree` from `free_main.c`.  It writes
  `work/rebuild/free.exe` (1082 bytes) and the linker map
  `work/rebuild/free.map`.
  Evidence: `sh tools/rebuild/build_free.sh` and the map file.

## The module diff and the run

- The diff reads the original `free` at linear `0x300ec` in `STARTH.EXE` and the
  rebuilt `free` at linear `0x10000` in `free.exe`, and the rebuilt module lands
  first in the code object.  The byte compare holds 6 of 9 equal bytes, and the
  three different bytes are the `rel32` displacement of the tail jump.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x300ec --rebuilt work/rebuild/free.exe --rebuilt-map
  work/rebuild/free.map --rebuilt-symbol free_`.
- The instruction compare holds 3 of 3 equal instructions, no opcode
  differences, and one operand difference: `jne 0xd2008` against `jne 0x10034`.
  The operand is the link-time displacement of the tail jump, so the rebuilt module
  holds the original instruction sequence.  The jump goes forward in both images.
  Evidence: the diff output and the map file.
- The rebuilt module behaves like the original.  The headless run writes the four
  markers `NULL`, `DONE`, `FREE`, and `AFTR` on their own lines: the marker
  `NULL` before the null call, the marker `DONE` after it, the marker `FREE` from
  the stand-in `nfree`, and the marker `AFTR` after the non-null call.  The module
  therefore does nothing for a null pointer and forwards a non-null pointer, the same
  behavior as the original three instructions.
  Evidence: `sh tools/rebuild/run_free.sh` and `work/rebuild/free_out.txt`.
- The pipeline check holds the rebuild.  The check runs the build and the diff as
  stage 11 and reports 34 checks and 0 failures with `--skip-runtime`.
  Evidence: `python3 tools/pipeline_check.py --game-dir ... --skip-runtime`.

## The harness print helper and the string literal

- The `print` helper of the harness writes a long block of code bytes for a string
  literal.  The driver output holds 8267 code-like bytes before the `W` lines, and the
  first driver print is the literal `REBUILT$` at `DGROUP` offset 0.  The rebuilt
  module showed the same fault on its first run: the four literal prints wrote 33068
  code-like bytes and no marker text.
  Evidence: `work/rebuild/out.txt` and the first `free.exe` run.
- The `print` helper writes a program-written buffer correctly.  The driver prints the
  eight pairs through the `line` array, and the rebuilt module prints its markers
  through the `text` array after the markers moved out of the literals.
  Evidence: the two output files.
- The pointer value is not the fault.  The driver literal print passes `9` for the
  `DONE$` literal at `DGROUP` offset `9`, and the module literal prints pass `8`,
  `0xf`, and `0x16` for the three markers at the same offsets, so each pointer holds
  its literal.
  Evidence: the `main` disassembly and the `DGROUP` dump.
- The open question is the `CONST` content at run time.  The earlier finding that the
  string constants work came from the `W` letter, and the `W` is `line[0]`, a byte the
  program writes itself, so that finding covers the written bytes only.  The `hex` table
  read as zero and the literals read as code-like bytes, so the loaded content of the
  `CONST` segment is the suspect.
  Evidence: the earlier section, the disassembly, and the output files.

## The next module: the run-time exit chain

- The program map lists `exit` at linear `0x300be` with 8 instructions, the next smallest
  named function after `free`.  The bytes are `52 89 c2` (`push edx`, `mov edx,eax`), two
  `ff 15 <addr>` calls through the function-pointer slots `0x2f84` and `0x2f88`,
  `89 d0` (`mov eax,edx`), `e8 02 00 00 00` (`call 0x300d6`), and `5a c3` (`pop edx`,
  `ret`).
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x300be --count 8`.
- The helper at `0x300d6` holds 6 instructions and 22 bytes: `push edx`, `mov edx,eax`,
  the calls through the slots `0x2f88` and `0x2f8c`, `mov eax,edx`, and
  `e9 9b 02 00 00` (`jmp 0x30387`).  The helper calls the slot `0x2f88` again, so the
  two functions share one handler slot.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x300d6 --count 6`.
- The terminator at `0x30387` jumps over a message writer to `0x303ac`, which calls
  `0xd43bf` and then terminates the process with `mov ah,0x4c` and `int 0x21`.
  Evidence: the disassembly.
- The OpenWatcom V2 run-time source `bld/clib/startup/c/exit.c` holds the same shape for
  the DOS build: `exit` calls `(*__int23_exit)()` and `__FiniRtns`, `_exit` calls
  `(*__int23_exit)()` and `(*__FPE_handler_exit)()`, and both end in a call that never
  returns.  The shared slot is therefore the interrupt handler slot.
  Evidence: the source file in the OpenWatcom v2 repository.

## The rebuilt `exit` module

- The module source holds the two functions and three hook slots: `exit` calls the slots 1
  and 2 and then `exit_flush`, and `exit_flush` calls the slots 2 and 3 and then
  `exit_terminate`.  The terminator carries `__declspec(__aborts)`, the OpenWatcom V2
  spelling of the run-time `_WCNORETURN` macro.
  Evidence: `tools/rebuild/exit.c`.
- The position of `__declspec(__aborts)` matters.  The suffix form
  `void f(int) __declspec(__aborts);` fails with the error `E1026: Invalid declarator`
  and mis-parses the declarations after it; the prefix form compiles.
  Evidence: the two container compiles.
- OpenWatcom V2 compiles the source to the original instruction sequences.  `exit` holds
  8 instructions and 24 bytes, and `exit_flush` holds 6 instructions and 22 bytes, the same
  as the original.  The terminator attribute drops the epilogue of `exit_flush` and turns its
  last call into a jump, so the rebuild holds the original `jmp` form.
  Evidence: `wdis work/rebuild/exit.obj`.
- The build writes `work/rebuild/exitmod.exe` (1094 bytes) and the linker map
  `work/rebuild/exit.map`.
  Evidence: `sh tools/rebuild/build_exit.sh` and the map file.

## The exit diff and the run

- The diff of `exit` reports 8 of 8 instructions equal, no opcode differences, and three
  operand differences: the slot addresses `0x2f84` and `0x2f88` and the helper call.  The
  diff of the helper reports 6 of 6 instructions equal and three operand differences: the
  slot addresses `0x2f88` and `0x2f8c` and the terminator jump.  Both verdicts are `the
  instruction sequences match; the differing operands are the link-time addresses`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x300be --rebuilt work/rebuild/exitmod.exe --rebuilt-map
  work/rebuild/exit.map --rebuilt-symbol exit_` and the same command for `0x300d6` with
  `--count 6` and `--rebuilt-symbol exit_flush_`.
- The rebuilt module keeps the shared slot: the original calls `0x2f88` from `exit` and from
  the helper, and the rebuilt `exit` and `exit_flush` both read the slot `exit_hook_2`.
  Evidence: the two diff outputs.
- The headless run writes the markers `A`, `B`, `B`, `C`, and `T` on their own lines: the
  first hook, the second hook from `exit`, the second hook again from the helper, the third
  hook, and the terminator.  The repeated `B` shows the shared slot, so the module runs the
  original chain.
  Evidence: `sh tools/rebuild/run_exit.sh` and `work/rebuild/exit_out.txt`.
- The test program carries the name `exitmod.exe`.  A program named `exit.exe` leaves the
  DOSBox-X session before the debugger connects, because the DOS shell reads `exit` as its own
  command, and a name over eight characters fails with `Bad command or filename`.
  Evidence: the two failed runs.
- The harness sets the three hook slots in `main`, because the initialized data of the rebuilt
  image reads wrong at run time.  The assignments write the addresses of the three stub
  functions, so the module calls the stubs.
  Evidence: `tools/rebuild/exit_main.c` and the run output.

## The pipeline check holds two modules

- Stage 9 rebuilds and diffs both modules.  The check reports 20 checks and 0 failures with
  `--skip-runtime`, which is three more checks than the `free` module alone.
  Evidence: `python3 tools/pipeline_check.py --game-dir ... --skip-runtime`.
- The linker map marks a symbol that only the module references with a trailing `+`, for example
  `0001:00000000+ exit_flush_`.  The diff tool accepts the mark.
  Evidence: `work/rebuild/exit.map` and the diff run.

## The frame of the simple game functions

- `engine_shutdown` at `0x128ac` is the next named module in the program map: 11 instructions
  with an EBP frame, `push ebp`, `mov ebp,esp`, eight calls, `pop ebp`, and `ret`.  The frame
  holds no local variable.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x128ac --count 11`.
- OpenWatcom V2 does not generate the frame.  The same source with eight calls compiles to 47
  bytes without a frame, and the last call becomes `jmp f8_`, because the function returns
  nothing and the call is last.  The original holds `call 0x25f14`, `pop ebp`, and `ret`.
  Evidence: the container compile and `wdis work/rebuild/probe/shutdown_probe.obj`.
- The two differences repeat the earlier empty-stub result: the original compiler creates an EBP
  frame for a function without a local variable, and OpenWatcom V2 does not.  The frame question
  therefore covers every simple game function, not only the six stubs.
  Evidence: the empty-stub section and the probe.
- `engine_shutdown` stays open until the frame question is answered.  The next named module with a
  register-save prologue and a local slot is `printf` at `0x3009b`: `push ebx`, `push edx`,
  `sub esp,4`, the print-engine call at `0xd429f`, and the matching epilogue, with no EBP frame.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x3009b --count 13`.

## The frame answer: the stack-frame option

- The Watcom C/C++ compiler option `-of+` generates a stack frame for every function, and the
  OpenWatcom V2 default is `-of-`, no frame.  The option sweep over the 47 candidate options finds
  `-of+` alone as the frame producer.
  Evidence: the container sweep and `wdis work/rebuild/probe/shut_of.obj`.
- The original game files were built with the frame option.  With `-of+`, the `engine_shutdown` source
  holds the original frame `push ebp`, `mov ebp,esp` ... `pop ebp`, `ret`, and the last call stays a
  plain `call`, which matches the original `call 0x25f14`.  The frame option therefore also answers the
  tail-call half of the question.
  Evidence: the container compile and the original decode.
- The run-time wrappers carry no frame, so the game and the library differ.  `free`, `exit`, and
  `printf` all hold a register-save prologue, and the library build of the Watcom run-time leaves the
  frame out.  The frame rule is therefore per file, not per program.
  Evidence: the `free`, `exit`, and `printf` decodes and the `-of+` sweep.
- One difference stays.  With `-of+`, OpenWatcom V2 adds `mov esp,ebp` to the epilogue of a function
  that does not change ESP: the `engine_shutdown` rebuild holds 54 bytes against the original 52, and the
  empty stub holds `55 89 e5 89 ec 5d c3` against the original `55 89 e5 5d c3`.  The original
  compiler omits the restore when nothing moved.
  Evidence: the two container compiles.
- The frame answer unblocks the game files.  A game file rebuild takes `-of+`, and a run-time or library
  file rebuild takes the default.  The `printf` rebuild below is the first module to test the rule.
  Evidence: the module results.

## The rebuilt `printf` module

- The module source follows the OpenWatcom V2 run-time file
  `bld/clib/streamio/c/printf.c`: `va_start` over the format argument, the print-engine call with the
  standard-output file, the format, and the argument list, and the return of the count.
  Evidence: the source file in the OpenWatcom v2 repository.
- The V2 header defines `stdout` as `((__w_FILE *)&__iob[1])` and `va_start` as the intrinsic
  `__builtin_va_start`, so the compiler emits the address of the standard-output entry, which matches the
  original `mov eax, 0x2fd2`.
  Evidence: `/opt/watcom/h/stdio.h` and `/opt/watcom/h/stdarg.h`.
- The V2 `va_list` is a plain pointer, so the source passes `&args` to keep the original form, which
  passed the `va_list` by reference.  The `va_end` call is absent, because the original epilogue holds no
  trace of it.
  Evidence: `tools/rebuild/printf.c` and the two container compiles.
- OpenWatcom V2 compiles the source to 34 bytes and 13 instructions, the same as the original, with the
  register-save prologue `push ebx`, `push edx`, `sub esp,4` and no EBP frame.  The register-save
  prologue therefore avoids the frame question, which is the answer the module was built to test.
  Evidence: `wdis work/rebuild/printf.obj` and the diff.
- Three instructions come in a different order.  The original computes the argument address into `ebx`,
  loads the standard-output entry into `eax`, loads the format into `edx`, stores the argument list, and then
  passes its address; the V2 build computes the argument address into `eax`, stores it, passes the address,
  loads the format, and then loads the standard-output entry.  The instructions are the same and only the
  register allocation and the spill position differ.
  Evidence: the two decodes.
- The build writes `work/rebuild/prtfmod.exe` (4987 bytes) and the linker map
  `work/rebuild/printf.map`.
  Evidence: `sh tools/rebuild/build_printf.sh` and the map file.

## The printf diff and the run

- The diff reports 13 of 13 instructions equal, no opcode differences, and 6 operand differences.  The
  verdict is `the modules hold the same instructions in a different order`, which names the register
  allocation difference and separates it from the link-time addresses.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x3009b --rebuilt work/rebuild/prtfmod.exe --rebuilt-map
  work/rebuild/printf.map --rebuilt-symbol printf_`.
- The diff tool now names three verdicts: the link-time addresses, the same instructions in a different
  order, and a real difference.  It replaces every absolute memory address and every branch target with a
  placeholder for the shape compare, and a memory operand with a register base keeps its offset.
  Evidence: `tools/rebuild/diff_module.py` and the four module diffs.
- The headless run writes `P` and `4`: the module reaches the print-engine stub, and the count returns
  through the module.
  Evidence: `sh tools/rebuild/run_printf.sh` and `work/rebuild/prtf_out.txt`.
- The test program carries the name `prtfmod.exe`, because DOS truncates an output file name over eight
  characters to its short form: `printf_out.txt` becomes `PRINTF_O.TXT`.
  Evidence: the two runs and the work directory listing.

## The engine_shutdown rebuild and the restore instruction

- The `engine_shutdown` module rebuilds with the stack-frame option `-of+` to 54 bytes and 15 instructions
  against the original 52 bytes and 14 instructions.  The frame and the eight calls match, so the frame
  answer holds for the game files.
  Evidence: `sh tools/rebuild/build_shutdown.sh` and the diff.
- The extra instruction is `mov esp,ebp`, the ESP restore of the epilogue.  The original holds `pop ebp`
  and `ret` alone, so the original compiler dropped the restore because the frame holds no local variable
  and nothing moved the stack pointer.
  Evidence: the two decodes.
- No compiler option suppresses the restore.  The full option list of the compiler itself holds `-of` for
  `as needed`, `-of+` for `always`, `-ok` for the flow graph, and `-oc` for the call-to-jump form, and the
  sweep of the 47 candidate options found no other frame control.  The `-of+ -ok` and `-of+ -oc` builds keep
  the restore, the `-of+ -od` build replaces the prologue with the 5-register debug frame, and
  `#pragma on (frame)` changes nothing.
  Evidence: `strings /opt/watcom/binl64/wcc386` and the container compiles.
- The rest of the module matches.  The diff holds 12 of 14 instructions equal and 8 operand differences,
  which are the link-time addresses of the eight calls, and the run writes the markers `1` to `8` in order, so
  the call chain matches the original.
  Evidence: the diff output and `sh tools/rebuild/run_shutdown.sh`.
- The C rebuild stays open as a byte-level match.  The C source holds the original code shape with one extra
  restore instruction, and the difference is a code generator change between the original compiler and OpenWatcom
  V2 that no C source form can remove.  The hand-written assembly source below holds the original shape exactly.
  Evidence: the section above and `tools/rebuild/shutdown.asm`.
- The diff tool now names the case.  When the instruction counts differ and one sequence holds the other plus one
  instruction, the tool reports the extra instruction instead of only the counts.
  Evidence: `tools/rebuild/diff_module.py` and the five module diffs.

## The memcpy module matches byte for byte

- `memcpy` at `0x8c150` holds 14 bytes and 8 instructions: `pushad`, the register moves, `shr ecx,2`,
  `rep movsd`, `popad`, and `ret`.  The function carries no call and no data reference, so its code is
  position-independent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x8c150 --count 8`.
- The module is assembly, because the original routine is assembly too.  The Watcom assembler takes the Intel names
  `pushad` and `popad`: `pushal` and `popal` fail with `E032: Syntax error`.
  Evidence: the two container compiles and `wdis work/rebuild/memcpy.obj`.
- The rebuilt module matches the original byte for byte.  The diff reports 14 of 14 bytes equal, 0 different, 8 of 8
  instructions equal, no opcode differences, and no operand differences, and the verdict is `the modules are
  byte-identical`.  This is the first module with an exact match.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x8c150
  --rebuilt work/rebuild/memmod.exe --rebuilt-map work/rebuild/memcpy.map --rebuilt-symbol memcpy_`.
- The byte compare also settles the behavior: two identical instruction sequences carry the same behavior, so the module
  needs no register-level argument beyond the code match.
  Evidence: the byte-identical diff.
- The behavior run prints `N`, and the copy does not arrive.  The rebuilt `main` fills the source buffer at the `DGROUP`
  offset `0x10`, clears the target at `0x8`, calls the module with `eax=8`, `edx=0x10`, and `ebx=8`, and then compares
  the two buffers byte by byte.  The disassembly of the rebuilt `main` shows the correct order and the correct addresses, so
  the run result contradicts the code, and the harness needs more work.  The module stays verified by the byte compare.
  Evidence: `sh tools/rebuild/run_memcpy.sh` and `python3 tools/le.py disasm work/rebuild/memmod.exe 0x10029 --count 40`.

## The memcpy harness and the ES segment

- The harness fault is the `ES` segment register, not the module.  The diagnostic run prints the source bytes `A` to `H` and
  the target bytes as eight non-printing characters, so the module reads the source correctly through `DS:ESI` and writes through
  `ES:EDI` to the wrong segment.  The module itself is byte-identical to the original, so it cannot be at fault.
  Evidence: the diagnostic run and the byte-identical diff.
- The start stub of the harness never set `ES`.  `tools/rebuild/chk.asm` held `_cstart_` as `jmp main_` alone, so the
  program ran with whatever `ES` the loader left, while the game start code sets `ES` to the data segment.
  Evidence: `tools/rebuild/chk.asm`.
- The fix is `push ds` and `pop es` in `_cstart_`.  The copy then lands, and the run prints the target bytes `A` to `H`
  and the flag `Y`, so the harness passes.
  Evidence: `sh tools/rebuild/run_memcpy.sh` and `work/rebuild/mem_out.txt`.
- The fix is safe for every module.  The shared stub carries the two extra bytes for the driver, `free`, `exit`, `printf`,
  `engine_shutdown`, and `memcpy`, and the full check still reports 35 checks and 0 failures.
  Evidence: `python3 tools/pipeline_check.py --game-dir ... --port 9895`.

## The segment guard in the start stub

- The start stub now runs a guard before `main_`.  It saves the flags and the registers, sets `ES` to `DS`, writes
  the pattern `A5A5A5A5` to a stack slot through `ES:EDI`, and reads the slot back through `DS:ESI` and through
  `SS:ESP`.  A mismatch means the segments do not describe the same memory, so the harness spells `SEGFAIL` and exits
  with the code 1.
  Evidence: `tools/rebuild/chk.asm`.
- The guard fires when the segments disagree.  A build with the `push ds` and `pop es` lines removed prints `SEGFAIL` on
  the screen and leaves the module output empty, so the fault class can no longer reappear in silence.
  Evidence: the deliberate broken build and `work/rebuild/mem_out.txt`.
- The failure path uses the BIOS teletype, not the DOS print.  The DOS print reads the message through `DS:EDX`, so it
  writes garbage when `DS` is the broken segment; the teletype prints one immediate character at a time and needs no segment
  setup.  The guard spells `SEGFAIL` and then exits with the code 1.
  Evidence: the two runs, one with the guard message and one without.
- The guard is silent when the segments agree, and the module passes.  The run prints the target bytes `A` to `H` and the
  flag `Y`, and the full check reports 35 checks and 0 failures.
  Evidence: `sh tools/rebuild/run_memcpy.sh` and `python3 tools/pipeline_check.py --game-dir ... --port 9896`.

## The error_report module

- `error_report` at `0x10978` is the first game module of the rebuild and the next one in size order.  It holds 16 dispatch
  instructions and 21 message bodies: `push ebp`, `mov ebp,esp`, `sub esp,4`, the code argument stored at `[ebp-4]`, `inc eax`,
  `cmp eax,0x17`, the default jump to `0x10b87`, and the jump through `cs:[eax*4 + 0x918]`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x10978 --count 16`.
- The jump table at the linear `0x10918` holds 23 object-relative entries, and the codes 20, 21, and 22 point at the default
  body at `0x10b87`.  The bodies sit at `0x10996` to `0x10b86`, one per code, and each pushes message offsets and the
  message-table base `0x2fd2`, calls the printer at `0x4bf64`, and returns.
  Evidence: the table and body dumps.
- The message offsets are `0x44`, `0x6f`, `0x74`, `0xa3`, `0xd1`, `0xff`, `0x116`, `0x141`, `0x16e`, `0x18b`, `0x1c0`,
  `0x1f8`, `0x232`, `0x252`, `0x288`, `0x2a7`, `0x2d8`, `0x307`, `0x330`, `0x35a`, `0x369`, and `0x383`.  The codes 2, 3,
  4, 5, 6, 8, 11, and the default push the format offset `0x7fe8` as well, so the printer is a printf-style call.
  Evidence: the body dump.
- The rebuild source holds the switch with the codes 0 to 19 and the default, and the printer declaration carries `__cdecl`, so the
  compiler pushes the message table and the offsets instead of passing the first argument in a register.  The Watcom default calling
  convention passes the first argument in `eax`, which does not match the original.
  Evidence: `tools/rebuild/error_report.c` and `wdis work/rebuild/error_report.obj`.
- The rebuild does not match byte for byte yet.  Three differences stay: the original spills the code argument to a local and keeps an EBP
  frame, the rebuild keeps the argument in a register and saves `ecx` and `edx`; the rebuild shares the common case tails through jumps while
  the original expands every body; and the rebuild compares against `0x13` while the original compares against `0x17`.
  Evidence: the two disassemblies and the diff.
- The harness run crashes at a null selector.  The guard passes, because the dump shows `DS` and `ES` equal at `0x200`, so the fault is
  past the start stub.  The jump table is the suspect: its entries are absolute addresses that the loader must relocate, and the same
  class of fault already hit the `ES` register.  The module stays open as a byte-level match.
  Evidence: the run dump and the crash address `3:00000001`.
- The module is not in the pipeline check stage 11, because the diff does not match yet.
  Evidence: `tools/pipeline_check.py`.

## The error_report jump table at run time

- The crash is the compiler's jump table, and the fault is now pinned.  The DOS/4G dump reports the fault at
  `1A0:00000001`, and `EIP` is `1`, so the jump reached offset `1`.  The rebuilt `jmp` instruction holds the
  displacement `0`, so the effective address is `CS:0 + eax*4` and not the table, and the entry read there is the
  object-relative value `1`.  The table entries are also unrelocated, because the bytes at the table address are the
  first case body and not addresses.
  Evidence: `python3 tools/le.py disasm work/rebuild/errmod.exe 0x10050 --count 8` and the run dump.
- The rebuilt image therefore does not relocate the jump table.  The table lands in the code object, and the loader
  applies no fixup to its displacement or to its entries, while the data fixups work: the `exit` module reaches its
  function-pointer slots and the `memcpy` module matches byte for byte.  The code-object fixups are the open question.
  Evidence: the four module results.
- The rebuild now builds the table at run time.  The handlers are real functions, so the compiler emits their addresses
  as data that the loader relocates, the same way the `exit` module reaches its slots, and the module no longer holds a
  compiler jump table.
  Evidence: `tools/rebuild/error_report.c`.
- The crash is gone.  The harness run prints the three `R` markers, one per code, and the `D` marker, and the program
  exits cleanly.  The guard stays silent, so the segments agree.
  Evidence: `sh tools/rebuild/run_error_report.sh` and `work/rebuild/err_out.txt`.
- The code shape differs from the original.  The diff reports 4 of 16 instructions equal and the verdict
  `the instruction sequences differ`, because the run-time table replaces the compiler table.  The module therefore trades
  the byte-level match for a behavior run, and stays out of the pipeline check stage 11.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x10978 --count 16
  --rebuilt work/rebuild/errmod.exe --rebuilt-map work/rebuild/error_report.map --rebuilt-symbol error_report_`.

## The jump table is absent from the object file

- The table never reaches the code object.  The object dump lists two routines, 80 bytes at `_TEXT + 0` and 263 bytes at
  `_TEXT + 0x50`, and the whole `_TEXT` holds 343 bytes.  The table of 23 entries alone needs 92 bytes, and the linked
  `error_report_` holds 263 bytes against the original span of about 551 bytes, so the table is absent from the image and not
  only from the link.
  Evidence: `wdis work/rebuild/er.obj` and `work/rebuild/error_report.map`.
- The compiler emits the jump against a symbol whose table it does not place in the code segment.  The object dump holds
  `jmp dword ptr L$1[eax*4]` and no `L$1` definition, so the reference and the table part company at the compiler, before the
  linker runs.
  Evidence: the object dump.
- The linker cannot be told to keep the table.  The `wlink` V2.0 beta holds `option eliminate` for dead-code elimination with
  no disable form: `option noeliminate` fails with `E3033: directive error near 'noeliminate'`, and `eliminate=off`, `ELiminate=no`,
  and `eliminate=false` fail the same way.
  Evidence: the four container links.
- The original build therefore used a compiler and a linker that place and keep the table.  The original Watcom C/C++ 10.x toolchain
  did so, and the V2 toolchain does not, so a byte-level match for this module needs the original toolchain.  The project now holds the
  period Watcom 10.0a toolchain (see the last section), which emits the table reference and relocates it, and the rebuilt program runs.
  The reconstructed source still differs from the original source, so the module stays out of the pipeline check stage 11.
  Evidence: the section above, the two diffs, and the period toolchain section below.
- The compiler-table rebuild crashes again.  The V2 compiler does not place the jump table in the code object, so the
  rebuilt `jmp` reads the entry at `CS:0` and reaches offset `1`, and the run prints the DOS/4G general-protection dump
  at `1A0:00000001` with none of the `R` and `D` markers.  The earlier "crash is gone" claim and the
  "run-time table stays as the working form" claim are superseded.
  Evidence: `python3 tools/rebuild/harness.py run --module error_report` and
  `python3 tools/le.py disasm work/rebuild/errmod.exe 0x10050 --count 8`.
- The store records the behavior crash.  The `run.py run --module error_report` command exits 1 with
  `error: the output lacks 'R'`, and the `0x10978` record holds `behavior: fail`.
  Evidence: `python3 tools/rebuild/run.py run --module error_report` and the run log under `work/rebuild/logs/`.

## The bios_video module and its instruction diff

- `bios_video` at `0x108e4` is the next named module, 22 instructions and 52 bytes.  It builds a
  general register set at `[ebp-0x2c]` of 0x1c bytes and a segment register set at `[ebp-0x10]` of 0xc
  bytes, sets the `ax` word from the mode argument, calls the fill helper at `0x4bf23`, and then calls the
  interrupt helper at `0x4bf4b` with the number `0x10`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x108e4 --count 22`.
- The two helpers are now known.  The fill helper at `0x4bf23` writes the live `es`, `cs`, `ss`, `ds`, `fs`,
  and `gs` words into the first pointer, and the call helper at `0x4bf4b` passes its four arguments to the
  DPMI entry at `0xd4410`.  The first pointer is therefore the 0xc-byte segment set and the second the
  0x1c-byte general set.
  Evidence: the two helper dumps.
- The rebuild holds 20 instructions and 48 bytes against the original 22 and 52, with 6 of 48 bytes equal.  The
  structure sizes match, because the rebuild subtracts 0x28 and the original 0x28 plus the 4-byte argument slot.
  Four differences stay: the prologue order, because the original saves `ebx`, `ecx`, `edx`, and `ebp` before the
  frame and the rebuild saves them after; the argument spill, because the original stores `eax` to `[ebp-4]` and
  reloads it while the rebuild stores the `ax` word once; the epilogue, because the original restores `esp` from
  `ebp` and the rebuild uses `lea esp,[ebp-0xc]`; and the argument order, because the original computes the first
  pointer first and the rebuild last.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x108e4 --rebuilt
  work/rebuild/bvmod.exe --rebuilt-map work/rebuild/bios_video.map --rebuilt-symbol bios_video_`.
- The module rebuilds with the stack-frame option `-of+`, which gives the frame and the matching structure layout.
  Evidence: `tools/rebuild/build_bios_video.sh`.
- The run passes.  The harness prints the fill marker, the call marker, and the exit marker, and the segment guard
  stays silent.
  Evidence: `sh tools/rebuild/run_bios_video.sh` and `work/rebuild/bv_out.txt`.
- The module stays out of the pipeline check stage 11, because the diff does not match byte for byte.
  Evidence: `tools/pipeline_check.py`.

## The list_destroy module and the unoptimized build

- `list_destroy` at `0x2ed77` is 25 instructions and 54 bytes: the four register saves `ebx`, `ecx`,
  `esi`, `edi`, and `ebp`, the frame `mov ebp,esp` and `sub esp,0xc`, the two arguments stored at
  `[ebp-8]` and `[ebp-4]`, the guard `cmp dword ptr [eax],0` with the short skip, the `free` call at
  `0xd2008`, the return of the first argument through the temporary at `[ebp-0xc]`, and the epilogue.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2ed77 --count 25`.
- The module is the first game file with an unoptimized shape.  The rebuild with the game-file option
  `-of+` alone holds 13 instructions, because the compiler keeps the argument in a register, drops the
  locals, and saves only `ecx`.  The rebuild with `-of+ -od` holds 26 instructions and 64 bytes against
  the original 25 and 54, with 15 of 54 bytes equal, which is the closest game-file result so far.
  Evidence: the container builds and the two diffs.
- The `-of+ -od` rebuild matches the original in the frame, the locals, the argument stores, the guard,
  the `free` call, and the return temporary.  Four differences stay: the prologue order, because the
  original pushes the four registers and then `ebp` while the rebuild pushes `ebp` and the frame first; the
  local offsets, because the four saves sit between the frame pointer and the locals in the rebuild; the
  branch form, because the original uses one short `je` while the rebuild inverts the test and adds a long
  `jmp`; and the epilogue, because the original restores `esp` from `ebp` and the rebuild uses
  `lea esp,[ebp-0x10]`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x2ed77
  --rebuilt work/rebuild/ldmod.exe --rebuilt-map work/rebuild/list_destroy.map --rebuilt-symbol
  list_destroy_`.
- The four differences repeat the `bios_video` set, and the register-save count pins the compiler
  version.  The original saves four registers including `ebp`; the V2 `-od` build saves six and the V2
  `-od` build for this module saves four plus the frame, so the original Watcom 10.x build saves one
  register fewer than the V2 build in the same mode.
  Evidence: the four diffs.
- The run passes.  The harness prints `F`, `R`, `Z`, and `N`: the guard frees the set pointer, the return
  is the first argument, the pointer itself stays set, and the null pointer skips the free call.  The segment
  guard stays silent.
  Evidence: `sh tools/rebuild/run_list_destroy.sh` and `work/rebuild/ld_out.txt`.
- The module stays out of the pipeline check stage 11, because the diff does not match byte for byte.
  Evidence: `tools/pipeline_check.py`.

## The memory_available module and the recurring differences

- `memory_available` at `0x2f8ca` holds 27 instructions and 58 bytes: the six register saves
  `ebx`, `ecx`, `edx`, `esi`, `edi`, and `ebp`, the frame `mov ebp,esp` and `sub esp,0xc`, the
  argument stored at `[ebp-4]`, the first call at `0x2f758` with its result moved to `edx`, the
  second call at `0x2f82e` with the argument reloaded, the sum `add edx,eax`, the sum stored at
  `[ebp-8]`, the return through the temporary at `[ebp-0xc]`, and the epilogue.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2f8ca --count 27`.
- The rebuild with `-of+ -od` holds 27 instructions and 59 bytes against the original 27 and 58,
  with 31 of 58 bytes equal.  This is the first game module whose instruction count matches, and
  the byte count differs by one.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x2f8ca
  --rebuilt work/rebuild/mamod.exe --rebuilt-map work/rebuild/memory_available.map
  --rebuilt-symbol memory_available_`.
- Three of the four `bios_video` differences recur.  The prologue order recurs, because the
  original pushes the five registers and then `ebp` while the rebuild pushes `ebp` and the frame
  first.  The local offsets recur, because the saves sit between the frame pointer and the locals.
  The epilogue recurs, because the original restores `esp` from `ebp` and the rebuild uses
  `lea esp,[ebp-0x14]`.  The branch form does not apply, because this module has no conditional.
  Evidence: the two diffs.
- The register-save count follows the registers the function uses.  `list_destroy` uses `edx` only for
  the free call, and the original saves four registers; `memory_available` holds the first call result
  in `edx` across the second call, and the original saves six.  The V2 `-od` build saves the same
  six here, so the save set is not the difference for this module.
  Evidence: the three module diffs.
- The run passes.  The harness prints `F`, `S`, `T`, and `D`: the first probe runs once, the second
  probe runs once, the total is the sum, and the program exits.  The segment guard stays silent.
  Evidence: `sh tools/rebuild/run_memory_available.sh` and `work/rebuild/ma_out.txt`.
- The module stays out of the pipeline check stage 11, because the diff does not match byte for byte.
  Evidence: `tools/pipeline_check.py`.

## The Phase A library modules match byte for byte

- `memset` at `0x8c160` holds 12 instructions and 25 bytes: `pushad`, the register moves, the
  value replication, `shr ecx,2`, `rep stosd`, `popad`, and `ret`.  The function carries no call
  and no data reference.  The call sites load the destination into `eax`, the byte count into
  `edx`, and the fill value into `ebx`, and the function fills `count >> 2` dwords with the value
  byte replicated.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x8c160 --count 12`.
- The module is assembly, because the original routine is library assembly.  The rebuilt module
  matches the original byte for byte: 25 of 25 bytes equal, 12 of 12 instructions equal, and the
  verdict is `the modules are byte-identical`.  This is the third exact match.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x8c160
  --rebuilt work/rebuild/msetmod.exe --rebuilt-map work/rebuild/memset.map --rebuilt-symbol
  memset_`.
- The run passes.  The harness prints `V` and `D`: the first 16 bytes hold the repeated value and
  the rest stays the pattern.
  Evidence: `python3 tools/rebuild/harness.py run --module memset` and `work/rebuild/mset_out.txt`.
- `dpmi_fill_regs` at `0x4bf23` holds 16 instructions and 40 bytes: the `edx` save, the pointer
  moved to `edx`, the six segment reads `cs`, `ds`, `es`, `ss`, `fs`, and `gs`, the six word
  stores, and the epilogue.  It fills the 12-byte real-mode segment set in the order `es`, `cs`,
  `ss`, `ds`, `fs`, and `gs` at offsets 0, 2, 4, 6, 8, and 10.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4bf23 --count 16`.
- The rebuilt module matches byte for byte: 40 of 40 bytes equal.  The byte directive
  `db 89h, 0C2h` pins the `mov edx,eax` encoding, because the assembler emits the `8Bh` form of
  the same instruction.
  Evidence: `tools/rebuild/dpmi_fill_regs.asm` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x4bf23
  --rebuilt work/rebuild/dprmod.exe --rebuilt-map work/rebuild/dpmi_fill_regs.map --rebuilt-symbol
  dpmi_fill_regs_`.
- `memmove` at `0xd2066` holds 23 instructions and 37 bytes: the saves of `ecx`, `esi`, `edi`, and
  `es`, the destination in `edi`, the source in `esi`, the count in `ecx`, `es` set to `ds`, the
  `repne movsd` of the whole dwords, the `repne movsb` of the remainder, the destination returned
  in `eax`, and the epilogue.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0xd2066 --count 23`.
- The rebuilt module matches byte for byte: 37 of 37 bytes equal.  The five byte directives pin the
  `89h` form of the register moves, and two byte directives pin the `0F2h` prefix of the string
  moves, because the assembler rejects `repne` on a `movs` instruction and emits the `8Bh` form
  of a register move.
  Evidence: `tools/rebuild/memmove.asm` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0xd2066
  --rebuilt work/rebuild/mmmod.exe --rebuilt-map work/rebuild/memmove.map --rebuilt-symbol
  memmove_`.
- `fp_helper` at `0x4d9f4` holds 12 instructions and 29 bytes: the `eax` save, the control word
  saved to the stack, the rounding control set to truncate, `frndint`, the control word restored,
  and the stack adjusted.  The function is the Watcom floating-point helper that rounds the value
  on the FPU stack to an integer.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4d9f4 --count 12`.
- The rebuilt module matches byte for byte: 29 of 29 bytes equal.  The harness prints `F` and `D`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x4d9f4
  --rebuilt work/rebuild/fphmod.exe --rebuilt-map work/rebuild/fp_helper.map --rebuilt-symbol
  fp_helper_` and `python3 tools/rebuild/harness.py run --module fp_helper`.
- The store records the result of all four modules: build pass, identity pass, behavior pass, and
  the status `verified`.
  Evidence: `python3 tools/rebuild/run.py run --function 0x8c160`,
  `python3 tools/rebuild/run.py run --function 0x4bf23`,
  `python3 tools/rebuild/run.py run --function 0xd2066`, and
  `python3 tools/rebuild/run.py run --function 0x4d9f4`.

## The vesa_set_mode, memset_runtime, dpmi_call, and fprintf modules

- `vesa_set_mode` at `0x8bdec` holds 51 instructions and 167 bytes: the mode argument stored at
  `DAT_0007c17c`, the DOS block allocated through `dpmi_alloc_dos` at `0x8be93`, the real-mode
  register set at `DAT_0007c190` for the VBE mode info call, the mode set through INT 10h, and
  the memory model and the window bits stored from the info block.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x8bdec --count 60`.
- The module is assembly, because the original is library-style code: it holds `pushad`/`popad`,
  the direct INT 31h and INT 10h calls, and no EBP frame.  The diff reports 8 of 8 compared
  instructions equal and the verdict `the instruction sequences match; the differing operands are
  the link-time addresses`; the diff compares the early-return path, because the diff tool stops
  the body at the first terminator.
  Evidence: `tools/rebuild/vesa_set_mode.asm` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x8bdec
  --rebuilt work/rebuild/vsmmod.exe --rebuilt-map work/rebuild/vesa_set_mode.map --rebuilt-symbol
  vesa_set_mode_`.
- The harness allocator stub returns the info block in `edi` through the small assembly source
  `vesa_alloc.asm`, because the `#pragma aux` form of the compiler rejects a data symbol in its
  instruction string.  The run passes: the mode argument reaches the data area and the real-mode
  interrupt path runs under DOSBox-X.
  Evidence: `python3 tools/rebuild/harness.py run --module vesa_set_mode` and
  `work/rebuild/vsm_out.txt`.
- `memset` at `0xd1ec0` is the run-time library version, 12 instructions and 24 bytes: the value
  byte replicated over the dword, the count moved to `ecx`, and the call to the run-time core at
  `0xe0860`.  The module is named `memset_runtime`, because the game-side `memset` module at
  `tools/rebuild/memset.asm` already carries the name `memset`.  The rebuilt module matches with
  22 of 24 bytes equal, and the only difference is the call displacement.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0xd1ec0 --count 12` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0xd1ec0
  --rebuilt work/rebuild/msrtmod.exe --rebuilt-map work/rebuild/memset_runtime.map
  --rebuilt-symbol memset_runtime_`.
- `dpmi_call` at `0x4bf4b` holds 14 instructions and 25 bytes: the four arguments moved to the
  register set of the DPMI INT 31h entry at `0xd4410`, and the result read from the result block.
  The rebuilt module matches with 22 of 25 bytes equal, and the only difference is the call
  displacement.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4bf4b --count 14` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x4bf4b
  --rebuilt work/rebuild/dpcmodex.exe --rebuilt-map work/rebuild/dpmi_call.map --rebuilt-symbol
  dpmi_call_`.
- The function at `0x4bf64` is `fprintf`, not `dpmi_real_copy`.  The shape matches `printf` at
  `0x3009b`: the stack arguments, the `va_start` address, and the print-engine call at `0xd429f`.
  The call sites are the message stubs of `error_report`, the `stream_release` cases, and the
  stream functions, so the earlier name is a misnomer.  The store record and the program-map row
  now name `fprintf`, and the `stream_release` role names `fprintf` instead of the DPMI helper.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4bf64 --count 13` and
  `python3 tools/le.py disasm STARTH.EXE 0x3009b --count 13`.
- The `fprintf` module is C, like the `printf` module.  The diff reports 13 of 13 instructions
  equal and the verdict `the modules hold the same instructions in a different order`, because
  the rebuild evaluates the argument loads in a different order than the original.
  Evidence: `tools/rebuild/fprintf.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x4bf64
  --rebuilt work/rebuild/fprfmod.exe --rebuilt-map work/rebuild/fprintf.map --rebuilt-symbol
  fprintf_`.
- The store records the result of all four modules: build pass, identity pass, behavior pass, and
  the status `verified`.
  Evidence: `python3 tools/rebuild/run.py run --function 0x8bdec`,
  `python3 tools/rebuild/run.py run --function 0xd1ec0`,
  `python3 tools/rebuild/run.py run --function 0x4bf4b`, and
  `python3 tools/rebuild/run.py run --function 0x4bf64`.

## The default_data_copy, hud_update, stream_status, and race_state_update modules

- `default_data_copy` at `0x10193` holds 16 instructions and 33 bytes: the register saves, the
  segment saves, the two block addresses, `cld`, the `rep movsd` of 32 dwords, and the epilogue.
  It copies the default data block from `DAT_00010113` in object 1 into `DAT_000f0002` in object 2.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x10193 --count 16`.
- The module is assembly, because the original is library-style code: it holds `pushad`/`popad`,
  the segment register saves, and the `rep movsd`, and no EBP frame.  The diff reports 30 of 33
  bytes equal and the verdict `the modules hold the same instructions in a different order`, which
  the store accepts; the two differing operands are the relocated block addresses.
  Evidence: `tools/rebuild/default_data_copy.asm` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x10193
  --rebuilt work/rebuild/ddcmod.exe --rebuilt-map work/rebuild/default_data_copy.map
  --rebuilt-symbol default_data_copy_`.
- The run passes: the target block holds the source pattern.  The store records the status
  `verified`.
  Evidence: `python3 tools/rebuild/harness.py run --module default_data_copy` and
  `work/rebuild/ddc_out.txt`.
- `hud_update` at `0x27a34` holds 11 instructions and 38 bytes: the frame, the two guards on the
  session state `DAT_0011ce44`, and the three overlay calls.  The C rebuild with `-of+` held 12
  instructions and 40 bytes, with one extra `mov esp,ebp` in the epilogue, the same code-generator
  difference as `frame_step` and `engine_shutdown`.  The hand-written assembly source below holds
  the original shape exactly.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x27a34 --count 11` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x27a34
  --rebuilt work/rebuild/hudmod.exe --rebuilt-map work/rebuild/hud_update.map --rebuilt-symbol
  hud_update_`.
- The run passes.  The state 4 runs the first two overlays, the state 3 runs only the third, and
  the harness prints `0`, `2`, and `D`.
  Evidence: `python3 tools/rebuild/harness.py run --module hud_update` and
  `work/rebuild/hud_out.txt`.
- `stream_status` at `0x4c7e0` holds 21 instructions and 38 bytes: the six register saves, the
  frame, the two local slots, the word read at `stream + 0x70`, and the epilogue.  The C rebuild
  with `-of+ -od` held 21 instructions and 39 bytes, and the `bios_video` difference set recurred:
  the prologue order, the local offsets, and the `lea` epilogue against the original `mov esp,ebp`.
  The hand-written assembly source below holds the original shape exactly.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4c7e0 --count 21` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x4c7e0
  --rebuilt work/rebuild/stsmod.exe --rebuilt-map work/rebuild/stream_status.map --rebuilt-symbol
  stream_status_`.
- `race_state_update` at `0x29564` holds 13 dispatch instructions plus the case bodies: the frame,
  the state word read from `DAT_0011fac4`, the bound check against 8, and the compiler jump table
  at `cs:[eax*4 + 0x19540]`.  The case bodies call the state step functions, and case 1 also
  advances the frame timing value and copies the next state.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x29564 --count 40`.
- The C switch of the first rebuild emits a compiler jump table, and the jump table crashes the
  harness run, the same fault as the first `error_report` rebuild.  The rebuild now uses an
  if-else chain, which holds no jump table, and the run passes: the states 0, 2, and 7 run their
  step, and the state 8 runs nothing.
  Evidence: `tools/rebuild/race_state_update.c` and
  `python3 tools/rebuild/harness.py run --module race_state_update`.
- The float code of the module references the FPU run-time symbols `__8087` and
  `__init_387_emulator`.  The compiler emits these names without the trailing underscore, so a C
  stub cannot provide them; the stub source `fpu_stub.asm` stands in for the FPU control init and
  the 387 emulator init.
  Evidence: `tools/rebuild/fpu_stub.asm` and the link errors.
- The identity check stays open for `race_state_update`, so the store records the status
  `rebuilt` for it.  The `stream_status` record is `verified`; see the finding "The hand-written
  stream_status assembly matches the original".
  Evidence: `python3 tools/rebuild/run.py run --function 0x29564`.

## The record-run budget of the pipeline

- The record run of stage 11 builds and runs every module record through the
  runner, so it needs more time as the module count grows.  The 300-second
  default of the pipeline subprocess no longer fits the 24 records, and the
  stage timed out.  `pipeline_check.py` now holds `--record-timeout` (default
  1200 seconds) for the stage-11 subprocess.
  Evidence: `python3 tools/pipeline_check.py --game-dir
  ~/Documents/Screamer.app/Contents/Resources/game --record-timeout 1800` and
  `tools/pipeline_check.py`.
- The full check reports 70 checks and 0 failures with the 24 module records,
  and the check count grows with the module count.  The earlier sections in
  this file record the check count at the time they were written, not the
  current count.
  Evidence: `python3 tools/pipeline_check.py --game-dir
  ~/Documents/Screamer.app/Contents/Resources/game`.

## The first Phase B modules

- `music_bank_reset` at `0x2f9b0` holds 14 instructions and 46 bytes: the frame, the argument
  slot, the three field stores at `+0x04`, `+0x10`, and `+0x14`, and the epilogue.  The C rebuild
  with `-of+` held 8 instructions, because the OpenWatcom V2 compiler keeps the argument in a
  register and drops the argument slot and its reloads.  The hand-written assembly source below
  holds the original shape exactly.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2f9b0 --count 14` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x2f9b0
  --rebuilt work/rebuild/mbrmod.exe --rebuilt-map work/rebuild/music_bank_reset.map
  --rebuilt-symbol music_bank_reset_`.
- `object_gear_state` at `0x1db38` holds 16 instructions and 47 bytes: the frame, the argument
  slot, the gear state read from `+0x3f4`, the compare chain, and the three stores to `+0x4e8`.
  The original uses a compare chain, so the rebuild uses an if-else chain.  The rebuild holds 20
  instructions against 16, because the compiler saves `edx` and computes the field pointer once.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1db38 --count 16` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x1db38
  --rebuilt work/rebuild/ogsmod.exe --rebuilt-map work/rebuild/object_gear_state.map
  --rebuilt-symbol object_gear_state_`.
- `sprintf` at `0x4d32c` holds 18 instructions and 48 bytes: the register-save prologue, the
  `va_start` address, the format and the buffer arguments from the stack, the fourth argument
  `0x3d319`, the format-engine call at `0xd68c0`, and the terminator write.  The function holds no
  EBP frame, so the rebuild uses the default compiler settings like the `printf` module.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4d32c --count 18`.
- The `sprintf` rebuild passes the identity check.  The diff reports 18 of 18 instructions equal
  and 32 of 48 bytes equal, and the verdict is `the modules hold the same instructions in a
  different order`, because the rebuild evaluates the argument loads in a different order.  The
  fourth argument of the format engine is a pointer the run-time uses for the output; the raw
  value `0x3d319` is a relocated placeholder, and the rebuild passes the address of the harness
  `__iob` table.  The store records `verified`.
  Evidence: `tools/rebuild/sprintf.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x4d32c
  --rebuilt work/rebuild/sprmod.exe --rebuilt-map work/rebuild/sprintf.map --rebuilt-symbol
  sprintf_`.
- `object_prepare` at `0x1c79c` holds 16 instructions and 49 bytes: the frame, the argument slot,
  the three preparation calls at `0x1db94`, `0x1ce38`, and `0x1cbac`, the guard on the state at
  `+0x414`, and the epilogue.  The rebuild holds 15 instructions against 16, and the identity
  check stays open.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1c79c --count 16` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x1c79c
  --rebuilt work/rebuild/oprmod.exe --rebuilt-map work/rebuild/object_prepare.map
  --rebuilt-symbol object_prepare_`.
- The `-of+ -od` builds of the three game modules are worse than the `-of+` builds.  The
  OpenWatcom V2 `-od` mode saves the full register set and adds local slots, while the original
  compiler's `-of+` style saves only `ebp` and the argument slot, so the `-od` builds hold 23, 38,
  and 27 instructions against the originals 14, 16, and 16.
  Evidence: the container compiles with the two option sets and the three diffs.
- The run passes for all four modules.  The store records `rebuilt` for
  `object_gear_state` and `object_prepare`, and `verified` for `sprintf` and `music_bank_reset`.
  Evidence: `python3 tools/rebuild/run.py run --function 0x2f9b0`,
  `python3 tools/rebuild/run.py run --function 0x1db38`,
  `python3 tools/rebuild/run.py run --function 0x4d32c`, and
  `python3 tools/rebuild/run.py run --function 0x1c79c`.

## The filter_update module and the data pointer

- `filter_update` at `0x22b80` holds 20 instructions and 49 bytes: the `edx` save, the frame, the
  argument slot, the table pointer loaded from `DAT_0010cf08`, the index scaled by 4, the one-pole
  filter arithmetic, and the `ret 8` epilogue.  The function applies
  `*p = (*p - f) * table[i] + f`.  The first argument arrives in `eax`, and the float argument
  makes the Watcom compiler pass the remaining arguments on the stack.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x22b80 --count 20`.
- The rebuild with `-of+` holds 16 instructions, because the V2 compiler keeps the pointer in a
  register and drops the argument slot and its reloads.  The identity check stays open and the
  store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x22b80
  --rebuilt work/rebuild/fupmod.exe --rebuilt-map work/rebuild/filter_update.map --rebuilt-symbol
  filter_update_`.
- A global pointer initializer in a harness did not reach the real address in the first
  `filter_update` build.  The assignment `DAT_0010cf08 = table;` compiled to
  `mov dword ptr [0x1c], 4`, and the module dereferenced the raw offset.  The later
  `video_block_swap_b` harness shows that a static buffer with a pointer assignment works, so the
  fault was specific to the `filter_update` data layout: the linker placed a 4-byte gap before the
  initialized static table.  The `filter_update` harness uses a stack-local table, whose address
  the compiler computes at run time and which needs no relocation.
  Evidence: `work/rebuild/fupmod.exe` and `tools/rebuild/filter_update_main.c`.
- The run passes.  The harness prints `V` and `D`: the value holds
  `(1.0 - 0.5) * 0.25 + 0.5`.
  Evidence: `python3 tools/rebuild/harness.py run --module filter_update` and
  `work/rebuild/fup_out.txt`.
- The float code of the module needs the FPU stubs, and the stub source now also provides the
  `_fltused_` marker that the linker needs.
  Evidence: `tools/rebuild/fpu_stub.asm`.

## The video_block_swap_b module

- `video_block_swap_b` at `0x107a1` holds 18 instructions and 52 bytes: `pushad`, the video buffer
  pointer loaded from `DAT_00121528`, the far pointer at `0x3e580` bytes ahead, the two counters,
  the dword swap loop, and the epilogue.  The module swaps 200 rows of 160 dwords between the
  front of the video buffer and the region `0xf960` dwords ahead.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x107a1 --count 18`.
- The module is assembly, because the original is library-style code: it holds `pushad`/`popad`
  and no EBP frame.  The diff reports 49 of 52 bytes equal and the verdict `the instruction
  sequences match; the differing operands are the link-time addresses`, and the only difference is
  the buffer pointer displacement.
  Evidence: `tools/rebuild/video_block_swap_b.asm` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x107a1
  --rebuilt work/rebuild/vbsbmod.exe --rebuilt-map work/rebuild/video_block_swap_b.map
  --rebuilt-symbol video_block_swap_b_`.
- The run passes.  The harness fills the buffer with the index value, and after the swap the first
  row holds the far row values and the far row holds the first row values.  The harness prints `V`
  and `D`.  The static buffer with the pointer assignment works here, which corrects the data
  pointer finding of the `filter_update` section.
  Evidence: `python3 tools/rebuild/harness.py run --module video_block_swap_b` and
  `work/rebuild/vbsb_out.txt`.
- The store records the result: build pass, identity pass, behavior pass, and the status
  `verified`.
  Evidence: `python3 tools/rebuild/run.py run --function 0x107a1`.

## The video_buf_load module

- `video_buf_load` at `0x4c8f0` holds 18 instructions and 53 bytes: the register saves, the frame,
  the three arguments of the resource lookup, the call at `0x2eed0`, the two pointer stores, the
  null guard, the release call at `0x2edad`, and the epilogue.  The function looks the `VIDEO_BUF`
  resource up through `resource_get`, stores the returned pointer in `DAT_00021504` and
  `DAT_00121528`, and releases the stream when the lookup fails.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4c8f0 --count 18`.
- The two lookup constants `0x16c4` and `0x4b500` are the resource key and the name table address
  of the original image.  The rebuild passes the literals, so the argument loads hold the same
  immediates as the original.  The rebuild holds 19 instructions and 56 bytes against 18 and 53,
  and the difference is the recurring frame set: the prologue order and the `lea` epilogue against
  the original `mov esp,ebp`.
  Evidence: `tools/rebuild/video_buf_load.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x4c8f0
  --rebuilt work/rebuild/vblmod.exe --rebuilt-map work/rebuild/video_buf_load.map
  --rebuilt-symbol video_buf_load_`.
- The run passes.  The harness prints `0`, `1`, and `D`: with the non-null lookup result both
  result pointers hold it and the release stub does not run, and with the null result the release
  stub runs with the stream.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/harness.py run --module video_buf_load` and
  `work/rebuild/vbl_out.txt`.

## The menu_enter module

- `menu_enter` at `0x28b5c` holds 19 instructions and 54 bytes: the register saves, the frame,
  the saved value of `DAT_0011fb58`, the two state stores, the menu frame call at `0x29f80`, the
  restore, and the epilogue.  The function saves the value of `DAT_0011fb58`, sets `DAT_0011fb64`
  to -1 and clears the fade `DAT_0011d224`, runs the menu frame, and restores the saved value.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x28b5c --count 19`.
- The rebuild with `-of+` holds 15 instructions and 47 bytes, because the V2 compiler keeps the
  saved value in a register and drops the argument slot and the register reloads.  The identity
  check stays open and the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x28b5c
  --rebuilt work/rebuild/mntrmod.exe --rebuilt-map work/rebuild/menu_enter.map --rebuilt-symbol
  menu_enter_`.
- The run passes.  The harness prints `0`, `1`, and `D`: the state words hold the reset values and
  the frame stub ran once, and the saved value is restored.
  Evidence: `python3 tools/rebuild/harness.py run --module menu_enter` and
  `work/rebuild/mntr_out.txt`.

## The player_update module

- `player_update` at `0x19ab0` holds 14 instructions and 55 bytes: the frame, the five player
  structure addresses, the five step calls at `0x19ae8`, `0x19e78`, `0x19f00`, `0x1a144`, and
  `0x212d0`, and the epilogue.  The function runs the five steps on the player structure
  `DAT_00110a20` in call order.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x19ab0 --count 14`.
- The rebuild with `-of+` holds 15 instructions and 57 bytes, with one extra `mov esp,ebp` in the
  epilogue, the same code-generator difference as `frame_step` and `hud_update`.  The identity
  check stays open and the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x19ab0
  --rebuilt work/rebuild/pupmod.exe --rebuilt-map work/rebuild/player_update.map --rebuilt-symbol
  player_update_`.
- The run passes.  The harness prints `0` to `4` in order and `D`, so the five steps run in the
  same order as the original.
  Evidence: `python3 tools/rebuild/harness.py run --module player_update` and
  `work/rebuild/pup_out.txt`.

## The race_timeout module

- `race_timeout` at `0x27f44` holds 16 instructions and 56 bytes: the `edx` save, the frame, the
  two counter arguments, the frame delta call at `0x1fa04`, the mask, the compare against 1000,
  the two state stores, and the epilogue.  The function computes the frame delta between
  `DAT_000107fd` and `DAT_0011d228`, and when the low word of the delta is greater than 1000, it
  resets the counter to `DAT_000107fd` and sets the timeout flag `DAT_00101e04`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x27f44 --count 16`.
- The rebuild with `-of+` holds 16 instructions and 58 bytes, so the instruction count matches,
  but the instruction sequences differ: the prologue order recurs, and the V2 compiler stores the
  timeout flag with an immediate while the original moves the value through `edx`.  The identity
  check stays open and the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x27f44
  --rebuilt work/rebuild/rtofmod.exe --rebuilt-map work/rebuild/race_timeout.map --rebuilt-symbol
  race_timeout_`.
- The run passes.  The harness prints `0`, `1`, and `D`: with the delta 1500 the counter resets
  and the flag sets, and with the delta 500 both stay.
  Evidence: `python3 tools/rebuild/harness.py run --module race_timeout` and
  `work/rebuild/rtof_out.txt`.

## The dpmi_int31_entry module

- `dpmi_int31_entry` at `0xd4410` holds 28 instructions and 56 bytes: the saves, the dispatcher
  call at `0xd4448`, the register block writes, the flag write, the pops, the segment writes, and
  the epilogue.  The dispatcher turns the interrupt number into the `int N; ret` stub address from
  the table at `cs:0xc44ee`, loads the real-mode registers from the block, and runs the stub
  through the real-mode interrupt path.  The module writes the returned registers, the flags, and
  the segment values back into the block.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0xd4410 --count 28`.
- The module is assembly, because the original is a run-time routine with the segment and flag
  juggling.  The diff reports 28 of 28 instructions equal and the verdict `the instruction
  sequences match; the differing operands are the link-time addresses`, so the identity check
  passes and the store records `verified`.
  Evidence: `tools/rebuild/dpmi_int31_entry.asm` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0xd4410
  --rebuilt work/rebuild/dpiexmod.exe --rebuilt-map work/rebuild/dpmi_int31_entry.map
  --rebuilt-symbol dpmi_int31_entry_`.
- The behavior run is not included for this module.  The original writes the interrupt results
  through the real-mode segment path: it loads `ds` from the block and writes to the
  segment-relative address.  The flat harness cannot provide that path, and the run faults before
  the markers, so the module record declares the build and the identity checks only.
  Evidence: `python3 tools/rebuild/harness.py run --module dpmi_int31_entry` and
  `work/rebuild/dpiex_out.txt`.

## The object_physics module

- `object_physics` at `0x1c810` holds 18 instructions and 57 bytes: the frame, the argument slot,
  the guard on the state at `+0x414`, the two gated steps at `0x1df34` and `0x1db38`, the two
  unconditional steps at `0x1d834` and `0x1e00c`, and the epilogue.  The function runs the
  physics steps: when the state at `+0x414` is set, the slide step and the gear state run, and
  then the gear step and the collision step run.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1c810 --count 18`.
- The rebuild with `-of+` holds 17 instructions and 47 bytes, because the V2 compiler keeps the
  object pointer in a register and drops the argument slot and its reloads.  The identity check
  stays open and the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x1c810
  --rebuilt work/rebuild/obphmod.exe --rebuilt-map work/rebuild/object_physics.map --rebuilt-symbol
  object_physics_`.
- The run passes.  The harness prints `0`, `1`, and `D`: with the state cleared only the gear step
  and the collision step run, and with the state set all four steps run.
  Evidence: `python3 tools/rebuild/harness.py run --module object_physics` and
  `work/rebuild/obph_out.txt`.

## The dpmi_real_int module

- `dpmi_real_int` at `0x25db0` holds 17 compared instructions and 58 bytes plus the real-mode
  interrupt path: the state flags, the DOS multiplexer call INT 2Fh AX=1500h, the success return,
  and the real-mode path through INT 31h with the register block at `DAT_001176c0`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x25db0 --count 60`.
- The module is assembly, because the original is library-style code: it holds `pushad`/`popad`,
  the direct INT 2Fh and INT 31h calls, and no EBP frame.  The diff reports 17 of 17 compared
  instructions equal and the verdict `the instruction sequences match; the differing operands are
  the link-time addresses`, so the identity check passes and the store records `verified`.
  Evidence: `tools/rebuild/dpmi_real_int.asm` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x25db0
  --rebuilt work/rebuild/dprimod.exe --rebuilt-map work/rebuild/dpmi_real_int.map
  --rebuilt-symbol dpmi_real_int_`.
- The run passes.  The harness prints `V` and `D`: the module runs and sets the state flag
  `DAT_00116684`.  The error path stubs stand in for the report and the driver calls.
  Evidence: `python3 tools/rebuild/harness.py run --module dpmi_real_int` and
  `work/rebuild/dpri_out.txt`.

## The object_player_input module

- `object_player_input` at `0x1c760` holds 18 instructions and 58 bytes: the frame, the argument
  slot, the proximity check at `0x1e30c`, the guard on the state at `+0x2f8` and the input bits
  `DAT_00118e1c`, the drive step at `0x1cee0`, the revolution step at `0x1d30c`, and the
  epilogue.  The function runs the proximity check, and when the state at `+0x2f8` is clear and
  the input bits hold bit 0 or bit 2, it runs the drive step.  The revolution step always runs.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1c760 --count 18`.
- The rebuild with `-of+` holds 17 instructions and 49 bytes, because the V2 compiler keeps the
  object pointer in a register and drops the argument slot and its reloads.  The identity check
  stays open and the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x1c760
  --rebuilt work/rebuild/opimod.exe --rebuilt-map work/rebuild/object_player_input.map
  --rebuilt-symbol object_player_input_`.
- The run passes.  The harness prints `0`, `1`, `2`, and `D`: with the state clear and the bits
  set all three steps run, and with the state set or the bits clear the drive step does not run.
  Evidence: `python3 tools/rebuild/harness.py run --module object_player_input` and
  `work/rebuild/opi_out.txt`.

## The last Phase B modules

- `video_block_swap_a` at `0x1075f` holds 25 instructions and 66 bytes: `pushad`, the video buffer
  pointer, the far pointer at `0x27c` bytes ahead, the two counters, the byte-swap and dword swap
  loop, and the epilogue.  The module is assembly, because the original is library-style code.
  The diff reports 25 of 25 instructions equal and the identity check passes; the store records
  `verified`.  The assembler needs the `.486` model, because `bswap` is a 486 instruction.
  Evidence: `tools/rebuild/video_block_swap_a.asm` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x1075f
  --rebuilt work/rebuild/vbsamod.exe --rebuilt-map work/rebuild/video_block_swap_a.map
  --rebuilt-symbol video_block_swap_a_`.
- `sound_driver_set` at `0x262a8` holds 24 compared instructions and 79 bytes: the state guards,
  the HMI request block built at `DAT_001166a4`, the driver call, and the epilogue.  The module is
  assembly, the identity check passes, and the store records `verified`.
  Evidence: `tools/rebuild/sound_driver_set.asm` and the diff.
- `main invoker` at `0xd4324` holds 26 instructions and 79 bytes: the stack alignment, the two
  stack grow calls, the main call with the argument pair, the exit call, and the epilogue.  The
  module is assembly, because the original adjusts the stack pointer directly.  The diff reports
  the link-time addresses verdict, the identity check passes, and the store records `verified`.
  Evidence: `tools/rebuild/main_invoker.asm` and the diff.
- `stream_layer_init`, `view_move`, `panel_show`, `table_lookup`, `memory_alloc`, and
  `timing_update` are C modules.  The rebuilds hold fewer instructions than the originals, because
  the V2 compiler keeps the arguments in registers and drops the argument slots and their reloads,
  so the identity checks stay open and the store records `rebuilt` for all six.  The run passes for
  each one.
  Evidence: the six `diff_module.py` runs and the six harness runs.
- The `timing_update` module needs the FPU stubs, and the stub source now also provides the
  `__CHP` FPU exception check that the compiler emits before a float-to-int conversion.
  Evidence: `tools/rebuild/fpu_stub.asm` and the link errors.
- The Phase B batch is complete: all twenty-three Phase B functions carry a recipe and a result.
  Evidence: `python3 tools/rebuild/store.py list --module stream_layer_init` and the status page.

## The graphics_load module and the LE fixups

- `graphics_load` at `0x10ba4` holds 40 instructions and 88 bytes: the register saves, the frame,
  the two argument slots, two inline two-byte string copy loops, and the epilogue.  The function
  copies the first name argument into the buffer at object-3 offset `0x7f80` and the second name
  argument into the buffer at object-3 offset `0x7fb4`.  The two buffers are `0x34` bytes apart, so
  each holds a path of up to `0x33` characters and the terminator.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x10ba4 --count 40`.
- Observation: the raw instruction operands are `0x7f80` and `0x7fb4`, but the run-time addresses
  are `0x107f80` and `0x107fb4`.  Object 3 has base `0x100000`, and the LE fixup records relocate
  the two operands by that base.  Object 3 has seven file-backed pages (`0x7000` bytes), so both
  destinations are uninitialized data below the stack at object offset `0x472c0`.
  Evidence: `python3 tools/le.py info STARTH.EXE` (object 3 base `0x100000`, size `0x472c0`, seven
  pages) and the two raw operands in the disassembly.
- The rebuilt source uses `strcpy` with the `-oi` option.  The option expands the call into the same
  two-byte copy loop as the original.  The frame differs: OpenWatcom V2 saves the registers after
  the frame and keeps the arguments in registers, while the original compiler saves the registers
  before the frame and spills the arguments to the two slots.  The rebuild holds 37 instructions
  and the identity check stays open, so the store records `rebuilt`.
  Evidence: `tools/rebuild/graphics_load.c`, `tools/rebuild/modules/graphics_load.json`, and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x10ba4
  --rebuilt work/rebuild/glmod.exe --rebuilt-map work/rebuild/graphics_load.map
  --rebuilt-symbol graphics_load_`.
- The behavior check passes.  The harness prints `0`, `1`, and `D`: both calls store both names,
  and the second call stores the empty first name and the one-character second name.  The run
  exits 1, because the open identity check is a failed check and the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/harness.py run --module graphics_load`,
  `work/rebuild/gl_out.txt`, and `python3 tools/rebuild/run.py run --function 0x10ba4`.
- The record size was `2`, because the candidate scan treats the `push ebp` at `0x10ba6` as a
  second function start.  The record now holds the measured size `88`.
  Evidence: the record and `python3 tools/le.py near STARTH.EXE 0x10ba6`.

## The stream_close module

- `stream_close` at `0x4c38d` holds 34 instructions and 89 bytes: the six register saves, the
  frame, the stream slot, the buffer at `stream + 0x1c` released through the run-time `free` at
  `0xd2008`, the buffer at `stream + 0xb0` released through the wrapper `free` at `0x300ec` when
  the flag at `stream + 0x10` is set, the handle at `stream + 0x00` closed through `0xc6245`, and
  the zero return.  The function is the counterpart of `fopen`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4c38d --count 34`.
- The rebuild with `-of+` holds 19 instructions, because OpenWatcom V2 keeps the stream pointer in
  `edx` and drops the repeated loads from the stream slot.  The identity check stays open and the
  store records `rebuilt`.
  Evidence: `tools/rebuild/stream_close.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x4c38d
  --rebuilt work/rebuild/scmod.exe --rebuilt-map work/rebuild/stream_close.map
  --rebuilt-symbol stream_close_`.
- The behavior check passes.  The harness prints `0`, `1`, and `D`: the first call releases both
  buffers and closes the handle, and the second call releases no buffer and closes the handle.
  Evidence: `python3 tools/rebuild/harness.py run --module stream_close` and
  `work/rebuild/sc_out.txt`.

## The memory_free module

- `memory_free` at `0x2f205` holds 33 instructions and 90 bytes: the four register saves, the
  frame, the two argument slots, the local register block and segment set, the `dpmi_fill_regs`
  call at `0x4bf23`, the `dpmi_call` call at `0x4bf4b`, and the epilogue.  The function calls the
  DPMI function 0x0502 (free memory block): it stores 0x0502 at block offset 0, the high address
  word at offset `0x10`, and the low address word at offset `0x14`, then returns the status dword
  at offset `0x18`.  The first argument is stored and not used.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2f205 --count 33`.
- The only caller is `resource_free` at `0x2f470`, which passes the resource pointer in `eax` and
  the block address in `edx` at the call site `0x2f4ca`.  The rebuilt module keeps the two-argument
  signature, so the calling convention stays the same.
  Evidence: the call site at `0x2f4ca` and `tools/rebuild/functions/0x2f470.json`.
- The rebuild with `-of+` holds 24 instructions, because OpenWatcom V2 keeps the local block
  address in a register and collapses the repeated loads.  The identity check stays open and the
  store records `rebuilt`.
  Evidence: `tools/rebuild/memory_free.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x2f205
  --rebuilt work/rebuild/mfmod.exe --rebuilt-map work/rebuild/memory_free.map
  --rebuilt-symbol memory_free_`.
- The behavior check passes.  The harness prints `0`, `1`, and `D`: the call holds the interrupt
  0x31, one block for the input and the result, the segment set, the free request 0x0502, and the
  split address, and the module returns the status at block offset 0x18.
  Evidence: `python3 tools/rebuild/harness.py run --module memory_free` and
  `work/rebuild/mf_out.txt`.

## The panel_draw_centered module

- `panel_draw_centered` at `0x2e7cc` holds 34 instructions and 93 bytes: the frame, the four
  argument slots, the measure call at `0x254a8`, the signed halving of the measured width, the
  centering on the `0x140` screen width, the draw call at `0x25020`, and the epilogue.  The
  function measures the text, halves the measured width, centers that width, and draws the text
  with the doubled position.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2e7cc --count 34`.
- The measure call receives the two output pointers in `eax` and `edx`, the text in `ebx`, and the
  font in `ecx`.  The draw call receives the position in `eax` and `edx`, the text in `ebx`, the
  font in `ecx`, and the fifth argument on the stack.
  Evidence: the register loads before the two calls at `0x2e7ea` and `0x2e820`.
- The rebuild with `-of+` holds 37 instructions, because OpenWatcom V2 saves the two registers it
  uses and lays out the halves and the product differently.  The identity check stays open and the
  store records `rebuilt`.
  Evidence: `tools/rebuild/panel_draw_centered.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x2e7cc
  --rebuilt work/rebuild/pdcmod.exe --rebuilt-map work/rebuild/panel_draw_centered.map
  --rebuilt-symbol panel_draw_centered_`.
- The behavior check passes.  The harness prints `0`, `1`, `2`, and `D`: the even width centers on
  `0x140`, the negative width uses the signed division, and the odd width rounds toward zero.
  Evidence: `python3 tools/rebuild/harness.py run --module panel_draw_centered` and
  `work/rebuild/pdc_out.txt`.

## The run-time free module

- The run-time `free` at `0xd2008` holds 35 instructions and 94 bytes: the four register saves,
  the free-list walk from `DAT_001031d0`, the heap release call at `0xe0a84`, the largest-free
  size update in `DAT_001031d8`, the heap flag clear at `0x27de5`, and the epilogue.  The module is
  assembly, because the original is the run-time allocator code.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0xd2008 --count 35`.
- The rebuild matches all 35 instructions.  The only differing operands are the link-time addresses
  of the three data words and the heap call, so the identity check passes and the store records
  `verified`.
  Evidence: `tools/rebuild/runtime_free.asm` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0xd2008
  --rebuilt work/rebuild/rfmod.exe --rebuilt-map work/rebuild/runtime_free.map
  --rebuilt-symbol free_`.
- Observation: the heap routine at `0xe0a84` does not use the C register calling convention.  The
  run-time `free` passes the released block in `eax` and the free-list node in `ebx`.  The C stub
  in the harness therefore carries `#pragma aux heap_free parm [eax] [ebx]`; without the pragma the
  stub reads the node from `edx` and the behavior check fails.
  Evidence: the call at `0xd2039` and `tools/rebuild/runtime_free_main.c`.
- The behavior check passes.  The harness prints `0`, `1`, `2`, and `D`: the walk stops at the
  first free node, the largest-free size rises to the node value, and the end-pointer guard keeps
  the size.
  Evidence: `python3 tools/rebuild/harness.py run --module runtime_free` and
  `work/rebuild/rf_out.txt`.

## The race_cleanup module

- `race_cleanup` at `0x2a1b0` holds 24 instructions and 97 bytes: the two register saves, the
  frame, twelve cleanup calls, and the epilogue.  The calls run in this order: `0x264ec`,
  `0x1be6c`, `0x25640`, `0x258b8`, `0x115f8`, `0x11304`, `0x11a08`, `0x4cecc`, `0x1b58c`,
  `0x2f326`, `0x1f6f8`, and `0x2fe68`.  The first two calls that take arguments receive the data
  addresses `0x6530` and `0x6350`, then `0x6530` and `0xfed`; the last two receive the pointer at
  `0x62ac` and the address `0x62b0`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2a1b0 --count 24`.
- The call targets have no recovered names, so the rebuilt module names them `cleanup_01` through
  `cleanup_12` and records the addresses in the source comment.  A later analysis can replace a
  name without a rebuild change.
  Evidence: `tools/rebuild/race_cleanup.c`.
- The rebuild with `-of+` holds 25 instructions, because OpenWatcom V2 adds the extra
  `lea esp,[ebp-4]` restore and keeps the data addresses in different registers.  The identity
  check stays open and the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x2a1b0
  --rebuilt work/rebuild/rcmod.exe --rebuilt-map work/rebuild/race_cleanup.map
  --rebuilt-symbol race_cleanup_`.
- The behavior check passes.  The harness prints `0`, `1`, and `D`: the twelve stubs run in the
  original order, and the argument stubs receive the imported data addresses.
  Evidence: `python3 tools/rebuild/harness.py run --module race_cleanup` and
  `work/rebuild/rc_out.txt`.

## The vis_tab_load module

- `vis_tab_load` at `0x4d030` holds 31 instructions and 101 bytes: the three register saves, the
  frame, the stream slot, the `fopen` call at `0xc5fe5`, the three-call error path, the `fread`
  call at `0xc607b`, the handle close at `0xc6245`, and the epilogue.  The function opens
  `TABS\VIS.TAB` with the mode `rb`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4d030 --count 31`.
- The read count is `0x27500`, not the `0x40740` of the earlier note.  The `TABS/VIS.TAB` file is
  `161024` bytes, which is `0x27500`, and the `fread` call passes that count in `ebx`.  The
  `TABS/PERSP01.TAB` loader at `0x4cf94` uses `0x40740`, so the earlier note copied the wrong
  count from that loader.
  Evidence: the `fread` call at `0x4d081` and the `ls -l` size of `TABS/VIS.TAB`.
- On failure the function runs `error_report(3)`, `bios_video(0x16)`, and `exit(0x16)`.  The `exit`
  call does not return, so the read path that follows the three calls is unreachable in the game.
  The harness stub returns, so the rebuilt module falls through to the read path, and the failure
  marker checks only the three error calls.
  Evidence: the three calls at `0x4d056`, `0x4d060`, and `0x4d06a`, and `tools/rebuild/vis_tab_load_main.c`.
- The rebuild with `-of+` holds 32 instructions, because OpenWatcom V2 saves `esi` and lays out the
  frame differently.  The identity check stays open and the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x4d030
  --rebuilt work/rebuild/vtlmod.exe --rebuilt-map work/rebuild/vis_tab_load.map
  --rebuilt-symbol vis_tab_load_`.
- The behavior check passes.  The harness prints `0`, `1`, and `D`: the success path opens, reads
  `0x27500` bytes into the buffer, and closes the handle, and the failure path runs the three error
  calls.
  Evidence: `python3 tools/rebuild/harness.py run --module vis_tab_load` and
  `work/rebuild/vtl_out.txt`.

## The object_state_save module

- `object_state_save` at `0x1ccd8` holds 29 instructions and 105 bytes: the two register saves, the
  frame, the object slot, the four dword copies, the float copy, and the epilogue.  It copies the
  fields at `+0x454`, `+0x45c`, `+0x498`, and `+0x4a0` into `+0x160`, `+0x168`, `+0x16c`, and
  `+0x174`, and the float at `+0x418` into `+0x178`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1ccd8 --count 29`.
- The rebuild with `-of+` holds 17 instructions, because OpenWatcom V2 keeps the object pointer in
  a register and drops the repeated loads from the object slot.  The identity check stays open and
  the store records `rebuilt`.
  Evidence: `tools/rebuild/object_state_save.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x1ccd8
  --rebuilt work/rebuild/ossmod.exe --rebuilt-map work/rebuild/object_state_save.map
  --rebuilt-symbol object_state_save_`.
- The module uses the FPU for the float copy, so the recipe links the shared `fpu_stub.asm`.
  Evidence: `tools/rebuild/modules/object_state_save.json` and the build.
- The behavior check passes.  The harness prints `0`, `1`, and `D`: the four dword fields hold the
  four source values, and the float field holds the source bit pattern.
  Evidence: `python3 tools/rebuild/harness.py run --module object_state_save` and
  `work/rebuild/oss_out.txt`.

## The input_poll module

- `input_poll` at `0x26448` holds 27 instructions and 108 bytes up to the first return, plus the
  early-exit block at `0x264b4`.  The function tests the flag bit 0 at `DAT_0016669e` and
  `DAT_00166684`, rejects a set flag bit 0 at `DAT_00166688`, counts `DAT_001666bc` to 8, resets
  the counter, reads the input through `0x260e0`, and applies the input through `0x263cc` when the
  device word bit 9 is clear and `DAT_001666ac` is set.  It returns 0 on every early exit and
  `DAT_00166696` on the normal path.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x26448 --count 30`.
- The apply call receives the state `DAT_00166696` in `eax` and the device pointer in `edx`.  The
  device word is read at `device + 3`, not at the start of the device block.
  Evidence: the loads at `0x26489` and `0x2649d` and the call at `0x264a8`.
- The `function_body` sweep stops at the first `ret` at `0x264b3`, so the record carries the size
  108 and the identity check compares only the first 27 instructions.  The four-byte early-exit
  block at `0x264b4` is part of the function, and the behavior check covers it.
  Evidence: `tools/le.py near STARTH.EXE 0x264b4` and the record size.
- The rebuild with `-of+` holds 37 instructions, because OpenWatcom V2 does not use `pushal`, keeps
  the state words in registers, and lays out the early exits differently.  The identity check stays
  open and the store records `rebuilt`.
  Evidence: `tools/rebuild/input_poll.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x26448
  --rebuilt work/rebuild/ipmod.exe --rebuilt-map work/rebuild/input_poll.map
  --rebuilt-symbol input_poll_`.
- The behavior check passes.  The harness prints `0`, `1`, `2`, `3`, `4`, and `D`: the normal path
  resets the counter, reads the input, and applies it; a counter below 8 returns 0; a clear first
  flag returns 0; a set device bit 9 skips the apply; and a null device pointer returns 0.
  Evidence: `python3 tools/rebuild/harness.py run --module input_poll` and
  `work/rebuild/ip_out.txt`.

## The race_loop module

- `race_loop` at `0x27700` holds 34 instructions and 110 bytes: the three register saves, the frame,
  the `race_start` call at `0x27770`, the start-result test, the loop body, the exit test on
  `DAT_0011ce44`, the `race_end` call at `0x279a4`, and the epilogue.  The loop body calls
  `motion_update`, `default_data_copy`, `joystick_update`, the gated `input_poll`, `frame_timer`,
  `input_map`, `object_update_all`, and `video_fill`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x27700 --count 34`.
- The loop is a do-while: the body runs once before the first `DAT_0011ce44 != 4` test, and the
  test at `0x2775f` jumps back to the body start at `0x2771b`.  A non-zero start result returns at
  once and skips the loop and `race_end`.
  Evidence: the `jne 0x2771b` at `0x2775f` and the start test at `0x27714`.
- The rebuild with `-of+` holds 24 instructions, because OpenWatcom V2 keeps the loop state in
  registers and drops the repeated loads.  The identity check stays open and the store records
  `rebuilt`.
  Evidence: `tools/rebuild/race_loop.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x27700
  --rebuilt work/rebuild/rlmod.exe --rebuilt-map work/rebuild/race_loop.map
  --rebuilt-symbol race_loop_`.
- The behavior check passes.  The harness prints `0`, `1`, `2`, `3`, and `D`: a non-zero start
  result returns at once, the state 4 runs the loop once, a clear input flag skips the input poll,
  and the second iteration sees the state 4 and exits.  The order checks use explicit step
  comparisons.  The first harness version compared the order through a string pointer, and that
  comparison failed even when the recorded order was correct; the explicit comparisons pass.
  Evidence: `python3 tools/rebuild/harness.py run --module race_loop` and `work/rebuild/rl_out.txt`.

## The setup_load module

- `setup_load` at `0x1bb58` holds 35 instructions and 111 bytes: the three register saves, the
  frame, the 0x68-byte path buffer, the `sprintf` call at `0x4d32c`, the `fopen` call at
  `0xc5fe5`, the two-call error path, the `fread` call at `0xc607b`, the handle close at
  `0xc6245`, and the epilogue.  The function builds `%s\config\setup.cfg` with the base path at
  `DAT_00007edc`, opens the path with the mode `rb`, and reads `0x90` bytes into the buffer at
  `DAT_00008058`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1bb58 --count 35`.
- On failure the function reports `Run SETUP before.` through `0xc656e` and calls `exit(-1)` at
  `0x300be`.  The exit call does not return, so the read path is unreachable in the game.  The
  harness exit stub returns, so the failure marker checks the two error calls.
  Evidence: the calls at `0x1bb92` and `0x1bb9c`.
- The read buffer at `0x8058` is the same address that `table_lookup` tests as a flag.  The first
  dword of the setup data therefore selects the table behavior.
  Evidence: `tools/rebuild/table_lookup.c` and the `fread` call at `0x1bbb3`.
- The rebuild with `-of+` holds 37 instructions, because OpenWatcom V2 lays out the vararg call and
  the frame differently.  The identity check stays open and the store records `rebuilt`.
  Evidence: `tools/rebuild/setup_load.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x1bb58
  --rebuilt work/rebuild/slmod.exe --rebuilt-map work/rebuild/setup_load.map
  --rebuilt-symbol setup_load_`.
- The behavior check passes.  The harness prints `0`, `1`, and `D`: the success path builds the
  path, opens it, reads `0x90` bytes, and closes the handle; the failure path reports the message
  and exits with -1.
  Evidence: `python3 tools/rebuild/harness.py run --module setup_load` and `work/rebuild/sl_out.txt`.

## The music_play module

- `music_play` at `0x2ff14` holds 39 instructions and 111 bytes: the three register saves, the
  frame, the request slot, the two state guards, the ready check at `0xd4239`, the start call at
  `0xd38e6`, the selection call at `0x2febc`, the state stores, and the epilogue.  The function
  processes a music request record and returns the state it wrote.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2ff14 --count 39`.
- A negative word at `+0x04` writes the state 6 and returns at once.  A zero ready result writes the
  state 5 and skips the start call.  Otherwise the function starts the track, runs the selection
  when the word at `+0x0c` is 1, writes the state 0, and returns 0.
  Evidence: the guards at `0x2ff22` and `0x2ff32` and the selection test at `0x2ff46`.
- The rebuild with `-of+` holds 32 instructions, because OpenWatcom V2 keeps the record pointer in a
  register and folds the repeated loads.  The identity check stays open and the store records
  `rebuilt`.
  Evidence: `tools/rebuild/music_play.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x2ff14
  --rebuilt work/rebuild/mpmod.exe --rebuilt-map work/rebuild/music_play.map
  --rebuilt-symbol music_play_`.
- The behavior check passes.  The harness prints `0`, `1`, `2`, `3`, and `D`: the negative word
  sets the state 6, the zero ready result sets the state 5, the ready track starts and selects, and
  the ready track without the select word starts only.
  Evidence: `python3 tools/rebuild/harness.py run --module music_play` and `work/rebuild/mp_out.txt`.

## The race_state_begin module

- `race_state_begin` at `0x294c8` holds 35 instructions and 117 bytes: the six register saves, the
  frame, the parameter slot, the six constant stores, the saved-state copy, the parameter store,
  the state store, and the epilogue.  The function sets the race constants 2.0, 0.6, 1.0, 20000,
  and -12.566371, sets the previous-state word to -1, copies the old state into the saved-state
  word, stores the parameter in the race parameter word, and sets the state to 1.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x294c8 --count 35`.
- The float constants travel as the immediate bit patterns `0x40000000`, `0x3f19999a`,
  `0x3f800000`, and `0xc1490fdb`.  The last value is -4π rounded to a float.  The module uses the
  literal `-12.566370964f`, which builds to the same bit pattern.
  Evidence: the immediates at `0x294d6` and the behavior check.
- The rebuild with `-of+` holds 17 instructions, because OpenWatcom V2 stores the immediates
  directly and does not load them into the six registers first.  The identity check stays open and
  the store records `rebuilt`.
  Evidence: `tools/rebuild/race_state_begin.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x294c8
  --rebuilt work/rebuild/rsbmod.exe --rebuilt-map work/rebuild/race_state_begin.map
  --rebuilt-symbol race_state_begin_`.
- The module uses the FPU support, so the recipe links the shared `fpu_stub.asm`.
  Evidence: `tools/rebuild/modules/race_state_begin.json` and the link errors.
- The behavior check passes.  The harness prints `0`, `1`, and `D`: the five constants hold the
  original bit patterns, the saved state holds the old state 7, the parameter holds 0x1234, and the
  state is 1.
  Evidence: `python3 tools/rebuild/harness.py run --module race_state_begin` and
  `work/rebuild/rsb_out.txt`.

## The net_race_screen function extent

- The first-return body of `net_race_screen` at `0x28ba4` holds 34 instructions and 118 bytes: the
  state guard, the menu index and the two table reads, the menu-id switch, and the epilogue.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x28ba4 --count 34`.
- Observation: the function does not end at that return.  The case blocks after it run to
  `0x293a7`, so the full function holds 646 instructions and 2052 bytes.  The `function_body` sweep
  in `tools/le.py` stops at the first `ret`, so the plan table and the record carry the first-return
  size.  The same limitation applies to `race_state_update` at `0x29564`, which the project already
  rebuilt as a dispatcher with step stubs.
  Evidence: the disassembly from `0x28ba4` to `0x293a7` and the `race_state_update` module.
- The dispatcher reads the menu index at `DAT_0011fb60`, the entry at `DAT_0011fb08`, and the record
  base at `DAT_00102de8`, then switches on the record id at offset `+4`.  The case ids are 0x00,
  0x01, 0x02, 0x03, 0x04, 0x15, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, and 0x4d2.
  The id 0x1c selects the low case when `DAT_00120880 < 0x7a1200` and the high case otherwise.
  Evidence: the dispatch at `0x28bbf` and the case targets in `tools/rebuild/net_race_screen.c`.
- The rebuild keeps the dispatcher and represents each case body and the state 2 overlay body at
  `0x28fd6` as a step function.  The behavior harness runs the overlay entry, the ids 0, 0x1c, 0x1f,
  and 0x4d2, and an unknown id.  The store records `rebuilt`.
  Evidence: `tools/rebuild/net_race_screen.c`, `tools/rebuild/net_race_screen_main.c`, and
  `python3 tools/rebuild/harness.py run --module net_race_screen`.
- The behavior check passes.  The harness prints `0`, `1`, `2`, `3`, `4`, `5`, and `D`: the overlay
  entry runs the overlay, each known id runs its case, and the unknown id runs no case.
  Evidence: `work/rebuild/nrs_out.txt`.

## The dos_memory_alloc module

- `dos_memory_alloc` at `0x2f0fc` holds 41 instructions and 118 bytes: the four register saves, the
  frame, the 0x3c-byte local area, the DPMI function 0x0100 setup, the `dpmi_fill_regs` call at
  `0x4bf23`, the `dpmi_call` call at `0x4bf4b`, the status test, the segment shift, the selector
  store, and the epilogue.  The function allocates a DOS memory block through DPMI.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2f0fc --count 41`.
- The paragraph count is `(size >> 4) + 1`.  The function writes `0x0100` to result word 0 and the
  paragraph count to word 1.  After the call it returns 0 when word 6 is not zero.  Otherwise it
  returns the segment in word 0 times 16 and stores the selector in word 3 at the third argument.
  The first argument is stored and never used.
  Evidence: the stores at `0x2f111` and `0x2f11f` and the reads at `0x2f13d`, `0x2f14c`, and
  `0x2f155`.
- The result block and the request block are the same local area.  The rebuild therefore captures
  the two request words in the `dpmi_call` stub before the stub writes the result.  A harness that
  reads the block after the call sees the result word in place of the request word.
  Evidence: `tools/rebuild/dos_memory_alloc_main.c`.
- The rebuild with `-of+` holds 32 instructions, because OpenWatcom V2 keeps the block address in
  registers and folds the repeated loads.  The identity check stays open and the store records
  `rebuilt`.
  Evidence: `tools/rebuild/dos_memory_alloc.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x2f0fc
  --rebuilt work/rebuild/dmamod.exe --rebuilt-map work/rebuild/dos_memory_alloc.map
  --rebuilt-symbol dos_memory_alloc_`.
- The behavior check passes.  The harness prints `0`, `1`, and `D`: the success path requests 0x13
  paragraphs, returns the segment times 16, and stores the selector; the non-zero status returns 0
  and leaves the selector.
  Evidence: `python3 tools/rebuild/harness.py run --module dos_memory_alloc` and
  `work/rebuild/dma_out.txt`.

## The video_present module

- `video_present` at `0x27ec8` holds 40 instructions and 122 bytes: the three register saves, the
  frame, the 0x310-byte local area, the loop over the 0x300 source bytes from `0xcf5c` to `0xd25c`,
  the scale and offset arithmetic, the `0x4d9f4` rounding helper, the `palette_set` call at
  `0x81a05`, and the `ret 8` epilogue.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x27ec8 --count 40`.
- The function maps each source byte with `(byte - offset) * scale + offset`, rounds it, stores it
  in the local palette, and passes the palette to `palette_set`.  The scale at `[ebp + 0x10]` and
  the offset at `[ebp + 0x14]` arrive on the stack; the `ret 8` epilogue pops them.
  Evidence: the stack reads at `0x27f0f` and `0x27f12` and the `ret 8` at `0x27f3f`.
- The source bytes are unsigned.  The harness fills the second case with the even values `2 * i`,
  so the half scale produces exact integers and the check does not depend on the rounding mode.  The
  value at index `0xff` is `0xfe`, so the half scale gives `0x7f`, not `0xff`.
  Evidence: `tools/rebuild/video_present_main.c`.
- The rebuild with `-of+` holds 29 instructions, because OpenWatcom V2 keeps the loop variables in
  registers and uses the register calling convention instead of the stack convention.  The identity
  check stays open and the store records `rebuilt`.
  Evidence: `tools/rebuild/video_present.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x27ec8
  --rebuilt work/rebuild/vpmod.exe --rebuilt-map work/rebuild/video_present.map
  --rebuilt-symbol video_present_`.
- The behavior check passes.  The harness prints `0`, `1`, `2`, and `D`: the scale 1.0 copies the
  source, the scale 0.5 halves the even bytes, and the scale 2.0 with the offset 10 lifts the byte
  20 to 30.
  Evidence: `python3 tools/rebuild/harness.py run --module video_present` and
  `work/rebuild/vp_out.txt`.

## The resource_free module

- `resource_free` at `0x2f470` holds 47 instructions and 126 bytes: the four register saves, the
  frame, the two argument slots, the record address arithmetic, the flag test at `+0x18` bit 0, the
  two free calls at `0x2f40b` and `0x2f205`, the return paths, and the epilogue.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2f470 --count 47`.
- The function selects the 0x1c-byte record at `*resource + index * 0x1c` and reads the address at
  `+0x10`.  A set flag bit 0 uses the DOS memory free at `0x2f40b`; a clear bit uses
  `memory_free` at `0x2f205`.  A non-zero free result returns -1, and a success returns 0.
  Evidence: the flag test at `0x2f48e` and the two calls at `0x2f4a5` and `0x2f4ca`.
- The rebuild with `-of+` holds 23 instructions, because OpenWatcom V2 keeps the record address in
  registers and folds the repeated arithmetic.  The identity check stays open and the store records
  `rebuilt`.
  Evidence: `tools/rebuild/resource_free.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x2f470
  --rebuilt work/rebuild/rsfmod.exe --rebuilt-map work/rebuild/resource_free.map
  --rebuilt-symbol resource_free_`.
- The behavior check passes.  The harness prints `0`, `1`, `2`, `3`, and `D`: the clear flag calls
  `memory_free`, the set flag calls `dos_memory_free`, and the two non-zero results return -1.
  Evidence: `python3 tools/rebuild/harness.py run --module resource_free` and
  `work/rebuild/rsf_out.txt`.

## The list_create module

- `list_create` at `0x2ecf8` holds 46 instructions and 127 bytes: the four register saves, the
  frame, the two argument slots, the `pool_init` call at `0x2f904`, the `malloc` call at `0xd1f25`,
  the pointer check, the two store paths, the `memset` call at `0xd1ec0`, and the epilogue.  The
  function allocates `count` records of 0x1c bytes.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2ecf8 --count 46`.
- The list record holds the pointer at `+0`, the cursor at `+4`, and the count at `+8`.  A success
  stores the pointer, the count, and the cursor 0, then zeroes `count * 0x1c` bytes.  A failed
  allocation stores the pointer 0, the cursor 0, and the count 0.  The function returns the list
  pointer.
  Evidence: the stores at `0x2ed21`, `0x2ed49`, and `0x2ed4f`, and the failure path at `0x2ed2b`.
- The rebuild with `-of+` holds 26 instructions, because OpenWatcom V2 keeps the list pointer in a
  register and folds the repeated loads.  The identity check stays open and the store records
  `rebuilt`.
  Evidence: `tools/rebuild/list_create.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x2ecf8
  --rebuilt work/rebuild/lcrmod.exe --rebuilt-map work/rebuild/list_create.map
  --rebuilt-symbol list_create_`.
- The behavior check passes.  The harness prints `0`, `1`, and `D`: the success path stores the
  pointer, the cursor 0, and the count 3, and zeroes 0x54 bytes; the failure path stores the pointer
  0, the cursor 0, and the count 0.
  Evidence: `python3 tools/rebuild/harness.py run --module list_create` and
  `work/rebuild/lcr_out.txt`.

## The pool_init module

- `pool_init` at `0x2f904` holds 44 instructions and 129 bytes: the six register saves, the frame,
  the 0x30-byte local area, the DPMI function 0x0500 setup, the `dpmi_fill_regs` call at `0x4bf23`,
  the `dpmi_call` call at `0x4bf4b`, the status test, the two page conversions, and the epilogue.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2f904 --count 44`.
- The function sets `0x0500` at result word 0 and the result buffer `pool + 0x1c` at word 5.  A
  non-zero status returns 1.  Otherwise the field at `+0x24` is multiplied by 4096, the field at
  `+0x3c` is multiplied by 4096 when it is positive and set to 0 otherwise, and the function
  returns 0.
  Evidence: the stores at `0x2f915` and `0x2f922`, the status test at `0x2f940`, and the shifts at
  `0x2f952` and `0x2f962`.
- The harness stub writes the free-memory fields only when the status is 0.  A stub that writes the
  fields on failure would overwrite the values the failure test wants to keep.
  Evidence: `tools/rebuild/pool_init_main.c`.
- The rebuild with `-of+` holds 42 instructions, because OpenWatcom V2 keeps the pool pointer in a
  register and lays out the two shifts differently.  The identity check stays open and the store
  records `rebuilt`.
  Evidence: `tools/rebuild/pool_init.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x2f904
  --rebuilt work/rebuild/pimod.exe --rebuilt-map work/rebuild/pool_init.map
  --rebuilt-symbol pool_init_`.
- The behavior check passes.  The harness prints `0`, `1`, `2`, and `D`: the positive field converts
  to bytes, the zero field stays 0, and the non-zero status returns 1 and leaves the fields.
  Evidence: `python3 tools/rebuild/harness.py run --module pool_init` and
  `work/rebuild/pi_out.txt`.

## The race_session_start module

- `race_session_start` at `0x128e0` holds 42 instructions and 136 bytes up to the
  second error return.  The true extent continues to `0x12c0b` and holds 812
  bytes.  The plan row counts the body up to the second error return.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x128e0 --count 200` and the
  record extent.
- The function sets `_DAT_00118e1c` to 0x102, calls `0x131a8`, sets
  `DAT_0011521c` to 0x1234 and `_DAT_0011cb14` to 1, clears the five state
  globals, calls `0x13580` and `0x13540`, and returns a non-zero result at once
  from each of the two load calls.
  Evidence: the stores at `0x128ee`, `0x1293a`, and `0x12940`, and the calls at
  `0x128fb`, `0x12946`, and `0x1294e`.
- Observation: the switch sends the modes 1 and 2 to `0x12c0c`, the mode 4 to
  `0x12e18`, the mode 0x10 to `0x12d0c`, the mode 0x20 to `0x13120`, the mode
  0x40 to `0x130a0`, and the mode 0x100 to `0x12f50`.  The mode 3 takes the
  default path.  This corrects the earlier note that groups the mode 3 with the
  modes 1 and 2.
  Evidence: the comparisons at `0x1296d` through `0x1298e` and at `0x12b0e`
  through `0x12b30`.
- Inference: the handler names follow the mode value.  The other call targets
  keep the address in the source comment, because the analysis has no name for
  them.
  Evidence: `tools/rebuild/race_session_start.c`.
- The default path runs `view_move(0, 0, 0)` at `0x1f94c` and
  `0x21da8(value ^ 1)`.  A non-zero value also runs `0x4cf3c` and `0x258d0` with
  the buffer `0xcf5c`.  The function then clears 0x3e800 bytes at the pointer
  `DAT_00021528`, runs `video_fill`, prepares the panel, refreshes the input,
  runs `player_update`, and clears the five frame words.
  Evidence: the calls at `0x12a56` through `0x12c01`.
- The rebuild with `-of+` holds 140 instructions, because OpenWatcom V2 puts the
  frame first, saves three registers, keeps the parameter in `ecx`, and shares
  one error epilogue.  The identity check stays open at 13 of 42 instructions
  and 12 of 136 bytes, and the store records `rebuilt`.
  Evidence: `tools/rebuild/race_session_start.c` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x128e0 --rebuilt work/rebuild/rssmod.exe --rebuilt-map
  work/rebuild/race_session_start.map --rebuilt-symbol race_session_start_`.
- The behavior check passes.  The harness prints `0` through `9`, `A`, `B`,
  `C`, and `D`: the two error paths return at once, every mode runs the correct
  handler, the default path runs for the mode 3, and the input and table
  branches set the expected words.
  Evidence: `python3 tools/rebuild/harness.py run --module race_session_start`
  and `work/rebuild/rss_out.txt`.

## The frame_step module and the call order

- `frame_step` at `0x2aa84` holds 7 instructions and 20 bytes: the frame `push ebp` and
  `mov ebp,esp`, the call to `default_data_copy` at `0x10193`, the call to `joystick_update` at
  `0x24da0`, the call to `frame_timer` at `0x1fc38`, and the epilogue `pop ebp` and `ret`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2aa84 --count 7`.
- The three calls run in this order: the default data copy, the joystick update, and the frame timer.
  The hand-written row named the callees in address order (`0x10193`, `0x1fc38`, `0x24da0`), so the
  row now names the call order and the rebuild run confirms it.
  Evidence: `python3 tools/rebuild/harness.py run --module frame_step` and
  `work/rebuild/fs_out.txt`.
- The rebuild with `-of+` holds 8 instructions and 22 bytes against the original 7 and 20: the frame,
  the three calls, the epilogue, and one `mov esp,ebp`.  The `-of` build holds 11 instructions, so
  the `as needed` form is not the answer here.
  Evidence: `python3 tools/rebuild/harness.py build --module frame_step` and the two diffs.
- The diff holds 5 of 7 instructions equal and the verdict `the rebuilt module holds one instruction
  the original does not: mov esp, ebp`.  The difference is the same code-generator change as
  `engine_shutdown`, because `frame_step` changes no ESP.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x2aa84
  --rebuilt work/rebuild/fsmod.exe --rebuilt-map work/rebuild/frame_step.map --rebuilt-symbol
  frame_step_`.
- The run passes.  The harness prints `D`, `J`, `F`, and `T`: the copy runs, the joystick update
  runs, the frame timer runs, and the program exits.  The segment guard stays silent.
  Evidence: `python3 tools/rebuild/harness.py run --module frame_step` and `work/rebuild/fs_out.txt`.
- The C build recorded the result: build pass, identity fail, behavior pass, and the status
  `rebuilt`.  The hand-written assembly below replaces the C source in the module recipe and raises
  the status to `verified`.
  Evidence: `python3 tools/rebuild/run.py run --function 0x2aa84`.

## The hand-written frame_step assembly matches the original

- The `frame_step` module now uses the hand-written `tools/rebuild/frame_step.asm` source with the
  `wasm` assembler.  The C source `frame_step.c` stays alongside as the C reference version.  The
  module recipe `tools/rebuild/modules/frame_step.json` lists the assembly source and the unchanged
  `frame_step_main.c` harness.
  Evidence: `tools/rebuild/modules/frame_step.json`.
- The original disassembly is the reference.  The command
  `python3 tools/le.py disasm STARTH.EXE 0x2aa84 --count 7` prints 7 instructions and 20 bytes,
  from `push ebp` to `ret`.  The assembly follows that sequence instruction by instruction.
- The assembler chooses the load encoding for `mov ebp,esp`.  The original uses the store encoding
  (`89 e5`), so the source pins it with `db 89h, 0e5h`.
  Evidence: `tools/rebuild/frame_step.asm`.
- The rebuilt body holds 7 of 7 instructions and 20 bytes.  8 of 20 bytes are equal.  The 12
  differing bytes are the three `call` displacements at offsets 4 through 17.  The three opcode
  bytes stay equal, and the only differing operands are the three direct branch targets, which are
  link-time addresses.
  Evidence: `python3 tools/rebuild/diff_module.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address 0x2aa84
  --count 7 --rebuilt work/rebuild/fsmod.exe --rebuilt-map work/rebuild/frame_step.map
  --rebuilt-symbol frame_step_`.
- The oracle verdict is `exact-relocations`: "the instruction sequences match; the differing
  operands are the link-time addresses".  The strict gate passes.  The behavior check prints the
  four markers `D`, `J`, `F`, and `T`: the copy runs, the joystick update runs, the frame timer
  runs, and the program exits.
  Evidence: `python3 tools/rebuild/run.py run --function 0x2aa84` and
  `work/rebuild/fs_out.txt`.
- The record `0x2aa84` is `verified` with 7 of 7 instructions, 8 of 20 bytes, and a behavior pass.
  The record holds the instruction count 7 and marks the source language with
  `implementation: assembly`.
  Evidence: `python3 tools/rebuild/store.py set --function 0x2aa84 --implementation assembly`.

## The video_fill module and the video buffer pointer

- `video_fill` at `0x2aa98` holds 12 instructions and 30 bytes: the register saves `ebx` and `edx`,
  the frame `push ebp` and `mov ebp,esp`, the size pair `edx=0x190` (400) and `eax=0x1e0` (480), the
  buffer pointer loaded from `DAT_00121528` into `ebx`, the call to the video copy at `0x8beb8`, and
  the epilogue `pop ebp`, `pop edx`, `pop ebx`, and `ret`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2aa98 --count 12`.
- The pointer is the video buffer.  `video_buf_load` at `0x4c8f0` loads the `VIDEO_BUF` resource and
  stores the pointer at `DAT_00121528`, and `frame_update` clears the buffer through the same pointer,
  so `video_fill` passes the screen buffer to the copy.  The raw displacement in the instruction is
  `0x21528`, which the loader relocates to the data object (base `0x100000`) at `0x121528`, the
  address the program map names `DAT_00121528`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4c8f0 --count 12` and the program map.
- The callee at `0x8beb8` copies the buffer to the VGA display at `0xa0000` through the VBE bank
  switch at `0x8c0fc`.  The size pair and the pointer are its only arguments, so the wrapper holds no
  logic of its own.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x8beb8 --count 40`.
- The C rebuild failed the identity check.  The original compiler saves the registers before the
  frame and omits the ESP restore when the frame holds no local variable, while OpenWatcom V2 with
  `-of+` saves the registers after the frame and always emits an ESP restore, so the rebuild held 13
  instructions and 33 bytes against the original 12 and 30 with one extra `lea esp,[ebp-8]`.  The
  extra instruction keeps the instruction multiset unequal, so the diff tool reports the counts
  differ instead of the same instructions in a different order.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x2aa98
  --rebuilt work/rebuild/vfmod.exe --rebuilt-map work/rebuild/video_fill.map --rebuilt-symbol
  video_fill_`.
- The option sweep explains the C codegen.  `-of+ -od` gives the original's save-before-frame order
  but saves the whole register set and adds a local slot, `-of` alone drops the frame, and
  `-of+ -ot`, `-of+ -os`, and `-of+ -ox` change nothing against `-of+` alone.  The container holds
  OpenWatcom V2 only, so no original Watcom 10.x compiler is available, and no C source form removes
  the restore.
  Evidence: the container compiles of `video_fill.c` with the eight option sets and the `wdis`
  dumps of the objects.
- The hand-written assembly build at `tools/rebuild/video_fill.asm` holds the original shape exactly.
  The only differences are the two link-time address fields: the buffer pointer displacement and the
  call displacement.  The diff reports 12 of 12 instructions equal, 0 opcode differences, 2 operand
  differences, and 24 of 30 bytes equal, and the verdict is `the instruction sequences match; the
  differing operands are the link-time addresses`.  The byte directive `db 89h, 0E5h` pins the
  `mov ebp,esp` encoding, because the assembler emits the `8Bh` form of the same instruction.
  Evidence: `tools/rebuild/video_fill.asm` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x2aa98
  --rebuilt work/rebuild/vfmod.exe --rebuilt-map work/rebuild/video_fill.map --rebuilt-symbol
  video_fill_`.
- The assembly build replaced the C build as the active module source.  The C source
  `video_fill.c` stays alongside as the C reference.  The project returns to the original Watcom
  10.x compiler later; that compiler saves the registers before the frame and omits the restore, so a
  Watcom 10.x build of the C source should reproduce the original shape and make the C source the
  exact match.
  Evidence: `tools/rebuild/video_fill.c`, `tools/rebuild/video_fill.asm`, and
  `tools/rebuild/modules/video_fill.json`.
- The run passes.  The harness prints `V` and `D`: the copy stub runs with 480, 400, and
  `DAT_00121528`, and the program exits.  The segment guard stays silent.
  Evidence: `python3 tools/rebuild/harness.py run --module video_fill` and `work/rebuild/vf_out.txt`.
- The store recorded the result of the C build: build pass, identity fail, behavior pass, and the
  status `rebuilt`.  The assembly build replaces the C source in the module recipe and raises the
  status to `verified`.
  Evidence: `python3 tools/rebuild/run.py run --function 0x2aa98`.

## The hand-written video_fill assembly matches the original

- The `video_fill` module now uses the hand-written `tools/rebuild/video_fill.asm` source with the
  `wasm` assembler.  The C source `video_fill.c` stays alongside as the C reference version.  The
  module recipe `tools/rebuild/modules/video_fill.json` lists the assembly source and the unchanged
  `video_fill_main.c` harness.
  Evidence: `tools/rebuild/modules/video_fill.json`.
- The original disassembly is the reference.  The command
  `python3 tools/le.py disasm STARTH.EXE 0x2aa98 --count 12` prints 12 instructions and 30 bytes,
  from `push ebx` to `ret`.  The assembly follows that sequence instruction by instruction.
- The assembler chooses the load encoding for `mov ebp,esp`.  The original uses the store encoding
  (`89 e5`), so the source pins it with `db 89h, 0E5h`.
  Evidence: `tools/rebuild/video_fill.asm`.
- The rebuilt body holds 12 of 12 instructions and 30 bytes.  24 of 30 bytes are equal.  The 6
  differing bytes are the buffer pointer displacement and the call displacement.  The opcode bytes
  stay equal, and the only differing operands are the two direct addresses, which are link-time
  addresses.
  Evidence: `python3 tools/rebuild/diff_module.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address 0x2aa98
  --count 12 --rebuilt work/rebuild/vfmod.exe --rebuilt-map work/rebuild/video_fill.map
  --rebuilt-symbol video_fill_`.
- The oracle verdict is `the instruction sequences match; the differing operands are the link-time
  addresses`.  The strict gate passes.  The behavior check prints the two markers `V` and `D`: the
  copy stub runs and the program exits.
  Evidence: `python3 tools/rebuild/run.py run --function 0x2aa98` and `work/rebuild/vf_out.txt`.
- The record `0x2aa98` is `verified` with 12 of 12 instructions, 24 of 30 bytes, and a behavior pass.
  The record holds the instruction count 12 and marks the source language with
  `implementation: assembly`.
  Evidence: `python3 tools/rebuild/store.py set --function 0x2aa98 --implementation assembly`.

## The hand-written hud_update assembly matches the original

- The `hud_update` module now uses the hand-written `tools/rebuild/hud_update.asm` source with the
  `wasm` assembler.  The C source `hud_update.c` stays alongside as the C reference version.  The
  module recipe `tools/rebuild/modules/hud_update.json` lists the assembly source and the unchanged
  `hud_update_main.c` harness.
  Evidence: `tools/rebuild/modules/hud_update.json`.
- The original disassembly is the reference.  The command
  `python3 tools/le.py disasm STARTH.EXE 0x27a34 --count 11` prints 11 instructions and 38 bytes,
  from `push ebp` to `ret`.  The assembly follows that sequence instruction by instruction, with the
  two local labels `skip_calls` and `done` for the two branches.
- The assembler chooses the load encoding for `mov ebp,esp`.  The original uses the store encoding
  (`89 e5`), so the source pins it with `db 89h, 0e5h`.
  Evidence: `tools/rebuild/hud_update.asm`.
- The rebuilt body holds 11 of 11 instructions and 38 bytes.  27 of 38 bytes are equal.  The seven
  differing operands are the two `DAT_0011ce44` displacements, the two branch targets, and the three
  call targets; 11 of their bytes differ in value.  The opcode bytes stay equal, and the only
  differing operands are the data address and the five direct branch targets, which are link-time
  addresses.
  Evidence: `python3 tools/rebuild/diff_module.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address 0x27a34
  --count 11 --rebuilt work/rebuild/hudmod.exe --rebuilt-map work/rebuild/hud_update.map
  --rebuilt-symbol hud_update_`.
- The oracle verdict is `the instruction sequences match; the differing operands are the link-time
  addresses`.  The strict gate passes.  The behavior check prints the three markers `0`, `2`, and
  `D`: the state 4 runs the first two overlays, the state 3 runs only the third overlay, and the
  program exits.
  Evidence: `python3 tools/rebuild/run.py run --function 0x27a34` and
  `work/rebuild/hud_out.txt`.
- The record `0x27a34` is `verified` with 11 of 11 instructions, 27 of 38 bytes, and a behavior pass.
  The record holds the instruction count 11 and marks the source language with
  `implementation: assembly`.
  Evidence: `python3 tools/rebuild/store.py set --function 0x27a34 --implementation assembly`.

## The hand-written music_bank_reset assembly matches the original

- The `music_bank_reset` module now uses the hand-written
  `tools/rebuild/music_bank_reset.asm` source with the `wasm` assembler.  The C
  source `music_bank_reset.c` stays alongside as the C reference version.  The
  module recipe `tools/rebuild/modules/music_bank_reset.json` lists the assembly
  source and the unchanged `music_bank_reset_main.c` harness.
  Evidence: `tools/rebuild/modules/music_bank_reset.json`.
- The original disassembly is the reference.  The command
  `python3 tools/le.py disasm STARTH.EXE 0x2f9b0 --count 14` prints 14
  instructions and 46 bytes, from `push ebp` to `ret`.  The assembly follows
  that sequence instruction by instruction.  The parameter arrives in `EAX`
  and the function keeps it in the local slot `[ebp-4]`.
- The assembler can choose another encoding for `mov ebp,esp` and
  `mov esp,ebp`.  The original uses the store encodings (`89 e5` and `89 ec`),
  so the source pins them with `db 89h, 0e5h` and `db 89h, 0ech`.
  Evidence: `tools/rebuild/music_bank_reset.asm`.
- The rebuilt body holds 14 of 14 instructions and 46 bytes.  All 46 bytes are
  equal.  The oracle verdict is `the modules are byte-identical`.  The strict
  gate passes.
  Evidence: `python3 tools/rebuild/diff_module.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address
  0x2f9b0 --count 14 --rebuilt work/rebuild/mbrmod.exe --rebuilt-map
  work/rebuild/music_bank_reset.map --rebuilt-symbol music_bank_reset_`.
- The behavior check prints the two markers `V` and `D`: the three fields hold
  the reset values and the program exits.
  Evidence: `python3 tools/rebuild/run.py run --function 0x2f9b0` and
  `work/rebuild/mbr_out.txt`.
- The record `0x2f9b0` is `verified` with 14 of 14 instructions, 46 of 46 bytes,
  and a behavior pass.  The record holds the instruction count 14 and marks the
  source language with `implementation: assembly`.
  Evidence: `python3 tools/rebuild/store.py set --function 0x2f9b0
  --implementation assembly`.

## The hand-written stream_status assembly matches the original

- The `stream_status` module now uses the hand-written
  `tools/rebuild/stream_status.asm` source with the `wasm` assembler.  The C
  source `stream_status.c` stays alongside as the C reference version.  The
  module recipe `tools/rebuild/modules/stream_status.json` lists the assembly
  source and the unchanged `stream_status_main.c` harness.
  Evidence: `tools/rebuild/modules/stream_status.json`.
- The original disassembly is the reference.  The command
  `python3 tools/le.py disasm STARTH.EXE 0x4c7e0 --count 21` prints 21
  instructions and 38 bytes, from `push ebx` to `ret`.  The assembly follows
  that sequence instruction by instruction.  The parameter arrives in `EAX`
  and the function keeps it in the local slot `[ebp-4]`.
- The assembler can choose another encoding for `mov ebp,esp`, `mov esp,ebp`,
  and `sub esp,8`.  The original uses the store encodings (`89 e5` and `89 ec`)
  and the 32-bit immediate form `81 ec 08 00 00 00`, so the source pins them
  with `db` directives.
  Evidence: `tools/rebuild/stream_status.asm`.
- The rebuilt body holds 21 of 21 instructions and 38 bytes.  All 38 bytes are
  equal.  The oracle verdict is `the modules are byte-identical`.  The strict
  gate passes.
  Evidence: `python3 tools/rebuild/diff_module.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address
  0x4c7e0 --count 21 --rebuilt work/rebuild/stsmod.exe --rebuilt-map
  work/rebuild/stream_status.map --rebuilt-symbol stream_status_`.
- The behavior check prints the two markers `V` and `D`: the module returns the
  status word and the program exits.
  Evidence: `python3 tools/rebuild/run.py run --function 0x4c7e0` and
  `work/rebuild/sts_out.txt`.
- The record `0x4c7e0` is `verified` with 21 of 21 instructions, 38 of 38 bytes,
  and a behavior pass.  The record holds the instruction count 21 and marks the
  source language with `implementation: assembly`.
  Evidence: `python3 tools/rebuild/store.py set --function 0x4c7e0
  --implementation assembly`.

## The hand-written engine_shutdown assembly matches the original

- The `engine_shutdown` module now uses the hand-written
  `tools/rebuild/shutdown.asm` source with the `wasm` assembler.  The C source
  `shutdown.c` stays alongside as the C reference version.  The module recipe
  `tools/rebuild/modules/shutdown.json` lists the assembly source and the
  unchanged `shutdown_main.c` harness.
  Evidence: `tools/rebuild/modules/shutdown.json`.
- The original disassembly is the reference.  The command
  `python3 tools/le.py disasm STARTH.EXE 0x128ac --count 14` prints 14
  instructions and 52 bytes, from `push ebp` to `ret`.  The assembly follows
  that sequence instruction by instruction.
- The assembler can choose another encoding for `mov ebp,esp` and
  `xor eax,eax`.  The original uses the store encodings (`89 e5` and `31 c0`),
  so the source pins them with `db` directives.
  Evidence: `tools/rebuild/shutdown.asm`.
- The original `mov eax,0x62b0` carries a 4-byte internal fixup to object 3 at
  offset `0x62b0`.  The assembly takes the address of the harness symbol
  `DAT_000062b0`, so the rebuilt instruction carries the same fixup.  The
  harness holds that data symbol and the eight markers `f1` to `f8`.
  Evidence: `tools/rebuild/shutdown_main.c`.
- The rebuilt body holds 14 of 14 instructions and 52 bytes.  The oracle masks
  the link-time operands and the verdict is `the instruction sequences match;
  the differing operands are the link-time addresses`.  The strict gate passes.
  Evidence: `python3 tools/rebuild/diff_module.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address
  0x128ac --count 14 --rebuilt work/rebuild/shutmod.exe --rebuilt-map
  work/rebuild/shutdown.map --rebuilt-symbol engine_shutdown_`.
- The behavior check prints the eight markers `1` to `8` in order: the module
  calls the eight callees and the program exits.
  Evidence: `python3 tools/rebuild/run.py run --function 0x128ac` and
  `work/rebuild/shut_out.txt`.
- The record `0x128ac` is `verified` with 14 of 14 instructions, 52 of 52
  bytes, and a behavior pass.  The record holds the instruction count 14 and
  marks the source language with `implementation: assembly`.
  Evidence: the store API sets the count 14 and the assembly implementation for
  the record `0x128ac`.

## The rebuild framework

- The rebuild framework holds one record per function and one record per build
  unit.  `store.py` is the record store and the validation, `harness.py` is the
  pinned container and DOSBox-X harness, `run.py` is the runner, `view.py`
  renders the self-contained status page, and `docs.py` renders the
  `program-map.md` tables.  The function records live at
  `tools/rebuild/functions/0x<address>.json`, the ten module recipes at
  `tools/rebuild/modules/*.json`, and the record contracts at
  `tools/rebuild/contracts/*.schema.json`.
  Evidence: `tools/rebuild/store.py`, `harness.py`, `run.py`, `view.py`, and
  `docs.py`.
- The seed added 644 records and lost none: 643 records in its first run and one
  more, the exit helper at `0x300d6`, in a correction run.
  Evidence: `python3 tools/rebuild/store.py seed`.
- The store holds 757 function records and 10 module records, and
  `store.py check` reports `767 records, 0 violations`.
  Evidence: `python3 tools/rebuild/store.py check`.
- The import of the hand-written program map reproduces the tables: the render
  writes them between 21 marker pairs, and `docs.py check` reports no drift.
  The import ran once with the one-time importer; the migration is complete and
  the tool is removed.
  Evidence: `python3 tools/rebuild/docs.py render` and
  `python3 tools/rebuild/docs.py check`.
- The framework replaces the twenty shell scripts and the module registry: the
  ten module recipes and `harness.py` build and run each module, and the store
  is the only registry.  The twenty shell scripts are removed.
  Evidence: `tools/rebuild/modules/` and `python3 tools/rebuild/harness.py
  build --module free`.
- The generated tables differ from the hand-written ones in four ways: the 21
  marker pairs; the rows of every function table reordered to address ascending;
  11 duplicate rows removed from a later table when an earlier table names the
  same address; and 3 rows corrected from the disassembly of `0x29d08` and
  `0x4c810`: the name and the evidence cell of `0x29d08` become
  `menu_space_init`, the evidence cell of `0x28ba4` follows the new name, and
  the name of `0x4c810` becomes `stream_layer_init`.
  Evidence: the render diff and
  `python3 tools/le.py disasm STARTH.EXE 0x29d08`.
- The runner builds and runs the declared checks through the harness, and the
  store records the result of each one.
  Evidence: `python3 tools/rebuild/harness.py build --module free` and
  `python3 tools/rebuild/run.py run --all --no-write`.
- The runner writes no result when a declared check is a skip, so a stored result
  never holds a `skip` verdict.  A missing original image gives a clear error and
  exit 2.  The DOSBox-X conf quotes the mount path, so a checkout path with a
  space still mounts.
  Evidence: `python3 -m unittest discover -s tools/rebuild/tests`,
  `python3 tools/rebuild/run.py run --function 0x300ec --skip-runtime`, and
  `python3 tools/rebuild/run.py run --function 0x300ec --no-write --original
  /nonexistent/STARTH.EXE`.
- The status page renders the store.
  Evidence: `python3 tools/rebuild/view.py html`.
- The pipeline check reports 152 checks and 0 failures and takes about 30
  minutes.  Inside the Nix dev shell the radare2 stage adds four checks, so the
  run reports 156 checks.  With `--skip-runtime` it reports 137 checks and takes
  about a minute.  The earlier sections in this file record the check
  count at the time they were written, not the current count.
  Evidence: `python3 tools/pipeline_check.py --game-dir
  ~/Documents/Screamer.app/Contents/Resources/game`.

## The status page in the hooks and CI

- The pre-commit hook regenerates `work/rebuild/status.html`, so the local page
  is current at every commit.  The page is generated output and stays out of git.
  Evidence: `.githooks/pre-commit` and `python3 tools/rebuild/view.py html`.
- The CI `status` job regenerates the page on every push and pull request and
  publishes it as the `rebuild-status` artifact.  The job needs the standard
  library only, because the page reads the committed store.
  Evidence: `.github/workflows/pipeline.yml` and the `rebuild-status` artifact.
- On a push to `master`, the CI `status` job renders the page to
  `_site/index.html` and packages the site for GitHub Pages.  The `deploy` job
  runs only after the `syntax` job passes and publishes the site with
  `actions/deploy-pages`.  A pull request uploads the artifact but does not
  deploy.  The page is <https://i-zhirov.github.io/screamer_re/>.
  Evidence: `.github/workflows/pipeline.yml` and
  `python3 tools/rebuild/view.py html --out _site/index.html`.
- The page has no external asset, no `<base>` element, and no absolute path, so
  the `/screamer_re/` project subpath needs no change.
  Evidence: `python3 tools/rebuild/view.py html --out _site/index.html` and a
  search of the output for `<link`, `<img`, `src=`, and `href=`.
- GitHub Free publishes Pages only for a public repository.  The repository is
  public, the Pages source is `GitHub Actions`, and the `github-pages`
  environment allows a deployment from `master`.
  Evidence: the repository settings and the `deploy` job in
  `.github/workflows/pipeline.yml`.
- The hook skips with a clear line when the view tool or `python3` is missing,
  and it fails the commit when the page cannot render.
  Evidence: `sh .githooks/pre-commit`.
- The pre-push hook is gone.  It ran the full check in the Nix dev shell before
  every push, and the SSH connection to the remote idled for the whole run, so
  the remote closed it and the push failed.  The syntax gate, the unit suite, and
  the full check run in CI instead, and the pre-commit hook still regenerates the
  status page.
  Evidence: the failed pushes and `.github/workflows/pipeline.yml`.

## The period Watcom 10.0a toolchain

- The original build uses the Watcom C/C++ 10.0a toolchain.  The run-time banner of the game,
  `WATCOM C/C++32 Run-Time system. (c) Copyright by WATCOM International Corp. 1988-1994.`,
  is byte-identical to the string in the 10.0a `LIB386/DOS/CLIB3R.LIB`.  The 10.5 run-time banner
  ends in `1988-1995`, so 10.5 is excluded; the initial 10.0 release stays unverified (see the open
  question below).
  Evidence: `grep -a 'WATCOM International Corp. 1988-1994' work/watcom10/WATCOM/LIB386/DOS/CLIB3R.LIB`
  and the strings of `LIB386/DOS/CLIB3R.LIB` from the 10.5 image (Internet Archive item
  `Watcom_C_10.5`, image `Watcom_C++_10.5.iso`, SHA-256
  `e7d8275b71f05c782b69918365c770716bde8f56a08fcdc62fe23dc43d808b9b`).
- The linked stub is the 10.0a stub.  The first 10832 bytes of `STARTH.EXE` are byte-identical to the
  10.0a `WSTUB.EXE`, and the stub carries the marker `WATCOM patch level .a`, so the linker is the
  10.0a linker.
  Evidence: the byte compare of the two stubs and `strings work/watcom10/WATCOM/BINB/WSTUB.EXE`.
- The period toolchain is now set up.  The tool `tools/rebuild/watcom10_setup.py` downloads the
  Watcom C/C++ 10.0a CD-ROM image from the Internet Archive item `watcomcc10.0cd-rom`, verifies the
  SHA-256 `f51ed9358e7eaf6a0d292268bbbc15832530489dd8239a98f6938c244f1736fc`, and extracts the
  `WATCOM` tree to `work/watcom10/WATCOM`.  The DOS tools run under DOSBox-X, because they are DOS
  programs.  The toolchain runs alongside the pinned OpenWatcom V2 container; it does not replace it.
  Evidence: `python3 tools/rebuild/watcom10_setup.py` and the extracted tree.
- The build harness `tools/rebuild/harness_dos.py` compiles and links one module with the period
  toolchain.  The link uses the standard run-time library (the default libs), because that flow
  produces a runnable DOS/4G executable; the module symbol comes from the linker map.
  Evidence: `python3 tools/rebuild/harness_dos.py build --module free`.
- The period `free` matches as the V2 build does: 3 of 3 instructions, and the only differing
  operand is the link-time address of the tail jump.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x300ec
  --rebuilt work/rebuild/free.exe --rebuilt-map work/rebuild/free.map --rebuilt-symbol free_`.
- The period compiler places and relocates the jump table of `error_report`.  The rebuilt code holds
  `jmp dword ptr cs:[eax*4+0x10]` (the same shape as the original `jmp dword ptr cs:[eax*4+0x918]`),
  the linker relocates the table address, and the program prints all four markers.  The V2 build
  emits the same reference without the table and crashes.  The diff still differs, because the
  reconstructed source holds a switch of 0x13 cases while the original holds one of 0x17.
  Evidence: `python3 tools/rebuild/harness_dos.py build --module error_report`,
  `python3 tools/rebuild/harness.py run --module error_report`, and the two diffs.
- The period `-of+ -od` build of `list_destroy` is much closer than the V2 build: 33 of 54 bytes
  equal (the V2 build: 15 of 54) and 22 of 25 instructions equal.  The register-save count now
  matches the original: five registers including `ebp`.  Three differences stay: the prologue order,
  the local offsets, and the epilogue (`lea esp,[ebp-0x10]` and the pop order against `mov esp,ebp`
  and the original pop order).
  Evidence: `python3 tools/rebuild/harness_dos.py build --module list_destroy` and the diff.
- The period `-of+` build of `frame_step` still holds the extra `mov esp,ebp`.  The 10.0a compiler
  emits it exactly as the V2 compiler does, and the original omits it, so the 10.0a build is not
  byte-perfect for this case either.
  Evidence: `python3 tools/rebuild/harness_dos.py build --module frame_step` and the diff.
- The initial 10.0 compiler is verified and runs.  The earlier `Loader read error` was not a
  property of the compiler: the WPACK unpack writes the output file progressively, and a harness
  that stops DOSBox-X as soon as the file appears leaves a truncated image (65 KB to 81 KB of
  517764 bytes), which the loader rejects.  When the unpack runs to completion, the full
  `WCC386.EXE` loads and prints `WATCOM C32 Optimizing Compiler  Version 10.0 Limited
  Availability`.  The full toolchain builds and runs a module: the period `free` prints all four
  markers and the diff holds 3 of 3 instructions.
  Evidence: the unpack to completion, `WATCOM C32 Optimizing Compiler  Version 10.0 Limited
  Availability`, the `free` run, and the diff.
- The registers-before-frame prologue comes from the `-od` flag without `-of+`, not from a
  different compiler.  A sweep of 35 option combinations on the 10.0a compiler, four calling
  conventions, and five `#pragma aux` variants kept the frame-first prologue only because each
  combination also passed `-of+`.  Plain `-od` gives the original prologue
  `push ebx; push ecx; push esi; push edi; push ebp; mov ebp,esp; sub esp,0xc` and the original
  epilogue `mov esp,ebp; pop ebp; pop edi; pop esi; pop ecx; pop ebx; ret`.  The module recipe
  passes `-of+ -od`; the original used `-od` alone.
  Evidence: `python3 tools/rebuild/harness_dos.py build --module list_destroy` with the recipe
  flags changed to `-od` gives 25 of 25 instructions equal and 44 of 54 bytes equal (the recipe
  with `-of+ -od` gives 22 of 25 and 33 of 54).  The only remaining difference is the local slot
  order: the original stores `list` at `[ebp-8]`, `unused` at `[ebp-4]`, and the temporary at
  `[ebp-0xc]`, while the 10.0a build stores them at `[ebp-0xc]`, `[ebp-8]`, and `[ebp-4]`.
  The original text uses this form widely: 205 prologues of the form
  `push ebx; push ecx; [push edx]; push esi; push edi; push ebp; mov ebp,esp` (for example at
  0x11c00 and 0x1821c), and 116 frame functions without locals end in `pop ebp; ret`.
  Evidence: the byte census of `STARTH.EXE` object 1 and the disassembly at 0x11c00, 0x1821c,
  0x2aa84, and 0x2ed77.
- The same `-od` prologue appears in every tested pre-10.0 and 10.x compiler, so the layout does
  not separate the versions.  The 9.5 compiler (extracted non-interactively with
  `Z:\AUTOTYPE.COM` and the installer `unpack` command; `WCC386.EXE` 627702 bytes, banner
  `WATCOM C32 Optimizing Compiler  Version 9.5`), the 9.01d compiler (banner `WATCOM C 386
  Optimizing Compiler  Version 9.01`), and the 8.5a compiler (banner `WATCOM C 386 Optimizing
  Compiler  Version 8.5a`) all produce the registers-first prologue with `-od` and the frame-first
  prologue with `-of+`.  The 8.5a `-od` build is frameless.
  Evidence: the 9.5, 9.01d, and 8.5a builds of `list_destroy.c` and the version banners.
- The `-of+` epilogue stays conservative in every tested compiler: 8.5a, 9.01d, 9.5, 10.0 Limited
  Availability, 10.0a, 10.5, 10.6, and 11.0b all emit `mov esp,ebp; pop ebp; ret` for
  `frame_step`.  The original emits `pop ebp; ret`.  The 9.5 `-of+` build goes further and adds
  `lea esp,[ebp-0x14]` with five register pops.  So `frame_step` is not explained by the compiler
  version, the flags, or the pragmas; a different optimization setting or a hand-written routine
  remains the explanation.
  Evidence: the 9.5, 9.01d, 8.5a, and 10.0a `-of+` builds of `frame_step.c`.
- The module record holds a per-harness flag set: `period_flags` replaces `flags` in
  `harness_dos.py`, and the V2 harness keeps `flags`.  No module needs a split, so every record
  uses one flag set and no record holds `period_flags`.  The `list_destroy` and
  `memory_available` records use `-od` for both harnesses.  Both period builds match the
  original instruction sequence: 25 of 25 and 27 of 27 instructions.  The V2 declared checks now
  hold `list_destroy` 19 of 25 and `memory_available` 27 of 27 (status `verified`).  These
  numbers correct an earlier note: 22 of 25 was the period build of `list_destroy`, not the V2
  build.
  Evidence: `python3 tools/rebuild/harness_dos.py build --module list_destroy` and
  `memory_available`, the diffs, `python3 tools/rebuild/harness.py build --module ...`, and
  `python3 tools/rebuild/run.py run --module ...`.
- The period build of `shutdown` did not link, and the cause was not a missing stub.  The DOS
  tools truncate a long file name to eight characters, so `shutdown.c` and `shutdown_main.c` both
  become `SHUTDOWN.C`, and the compiler reads `shutdown.c` twice.  The same collision hits
  `bios_video`, `error_report`, `frame_step`, `list_destroy`, and `memory_available`.  The
  `harness_dos.py` copy step now gives each source an indexed 8.3 name (`S0.C`, `S1.C`, ...)
  before the compile.  The `shutdown` period build links, runs with markers 1 to 8, and diffs
  12 of 14 instructions; the other period builds keep their diffs.  The `bios_video` and
  `error_report` period builds also link, hold `main_` in the map, and print their markers
  (`F C D` and `R R R D`).
  Evidence: `python3 tools/rebuild/harness_dos.py build --module shutdown`, `bios_video`, and
  `error_report`, the clean build logs, the maps, and the run output.
- To reproduce the original frame, use `-od` without `-of+` for the prologue and the
  `list_destroy` epilogue: the registers-before-frame layout follows from the flag, not from the
  version.  The open problem is the `frame_step` epilogue.  (1) Build `list_destroy` with `-od`
  and diff it; the instructions must match 25 of 25, and the local slot order may stay the only
  difference.  (2) For `frame_step`, the `-of+` epilogue is conservative in every tested compiler
  (8.5a to 11.0b), so the original's `pop ebp; ret` needs a setting, a source form, or a
  hand-written routine that is not yet found.  (3) Census the original text (object 1) for the
  prologue and epilogue forms after every change; one build must produce the same mix.  (4) Accept
  a compiler only with its binary.  The run-time banner (`1988-1994`) and the `WSTUB.EXE` stub
  (`patch level .a`) prove the link-time artifacts, not the code generator.

## The error_report switch range and the missing case

- The original switch holds the codes -1 through 22, not 0 through 19.  The dispatch at
  `0x10978` is `mov eax,[ebp-4]; inc eax; cmp eax,0x17; ja 0x10b87; jmp dword ptr
  cs:[eax*4+0x918]`, and a controlled 10.0a build shows the bias: a source with the cases
  0 through 23 emits `cmp eax,0x17` and no `inc`, and a source with the cases -1 through 22
  emits `inc eax` and `cmp eax,0x17`.  The original holds the `inc`, so its table holds 24
  entries and covers 24 codes.
  Evidence: `work/opt/T24.C` and `work/opt/TM1.C`, built with the 10.0a `wcc386` and linked
  with `wlink system dos4g`; `python3 tools/le.py disasm work/opt/T24.EXE 0x10070 --count 5`
  holds `cmp eax,0x17` and `python3 tools/le.py disasm work/opt/TM1.EXE 0x10070 --count 5`
  holds `inc eax; cmp eax,0x17`.
- The 24 table entries map the code c to the entry c+1: the entries 0 through 19 hold the
  message offsets `0x44` through `0x35a` for the codes -1 through 18, the entries 20 through
  22 hold the default body `0x10b87` for the codes 19, 20, and 21, and the entry 23 holds
  `0x10b71` for the code 22.  The code 0 therefore prints the offset `0x6f`, not `0x44`.
  Evidence: the jump table at the linear `0x10918` and the body dumps from `0x10996` to
  `0x10b87`.
- The reconstruction was shifted by one and lacked the code 22.  The source now holds `case
  -1` at `0x44`, the cases 0 through 18 at `0x6f` through `0x35a`, `case 22` at `0x369`, and
  the default at `0x383`; the codes 19, 20, and 21 fall to the default.  Two callers in the
  game pass the constant 22: `mov eax,0x16; call 0x10978` at `0x4cfbf` and `0x4d05b`.
  Evidence: `tools/rebuild/error_report.c`, the nine call sites to `0x10978`, and the body
  at `0x10b71` (`push 0x369; push 0x2fd2; call 0x4bf64`).
- The module compiles with `-of+` and the corrected range.  The original keeps an EBP frame
  and stores the code at `[ebp-4]`, and the `-of+` period build reproduces the frame and the
  dispatch (`inc eax; cmp eax,0x17`, 24 entries).  The period diff holds 10 of 16
  instructions, up from 1 of 14.
  Evidence: `python3 tools/rebuild/harness_dos.py build --module error_report` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x10978
  --rebuilt work/rebuild/errmod.exe --rebuilt-map work/rebuild/error_report.map
  --rebuilt-symbol error_report_`.
- Two differences stay.  The rebuild saves `ebx`, `ecx`, and `edx` around the printer call
  while the original saves none, so the original's printer declaration tells the compiler
  that the callee preserves those registers.  The rebuild also shares the body tails through
  jumps and one epilogue while the original expands every body: each of the 21 bodies holds
  its own `push`s, its own `call 0x4bf64`, and its own `mov esp,ebp; pop ebp; ret`.
  Evidence: the two disassemblies and the diff.
- The V2 declared check holds 3 of 16 instructions, and the behavior check still fails.  The
  harness prints a screen of `U` bytes and writes an empty `err_out.txt`, a fault of the
  harness print stub that already shows in the earlier record.
  Evidence: `python3 tools/rebuild/run.py run --function 0x10978` and the run log.

## The error_report register saves, the body expansion, and the frame local

- The printer declaration carries an auxiliary pragma, so the build saves no register
  around the call.  A plain `extern void __cdecl report(char *table, ...)` made the
  compiler save `ebx`, `ecx`, and `edx` for the whole function.  The pragma
  `#pragma aux report "_*" modify exact [eax];` reports that only `eax` changes, and the
  three saves and their epilogue pops disappear.  The name string `"_*"` is required: a
  pragma without it maps the symbol to `report_` instead of `_report`, and the link fails.
  Evidence: `work/opt/V1.C` (no pragma, `push ebx; push ecx; push edx`) against
  `work/opt/V8.C` and `V9.C` (`modify exact [eax ecx edx]`, one save fewer), and
  `work/opt/VEAX.C` and `VEMPTY.C` (`modify exact [eax]` and `[]`, no save), built with the
  10.0a compiler.
- The module builds with `-of+ -ot`.  The default optimization and `-os` merge the common
  call tails: after the first body, the remaining bodies push their constants and jump into
  the first body's `push` and `call`.  `-ot` keeps one full body per code, as the original
  does.  `-oc` alone expands only the first body, and `-od` recovers no jump table.
  Evidence: `work/opt/E1.C` (`-of+ -ot`, every body full) against `E2.C` (`-os`) and `E3.C`
  (`-ou`), which keep the jumps.
- The `code` parameter is volatile.  The original stores the argument to the frame and
  reloads it before the switch (`sub esp,4`, `mov [ebp-4],eax`, `mov eax,[ebp-4]`).  A plain
  parameter stays in `eax`, and neither `-of+`, `-ot`, `-d1`, nor `-d1+` produces the store.
  A `volatile int` parameter reproduces the store and the reload exactly.  The same
  frame-local pattern appears 32 times in the original text, so it is a code-generator trait
  of the original source form.
  Evidence: `work/opt/S2.C` against `S1.C` (a plain local copy) and the parameter census
  of `STARTH.EXE` object 1 for the byte sequence `83 ec 04 89 45 fc 8b 45 fc`.
- The period diff now holds 16 of 16 instructions, up from 10 of 16.  The four remaining
  operand differences are the link-time addresses: the default target, the jump-table
  address, the message-table base, and the printer address.  One byte-level difference
  stays: the original encodes a small `push` as `68 44 00 00 00` and the 10.0a build as
  `6a 44`; `-or`, `-oa`, `-os`, and `-ot` do not change it.
  Evidence: `python3 tools/rebuild/harness_dos.py build --module error_report` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x10978
  --rebuilt work/rebuild/errmod.exe --rebuilt-map work/rebuild/error_report.map
  --rebuilt-symbol error_report_`.

## The short push encoding and the format-string arguments

- The original passes format-string addresses, not integer offsets.  The offsets `0x44`, `0x6f`,
  `0x74`, `0xa3`, `0x369`, and `0x383` are data-segment addresses: `0x44` holds "Cannot free
  memory of a duplicated object!", `0x6f` holds "OK.\n", `0x74` holds "Error while searching for
  object file: %s.3dx\n", `0x369` holds "General failure: aborted.", and `0x383` holds
  "Unknown error in engine interface <%s>.\n".  The printer at `0x4bf64` loads the first argument
  into `eax` and the second into `edx` and calls `0xd429f`, which reads `[eax+0xc]` as a stream
  status byte, so the call is a `fprintf`-style call with a stream and a format string.
  Evidence: the object 3 data dump at the linear `0x100044`, the disassembly of `0x4bf64` and
  `0xd429f`, and the pushed values.
- The 10.0a compiler shortens an integer constant that fits in a signed byte to `push imm8`
  (`6A ib`), and no flag disables it.  `-ot`, `-os`, `-or`, `-oa`, `-oc`, `-od`, `-d1`, and
  `-d1+` all keep `6a 44`, and the 10.0 Limited Availability compiler does the same.
  Evidence: `work/opt/TY.C` (eleven argument types and casts, all `6a 44`), `H1.C`, `H2.C`, and
  `H3.C` (the optimization options), `F1.C` and `F2.C` (the debug options), and `G1.C` (the LA
  build).
- A relocatable operand restores the long encoding.  A string literal or a symbol address is
  unknown at compile time, so the compiler emits `push imm32` (`68 id`) with a relocation even
  when the value is small.  One test file holds `report(stream, "Cannot free memory ...")` as
  `68 04 00 00 00` (the literal at offset 4), `report(stream, "OK.\n")` as `68 30 00 00 00`, and
  `report(stream, 0x44)` as `6a 44`.
  Evidence: `work/opt/SL.C` and `python3 tools/le.py disasm work/opt/SL.EXE 0x10030 --count 8`.
- The module passes the game's format strings, so the period build emits the long encoding and
  the diff holds 16 of 16 instructions with the same 52-byte length.  The byte compare rises from
  29 of 49 to 44 of 52.  The eight differing bytes are the link-time addresses: the jump-table
  address, the format-string address, the stream address, the printer address, and the `ja`
  target.  The harness keeps the `message_table` stub as the first argument; the stub stands in
  for the stream, and the printer stub ignores it.
  Evidence: `python3 tools/rebuild/harness_dos.py build --module error_report` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE --original-address 0x10978
  --rebuilt work/rebuild/errmod.exe --rebuilt-map work/rebuild/error_report.map
  --rebuilt-symbol error_report_`.

## The format-string audit of the other modules

- Only `error_report` pushes immediate arguments.  The other nine modules push registers only
  (the prologue saves) or no registers at all: `free` and `memcpy` push nothing, `exit` and
  `printf` push `edx` or `ebx`, and `shutdown`, `bios_video`, `list_destroy`,
  `memory_available`, and `frame_step` push their saved registers.  Their calls pass the
  arguments in `eax`, `edx`, `ebx`, and `ecx` under the register convention, so no stack push
  exists, and the short-push form cannot appear.
  Evidence: the decode of the original functions at `0x300ec`, `0x300be`, `0x300d6`, `0x3009b`,
  `0x128ac`, `0x108e4`, `0x2ed77`, `0x2f8ca`, `0x2aa84`, and `0x8c150`.
- The remaining constant arguments are register arguments: `engine_shutdown` loads `0` with
  `xor eax,eax` and `0x62b0` with `mov eax,0x62b0`, and `bios_video` loads the interrupt number
  `0x10` with `mov eax,0x10`.  The callee of the `0x62b0` call at `0x2fc90` reads `[eax+0x14]`,
  so the value is a pointer, but it travels in `eax` as `mov eax,imm32`, which is the same
  encoding for an integer and a relocatable address, so the byte compare cannot separate them.
  Evidence: the two disassemblies.
- The game holds twelve further print sites, in two functions that start at `0x2edad` and
  `0x4bf88` (the register-save prologue begins before the `push ebp` that the prologue scan
  matches).  Each site already uses the relocatable form `mov eax,<address>; push eax` for the
  format string and the stream `0x2fd2`, so neither needs the fix.  Both functions now hold a
  named record: `memory_error_report` at `0x2edad` prints the memory-handler errors and reads
  the status at `+0xc`, and `file_error_report` at `0x4bf88` prints the engine file errors and
  reads the status at `+0xac`.  Every print site of the printer `0x4bf64` is covered by a
  record.
  Evidence: the caller scan of the printer `0x4bf64` (35 calls, 21 inside `error_report`), the
  disassembly at `0x2eddf` and `0x4bfbd`, and the rows of `docs/reference/program-map.md`.
- Result: no module besides `error_report` pushes a small integer constant, and no module still
  pushes one where a relocatable pointer is expected.

## The prologue scan and the mid-prologue records

- The prologue scan matched `push ebp; mov ebp,esp`, so it reported the `push ebp` inside the prologue of a function that saves registers first.  Of the 758 records, 545 (all unnamed candidates) sat one to five bytes after the true entry, and their `size` measured the span to the next `push ebp` instead of to the next function.
  Evidence: the byte scan of the original image at each record address and the `funcs` list.
- The scan now walks back over the canonical register saves (`ebx`, `ecx`, `edx`, `esi`, `edi`) before the `push ebp`, so it reports the first register save as the function entry.  The `near` command uses the same walk.
  Evidence: `python3 tools/le.py funcs STARTH.EXE` lists `0x2ed77`, `0x2edad`, `0x2f8ca`, and `0x4bf88` and no longer lists `0x2ed7b`, `0x2edb2`, `0x2f8cf`, or `0x4bf8d`.
- The seed deletes the lost unknown records, so a real run migrates the store.  The run added 459 records at the true entries and removed the 545 mid-prologue records; 86 of the mid-prologue records shared an entry with an existing record and only lost their duplicate.  The store holds 683 records, no record sits mid-prologue, and the seed reports no difference.
  Evidence: `python3 tools/rebuild/store.py seed --original STARTH.EXE` (added 459, lost 545), `store.py check` (683 records, 0 violations), and the record scan against the original bytes (0 mid-prologue).
- The invariant is now guarded.  `store.py check --original` reports every record that is not a function entry: one that sits inside a prologue, in a run of alignment padding, or inside a jump or call instruction such as a thunk.  A relative jump counts only when its target is mapped, so the data tables that hold the same bytes before a real entry do not raise a false alarm.  The pipeline stage 12 passes the image to the check.  The unit suite adds a pure test of the walk-back, a test of each form against a synthetic image, and a store test that skips when the image is absent.
  Evidence: `python3 tools/rebuild/store.py check --original STARTH.EXE` (683 records, 0 violations); a temporary record at `0x4bf85` reports `sits in alignment padding` and one at `0x300f9` reports `sits inside a jump`; `python3 -m unittest discover -s tools/rebuild/tests` (108 tests).

## The stale function extents

- 59 of the 672 records hold a `size` that reaches past the next entry.  55 reach one to five bytes past it, the count of the register saves that the old next candidate sat after: the entry was a mid-prologue address, so the span recorded at seed time ends a few bytes inside the next function.  The other four are `exit` (`0x300be`, 22 bytes past `0x300d6`, the helper entry added later), `music_start` (`0x2ffc4`, 168 bytes past `0x30050`), the `entry jump` (`0x300f8`, the whole start-up block), and an unnamed record at `0x4da3a` (263734 bytes past the `0x81a70` entry added later).
- The seed writes `size` only for a new record, as the distance to the next entry of the moment.  The mid-prologue migration moved the entries and later candidates were added without a recompute, so the surviving records kept a stale reach.
  Evidence: the scan of `tools/rebuild/functions/*.json` against the sorted addresses; `store.py seed` does not touch a record that already exists.
- The store now recomputes the field and guards it.  `store.py size` rewrites `size` for every record as the distance to the next entry and removes it from the last record; the run updated 604 records.  `store.py check` reports a record whose size reaches past the next entry, and the pipeline stage 12 runs the check with the image.  The check reported 59 overlaps before the pass and 0 after it.
  Evidence: `python3 tools/rebuild/store.py check` (683 records, 59 violations before the pass, 0 after), `store.py size` (updated 604 records), and the `ExtentTestCase` tests.
- The seed path holds the same rule.  A seed run recomputes every size after it adds the records, so a new entry shifts the previous record's reach instead of leaving an overlap behind.  The check mode prints `overlaps: N`, lists the offending records, and exits 1.
  Evidence: a temporary `size` of 9999 on `0x10978` makes `store.py seed --check` print `overlaps: 1` and `the size reaches 9443 bytes past the next entry`; the unit suite adds `test_seed_check_reports_an_overlap` and `test_seed_rewrites_the_sizes_after_an_addition`.
- The unit suite replays the migration end to end.  `test_mid_prologue_migration_leaves_no_overlap` builds a store whose records sit at the `push ebp` inside the prologue, feeds the fixed scan the true entries, runs the seed, and asserts that the mid-prologue records are gone, that the new entries hold the recomputed extents, and that no record overlaps the next entry.  With the recompute removed the test fails with `5 != 16`.
  Evidence: `python3 -m unittest discover -s tools/rebuild/tests` (113 tests).
- The other address shifts hold the same replay.  `test_initial_seed_leaves_no_overlap` starts from a store with the analysis record, whose extent spans the region that the scan now splits into three entries, and `test_manual_candidate_addition_leaves_no_overlap` runs on a store without the run-time entry and the exit helper: the scan reports only the prologue entries, the seed adds `0x300f8` and `0x300d6` itself, and the neighbours' extents shrink to the new entries.  With the recompute removed the first test fails with `64 != 16` and the second with `46 != 24`.
  Evidence: `python3 -m unittest discover -s tools/rebuild/tests` (115 tests).
- A store-level guard keeps the extents fresh.  `save_record` marks a new address and `remove_record` marks a deletion; `sync_extents()` recomputes the extents when the mark is set.  The seed, the command dispatch (`main`'s `finally`), and the exit hook (`atexit`) call it, so a later migration cannot leave a stale reach even when it forgets the recompute.  A clean store makes `sync_extents` a no-op, so the command dispatch and the exit hook stay cheap.  With the mark removed the new-record, the removed-record, and the three migration replays fail with `0 != 2`, `0 != 1`, `64 != 16`, `46 != 24`, and `5 != 16`.
  Evidence: `python3 -m unittest discover -s tools/rebuild/tests` (120 tests).
- The pipeline checks the debugger port before the run-time stages.  A bound port made the entry probe and the member probe fail with `Address already in use` inside stage 9.  `debugger_port_free` binds the loopback address with `SO_REUSEADDR`, the same bind as the control session, and `main` exits with code 2 with the message `error: the debugger control port 9876 is already in use; stop the other DOSBox-X session or run with --port` before any stage.
  Evidence: `python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game` with a listener on port 9876 exits 2 and prints the message.  The integration test `test_a_held_port_exits_two_before_any_stage` holds an ephemeral port, runs the command, and asserts the exit code 2, the message, no stage report, and no work directory.  The same preflight refuses a missing game directory, a missing reference executable, a reference executable that does not parse as an LE image, and a work root it cannot create, each with exit code 2 and no stage report; the integration tests `test_a_missing_game_dir_exits_two_before_any_stage`, `test_a_missing_exe_exits_two_before_any_stage`, `test_an_invalid_exe_exits_two_before_any_stage`, and `test_a_blocked_work_root_exits_two_before_any_stage` prove it.  `le.load` accepts the MZ header, the LE signature, and the tables of the reference image; the shipped `STARTH.EXE` is a DOS/4GW LE image, not a Windows PE image.
- The member probe ends the capture when the game closes the control connection.  The probe's loop requested the debugger after the game exit, and the `ConnectionError` from the closed socket made the probe exit 1, so stage 9 reported `the member probe runs` as a failure.  `run_probe` now catches `ConnectionError` and `TimeoutError` in the capture loop, prints the number of observed members, and breaks, so the observed members stay the result and the tool exits 0.
  Evidence: the full pipeline with the earlier failing run reported `1 failures` and `error: DOSBox-X closed the control connection`; after the change the same command reports `57 checks, 0 failures`.  The unit tests `test_a_closed_connection_ends_the_capture` and `test_the_command_line_exits_zero` simulate the closed connection mid-capture, `test_a_read_timeout_ends_the_capture` and `test_the_command_line_exits_zero_after_a_timeout` simulate a timed-out `BREAK` read, `test_a_timeout_on_the_first_read_observes_no_members` and `test_the_command_line_exits_zero_with_no_members` simulate the timeout before the first member open, `test_a_timeout_on_the_final_read_keeps_the_full_list` and `test_the_command_line_exits_zero_after_the_full_list` simulate the timeout after the last member open, and `test_a_failure_on_the_final_write_keeps_the_full_list` and `test_the_command_line_exits_zero_after_the_final_write` simulate the timeout on the last member's repair write; without the handling the first of each pair raises the exception and the second reports `1 != 0`.

- Open question: the `-of+` epilogue and the register-save order suggest the original build may use
  the initial 10.0 release.  The "10.0 Limited Availability" image (dated March 1994) holds such a
  compiler, but its `WCC386.EXE` does not load under DOSBox-X (the loader reports `Loader read
  error`), so it stays unverified.  The 10.0a toolchain is the closest verified match.
  Evidence: the Internet Archive item `watcom-c-cpp-compilers-collection`, file
  `watcom-10.0-la/watcom10la.iso` (SHA-256
  `91d3a2698c9f907994cecfae7e571edb904d31f38651aaf996c296c0e2badd5a`) and the failed run.


## The run-time stream formatter at 0xd429f

- The function at `0xd429f` is the run-time `__fprtf`, not a DPMI interrupt
  issuer.  It saves the file flag bits `0x30`, clears them, allocates the buffer
  through `0xddda9` when the buffer pointer at `+8` is zero, moves the `+0xd`
  bit 2 to bit 0, formats through `0xd68c0` with the table at `0xc428e`,
  restores the flags and calls `0xddfa8`, returns `-1` when the `+0xc` bit
  `0x20` is set, and restores the saved bits.  The callers are `printf` at
  `0x3009b` and `fprintf` at `0x4bf64`.  The earlier record name
  `dpmi_issue_int` is a misnomer.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0xd429f --count 60` and the
  rebuilt module `tools/rebuild/fprtf.c`.
- The rebuilt function passes all four behavior paths: the normal path clears
  and restores the `0x30` bits, the null buffer calls the allocate stub, the
  `+0xd` bit 2 change calls the restore stub, and the `0x20` flag returns `-1`.
  The identity stays open: 28 of 52 instructions equal, because the V2 compiler
  emits a stack frame and a different register order.
  Evidence: `python3 tools/rebuild/harness.py run --module fprtf` and
  `python3 tools/rebuild/run.py run --function 0xd429f`.
- A harness output file name longer than the 8.3 limit is truncated by DOS, so
  the harness does not find the file.  `fprtf_out.txt` became `FPRTF_OU.TXT`;
  the run reported every marker missing while the file held all five.  The
  module now writes `fp_out.txt`.  Keep every harness output name within 8.3.
  Evidence: `ls work/rebuild/FPRTF_OU.TXT` and the first failed run.
- The audit of every module recipe found one more long output name:
  `dpiex_out.txt` for `dpmi_int31_entry`; it is now `dpi_out.txt` and the
  behavior run finds both markers.  The store rejects a module whose
  `run.output` is not an 8.3 name or whose `run.command` redirects to a
  different name, and the unit suite checks every shipped recipe.
  Evidence: `python3 tools/rebuild/store.py check`,
  `python3 -m unittest discover -s tools/rebuild/tests`, and
  `python3 tools/rebuild/harness.py run --module dpmi_int31_entry`.

## The member probe control connection

- The member probe sometimes loses the DOSBox-X control connection or hits a
  request timeout after the member list is complete, because the game exits
  before the next debugger request.  The probe now ends the run with the
  collected names when the close or timeout follows the first member open, and
  the pipeline repeats the probe once when either happens before the first
  member.  It also repeats the probe once when a mid-run timeout ends the run
  before the sample loads, because the fallback then makes the run look
  successful.  Either before the first member still fails the stage.  A close while
  the probe restores the frame or the game copy also keeps the collected names,
  and the game copy is restored even when the child close raises.  A
  non-connection exception from the restore still propagates; only the
  control-connection errors are swallowed.  A generic `OSError` from the child
  close propagates after the game copy is restored.  An `OSError` from the
  game-copy restore propagates and leaves the copy patched, so a failed restore
  stays loud.  A patch-write failure restores the full backup before the error
  propagates, and a backup read failure leaves the copy untouched.  The backup
  restore and the final restore retry once after a partial write, so a
  transient partial failure still ends with the original bytes.  The change
  makes stage 9 pass on the first pipeline attempt; the earlier full runs
  failed only this stage.
  Evidence: the stage 9 failures (`error: DOSBox-X closed the control
  connection`), the direct probe run (`error: timed out` after 50 member
  opens), and the clean stage 9 after the change.

## The music_start module

- Observation: `music_start` at `0x2ffc4` holds 41 instructions and 140 bytes up
  to the return at `0x3004f`.  The plan row holds the same body counts.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2ffc4 --count 41` and
  `python3 tools/le.py calls STARTH.EXE 0x2ffc4`.
- Observation: the function reads the status word at `+0x10` of the music bank
  struct.  The statuses 1 through 6 select six messages through the jump table
  at object offset `0x1ffac`.  The messages sit at data-segment offsets
  `0x1577`, `0x1596`, `0x15b6`, `0x15ca`, `0x15e2`, and `0x15f5`.  Each case
  pushes the message and calls `printf` at `0x3009b`.  A non-zero status then
  calls `exit(-1)` at `0x300be`.  A zero status returns at once.
  Evidence: the six call sites from `0x2ffe6` to `0x30031`, the exit call at
  `0x30047`, and the object 3 data at the linear `0x101577` through `0x1015f5`.
- Observation: the only caller is `engine_init` at `0x1284d`, after
  `music_bank_load` at `0x1283f`.  `music_bank_load` writes the status `1` and
  `2` at `0x2f9e0`.  `music_bank_reset` clears the status at `0x2f9b0`.
  Evidence: the caller scan and `python3 tools/le.py disasm STARTH.EXE 0x12820
  --count 12`.
- Inference: the description "starts the music from the bank" does not fit this
  address.  The function calls no playback routine.  It reports the load status
  and terminates on a failure.  The record role and the record evidence now say
  this, and the plan description deviates from the code.
  Evidence: the 41-instruction body and the seven call targets.
- Observation: the rebuild with `-of+` and a C switch crashed.  The pinned
  OpenWatcom V2 compiler does not place the compiler jump table in the code
  object, so the rebuilt `jmp` read the entry at `CS:0`.  The run stopped at
  `1A0:00000001`, the same fault as the `error_report` module.  The source now
  holds an if-else chain with one print call per state, and the behavior passes.
  Evidence: the first run and `python3 tools/rebuild/harness.py run --module
  music_start`.
- Observation: the rebuild holds 43 instructions and 134 bytes.  The identity
  check stays open at 8 of 41 instructions and 22 of 134 bytes, and the store
  records `rebuilt`.  The behavior check passes.  The harness prints `0` through
  `8` and `D`: the zero status prints nothing, the six in-range statuses print
  their message and exit, and the statuses 7 and -1 print nothing and exit.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2ffc4 --rebuilt work/rebuild/msmod.exe --rebuilt-map
  work/rebuild/music_start.map --rebuilt-symbol music_start_` and
  `python3 tools/rebuild/run.py run --function 0x2ffc4`.

## The race_end module

- Observation: `race_end` at `0x279a4` holds 36 instructions and 141 bytes up
  to the return at `0x27a30`.  The plan row holds the same body counts.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x279a4 --count 36` and
  `python3 tools/le.py calls STARTH.EXE 0x279a4`.
- Observation: the function calls thirteen targets in order: `0x1f8b4` with the
  value 0, `0x1f8dc`, `0x2fe68` with `DAT_000062b0` and `DAT_00006350`,
  `0x264ec`, `0x1b180`, `0x115f8` with `DAT_00006530`, `0x11304` with
  `DAT_00006530`, `0x15874`, `0x4cf2c`, `0x1f6f8`, `0x1b58c`, `0x25640`, and
  `0x258b8`.  The first two argument calls load the data addresses into `eax`
  and `edx`.
  Evidence: the call sites from `0x279ae` to `0x27a12`.
- Observation: the call at `0x15874` is gated.  The function tests the dword at
  `DAT_0018e3c` at `0x279e5` and skips the call when the dword is non-zero.
  When the dword is zero, the function loads 1 into `ebx`, calls `0x15874`, and
  stores `ebx` to `DAT_0018e3c`.
  Evidence: the test at `0x279e5`, the `jne 0x279fe` at `0x279ec`, and the
  store at `0x279f8`.
- Observation: the last two stores copy `DAT_001d22c` to `DAT_001d274` and
  `DAT_0018e3c` to `DAT_001d230`.
  Evidence: the loads and stores from `0x27a17` to `0x27a26`.
- Observation: the only caller is `race_loop` at `0x27761`, after the loop exit
  test on `DAT_0011ce44`.
  Evidence: the caller scan of the object 1 code and `python3 tools/le.py
  disasm STARTH.EXE 0x27700 --count 40`.
- Inference: the gated call and the flag set fit a one-time teardown step.  The
  name `race_end` and the plan description fit the data flow.  The thirteen
  targets have no recovered names.
  Evidence: the thirteen call sites and the gated flag store.
- Observation: the rebuild with `-of+` holds 32 instructions and 136 bytes.
  OpenWatcom V2 drops the saved `ebx`, the frame local, and the `ebx`-held
  constant, and stores 1 to `DAT_0018e3c` directly after the gated call.  The
  identity check stays open at 10 of 32 instructions and 20 of 136 bytes, and
  the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x279a4 --rebuilt work/rebuild/remod.exe --rebuilt-map
  work/rebuild/race_end.map --rebuilt-symbol race_end_`.
- Observation: the behavior check passes.  The harness prints `0`, `1`, and
  `D`.  A zero flag runs all thirteen calls, receives the data addresses, sets
  the flag to 1, and copies the two final values.  A non-zero flag keeps its
  value, skips the gated call, and still copies the two final values.
  Evidence: `python3 tools/rebuild/harness.py run --module race_end`,
  `work/rebuild/re_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x279a4`.

## The controls_load module

- Observation: `controls_load` at `0x1b304` holds 40 instructions and 144 bytes
  up to the return at `0x1b393`.  The plan row holds the same body counts.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1b304 --count 40`.
- Observation: the function builds `%s\config\controls.cfg` with the base path
  at `DAT_00007edc`.  It opens the file with the mode `rb` and keeps the handle
  in the frame slot at `[ebp+0x7e]`.  The format sits at object-3 offset `0x9af`
  (linear `0x1009af`) and the mode sits at object-3 offset `0x9c6`.
  Evidence: the `sprintf` call at `0x1b324`, the `fopen` call at `0x1b334`, the
  store at `0x1b339`, and the object-3 data at the two linear addresses.
- Observation: the function reads three blocks.  It reads six 28-byte records
  into `0x197d8`, forty 28-byte records into `0x192a4`, and ten 4-byte records
  into `0x19704`.  Then it closes the handle at `0xc6245`.
  Evidence: the three `fread` calls at `0x1b34e`, `0x1b365`, and `0x1b37c`, the
  size and count pairs `0x1c` and 6, `0x1c` and `0x28`, and 4 and `0xa`, and
  the close at `0x1b384`.
- Observation: the function has no open-failure test.  When `fopen` returns
  zero, the three reads and the close receive the zero handle.
  Evidence: the straight-line body from `0x1b339` to `0x1b384`.
- Inference: the three blocks are the controls configuration.  The record role
  now says this.  The first block holds six records, the second block holds
  forty, and the third block holds ten.
  Evidence: the three size and count pairs.
- Observation: the rebuild with `-of+` holds 41 instructions and 138 bytes.
  OpenWatcom V2 saves `ebp` before the other registers, keeps the handle in
  `esi`, and uses the local slot at `[ebp-0x90]`.  The identity check stays open
  at 36 of 40 instructions and 16 of 138 bytes, and the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1b304 --rebuilt work/rebuild/clmod.exe --rebuilt-map
  work/rebuild/controls_load.map --rebuilt-symbol controls_load_`.
- Observation: the behavior check passes.  The harness prints `0`, `1`, and
  `D`.  A valid handle reaches the three reads with the original sizes and
  counts and then the close.  A zero handle still reaches the three reads and
  the close.
  Evidence: `python3 tools/rebuild/harness.py run --module controls_load`,
  `work/rebuild/cl_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x1b304`.

## The memory_alloc module at 0x2f172

- Observation: `memory_alloc` at `0x2f172` holds 49 instructions and 147 bytes
  up to the return at `0x2f204`.  The plan row holds the same body counts.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2f172 --count 49`.
- Observation: the function calls the DPMI function `0x0501`.  It writes
  `0x0501` to block offset 0, the high size word to block offset 4, and the low
  size word to block offset 8.  Then it fills the 12-byte segment set through
  `dpmi_fill_regs` at `0x4bf23` and calls the interrupt 0x31 path through
  `dpmi_call` at `0x4bf4b`.
  Evidence: the stores at `0x2f187`, `0x2f194`, and `0x2f19f`, and the two
  calls at `0x2f1a5` and `0x2f1b8`.
- Observation: the status at block offset `0x18` selects the result.  A
  non-zero status returns 0.  A zero status stores the address from block
  offsets `0x10` and `0x14` at the third argument and returns the size words
  from block offsets 4 and 8.
  Evidence: the test at `0x2f1bd` and the two reassemblies at `0x2f1cc` through
  `0x2f1f8`.
- Observation: the three call sites at `0x2ef62`, `0x2ef8e`, and `0x2f02e` pass
  the resource object in `eax`, the size in `edx`, and the record address field
  at `+0x10` in `ebx`.  The call at `0x2ef62` runs after the DOS memory path at
  `0x2f0fc` fails, and the call at `0x2ef8e` handles a size at or above
  `0x96000`.
  Evidence: the call sites at `0x2ef62`, `0x2ef8e`, and `0x2f02e`.
- Inference: the resource layer keeps the DPMI block address at record offset
  `+0x10` and the DOS selector at the same field on the DOS path.  The flag at
  `+0x18` selects the free path, and `resource_free` reads the address from
  `+0x10`.
  Evidence: the `memory_alloc` store at `0x2f1e0`, the `memory_free` input at
  `0x2f21f` through `0x2f230`, and `tools/rebuild/functions/0x2f470.json`.
- Observation: the module recipe name is `memory_alloc_small`, because the
  `memory_alloc` recipe belongs to the different function at `0x30050`.  The
  record name stays `memory_alloc` and the record module field names the unique
  recipe.
  Evidence: `tools/rebuild/modules/memory_alloc_small.json` and
  `tools/rebuild/functions/0x2f172.json`.
- Observation: the rebuild with `-of+` holds 39 instructions and 112 bytes.
  OpenWatcom V2 saves `ebp` before the other registers, keeps the address
  pointer in `esi`, and folds the repeated loads.  The identity check stays open
  at 10 of 39 instructions and 4 of 112 bytes, and the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2f172 --rebuilt work/rebuild/masmod.exe --rebuilt-map
  work/rebuild/memory_alloc_small.map --rebuilt-symbol memory_alloc_small_`.
- Observation: the behavior check passes.  The harness prints `0`, `1`, and
  `D`.  The success path sends the request `0x0501` with the size words `0x1234`
  and `0x5678`, returns the size `0x12345678`, and stores the address
  `0x9abcdef0`.  The failure path returns 0 and leaves the address.
  Evidence: `python3 tools/rebuild/harness.py run --module memory_alloc_small`,
  `work/rebuild/mas_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x2f172`.

## The overlay_dispatch module at 0x28160

- Observation: the first-return body at `0x28160` holds 42 instructions and 152
  bytes, up to the return at `0x281f7`.  The plan row holds the same body
  counts.  The full function extends to `0x289bc` and holds 2140 bytes, and the
  store keeps that true extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x28160 --count 42` and the
  `size` field of `tools/rebuild/functions/0x28160.json`.
- Observation: the byte operand `[3]` selects the top-level path.  The Ghidra
  name of the slot is `DAT_000f0002`.  A clear flag jumps to the second overlay
  path at `0x283ff`.
  Evidence: the compare at `0x2816b` and the branch at `0x28172`.
- Observation: with the flag set, the function clears `DAT_0011d24c`, reads the
  mode `DAT_0011fb68`, and returns through the shared epilogue at `0x289b1`
  when the mode is `0x20`.  It then clears `DAT_00200988` and reads the state
  `DAT_0011fac4`.  A state above 8 jumps to the shared entry at `0x282e6`, and
  the other states run through the compiler jump table at
  `cs:[eax*4 + 0x1813c]`.
  Evidence: the stores at `0x28180` and `0x28194`, the compare at `0x28186`,
  the bounds check at `0x2819a`, and the jump at `0x281a3`.
- Observation: the jump table holds nine object offsets.  They map the state
  values 0 through 8 to the linear addresses `0x281ab`, `0x282e6`, `0x281f8`,
  `0x282a2`, `0x283da`, `0x2837d`, `0x282e6`, `0x282e6`, and `0x28358`.
  Evidence: the 36 bytes at linear `0x2813c`.
- Observation: case 0 at `0x281ab` pops the stack.  When `DAT_0011fb58` is not
  zero, it loads `DAT_00120350[DAT_0011fb58 - 1]` into `DAT_0011fb60`,
  decrements `DAT_0011fb58`, and calls `0x1f718` with 0.  When the stack is
  empty, it sets `DAT_0011d234` and `DAT_00007ed8` to 1, sets
  `DAT_0011fb68` to `0x20`, and returns.
  Evidence: the body at `0x281ab` through `0x281f7`.
- Observation: the rebuild keeps the dispatcher, implements case 0, and
  represents the other case bodies as step functions.  The DOS/4G loader does
  not relocate the compiler jump table of the rebuilt image, so the rebuild
  uses an if chain, as `race_state_update` does.
  Evidence: `tools/rebuild/overlay_dispatch.c` and the jump table at
  `0x1005d` of the first switch build, which jumps through the address 0.
- Observation: the rebuild with `-of+` holds 66 instructions and 223 bytes.
  The identity check stays open at 9 of 42 instructions and 19 of 152 bytes,
  and the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x28160 --rebuilt work/rebuild/odmod.exe --rebuilt-map
  work/rebuild/overlay_dispatch.map --rebuilt-symbol overlay_dispatch_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9`, `A`, and `D`.  It runs the two overlay paths, the mode path,
  every state case, and the two case 0 stack paths.
  Evidence: `python3 tools/rebuild/harness.py run --module overlay_dispatch`,
  `work/rebuild/od_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x28160`.
- Inference: `DAT_0011fac4` is the overlay state, `DAT_0011fb58` is the depth of
  a small stack, and `DAT_0011fb68` is the next overlay mode.  The function
  selects the overlay step for the current state and advances the stack.
  Evidence: the state dispatch at `0x281a3`, the stack pop at `0x281b5` through
  `0x281cc`, and the mode store at `0x281ef`.

## The palette_build module at 0x15720

- Observation: the body at `0x15720` holds 55 instructions and 152 bytes.  It
  saves four registers, makes the frame, reserves 0x14 bytes, and returns at
  `0x157b7`.  The plan row holds the same counts.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x15720 --count 55`.
- Observation: the two loops fill a table.  The outer loop runs from 0 to 49 and
  the inner loop runs from 0 to 99.  The row base is `0x80e8 + 400 * i` and the
  entry address is the row base plus `4 * j`.  The table therefore holds 50
  rows of 100 floats.  The row base has the linear address `0x1080e8`, in
  writable object 3.
  Evidence: the arithmetic at `0x15743` through `0x1575c`, and the store at
  `0x157a4` through `0x157ad`.
- Observation: the float constant at `[ebp - 4]` is `0x3ba3d70a` (0.005).  The
  `fadd qword ptr [0x66e]` adds the double 0.5 and the `fdivr qword ptr
  [0x676]` divides the double 50.0 by `i + 1`.  So the ramp value is
  `j * 0.005f + 0.5` and the exponent is `50.0 / (i + 1)`.
  Evidence: the bytes at `0x15729`, `0x1577a`, and `0x1578a`, and the double
  values at object 3 offsets `0x66e` and `0x676`.
- Observation: the call at `0x1579f` passes the exponent first and the ramp
  value second.  The two qword arguments reach the routine at `0xc21e2`, which
  reads both and calls the routine at `0xc20a6`.  That routine uses `fyl2x` and
  the `f2xm1` square-and-multiply loop at `0xc21bf`, so it computes a power.
  The function stores the result as a float.
  Evidence: the disassembly at `0xc21e2`, `0xc20a6`, and `0xc21bf`.
- Inference: the function computes
  `table[i][j] = (float)pow(j * 0.005f + 0.5, 50.0 / (i + 1))`.  The ramp
  starts at 0.5 for column 0 and the exponent falls from 50.0 to 1.0 over the
  rows.
  Evidence: the argument order and the power routine.
- Observation: the rebuild with `-of+` holds 40 instructions and 106 bytes.
  The identity check stays open at 5 of 55 instructions and 5 of 106 bytes,
  because the shared toolchain uses its own frame and register code.  The store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x15720 --rebuilt work/rebuild/pbmod.exe --rebuilt-map
  work/rebuild/palette_build.map --rebuilt-symbol palette_build_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `3` and `D`.  It checks 5000 stub calls, the first and the last
  argument pair, and the entries at the row and column edges.
  Evidence: `python3 tools/rebuild/harness.py run --module palette_build`,
  `work/rebuild/pb_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x15720`.
- Observation: in the shared harness image the double literals of this module
  read as zero at run time.  The rebuild therefore reads the three constants
  from the globals `palette_step`, `palette_base`, and `palette_scale`, and the
  harness sets the exact bit patterns.  A diagnostic build printed zero for the
  first base, the first exponent, and table entry 0 when the module used the
  literals.
  Evidence: the diagnostic harness run and `tools/rebuild/palette_build.c`.

## The fade_update module at 0x29818

- Observation: the body at `0x29818` holds 48 instructions and 154 bytes.  It
  saves five registers, makes the frame, and returns at `0x298af`.  The plan
  row holds the same counts.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x29818 --count 48`.
- Observation: the function reads the state `DAT_0011fac8`.  State 0 does
  nothing.  State 1 runs the fade-in path.  Every other state runs the fade-out
  path.
  Evidence: the reads at `0x2981f` and `0x2982d`, the compare at `0x29825`, and
  the branch at `0x29830`.
- Observation: the fade-in path computes
  `DAT_0011d224 = DAT_0018e08 * delta + DAT_0011d224`.  When the value is at
  least 1.0, it stores 1.0 and clears the state.  The float argument sits at
  `[ebp + 0x18]`, after the five saved registers.
  Evidence: the code at `0x29832` through `0x29869`.
- Observation: the fade-out path computes
  `DAT_0011d224 = DAT_0011d224 - DAT_0018e08 * delta`.  When the value is
  below 0.0, it stores 0.0 and clears the state.
  Evidence: the code at `0x2986b` through `0x29897`.
- Observation: every active path calls the routine at `0x27ec8` with the fade
  value and the offset 0.  That routine is `video_present`.  The callee removes
  the two stack arguments with `ret 8`.  The function removes the float argument
  with `ret 4`.
  Evidence: the pushes at `0x2989d` and `0x2989f`, the call at `0x298a5`, and
  the epilogue at `0x298af`.
- Inference: `DAT_0011d224` is the fade level, `DAT_0018e08` is the fade rate,
  and `DAT_0011fac8` is the fade direction.  The state 1 fades in.  The state 2
  fades out.
  Evidence: the two arithmetic paths and the `video_present` calls.
- Observation: the rebuild with `-of+` holds 37 instructions and 142 bytes.
  The identity check stays open at 2 of 48 instructions and 7 of 142 bytes.
  The shared toolchain uses its own prologue, its register allocation, and a
  `cmp` clamp for the fade-in path.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x29818 --rebuilt work/rebuild/fumod.exe --rebuilt-map
  work/rebuild/fade_update.map --rebuilt-symbol fade_update_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `6` and `D`.  It runs the idle path, the two fade-in paths, the two
  fade-out paths, the two exact clamp values, and the recorded
  `video_present` arguments.
  Evidence: `python3 tools/rebuild/harness.py run --module fade_update`,
  `work/rebuild/fu_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x29818`.

## The persptab_load module at 0x4cf94

- Observation: the body at `0x4cf94` holds 43 instructions and 155 bytes.  The
  plan row holds the same counts.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4cf94 --count 43`.
- Observation: the function opens `TABS\persp01.tab` with the mode `rb` through
  the call at `0xc5fe5`.  On failure it calls `0x108e4` with 3, `0x10978` with
  `0x16`, and `0x300be` with `0x16`.  The store names the three calls
  `bios_video`, `error_report`, and `exit`.
  Evidence: the calls at `0x4cfa7`, `0x4cfba`, `0x4cfc4`, and `0x4cfce`, and
  the store records `0x108e4`, `0x10978`, and `0x300be`.
- Observation: the function calls `resource_read` at `0x2efc2` with the
  resource structure `DAT_00021574`, the value `0x1747`, and the size `0x40740`.
  It stores the result in `DAT_000214ec`, reports a null result through the
  call at `0x2edad`, reads `0x40740` bytes through `fread` at `0xc607b`, adds
  `0x40000` to the cell, and closes the handle through `0xc6245`.
  Evidence: the code at `0x4cfd3` through `0x4d02e`.
- Inference: `DAT_000214ec` is the `PERSPTAB` cell, and the `0x40000` advance
  points the cell to the second half of the table.  The earlier finding holds
  the same reading.
  Evidence: the store note and the earlier `PERSPTAB` finding.
- Observation: the rebuild with `-of+` holds 44 instructions and 150 bytes.
  The identity check stays open at 37 of 43 instructions and 24 of 150 bytes.
  The shared toolchain uses its own prologue, keeps the handle in `esi`, and
  tests the register instead of the frame slot.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x4cf94 --rebuilt work/rebuild/ptlmod.exe --rebuilt-map
  work/rebuild/persptab_load.map --rebuilt-symbol persptab_load_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, and `D`.  It runs the success path, the open-failure path, and the
  resource-failure path.
  Evidence: `python3 tools/rebuild/harness.py run --module persptab_load`,
  `work/rebuild/PTL_OUT.TXT`, and `python3 tools/rebuild/run.py run --function
  0x4cf94`.

## The config_load module at 0x1bf38

- Observation: the original body at `0x1bf38` holds 46 instructions and 155
  bytes.  The record keeps the true extent of 2088 bytes, and the diff reads the
  155-byte body.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1bf38 --count 46`.
- Observation: the function opens `screamer.cfg` with the mode `rt` through the
  call at `0xc5fe5`.  On failure it calls `0xc656e` with
  `Files damaged. Re-install Screamer.` and `0x300be` with -1.
  Evidence: the code at `0x1bf3f` through `0x1bf66`, and the store records
  `0xc656e` and `0x300be`.
- Observation: the function reads `%s%s%s` into `DAT_00007edc`,
  `DAT_00007f10`, and `DAT_00007f44` through `fscanf` at `0xc6618`.  It then
  reads two `%c` values into the local at `[ebp - 4]`.  The flag
  `DAT_00007f78` receives 1 when the second character is `n`, and the function
  closes the handle through `0xc6245`.
  Evidence: the code at `0x1bf6b` through `0x1bfce`.
- Inference: the three string buffers hold the configuration fields, and the
  two characters encode two yes-or-no options.  The strings are `rt` and
  `screamer.cfg` at `0xab1` and `0xab4` in the data segment.  The purpose of
  each field is open.
  Evidence: the data dump and the earlier `Subsystem initialisation` finding.
- Observation: the rebuild with `-of+` holds 43 instructions and 141 bytes.
  The identity check stays open at 18 of 43 instructions and 18 of 141 bytes.
  The shared toolchain saves the registers after the frame, keeps the stream in
  `edx` instead of the frame slot, and uses `movzx` where the original uses
  `and eax,0xff`.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1bf38 --rebuilt work/rebuild/cfglmod.exe --rebuilt-map
  work/rebuild/config_load.map --rebuilt-symbol config_load_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, and `D`.  It runs the success path with the character `n`, the
  success path with another character, and the open-failure path.
  Evidence: `python3 tools/rebuild/harness.py run --module config_load`,
  `work/rebuild/CFG_OUT.TXT`, and `python3 tools/rebuild/run.py run --function
  0x1bf38`.

## The fopen module at 0x4c08d

- Observation: the original body at `0x4c08d` holds 48 instructions and 155
  bytes.  The plan row holds the same counts.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4c08d --count 48`.
- Observation: the function takes four arguments in `eax`, `edx`, `ebx`, and
  `ecx`: the stream record, the container path, the member name, and the
  category.  It opens the path with the mode `rb` through the call at
  `0xc5fe5` and stores the handle at `stream + 0x00`.
  Evidence: the four argument slots at `0x4c098` through `0x4c0a1` and the call
  at `0x4c0ac`.
- Observation: when the open fails, the function copies the path to
  `stream + 0x78`, sets the state at `stream + 0xac` to 1, and returns -1.
  When the open succeeds, it copies the category to `stream + 0x20`, copies the
  path to `stream + 0x24`, and calls the member search `0x4c128` with the member
  name.  A failed search returns -1.  A found member returns 0.
  Evidence: the copy calls at `0x4c0c9`, `0x4c0ed`, and `0x4c0fb`, the member
  call at `0x4c106`, and the two result stores at `0x4c10f` and `0x4c118`.
- Observation: the module is the member open of the container file layer, not
  the printf stream formatter.  The callers pass `melodic.bnk` with
  `MELODIC_BANK` and `drum.bnk` with `DRUM_BANK`.  The mode string at object-3
  offset `0x16ae` is `rb`.
  Evidence: the call site at `0x2facc` in `music_bank_load` and the data dump at
  linear `0x1016ae`.
- Observation: the rebuild with `-of+` holds 34 instructions and 94 bytes.  The
  identity check stays open at 11 of 34 instructions and 6 of 94 bytes.  The
  shared toolchain saves the registers after the frame, keeps the stream pointer
  and the path in `esi` and `edi`, and tests the returned handle directly
  instead of the stored slot.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x4c08d --rebuilt work/rebuild/fopmod.exe --rebuilt-map
  work/rebuild/fopen.map --rebuilt-symbol fopen_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, and `D`.  It runs the success path, the open-failure path, and the
  member-failure path.
  Evidence: `python3 tools/rebuild/harness.py run --module fopen`,
  `work/rebuild/FOP_OUT.TXT`, and `python3 tools/rebuild/run.py run --function
  0x4c08d`.

## The resource_read module at 0x2efc2

- Observation: the original body at `0x2efc2` holds 56 instructions and 160
  bytes.  The plan row holds the same counts.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2efc2 --count 56`.
- Observation: the function reads the resource structure.  The record array is
  at `+0x00`, the count at `+0x04`, the limit at `+0x08`, the status at `+0x0c`,
  and the name buffer at `+0x10`.
  Evidence: the loads and stores at `0x2efd7` through `0x2f000`.
- Observation: when the count equals the limit, the function sets the status at
  `+0x0c` to 2, copies the name to `resource + 0x10`, and returns 0.  The copy
  goes through the `sprintf` call at `0x4d32c` with the format `%.11s`.
  Evidence: the branch at `0x2efe3` and the call at `0x2f000`.
- Observation: the literals `0x14c5`, `0x14cb`, `0x14d1`, and `0x14d7` are
  four copies of `%.11s` in the data object at base `0x100000`.  The code holds
  the object offset, and the loader adds the object base.
  Evidence: the data dump at file offsets `0xf7cc5` through `0xf7cd7`.
- Observation: the normal path selects the record at
  `*resource + count * 0x1c`.  It calls `memory_alloc_small` at `0x2f172` with
  the resource, the size, and the address cell `record + 0x10`.  Then it clears
  the flag at `record + 0x18` and calls `0x2f062` with the resource, the record,
  the allocation result, the name, and the size.
  Evidence: the code at `0x2f011` through `0x2f055`.
- Inference: the record holds the name at `+0x00`, the allocation result at
  `+0x0c`, the address cell at `+0x10`, the requested size at `+0x14`, and the
  free flag at `+0x18`.  The routine at `0x2f062` writes the name, the
  allocation result, and the requested size.  `resource_free` frees the value at
  `+0x10`.
  Evidence: the cache routine at `0x2f062` and the `resource_free` record access
  at `0x2f4c4`.
- Observation: the rebuild with `-of+` holds 41 instructions and 104 bytes.  The
  identity check stays open at 12 of 41 instructions and 4 of 104 bytes.  The
  shared toolchain saves the frame before the register saves and keeps the
  resource and the name in `esi` and `ecx`.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2efc2 --rebuilt work/rebuild/rrout.exe --rebuilt-map
  work/rebuild/resource_read.map --rebuilt-symbol resource_read_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, `3`, and `D`.  It runs the full-table path, the first-record path,
  the second-record path with the 0x1c-byte stride, and the allocation-failure
  path.
  Evidence: `python3 tools/rebuild/harness.py run --module resource_read`,
  `work/rebuild/rr_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x2efc2`.

## The race_panel_4 module at 0x2e724

- Observation: the original body at `0x2e724` holds 50 instructions and 167
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  168-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2e724 --count 50` and
  `tools/rebuild/functions/0x2e724.json`.
- Observation: the function has a frame: it saves `ebx`, `ecx`, `edx`, and
  `ebp`, sets `ebp`, subtracts `0x94` from `esp`, and subtracts `0x82` from
  `ebp`.  The rebuild uses the option `-of+`.
  Evidence: the prologue at `0x2e724` through `0x2e736`.
- Observation: the centered title call at `0x2e7cc` receives the pointer at
  `DAT_0011fc38` in `eax`, the y `2` in `edx`, the font `3` in `ebx`, and the
  color `2` in `ecx`.
  Evidence: the loads at `0x2e736` through `0x2e747`.
- Observation: the format call at `0x4d32c` receives the local buffer at
  `[ebp-0x12]`, the format `\001%s` at object-3 offset `0x12f7`, and the player
  name at `DAT_001203cc` at object-3 offset `0x203cc`.  The code stores the
  value `0x83` from `edx` after the call, because the call preserves `edx`.
  Evidence: the pushes at `0x2e74c` through `0x2e75f` and the store at
  `0x2e767`.
- Observation: the first draw call at `0x25020` receives the doubled position
  `(0x106, 0x98)`, the buffer in `ebx`, the font `2` in `ecx`, and the color `2`
  on the stack.
  Evidence: the loads at `0x2e76a` through `0x2e785`.
- Observation: the second draw call shifts the raw x by `DAT_0011d244 * 21`,
  adds the base `0x83`, doubles the result, doubles the y `0x58`, and passes
  the separator `\001-` at object-3 offset `0x12fb`.  The font and the color
  stay `2`.
  Evidence: the arithmetic at `0x2e78a` through `0x2e7a2` and the draw at
  `0x2e7bb`.
- Observation: the literals and the globals are object-3 offsets in the code.
  The loader adds the object base `0x100000`.  The title pointer
  `DAT_0011fc38`, the count `DAT_0011d244`, and the name `DAT_001203cc` are
  object-3 cells.
  Evidence: the data at linear `0x1012f7`, `0x1012fb`, and `0x1203cc`.
- Inference: the title, the player name, and the separator form the overlay of
  the race state 4.  The 21-pixel shift places one name entry for each count
  unit.
  Evidence: the call sequence and the strings.
- Observation: the rebuild with `-of+` holds 37 instructions and 131 bytes.  The
  identity check stays open at 11 of 37 instructions and 24 of 131 bytes.  The
  shared toolchain saves the frame before the register saves, keeps the buffer
  at `[ebp-0x8c]`, and folds the constants.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2e724 --rebuilt work/rebuild/rp4mod.exe --rebuilt-map
  work/rebuild/race_panel_4.map --rebuilt-symbol race_panel_4_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, and `D`.  The first path uses the count 0, the second path uses the
  count 5, and the third path uses a second title and the count 2.  The `draw`
  stub copies the text at call time, because the module buffer is a stack local
  and a later call overwrites that stack area.
  Evidence: `python3 tools/rebuild/harness.py run --module race_panel_4`,
  `work/rebuild/rp4_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x2e724`.

## The engine_interface_init module at 0x10810

- Observation: the original body at `0x10810` holds 51 instructions and 183
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  184-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x10810 --count 51` and
  `tools/rebuild/functions/0x10810.json`.
- Observation: the function has a frame: it saves `ebp`, sets `ebp`, subtracts
  `0xc` from `esp`, and stores the two register arguments at `[ebp-0xc]` and
  `[ebp-8]`.  The rebuild uses the option `-of+`.
  Evidence: the prologue at `0x10810` through `0x10819`.
- Observation: the function calls `memory_alloc` at `0x30050` with the size
  `0x4c`.  When the result is not zero, it calls `list_create` at `0x2ecf8`
  with the result and the count `0xb4`.  It stores the final result at
  `DAT_0000801c`.
  Evidence: the calls at `0x1081c` through `0x10834`.
- Observation: the loop runs `0x78` times and addresses records of `0x1c`
  bytes at the first argument.  Each record sets the fields `+0x14`, `+0x0c`,
  and `+0x10` to 0 and the field `+0x18` to -1.
  Evidence: the loop at `0x10839` through `0x108b0`.
- Observation: the function clears the byte at `DAT_00007fe8` and the dword at
  the second argument.
  Evidence: the stores at `0x108b2` through `0x108bd`.
- Observation: the caller at `0x1286b` in `engine_init` passes the address
  `0x6530` in `eax` and the address `0x6350` in `edx`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x127a0 --count 71`.
- Inference: the allocated object is the engine interface list header, the
  first argument is the `0x78`-record table, and the second argument is a state
  word.  The first argument address `0x6530` is the record table that
  `race_session_start` passes to `object_list_link`.
  Evidence: the `list_create` source, the loop stride, and the shared address
  in `race_session_start`.
- Observation: the rebuild with `-of+` holds 31 instructions and 106 bytes.
  The identity check stays open at 10 of 31 instructions and 10 of 106 bytes.
  The shared toolchain keeps the arguments in `ecx` and `ebx`, uses `imul` for
  the `0x1c` stride, and folds the address calculation.  The original
  recomputes the address with `lea`, `sub`, and `shl` for each field.  The store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x10810 --rebuilt work/rebuild/eiimod.exe --rebuilt-map
  work/rebuild/engine_interface_init.map --rebuilt-symbol
  engine_interface_init_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, and `D`.  The first path uses the successful allocation and a distinct
  list result, and the second path uses a failed allocation.
  Evidence: `python3 tools/rebuild/harness.py run --module
  engine_interface_init`, `work/rebuild/eii_out.txt`, and `python3
  tools/rebuild/run.py run --function 0x10810`.

## The race_frame module at 0x2807c

- Observation: the original body at `0x2807c` holds 48 instructions and 189
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  228-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2807c --count 48` and
  `tools/rebuild/functions/0x2807c.json`.
- Observation: the function has a frame: it saves `ebx`, `edx`, and `ebp`,
  sets `ebp`, and subtracts `0x3c` from `esp`.  The rebuild uses the option
  `-of+`.
  Evidence: the prologue at `0x2807c` through `0x28081`.
- Observation: the function copies `0x3e800` bytes from `DAT_0011fad0` to
  `DAT_00121528` through `memcpy` at `0x8c150`.  It passes the destination in
  `eax`, the source in `edx`, and the count in `ebx`.
  Evidence: the call at `0x28094` and the `memcpy` module at
  `tools/rebuild/memcpy.asm`.
- Observation: the flag byte `DAT_0011d274` gates the two swaps.  The flag
  `0x10` calls `video_block_swap_a` at `0x1075f`, and the flag `0x20` calls
  `video_block_swap_b` at `0x107a1`.
  Evidence: the tests and the calls at `0x28099` through `0x280b5`.
- Observation: the function calls `overlay_dispatch` at `0x28160`,
  `race_heading_update` at `0x29610`, `race_state_update` at `0x29564`,
  `camera_update` at `0x8ac78` with the object `0x109a0`, and `sound_update`
  at `0x81a70` with the block `0x6350`.
  Evidence: the calls at `0x280b5` through `0x280d3`.
- Observation: the compiler jump table at `cs:[eax*4 + 0x18068]` selects the
  overlay for the race state `DAT_001203b8`.  The entries are `0x180ea`,
  `0x18104`, `0x180f1`, `0x180f8`, and `0x180ff`.  The state 0 calls
  `race_panel_0` at `0x2e0b4`, the state 1 calls nothing, the state 2 calls
  `race_panel_2` at `0x2e21c`, the state 3 calls `race_panel_3` at `0x2e58c`,
  and the state 4 calls `race_panel_4` at `0x2e724`.  A value above 4 skips the
  table.
  Evidence: the switch at `0x280d8` through `0x28104` and the table bytes at
  linear `0x28068`.
- Observation: the function calls `fade_update` at `0x29818` with the float
  `DAT_00120378`.  The argument goes on the stack, as the `fade_update` module
  requires.
  Evidence: the push at `0x28104` and the call at `0x2810a`.
- Observation: the function loads the float `DAT_0011d24c`, compares it with
  the constant at `0xee0`, and sets `DAT_0011d234` to 1 when the value is above
  the constant and `DAT_0011fac4` is 0.  The three FPU instructions are `fld`,
  `fcomp`, and `fnstsw`.
  Evidence: the compare and the store at `0x2810f` through `0x28129`.
- Inference: `DAT_0011d24c` is a frame value and the constant at `0xee0` is a
  frame limit.  The flag `DAT_0011d234` ends the `frame_update` loop.
  Evidence: the `frame_update` loop in this document.
- Observation: the constant at `0xee0` sits in a low segment that the shared
  harness image does not load.  The rebuild reads the threshold through the
  global `frame_limit`, and the harness sets the bit pattern.
  Evidence: the failed direct read at linear `0xee0` and the `palette_build`
  constant pattern.
- Observation: the rebuild with `-of+` holds 54 instructions and 197 bytes.
  The identity check stays open at 10 of 48 instructions and 23 of 189 bytes.
  The rebuild uses an if chain instead of the compiler jump table, saves the
  registers in another order, and reads the module globals through short
  offsets.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2807c --rebuilt work/rebuild/rcfmod.exe --rebuilt-map
  work/rebuild/race_frame.map --rebuilt-symbol race_frame_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, `3`, `4`, and `D`.  The paths cover the state 5 skip, the state 0
  path with the swap A, the state 2 path with the swap B, the state 3 path with
  both swaps, the state 4 path, the fade argument, and the two frame-flag
  conditions.
  Evidence: `python3 tools/rebuild/harness.py run --module race_frame`,
  `work/rebuild/rcf_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x2807c`.

## The game_main module at 0x10020

- Observation: the original body at `0x10020` holds 56 instructions and 194
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  371-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x10020 --count 56` and
  `tools/rebuild/functions/0x10020.json`.
- Observation: the function has a frame.  It saves `ebx`, `edx`, and `ebp`,
  sets `ebp`, and subtracts 4 from `esp`.  The rebuild uses the option `-of+`.
  Evidence: the prologue at `0x10020` through `0x10027`.
- Observation: the function calls `memory_alloc` at `0x30050` with the size
  `0x4c`.  When the result is not zero, it calls `list_create` at `0x2ecf8`
  with the result and the count `0x96`.  It stores the final result at
  `DAT_000062ac`.
  Evidence: the calls at `0x1002d` through `0x10040`.
- Observation: the function calls `memory_available` at `0x2f8ca` with the
  list pointer and compares the result with `0x61a800` (6400000).  When the
  result is lower, it prints two messages through `printf` at `0x3009b`.  The
  second message receives the shortfall `0x61a800 - memory_available(...)`.
  The format strings are at the object 3 offsets 4 and 0x29.  The image holds
  `\nNot enough memory to play SCREAMER\n` and `You need another %d bytes\n`.
  Evidence: the compare at `0x1004a`, the pushes at `0x10051` and `0x10070`,
  and the strings at file offset `0xf6805`.
- Observation: the normal path calls `engine_init` at `0x127a0`.  The loop
  tests `DAT_00007ed8` against 1, then calls `race_loop` at `0x27700` and
  `frame_update` at `0x27f94`.  A non-zero race result calls `bios_video` at
  `0x108e4` with the mode 3, `error_report` at `0x10978` with the code, and
  the run-time exit at `0x300be` with the code.  After the loop the function
  calls `engine_shutdown` at `0x128ac`.
  Evidence: the calls at `0x10084` through `0x100c2`.
- Observation: the cleanup guards on `DAT_000062ac`.  It calls `list_destroy`
  at `0x2ed77` with the pointer and the value 0, then passes the return value
  to `free` at `0x300ec`.  The nested form matches the original, because the
  second call reuses the pointer in `eax` and reloads nothing.
  Evidence: the block at `0x100cc` through `0x100d7`.
- Observation: the rebuild holds 55 instructions and 189 bytes.  The identity
  check stays open at 21 of 55 instructions and 20 of 189 bytes.  The compiler
  keeps the list pointer in `edx` and the race result in `edx` with no stack
  slot, while the original saves `ebx` and spills the result to `[ebp-4]`.
  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x10020 --rebuilt work/rebuild/gmmod.exe --rebuilt-map
  work/rebuild/game_main.map --rebuilt-symbol game_main_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, `3`, and `D`.  The paths cover the low-memory messages and the
  shortfall, the normal loop with the shutdown, the error path, and the failed
  allocation.
  Evidence: `python3 tools/rebuild/harness.py run --module game_main`,
  `work/rebuild/gm_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x10020`.

## The resource_find module at 0x2f25f

- Observation: the original body at `0x2f25f` holds 69 instructions and 199
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  199-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2f25f --count 69` and
  `tools/rebuild/functions/0x2f25f.json`.
- Observation: the function has a frame.  It saves `ebx`, `ecx`, `esi`, `edi`,
  and `ebp`, sets `ebp`, and subtracts 0x14 from `esp`.  The rebuild uses the
  option `-of+`.
  Evidence: the prologue at `0x2f25f` through `0x2f266`.
- Observation: the function walks the records of `*resource` up to the count at
  `+0x04`.  The record stride is 0x1c.  A key equal to the record field at
  `+0xc` calls `resource_free` at `0x2f470` with the resource and the index.
  Evidence: the loop at `0x2f288` through `0x2f2ac`.
- Observation: a non-zero free result returns -1.  Otherwise the function
  shifts the records after the match one slot to the left with the memmove at
  `0xd2066`.  The size is `(count - index) * 0x1c`.  Then it sets the result to
  0 and decrements the count at `+0x04`.
  Evidence: the block at `0x2f2be` through `0x2f2f5`.
- Observation: the status at `+0x0c` is 3 when the result is -1 and 0
  otherwise.  A missing key and a failed free return -1.
  Evidence: the block at `0x2f2f9` through `0x2f315`.
- Observation: the rebuild holds 53 instructions and 139 bytes.  The identity
  check stays open at 13 of 53 instructions and 1 of 139 bytes.  The compiler
  keeps the resource pointer in `ecx` and the index in `ebx`, uses `lea` for
  the next record, and folds the loop test, while the original spills both
  arguments to the frame and reloads them.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2f25f --rebuilt work/rebuild/rfndmod.exe --rebuilt-map
  work/rebuild/resource_find.map --rebuilt-symbol resource_find_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, `3`, and `D`.  The paths cover the match at index 0, the match at
  index 1, the missing key, and the failed free.  The harness copies the bytes
  in its memmove stub and checks the shifted record data.
  Evidence: `python3 tools/rebuild/harness.py run --module resource_find`,
  `work/rebuild/rfnd_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x2f25f`.

## The motion_update module at 0x21e6c

- Observation: the original body at `0x21e6c` holds 53 instructions and 202
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  212-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x21e6c --count 53` and
  `tools/rebuild/functions/0x21e6c.json`.
- Observation: the function has a frame.  It saves `ebx`, `edx`, and `ebp`, sets
  `ebp`, and subtracts 0xc from `esp`.  The rebuild uses the option `-of+`.
  Evidence: the prologue at `0x21e6c` through `0x21e71`.
- Observation: the function returns when the time base `DAT_00118e38` is below
  zero.  The compare is `0.0 > time`, and the `ja` at `0x21e7f` jumps to the
  shared epilogue.
  Evidence: the `fldz`, the `fcomp`, and the `ja` at `0x21e74` through
  `0x21e7f`.
- Observation: the function reads the three signed values `DAT_0011d2c0`,
  `DAT_0011d2bc`, and `DAT_0011d2c4`.  It multiplies each value by the time base
  and converts the result to an integer with the helper at `0x4d9f4`.  The
  helper rounds the value on the FPU stack toward zero.  The three results reach
  `view_move` at `0x1f94c` in `eax`, `edx`, and `ebx`.
  Evidence: the three `fild`/`fmul`/`call`/`fistp` groups at `0x21e85` through
  `0x21ebe` and the loads at `0x21ec1` through `0x21eca`.
- Observation: the function calls `video_present` at `0x27ec8` with the time
  base and the offset 0.
  Evidence: the pushes at `0x21ecf` and `0x21ed1` and the call at `0x21ed7`.
- Observation: the function advances the time base with
  `time = DAT_00118e08 * 2.0 + time`.  The constant 2.0 sits at the data slot
  `0x100d51`.  The function then returns while the new value is below 1.0.
  Evidence: the float block at `0x21edc` through `0x21f01` and the 2.0 value at
  module linear `0x100d51`.
- Observation: when the new value reaches 1.0, the function sets the time base
  to -1.0, calls `video_present` with 1.0 and the offset 0, and calls
  `view_move` with the three raw values.
  Evidence: the block at `0x21f03` through `0x21f2b`.
- Inference: the time base is a normalized animation phase.  The function
  interpolates the object position from the origin to the raw position, and it
  presents the palette at the matching scale.  At the phase end it restores the
  raw position and marks the phase with -1.0.  The next call returns at the
  negative guard until another writer starts a new phase.
  Evidence: the two scale paths, the 2.0 advance, and the -1.0 marker.
- Observation: the rebuild holds 48 instructions and 199 bytes.  The identity
  check stays open at 31 of 48 instructions and 84 of 199 bytes.  The compiler
  keeps the saved registers in another order, places the float compare as an
  integer compare, and reuses `fld1`.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x21e6c --rebuilt work/rebuild/mtnmod.exe --rebuilt-map
  work/rebuild/motion_update.map --rebuilt-symbol motion_update_`.
- Observation: the module uses the FPU, so the recipe links the shared
  `fpu_stub.asm`.
  Evidence: `tools/rebuild/modules/motion_update.json` and the build.
- Observation: a float literal of 2.0 in this harness got the constant-pool
  address 0, and the run read the wrong value.  The module now reads the 2.0
  value from the extern `DAT_00100d51`, and the harness defines it.  The
  behavior check passes.
  Evidence: the failing run of the first harness and the passing run after the
  change.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, and `D`.  The paths cover the negative guard, the normal scale and
  advance, and the wrap with the second raw move.
  Evidence: `python3 tools/rebuild/harness.py run --module motion_update`,
  `work/rebuild/mtn_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x21e6c`.

## The frame_update module at 0x27f94

- Observation: the original body at `0x27f94` holds 63 instructions and 212
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  232-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x27f94 --count 63` and
  `tools/rebuild/functions/0x27f94.json`.
- Observation: the function has a frame.  It saves `ebx`, `edx`, `esi`, `edi`,
  and `ebp`, sets `ebp`, and subtracts 8 from `esp`.  The rebuild uses the
  option `-of+`.
  Evidence: the prologue at `0x27f94` through `0x27f9b`.
- Observation: the function clears the 20 dwords at `DAT_0001fb08` with a local
  index.  It then sets `DAT_00020988` and the frame value `DAT_0011d24c` to 0.
  Evidence: the loop at `0x27f9e` through `0x27fbd` and the two stores at
  `0x27fc6` and `0x27fcc`.
- Observation: the function calls `memory_available` at `0x2f8ca` with the list
  pointer `DAT_000062ac` and stores the result at `DAT_00020880`.
  Evidence: the load at `0x27fc1`, the call at `0x27fd2`, and the store at
  `0x27fd7`.
- Observation: the function calls `menu_space_init` at `0x29d08` and
  `menu_frame` at `0x29f80`.
  Evidence: the calls at `0x27fdc` and `0x27fe1`.
- Observation: the counter `DAT_000207f8` increments and resets to 0 when the
  new value is above 5.  The counter therefore cycles through the six values 0
  to 5.
  Evidence: the block at `0x27fe6` through `0x27ff8`.
- Observation: the frame loop calls `frame_step` at `0x2aa84` and `race_frame`
  at `0x2807c`.  When the word at `DAT_00002dec + 0x48` is non-zero, the input
  poll at `0x26448` runs only when `DAT_0011d2c0` and `DAT_0011d2cc` are
  non-zero, and the music call at `0x2ff14` receives the record
  `DAT_000062b0`.  `video_fill` at `0x2aa98` runs on every turn.
  Evidence: the block at `0x28008` through `0x2803b`.
- Inference: `DAT_00002dec` is the race context pointer and the word at `+0x48`
  is an active flag.  The two input flags gate the local input poll, and
  `DAT_000062b0` is the music request record.
  Evidence: the context read, the two guards, and the fixed record address.
- Observation: the loop continues while the frame flag `DAT_0011d234` is 0.
  When the flag is not 0, the function calls `memset` at `0x8c160` with the
  destination `DAT_00121528`, the count `0x3e800`, and the value 0, then calls
  `video_fill` again and `race_cleanup` at `0x2a1b0`.
  Evidence: the test at `0x2803b`, the `memset` block at `0x28045` through
  `0x28051`, and the calls at `0x28056` and `0x2805b`.
- Observation: the rebuild holds 53 instructions and 189 bytes.  The identity
  check stays open at 14 of 53 instructions and 19 of 189 bytes.  OpenWatcom V2
  saves the registers in another order, uses registers for the key clear, and
  compares the context word and the frame flag directly.  The store records
  `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x27f94 --rebuilt work/rebuild/fupdmod.exe --rebuilt-map
  work/rebuild/frame_update.map --rebuilt-symbol frame_update_`.
- Observation: the module and the harness hold the float `DAT_0011d24c`, so the
  recipe links the shared `fpu_stub.asm`.
  Evidence: `tools/rebuild/modules/frame_update.json` and the build.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, `3`, and `D`.  The paths cover the full path with the input and
  music calls, the counter wrap that skips both calls, the input skip with the
  music call, and the three-turn loop.
  Evidence: `python3 tools/rebuild/harness.py run --module frame_update`,
  `work/rebuild/fupd_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x27f94`.

## The object_slide_step module at 0x1df34

- Observation: the original body at `0x1df34` holds 65 instructions and 215
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  216-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1df34 --count 65` and
  `tools/rebuild/functions/0x1df34.json`.
- Observation: the function has a frame.  It saves `edx` and `ebp`, sets `ebp`,
  and subtracts 8 from `esp`.  It stores the first argument from `eax` in the
  slot `[ebp-8]` and the slide in the slot `[ebp-4]`.  The rebuild uses the
  option `-of+`.
  Evidence: the prologue at `0x1df34` through `0x1df3e`.
- Observation: the guard is the low bit of the byte at `+0x25c`.  When the bit
  is clear, the slide is 0.
  Evidence: the `test` at `0x1df41`, the `je` at `0x1df48`, and the zero store
  at `0x1df77`.
- Observation: when the bit is set, the function computes
  `slide = (v * w) * (v * w) * (x - y)`, where `v`, `w`, `x`, and `y` are the
  floats at `+0x418`, `+0x2f4`, `+0x27c`, and `+0x280`.  The product `v * w` is
  squared with `fmul st(0)`.
  Evidence: the FPU block at `0x1df4d` through `0x1df72`.
- Observation: the function clamps the slide between 2.0 and -2.0.  The two
  float constants sit at module linear `0x100b4c` and `0x100b50`, in object 3.
  Evidence: the `fcomp` at `0x1df7f`, the store of `0x40000000` at `0x1df8a`,
  the `fcomp` at `0x1df96`, the store of `0xc0000000` at `0x1dfa1`, and the data
  at file offset `0xf734c`.
- Observation: the function calls `filter_update` at `0x22b80` three times.
  The values are `slide * 0.5`, `slide * 0.02`, and `-abs(slide * 0.02)`, and
  the targets are the floats at `+0x4c8`, `+0x4b8`, and `+0x4b0`.  The table
  indexes are `0x4f`, `0x4c`, and `0x4c`.  The doubles 0.5 and 0.02 sit at
  module linear `0x100b54` and `0x100b5c`.
  Evidence: the three call blocks at `0x1dfa8` through `0x1e001` and the data at
  file offset `0xf734c`.
- Inference: the function smooths the three parts of the object toward the
  slide offset.  The first part takes half of the slide, and the second and the
  third parts take the small rate with opposite signs.
  Evidence: the three arguments and the named callee `filter_update`.
- Observation: the rebuild holds 63 instructions and 213 bytes.  The identity
  check stays open at 4 of 63 instructions and 15 of 213 bytes.  The store
  records `rebuilt`.  The behavior check passes.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1df34 --rebuilt work/rebuild/oslmod.exe --rebuilt-map
  work/rebuild/object_slide_step.map --rebuilt-symbol object_slide_step_` and
  `python3 tools/rebuild/run.py run --function 0x1df34`.
- Observation: the module uses the FPU, so the recipe links the shared
  `fpu_stub.asm`.
  Evidence: `tools/rebuild/modules/object_slide_step.json` and the build.
- Observation: the first harness read the four float constants from the
  constant segment, which this harness does not load, so the clamp read the
  wrong values.  The module now reads the constants through the extern globals
  `slide_max`, `slide_min`, `slide_half`, and `slide_rate`, and the harness sets
  the exact bit patterns.  This follows the `palette_build` module.  The
  behavior check passes after the change.
  Evidence: the failing run of the first harness and the passing run after the
  change.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, `3`, and `D`.  The paths cover the guard clear, the high clamp, the
  low clamp, and a slide inside the range.
  Evidence: `python3 tools/rebuild/harness.py run --module object_slide_step`,
  `work/rebuild/osl_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x1df34`.

## The music_bank_load module at 0x2f9e0

- Observation: the original first block at `0x2f9e0` holds 72 instructions
  and 220 bytes up to the early `ret 4` at `0x2fab9`.  The merged store record
  keeps the 688-byte extent, and the function continues to the `ret 4` at
  `0x2fc8d`.  The plan row holds the first-block counts.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2f9e0 --count 72` and
  `tools/rebuild/functions/0x2f9e0.json`.
- Observation: the function has a frame.  It saves `esi`, `edi`, and `ebp`,
  sets `ebp`, subtracts `0xd4` from `esp`, and then subtracts `0x7e` from
  `ebp`.  The rebuild uses the option `-of+`.
  Evidence: the prologue at `0x2f9e0` through `0x2f9eb`.
- Observation: the function takes four arguments in `eax`, `edx`, `ebx`, and
  `ecx`, and the fifth argument from the stack.  It clears the field `+0x04`
  to -1, stores the fifth argument at `+0x98`, and copies the `ecx` name to
  `+0x18`.
  Evidence: the four argument slots at `0x2f9ee` through `0x2f9f7`, the two
  field stores at `0x2f9fd` and `0x2fa0d`, and the copy loop at `0x2fa1d`
  through `0x2fa33`.
- Observation: the first call at `0xd25d7` takes three zero arguments.  A
  non-zero result sets the status at `+0x10` to 1 and jumps to the epilogue.
  Evidence: the three zero loads at `0x2fa36` through `0x2fa3a`, the call at
  `0x2fa3c`, and the status store at `0x2fa48`.
- Observation: the function clears the 0x24-byte state buffer at object-1
  offset `0x20b7c` through the `memset` at `0xd1ec0`, copies the third
  argument to a local, and calls `0xd2783` with the second argument and the
  bank field `+0x08`.  The call passes two far pointers to the state buffer
  and to the bank field.  A non-zero result clears the handle at `0x2234c`,
  calls `0xd2743`, sets the status to 2, and returns at `0x2fab9`.
  Evidence: the `memset` call at `0x2fa60`, the argument store at `0x2fa68`,
  the far-pointer pushes at `0x2fa71` through `0x2fa7e`, the call at
  `0x2fa88`, and the error block at `0x2fa96` through `0x2fab9`.
- Observation: the success path opens `melodic.bnk` and then `drum.bnk`
  through the member open `0x4c08d` with the category `rb`.  It reads each
  member with `0x4c4e5` into a resource from `0x2efc2`, and stores the two
  resource pointers in the globals `0x20b6c` and `0x20b74`.  It stores the
  stream status after each read in `0x20b70` and `0x20b78`, and it closes each
  stream with `0x4c38d`.
  Evidence: the two open blocks at `0x2facc` and `0x2fb56`, the two read
  blocks at `0x2fae3` through `0x2fb3e` and `0x2fb6a` through `0x2fbc5`, and
  the module globals.
- Observation: the strings sit in object 3.  The offsets `0x14ec` and
  `0x1508` hold `rb`, `0x14ef` holds `melodic.bnk`, `0x14fb` holds
  `MELODIC_BANK`, `0x150b` holds `drum.bnk`, and `0x1514` holds `DRUM_BANK`.
  Evidence: the data dump at linear `0x1014ec` through `0x101514`.
- Observation: for the driver values `0xa002` and `0xa009`, the function
  installs the two resources through `0xd2ed9` and ORs the two results into
  the status at `+0x10`.
  Evidence: the two comparisons at `0x2fbca` and `0x2fbd3`, and the two call
  blocks at `0x2fbdc` and `0x2fc02`.
- Observation: a non-zero status releases `MELODIC_BANK` and `DRUM_BANK`
  through `0x2f326`, clears the handle at `0x2234c`, calls `0xd2743`, sets the
  status to 3, and returns.  The zero status calls `0xd3d14` with the value
  `0x7f`, sets the active flag at `+0x14` to 1, and returns the status 0.
  Evidence: the release block at `0x2fc21` through `0x2fc6a` and the success
  block at `0x2fc6d` through `0x2fc84`.
- Inference: the statuses 1, 2, and 3 name the sound-system failure, the
  driver-open failure, and the bank failure.  `music_start` reports the
  non-zero status and terminates the game.
  Evidence: the three status stores and the `music_start` message chain.
- Observation: the rebuild holds 145 instructions and 592 bytes up to its
  single return.  OpenWatcom V2 merges the returns into one epilogue, so the
  first return does not sit at the early block.  The identity check stays open
  at 19 of 72 instructions and 15 of 220 bytes.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2f9e0 --rebuilt work/rebuild/mblmod.exe --rebuilt-map
  work/rebuild/music_bank_load.map --rebuilt-symbol music_bank_load_`.
- Observation: the rebuild passes near pointers to `0xd2783` in place of the
  original far pointers and segment.  The flat target needs no far pointer.
  This difference does not change the behavior in the harness.
  Evidence: `tools/rebuild/music_bank_load.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `6` and `D`.  The paths cover the success path, the sound-system
  failure, the driver-open failure, the install failure for `0xa002`, the
  install success for `0xa009`, the zero melodic resource, and the failed
  melodic open.
  Evidence: `python3 tools/rebuild/harness.py run --module music_bank_load`,
  `work/rebuild/mbl_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x2f9e0`.

## The camera_update module at 0x8ac78

- Observation: the original body at `0x8ac78` holds 64 instructions and 225
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  4468-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x8ac78 --count 64` and
  `tools/rebuild/functions/0x8ac78.json`.
- Observation: the original has no EBP frame.  It saves all general registers
  with `pushal`, saves `ds`, `es`, `fs`, and `gs`, stores the argument in the
  global `0x7a7ac`, and sets `ds` from the selector at `cs:[0x214d4]`.  The
  rebuild does not use `-of+`.
  Evidence: the prologue at `0x8ac78` through `0x8ac8c`.
- Observation: the function calls the transform at `0x8b610` with the pointer
  and the projection at `0x82c14`.  The projection reads the same pointer from
  the global `0x7a7ac`.
  Evidence: the two calls at `0x8ac94` and `0x8ac99`, and the read at `0x82c23`.
- Observation: the record holds a 3x3 integer basis at `+0x18`, the position
  vector at `+0x3c`, a second 3x3 integer basis at `+0x48`, and the velocity
  vector at `+0x6c`.  The function copies the position to `saved_position` at
  `0x7ad94`, `0x7ad98`, and `0x7ad9c`, clears the velocity and then the
  position, and scales the nine values of the second basis by the float at
  `0x7ad90` into the nine floats at `0x7ad60`.
  Evidence: the field stores at `0x8aca4` through `0x8ad4d`.
- Observation: the nine basis values are visited in the order 0, 3, 6, 1, 4, 7,
  2, 5, 8.  The 27 FPU instructions are nine `fild`, `fmul`, and `fstp` groups.
  Evidence: the FPU block at `0x8ace5` through `0x8ad4d`.
- Inference: the transform and the projection prepare the screen position
  before the function saves the position and resets the motion state.  The
  scaled basis is the camera basis for the next frame.
  Evidence: the call order and the saved position.
- Observation: the rebuild holds 46 instructions and 221 bytes.  The identity
  check stays open at 7 of 46 instructions and 18 of 221 bytes.  The rebuilt
  prologue saves only `edx`, and the rebuilt code keeps the pointer in `edx`
  and stores the clears as immediate fields of the record.  The store records
  `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x8ac78 --rebuilt work/rebuild/camupd.exe --rebuilt-map
  work/rebuild/camera_update.map --rebuilt-symbol camera_update_`.
- Observation: the module uses the FPU, so the recipe links the shared
  `fpu_stub.asm`.
  Evidence: `tools/rebuild/modules/camera_update.json` and the build.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `8` and `D`.  The paths cover the scale 0.5, the scale -2.0, and the
  scale 1.0.  The transform and projection stubs check the pointer.
  Evidence: `python3 tools/rebuild/harness.py run --module camera_update`,
  `work/rebuild/cam_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x8ac78`.

## The stream_read module at 0x4c4e5

- Observation: the original body at `0x4c4e5` holds 80 instructions and 226
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  226-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4c4e5 --count 80` and
  `tools/rebuild/functions/0x4c4e5.json`.
- Observation: the original uses an EBP frame.  It saves `ecx`, `esi`, `edi`,
  and `ebp`, reserves 28 bytes, and stores the three register arguments at
  `[ebp - 0xc]`, `[ebp - 8]`, and `[ebp - 4]`.  The rebuild uses `-of+`.
  Evidence: the prologue at `0x4c4e5` through `0x4c4fa`.
- Observation: a set flag at `stream + 0x10` runs the compressed path.  The
  path adds the count to the position at `stream + 0x0c`, clamps the count to
  the size at `stream + 0x08`, sets the position to the size, and calls the
  expander at `0x4d3fc`.  The expander receives the buffer at `stream + 0xb0`,
  the output buffer, the field at `stream + 0x6c`, the count, and the handle at
  `stream + 0x00` as the fifth stack argument.
  Evidence: the block at `0x4c4fa` through `0x4c55d`.
- Observation: a clear flag at `stream + 0x10` runs the raw path.  The path
  calls `fread(buffer, 1, count, handle)` at `0xc607b`, adds the byte count to
  the position, and clamps the result and the position to the size.
  Evidence: the block at `0x4c55f` through `0x4c5b4`.
- Inference: the position is the number of bytes consumed, and the size is the
  total stream length.  The clamp keeps a short read at the stream end from
  moving the position past the size.
  Evidence: the two clamp blocks.
- Observation: the rebuild holds 51 instructions and 119 bytes.  The identity
  check stays open at 22 of 51 instructions and 4 of 119 bytes.  OpenWatcom V2
  keeps the arguments in `esi` and `edi` instead of the stack slots, so the two
  bodies differ.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x4c4e5 --rebuilt work/rebuild/srdmod.exe --rebuilt-map
  work/rebuild/stream_read.map --rebuilt-symbol stream_read_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `4` and `D`.  The paths cover the compressed path, the compressed
  clamp, the raw path, the raw clamp, and the read of zero bytes.
  Evidence: `python3 tools/rebuild/harness.py run --module stream_read`,
  `work/rebuild/srd_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x4c4e5`.

## The malloc module at 0xd1f25

- Observation: the original body at `0xd1f25` holds 84 instructions and 227
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  227-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0xd1f25 --count 84` and
  `tools/rebuild/functions/0xd1f25.json`.
- Observation: the original has an EBP frame.  It saves `ebx`, `ecx`, `edx`,
  `esi`, `edi`, `es`, `fs`, `gs`, and `ebp`, reserves four bytes for the grow
  flag, and passes the first argument in `eax`.  The rebuild uses `-of+`.
  Evidence: the prologue at `0xd1f25` through `0xd1f33`.
- Observation: the function rejects the size 0 and every size above
  `0xffffffd4`.  It rounds the size up to a multiple of 4 and raises a small
  size to 12.  The early return writes no heap flag.
  Evidence: the size checks at `0xd1f35` and `0xd1f39`, the round-up at
  `0xd1f4a`, the minimum at `0xd1f5a`, and the early return at `0xd1f3e`.
- Observation: the function walks the free list at `0x31d0`.  It stores the
  cursor in `0x31d4` and the largest failed block size in `0x31d8`.  A size at
  or below `0x31d8` restarts the walk at the head and clears `0x31d8`.  A larger
  size resumes at the cursor `0x31d4` when the cursor is set.
  Evidence: the branch at `0xd1f64`, the head reload at `0xd1f7c`, the cursor
  load at `0xd1f6c`, and the loop test at `0xd1fbc`.
- Observation: the block walk asks the helper at `0xe09dc` for a block.  The
  helper receives the requested size in `eax`, the data segment in `edx`, and
  the block in `ebx`.  A null result updates the largest failed size and moves
  to the next block at `+0x08`.
  Evidence: the call at `0xd1f9c` and the size read at `0xd1fac`.
- Observation: when the walk fails, a clear flag calls the grow helper at
  `0xe0f73` once.  A non-zero result sets the flag and restarts the walk.  The
  second walk failure calls the helper at `0xe112e`, and the branch at
  `0xd1fdc` restarts the walk after a non-zero result.  The helper at `0xe112e`
  is `xor eax,eax` and `ret` in this image, so the branch is not reachable in
  the shipped binary.
  Evidence: the grow block at `0xd1fc0` through `0xd1fea` and the helper at
  `0xe112e`.
- Inference: the flag at `[esp]` records the single grow attempt.  The second
  helper is a final chance that the shipped image does not supply.
  Evidence: the flag store at `0xd1f51`, the set at `0xd1fe6`, and the clear at
  `0xd1fdc`.
- Observation: a success returns the block and clears the heap flag at
  `0x27de5`.  The invalid-size return skips the flag clear.
  Evidence: the success block at `0xd1fef` and the early return at `0xd1f40`.
- Observation: the rebuild holds 70 instructions and 195 bytes.  The identity
  check stays open at 15 of 70 instructions and 27 of 195 bytes.  OpenWatcom V2
  keeps the size and the pointer in registers, lays the free-list loop out
  differently, and merges the clears, so the two bodies differ.  The store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0xd1f25 --rebuilt work/rebuild/mlcmod.exe --rebuilt-map
  work/rebuild/malloc_rt.map --rebuilt-symbol malloc_rt_`.
- Observation: the flat target has no data segment.  The original passes the
  data segment to the helper at `0xe09dc`, and the rebuild passes 0.  The helper
  uses the near block pointer.  This difference does not change the behavior in
  the harness.
  Evidence: `tools/rebuild/malloc_rt.c` and the call at `0xd1f9c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `8` and `D`.  The paths cover the head restart, the size 0, the
  oversized request, the failed first block, the cursor resume, the grow retry,
  the expand retry, the double failure, and the minimum size.
  Evidence: `python3 tools/rebuild/harness.py run --module malloc_rt`,
  `work/rebuild/mlc_out.txt`, and `python3 tools/rebuild/run.py run --function
  0xd1f25`.

## The replay_write module at 0x22f54

- Observation: the original body at `0x22f54` holds 64 instructions and 228
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  228-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x22f54 --count 64` and
  `tools/rebuild/functions/0x22f54.json`.
- Observation: the original has an EBP frame.  It saves `ebx`, `ecx`, `edx`,
  `edi`, and `ebp`, and reserves four bytes for the record slot.  The rebuild
  uses `-of+`.
  Evidence: the prologue at `0x22f54` through `0x22f5b`.
- Observation: the enable flag `DAT_00118e60` guards the body.  A zero flag
  skips the write and the index update.
  Evidence: the test at `0x22f5e` and the jump at `0x22f65`.
- Observation: the function writes the 20-byte record at
  `DAT_00118e5c + DAT_00118e64 * 0x14`.  It copies the two float fields at
  `+0x00` and `+0x04` with `fld` and `fstp`, and the six signed 16-bit fields at
  `+0x08`, `+0x0a`, `+0x0c`, `+0x0e`, `+0x10`, and `+0x12` from
  `DAT_00119880` through `DAT_00119894`.
  Evidence: the copies at `0x22f88` through `0x22fee`.
- Observation: the index advances by one.  When the new index equals the end
  marker `DAT_00118e68`, the function decrements the index again, so the writer
  stops at the last slot.  The second marker `DAT_00118e70` becomes
  `DAT_00118e68 - 2` when the index is the last slot, and equals the index
  otherwise.
  Evidence: the advance at `0x22ff2` through `0x2302b`.
- Observation: the rebuilt module holds 58 instructions and 219 bytes.  The
  identity check stays open: 24 of 58 instructions and 16 of 219 bytes match.
  OpenWatcom V2 saves fewer registers and computes the record offset with one
  `imul`, so the two bodies differ.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x22f54 --rebuilt work/rebuild/rpwrmod.exe --rebuilt-map
  work/rebuild/replay_write.map --rebuilt-symbol replay_write_`.
- Observation: the record pointer is volatile.  The original reloads the pointer
  from the frame before every field copy.  This reload keeps the same shape in
  the rebuild.
  Evidence: the loads at `0x22f8e` through `0x22feb` and
  `tools/rebuild/replay_write.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `3` and `D`.  The paths cover the clear enable flag, an ordinary
  index, an index one below the end, and an index whose advance reaches the
  end.
  Evidence: `python3 tools/rebuild/harness.py run --module replay_write`,
  `work/rebuild/rpwr_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x22f54`.

## The object_load module at 0x10bfc

- Observation: the plan row records a 232-byte first block of 69 instructions
  and 5 calls.  The function continues past the early return, and the merged
  store record keeps the 728-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x10bfc --count 69` and
  `tools/rebuild/functions/0x10bfc.json`.
- Observation: the original has an EBP frame.  It saves `ecx`, `esi`, `edi`, and
  `ebp`, reserves 0x100 bytes, and lowers `ebp` by 0x82.  The rebuild uses
  `-of+`.
  Evidence: the prologue at `0x10bfc` through `0x10c08`.
- Observation: the function scans `DAT_00021540` for the first handle slot
  whose value is -1.
  Evidence: the walk at `0x10c17` through `0x10c3b`.
- Observation: the function walks the 0x1c-byte records from the start index to
  the start index plus the count.  The load flag at `+0x14` equal to 1 selects a
  record.
  Evidence: the loop test at `0x10e1d` through `0x10e41` and the stride at
  `0x10e2c`.
- Observation: a selected record builds `%s.3DX` from the record name, copies
  the name to `DAT_00007fe8`, and opens the member through `0x4c08d` with the
  base path `DAT_00007f80` and the mode `rb` at `0x1003b3`.
  Evidence: the name build at `0x10e43` through `0x10e64`, the copy at
  `0x10e8d` through `0x10ea4`, and the open at `0x10ea7`.
- Observation: a failed open returns 2.
  Evidence: the test at `0x10eaf` and the return at `0x10eb7`.
- Observation: the function reads the file size from `stream + 0x70` through
  `0x4c7e0`, loads the `3D_HEADER` resource through `0x2eed0`, and reads the
  whole file into it through `0x4c4e5`.  A missing header returns 3.
  Evidence: the status call at `0x10c4f`, the resource call at `0x10c64`, the
  read at `0x10c85`, and the return at `0x10c72`.
- Observation: the object count at `+0x14` of the loaded data gives the block
  size `count * 0x20 + 0x60`.  The function loads `3D_BLKOBJ` with that size
  through `0x2eed0`.  A missing block returns 5.
  Evidence: the count at `0x10c95`, the size at `0x10c9b` through `0x10ca4`,
  the look-up at `0x10cb4`, and the return at `0x10cd4`.
- Observation: the function clears the six list words at `+0x24`, `+0x28`,
  `+0x2c`, `+0x30`, `+0x34`, and `+0x38` of the block object.
  Evidence: the six stores at `0x10cf7` through `0x10d7e`.
- Observation: the function stores the loaded file in the handle table, sets
  the record field at `+0x14` to the file, advances the slot, writes the -1
  marker, binds the block through `0x4cc10`, and stores `block + 0x40` at
  `record + 0x10`.
  Evidence: the handle store at `0x10d81` through `0x10dc6`, the bind at
  `0x10de5`, and the chunk link at `0x10e14`.
- Observation: the rebuilt module holds 104 instructions and 390 bytes.  The
  identity check stays open: 14 of 69 instructions and 11 of 232 bytes match.
  The plan row and the identity check compare only the first block, and the
  rebuild lays out the early return, the frame, and the loops differently.  The
  store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x10bfc --rebuilt work/rebuild/objlmod.exe --rebuilt-map
  work/rebuild/object_load.map --rebuilt-symbol object_load_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `5` and `D`.  The paths cover the success, the free-slot search with
  the skipped record, the open failure, the missing header, the missing block,
  and the skipped record.
  Evidence: `python3 tools/rebuild/harness.py run --module object_load`,
  `work/rebuild/objl_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x10bfc`.
- Observation: the harness selects the two resources by call order.  In this
  DOS/4G run the literal bytes at the two name pointers read as zero, so a
  content compare cannot separate them.  The stub checks instead that the two
  name pointers differ.
  Evidence: `tools/rebuild/object_load_main.c` and the run output in
  `work/rebuild/objl_out.txt`.

## The mouse_init module at 0x1f1a0

- Observation: the plan row records a 233-byte body of 67 instructions and 12
  calls.  The merged store record keeps the 236-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1f1a0 --count 67` and
  `tools/rebuild/functions/0x1f1a0.json`.
- Observation: the original has an EBP frame.  It saves `ebx`, `ecx`, `edx`,
  and `ebp`, reserves 8 bytes, and restores the stack with `mov esp,ebp`.
  Evidence: the prologue at `0x1f1a0` through `0x1f1a8` and the epilogue at
  `0x1f282` through `0x1f288`.
- Observation: the first call at `0xc8245` receives the rate `0xff00` in `eax`
  and the value 0 in `edx`.
  Evidence: the stores at `0x1f1a9` through `0x1f1b0`.
- Observation: the first timer install at `0xc835b` receives the rate `0x64`,
  the code handler `CS:0x20d`, and the id slot `DAT_00018d8c`.
  Evidence: the argument setup at `0x1f1b5` through `0x1f1c7`.
- Observation: a non-zero timer install status calls `exit` at `0x300be` with
  the value -1.
  Evidence: the test at `0x1f1cc` and the call at `0x1f1d5`.
- Observation: when `DAT_00008058` is zero, the function returns before the
  mouse calls.
  Evidence: the test at `0x1f1da` and the branch to `0x1f282`.
- Observation: the mouse system start at `0xc8f81` receives three zero values.
  Evidence: the three clears at `0x1f1e7` through `0x1f1ed`.
- Observation: the function stores the value `0x2b11` at `DAT_00001e50`.
  Evidence: the store at `0x1f20a`.
- Observation: the mouse handler install at `0xc903a` receives the setup word
  `DAT_00008058`, the handler `DAT_00008060`, the state block `DAT_00001e40`,
  and the id slot `DAT_00018d94`.
  Evidence: the argument setup at `0x1f1f2` through `0x1f215`.
- Observation: a non-zero handler install status runs `0xc8321` with 0, then
  `0xc8ffa`, and then `exit` at `0x300be` with the value 1.
  Evidence: the test at `0x1f220` through the call at `0x1f235`.
- Observation: the second timer install at `0xc835b` receives the rate `0x1e`,
  the far pointer `DAT_00001e5c`, and the id slot `DAT_00018d90`.
  Evidence: the argument setup at `0x1f23a` through `0x1f252`.
- Observation: a non-zero second timer status runs `0xc8321` with 0, then
  `0xc98f9` with `DAT_00018d94`, 1, and 1, and then `exit` at `0x300be` with
  the value 1.
  Evidence: the test at `0x1f25a` through the call at `0x1f27d`.
- Inference: `0xc8245` and `0xc835b` are timer calls, and `0xc8f81`,
  `0xc8ffa`, `0xc903a`, and `0xc98f9` are mouse calls.  The store records for
  these addresses stay unknown.
  Evidence: the register use, the call order, and the two EBP-frame call sites
  at `0x1f28c`.
- Correction: an earlier finding names `CS:0x18D8C` as the mouse handler.  The
  disassembly shows the code handler at `CS:0x20D`.  The value at
  `DS:0x18D8C` is the timer id.
  Evidence: the pushes at `0x1f1b5` through `0x1f1c0` and the handler code at
  `0x1020d`.
- Observation: the rebuilt module holds 62 instructions and 219 bytes.  The
  identity check stays open: 19 of 62 instructions and 24 of 219 bytes match.
  The store records the status `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1f1a0 --rebuilt work/rebuild/msimenu.exe --rebuilt-map
  work/rebuild/mouse_init.map --rebuilt-symbol mouse_init_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `4` and `D`.  The paths cover the two exit paths, the zero setup word,
  the success path, and the failed handler install.
  Evidence: `python3 tools/rebuild/harness.py run --module mouse_init`,
  `work/rebuild/msimenu.txt`, and `python3 tools/rebuild/run.py run --function
  0x1f1a0`.
- Limit: the library function names are inferred from the register use and the
  call order.  The original has no symbol name for these calls.

## The sound_effects_update module at 0x81ab0

- Observation: the plan row records a 235-byte body of 51 instructions and 10
  calls.  The body starts at `0x81ab0` and ends at the `ret` at `0x81b9a`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x81ab0 --count 51`.
- Observation: the merged store record keeps the 37320-byte extent.  The diff
  uses the body that ends at the first `ret`, not the record size.
  Evidence: `tools/rebuild/functions/0x81ab0.json` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x81ab0 --rebuilt work/rebuild/seumod.exe
  --rebuilt-map work/rebuild/sound_effects_update.map --rebuilt-symbol
  sound_effects_update_`.
- Observation: the neighbour `sound_update` starts at `0x81a70` and joins the
  shared body at `0x81ac6`.  The module starts at `0x81ab0`.  The module holds
  the two setup calls of its own entry and the shared body.  It does not hold
  the `0x81a70` entry.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x81a70 --count 80`.
- Observation: the entry stores the argument in the global `0x7451c` and calls
  the two helpers at `0x83ca8` and `0x83354`.  The body then calls the source
  update at `0x8a958`.
  Evidence: the stores and calls at `0x81ab0` through `0x81ac6`.
- Observation: the function returns when the active count `0x72488` is zero or
  negative.  Otherwise it calls the mix prepare at `0x8aa60` and returns when
  the voice count `0x7aa9c` is zero or negative.
  Evidence: the tests at `0x81acb` through `0x81aea`.
- Observation: the helper at `0x83354` writes the active count `0x72488`.
  Evidence: the store at `0x8339e`.
- Observation: the function calls `0x83500` with `edi` equal to `0x45790` and
  `eax` equal to the voice count.  It then stores `0x45790` at `0x7251c` and
  the voice count at `0x72c10`.
  Evidence: the setup at `0x81add` through `0x81b04`.
- Observation: the outer loop calls `0x8af08` and sets the index `0x7445c` to
  zero.  The helper at `0x8af08` calls `0x8adac`, which writes the voice count
  `0x74458`.
  Evidence: the loop at `0x81b09` through `0x81b8c` and the stores at
  `0x8ae92` and `0x8aed5`.
- Observation: the inner loop calls `0x8a8d8` and tests `edx`.  When `edx` is
  non-zero, it calls `0x835c0`, stores `0x694f0` at `0x694ec` and `0x17700000`
  at `0x7b804` and `0x7b814`, calls `0x836ac`, loads the source pointer
  `0x4d3c8` in `esi`, and calls `0x843a8`.
  Evidence: the event path at `0x81b2c` through `0x81b6b`.
- Observation: the inner loop increments the index `0x7445c` and decrements the
  voice count `0x74458` until zero.  The outer loop decrements the frame repeat
  `0x72c10` until zero.
  Evidence: the loop tails at `0x81b78` through `0x81b8c`.
- Observation: the original sets DS and ES from the selector at `CS:0x214d4`
  and uses a CS override for the selector read.  The rebuild omits the segment
  loads, because the flat target keeps one data segment.
  Evidence: the instructions at `0x81b09`, `0x81b20`, `0x81b3a`, and
  `0x81b70`.
- Observation: the rebuilt module holds 47 instructions and 198 bytes.  The
  identity check stays open: 15 of 47 instructions and 12 of 198 bytes match.
  The store records the status `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x81ab0 --rebuilt work/rebuild/seumod.exe --rebuilt-map
  work/rebuild/sound_effects_update.map --rebuilt-symbol
  sound_effects_update_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `4` and `D`.  The paths cover the two guards, the two frames with
  index reset, the event skip at one index, and the empty event path.
  Evidence: `python3 tools/rebuild/harness.py run --module sound_effects_update`,
  `work/rebuild/seu_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x81ab0`.
- Inference: `0x83ca8` is a timer helper, `0x83354` is a sequence helper,
  `0x8a958` is a source update, `0x8aa60` is a mix prepare, `0x83500` is a
  voice mix, `0x8af08` is a frame prepare, `0x8a8d8` is an event next helper,
  `0x835c0` is an event transform, `0x836ac` is an event render, and `0x843a8`
  is an event convert.  The names come from the register use and the call
  order.  The store records for these addresses stay unknown.
  Evidence: the ten call sites and the disassembly of each callee.
- Limit: the original has no symbol name for these calls.  The helper names in
  the module stay inferred.

## The input_map module at 0x211e0

- Observation: the plan row records a 239-byte body of 62 instructions and 2
  calls.  The body starts at `0x211e0` and ends at the `ret` at `0x212ce`.
  The `nop` at `0x212cf` is padding.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x211e0 --count 62`.
- Observation: the merged store record keeps the 240-byte extent.  The diff
  uses the body that ends at the first `ret`, not the record size.
  Evidence: `tools/rebuild/functions/0x211e0.json` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x211e0 --rebuilt work/rebuild/inpmod.exe
  --rebuilt-map work/rebuild/input_map.map --rebuilt-symbol input_map_`.
- Observation: the control byte at `0x18e1d` is the high byte of the input
  word at `0x18e1c`.
  Evidence: the word load at `0x21228` and the byte load at `0x212b3`.
- Observation: bit `0x20` of the control byte requests the race exit.  When
  one of the four flags `0x3b`, `0x1e`, `0x3`, and `0x19880` is set, the
  function sets bit 1 of the exit byte `0x1ce44`.  When all four flags are
  zero, the function returns.
  Evidence: the tests and the store at `0x211ea` through `0x21223`.
- Observation: the input word bit mask `0x401c` returns before the rest of the
  function.
  Evidence: the test at `0x2122f` and the jump at `0x21234`.
- Observation: the function calls `0x21338` and clears the dword `0x7f9` when
  the overlay flag `0x1b` is set and the mode `0x1d260` is not `0x10`.  The
  callee at `0x21338` saves and restores `ecx`, so the store writes zero.
  Evidence: the call at `0x2124f`, the store at `0x21254`, and the callee
  epilogue at `0x219e8`.
- Observation: the release flag `0x34` sets the overlay state `0x18f90` and
  advances the counter `0x1d2a4` from 0 through 3 only when `0x18f90` is zero.
  A clear release flag sets `0x18f90` to zero.
  Evidence: the tests and the stores at `0x2125a` through `0x21293`.
- Observation: the reset gate `0x3` stores the dead local `0xffffff`, calls
  `0x101b4` with the value 1, and sets the input bits `0x220` when the control
  byte holds bit 0.  The helper `0x101b4` adds `0x113` to its argument and
  clears the dword at the result.
  Evidence: the stores and the call at `0x212a2` through `0x212be`, and the
  helper at `0x101b4`.
- Observation: the rebuilt module holds 58 instructions and 233 bytes.  The
  identity check stays open: 13 of 58 instructions and 35 of 233 bytes match.
  The register save order and the missing `mov dl`, `mov cl`, `mov dx`, and
  `mov ah` forms cause the difference.  The store records the status `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x211e0 --rebuilt work/rebuild/inpmod.exe --rebuilt-map
  work/rebuild/input_map.map --rebuilt-symbol input_map_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `7` and `D`.  The paths cover the normal path, the two exit-request
  paths, the input mask return, the mode 0x10 skip, the counter wrap, the clear
  release flag, and the clear control bit.
  Evidence: `python3 tools/rebuild/harness.py run --module input_map`,
  `work/rebuild/inp_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x211e0`.
- Inference: `0x21338` is an overlay update, and `0x101b4` is an input reset
  helper.  The names come from the register use and the call order.  The store
  records for these addresses stay unknown.
  Evidence: the two call sites and the disassembly of each callee.
- Limit: the original has no symbol name for these calls.  The helper names in
  the module stay inferred.

## The object_proximity_check module at 0x1e30c

- Observation: the plan row records a 241-byte body of 74 instructions, 3
  calls, and 13 FPU instructions.  The body starts at `0x1e30c` and ends at the
  first `ret` at `0x1e3fc`.  The second `ret` at `0x1e411` closes the outside
  branch.  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1e30c --count
  74`.
- Observation: the merged store record keeps the 264-byte extent.  The diff
  uses the body that ends at the first `ret`, not the record size.
  Evidence: `tools/rebuild/functions/0x1e30c.json` and
  `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1e30c --rebuilt work/rebuild/opcmod.exe --rebuilt-map
  work/rebuild/object_proximity_check.map --rebuilt-symbol
  object_proximity_check_`.
- Observation: the guard compares the absolute value of the delta between
  `+0x498` and `DAT_00113ccc` with the immediate `0x88b8`.  A value of 35000 or
  more jumps to the outside branch, which stores zero at `+0x414` and returns.
  The same test then runs on `+0x4a0` and `DAT_00113cd0`.
  Evidence: the `cdq`, `xor`, and `sub` absolute-value sequence and the `cmp
  eax, 0x88b8` at `0x1e32a` through `0x1e355`.
- Observation: inside the range, the function forms `DAT_00113ccc - +0x498`
  and `DAT_00113cd0 - +0x4a0`, scales each delta by the double 0.1 at object 3
  offset `0xb88`, and truncates it to an integer with the helper at `0x4d9f4`.
  It squares and adds the two integers, calls `0x22bec`, scales the result by
  the double 10.0 at offset `0xb90`, and stores the float at `+0x4e4`.  It
  then compares that float with the float 35000.0 at offset `0xb98` and sets
  `+0x414` to the result.
  Evidence: the FPU sequence at `0x1e35b` through `0x1e3ef`, and the data at
  the object 3 offsets `0xb88`, `0xb90`, and `0xb98`.
- Observation: `0x22bec` returns zero for a negative argument and otherwise
  calls `0xc6cbe`, so it is a square-root helper.
  Evidence: the disassembly of `0x22bec`.
- Observation: the shared `fpu_stub.asm` defines `__CHP` as a bare return.
  The compiled `(int)` cast then rounds to nearest, so a negative delta rounds
  away from zero.  The original helper at `0x4d9f4` sets the truncate control
  word.  The module therefore uses the local `object_proximity_check_fpu.asm`,
  which keeps the no-op init symbols and makes `__CHP` truncate.
  Evidence: the `__CHP` stub in `fpu_stub.asm`, the helper at `0x4d9f4`, and
  the rebuilt call at `0x100d6`.
- Observation: the rebuilt module holds 59 instructions and 210 bytes.  The
  identity check stays open: 7 of 59 instructions and 12 of 210 bytes match.
  The register save order and the instruction selection cause the difference.
  The store records the status `rebuilt`.
  Evidence: the diff command above and `python3 tools/rebuild/run.py run
  --function 0x1e30c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `5` and `D`.  The paths cover a small inside position, the two
  single-axis guards, the distance at the limit, the distance above the limit,
  and the negative deltas that truncate toward zero.
  Evidence: `python3 tools/rebuild/harness.py run --module
  object_proximity_check`, `work/rebuild/opc_out.txt`, and `python3
  tools/rebuild/run.py run --function 0x1e30c`.
- Inference: `0x4d9f4` is the truncating FPU helper and `0x22bec` is the square
  root.  The names come from the instruction sequence and the call order.  The
  store records for these addresses stay unknown.
  Evidence: the two call sites and the disassembly of each callee.
- Limit: the original has no symbol name for these calls.  The helper names in
  the module stay inferred.

## The resource_get module at 0x2eed0

- Observation: the original body at `0x2eed0` holds 78 instructions and 242
  bytes.  The plan row holds the same counts.  The merged store record keeps
  the 242-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2eed0 --count 78` and
  `tools/rebuild/functions/0x2eed0.json`.
- Observation: the function reads the resource structure.  The record array is
  at `+0x00`, the count at `+0x04`, the limit at `+0x08`, the status at `+0x0c`,
  and the name buffer at `+0x10`.
  Evidence: the loads and stores at `0x2eedc` through `0x2eef3`.
- Observation: when the count equals the limit, the function sets the status at
  `+0x0c` to 2, copies the name to `resource + 0x10`, and returns 0.  The copy
  goes through the `sprintf` call at `0x4d32c` with the format `%.11s`.
  Evidence: the branch at `0x2eef1`, the call at `0x2ef0e`, and the literal
  `0x14c5`.
- Observation: the literals `0x14c5`, `0x14cb`, `0x14d1`, and `0x14d7` are
  four copies of `%.11s` in the data object at base `0x100000`.  The code holds
  the object offset, and the loader adds the object base.
  Evidence: the data dump at file offsets `0xf7cc5` through `0xf7cd7`.
- Observation: the normal path selects the record at
  `*resource + count * 0x1c`.  A size below `0x96000` first calls
  `dos_memory_alloc` at `0x2f0fc` with the resource, the size, and the address
  cell `record + 0x10`.  A non-zero result sets the flag at `record + 0x18` to
  1.  A zero result calls `memory_alloc_small` at `0x2f172` with the same
  arguments and clears the flag.  A size of `0x96000` or more calls
  `memory_alloc_small` directly and clears the flag.  The function then calls
  `0x2f062` with the resource, the record, the allocation result, the name, and
  the size.
  Evidence: the code at `0x2ef22` through `0x2efb5`.
- Inference: the threshold `0x96000` separates a DOS memory request from the
  small DPMI request.  The flag at `+0x18` selects the free path in
  `resource_free`.
  Evidence: the two allocation calls and the `resource_free` record access at
  `0x2f4c4`.
- Observation: the rebuild with `-of+` holds 55 instructions and 145 bytes.  The
  identity check stays open at 15 of 55 instructions and 3 of 145 bytes.  The
  shared toolchain saves the frame before the register saves and keeps the
  resource and the name in `esi` and `ecx`.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2eed0 --rebuilt work/rebuild/rgetmod.exe --rebuilt-map
  work/rebuild/resource_get.map --rebuilt-symbol resource_get_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, `3`, `4`, and `D`.  It runs the full-table path, the small-size DOS
  path, the failed DOS fallback, the size at the limit `0x96000`, and the
  second-record path.
  Evidence: `python3 tools/rebuild/harness.py run --module resource_get`,
  `work/rebuild/rget_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x2eed0`.

## The object_transform module at 0x1cd44

- Observation: the original body at `0x1cd44` holds 61 instructions and 244
  bytes.  The plan row holds the same counts.  The merged store record keeps
  the 244-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1cd44 --count 61` and
  `tools/rebuild/functions/0x1cd44.json`.
- Observation: the function first calls `object_integrate` at `0x1d6bc` and
  then `object_state_save` at `0x1ccd8`, and it passes the object pointer in
  `eax` to both calls.
  Evidence: the calls at `0x1cd51` and `0x1cd59`.
- Observation: the function writes the world position at `+0x498` from
  `+0x258 + +0x454`, and the world position at `+0x4a0` from
  `+0x260 + +0x45c`.
  Evidence: the loads, the adds, and the stores at `0x1cd5e` through
  `0x1cd8e`.
- Observation: when the flag at `+0x2f8` is zero, the function copies the
  height at `+0x238` to `+0x49c`.  A non-zero flag leaves `+0x49c` unchanged.
  Evidence: the compare and the branch at `0x1cd97` and `0x1cd9e`.
- Observation: the function writes three float sums: `+0x4a4` from `+0x23c`
  and `+0x314`, `+0x4a8` from `+0x27c` and `+0x318`, and `+0x4ac` from
  `+0x244` and `+0x31c`.  It then writes `+0x2c4` from `+0x238 - 2` and
  `+0x4bc` from the new `+0x4a8` value and `+0x4c8`.
  Evidence: the FPU sequences at `0x1cdb2` through `0x1ce2d`.
- Observation: the module uses the shared `fpu_stub.asm`.  The module adds
  float values and stores them, and it holds no float-to-integer conversion.
  The stub for `__CHP` therefore stays a bare return, and no local stub is
  necessary.
  Evidence: the twelve FPU instructions and the `object_transform` module
  record.
- Observation: the rebuild with `-of+` holds 36 instructions and 168 bytes.
  The identity check stays open at 11 of 36 instructions and 11 of 168 bytes.
  The V2 toolchain saves the frame before the `edx` save, keeps the object in
  `edx` across the two calls, and emits `lea esp,[ebp-4]` in the epilogue.  The
  store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1cd44 --rebuilt work/rebuild/otrmod.exe --rebuilt-map
  work/rebuild/object_transform.map --rebuilt-symbol object_transform_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, and `D`.  It runs the flag-zero path, the flag-set path with a
  preserved `+0x49c`, and a negative-value path.
  Evidence: `python3 tools/rebuild/harness.py run --module object_transform`,
  `work/rebuild/otr_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x1cd44`.
- Inference: the callees `0x1d6bc` and `0x1ccd8` are `object_integrate` and
  `object_state_save`.  The names come from the call order, the state copy, and
  the position update.  The `object_state_save` record carries the name, and
  the `object_integrate` record is still `named`.
  Evidence: the callee records and the disassembly of each callee.

## The race_start module at 0x27770

- Observation: the original body at `0x27770` holds 66 instructions and 251
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  252-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x27770 --count 66` and
  `tools/rebuild/functions/0x27770.json`.
- Observation: the function first stores the screen rectangle 0, 0x280, 0, and
  0x190 at `0x15ce4` through `0x15cf0`, then calls `0x1bbc8`.
  Evidence: the stores at `0x27786` through `0x27798` and the call at
  `0x2779e`.
- Observation: the call target `0x1bbc8` builds `%s\config\choice.dat` and
  reads the file.  The function therefore names it `choice_load`.
  Evidence: the `sprintf` call at `0x1bbe1`, the `fopen` call at `0x1bbf1`, and
  the format string at data offset `0xa75`.
- Observation: the function copies `DAT_001207f4`, `DAT_0012080c`,
  `DAT_00120808`, `DAT_00120810`, and `DAT_00120814` to `DAT_0011d2a8`,
  `DAT_0011d2c0`, `DAT_0011d2bc`, `DAT_0011d2c4`, and `DAT_0011d2a4`, and sets
  the level `DAT_0011d2b4` to 1.
  Evidence: the block at `0x277a3` through `0x277df`.
- Observation: the function reads the race context `DAT_00002dec`.  It copies
  the word at `+0x48` to `DAT_0011d2cc`, copies the index at `+8` to
  `DAT_0011d2b8`, and calls `0x8bd54` with `DAT_001021d8[index]`.
  Evidence: the block at `0x277e5` through `0x277fe`.
- Inference: `0x8bd54` stores its argument in eight data cells and sets two
  derived cells.  The module names it `state_cells_set`, because the call has
  no other visible effect.
  Evidence: the disassembly of `0x8bd54` through `0x8bd93`.
- Observation: the function clears `DAT_00118e30` and `DAT_0011ce54`, builds
  `MAP%d` from `DAT_001207f8 + 1` into `DAT_0011d294`, sets
  `DAT_0011d260` to 0x100, sets `DAT_0011d2b4` to 1 again, copies the old
  `DAT_0011d274` to `DAT_0011d22c`, clears `DAT_0011d274`, and calls
  `race_session_start(1)` at `0x128e0`.  A non-zero result returns at once.
  Evidence: the block at `0x27803` through `0x2786a`.
- Observation: the rebuild with `-of+` holds 57 instructions and 231 bytes.
  The identity check stays open at 29 of 57 instructions and 30 of 231 bytes.
  The V2 toolchain uses `ecx` in place of `ebx` and stores the constants as
  immediates where the original loads them into registers.  The store records
  `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x27770 --rebuilt work/rebuild/rstmod.exe --rebuilt-map
  work/rebuild/race_start.map --rebuilt-symbol race_start_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, and `D`.  It runs the success path with the index 2, the flag 7, and
  `MAP1`, and the non-zero result path with the counter 5, the index 3, the flag
  0xffff, `MAP6`, and the result 0x999.
  Evidence: `python3 tools/rebuild/harness.py run --module race_start`,
  `work/rebuild/rst_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x27770`.

## The engine_init module at 0x127a0

- Observation: the original body at `0x127a0` holds 71 instructions and 266
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  268-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x127a0 --count 71` and
  `tools/rebuild/functions/0x127a0.json`.
- Observation: the function calls `config_load` at `0x1bf38`, then
  `list_file_load` at `0x15460` with the arguments `0x437` (`GEN`), `0x432`
  (`MENU`), `0x428` (`CD_TRACKS`), the table `0x1f00`, and the two values 4.
  Then it calls `setup_load` at `0x1bb58`, `controls_load` at `0x1b304`, and
  `mouse_init` at `0x1f1a0`.
  Evidence: the calls at `0x127ad` through `0x127d9` and the strings at the
  object 3 offsets `0x428` through `0x437`.
- Observation: the function stores `dpmi_real_int() != 3` at `DAT_0016684`.
  The result of the call at `0x25db0` is 3 on the normal path.
  Evidence: the call at `0x127de`, the compare at `0x127e3`, and the store at
  `0x127ee`.
- Observation: the setup word `DAT_00008070` guards the sound and music block.
  When it is zero the block stays unrun.  When it is non-zero and the flag is
  set, the function calls `sound_driver_set(0)` at `0x262a8`.
  Evidence: the guard at `0x127f3` and the call at `0x12802`.
- Observation: the block builds `%s\DATA\MUSIC.DAT` from the config string at
  `0x7f44` into the local buffer through `sprintf` at `0x4d32c` with the format
  at `0x43b`.  It clears the flag and calls `music_bank_load` at `0x2f9e0`
  with the bank struct `0x62b0`, the mode `DAT_00008070`, the size
  `DAT_00008078`, the local path, and the engine list `DAT_000062ac`.  A
  non-zero status calls `music_start` at `0x2ffc4`.
  Evidence: the block at `0x12807` through `0x1284d`.
- Observation: the normal path calls `palette_build` at `0x15720`,
  `keyboard_hook(1)` at `0x14e20`, `engine_interface_init` at `0x10810` with
  the record table `0x6530` and the state word `0x6350`, `stream_layer_init` at
  `0x4c810` with the engine list, `video_buf_load` at `0x4c8f0`, and
  `vesa_set_mode(1)` at `0x8bdec`.  It then stores 0, 0, and 0x280 at
  `DAT_001544c`, `DAT_0015444`, and `DAT_0015448`.
  Evidence: the block at `0x12852` through `0x128a9`.
- Observation: the rebuild with `-of+` holds 70 instructions and 271 bytes.
  The identity check stays open at 44 of 70 instructions and 85 of 266 bytes.
  The V2 toolchain saves the frame pointer first and uses `esi` and `movzx`
  where the original saves the data registers first and uses `edi` and `and`.
  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x127a0 --rebuilt work/rebuild/engmod.exe --rebuilt-map
  work/rebuild/engine_init.map --rebuilt-symbol engine_init_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, `3`, and `D`.  It runs the zero setup word path, the successful
  music load, the non-zero music status path, and the DPMI result 3 path.
  Evidence: `python3 tools/rebuild/harness.py run --module engine_init`,
  `work/rebuild/eng_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x127a0`.

## The memory_error_report module at 0x2edad

- Observation: the original body at `0x2edad` holds 97 instructions and 291
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  291-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2edad --count 97` and
  `tools/rebuild/functions/0x2edad.json`.
- Observation: the function reads the status at `[arg+0xc]` and the handler name
  at `[arg+0x10]`.  It reads the slot count at `[arg+0x08]`.
  Evidence: the loads at `0x2edbe`, `0x2edc1`, `0x2edcf`, and `0x2ee0a`.
- Observation: the status selects one of six message bodies.  The statuses 1,
  2, 3, 7, and 9 select the five named messages.  Every other status selects the
  default message.
  Evidence: the comparison tree at `0x2ee84` through `0x2eebb`.
- Observation: the status 1 body prints "Cannot allocate memory for: %s. Out of
  memory." with the name at `+0x10`.  The status 2 body prints the no-slots
  message with the name and then "Actual number of slots: %d" with the count at
  `+0x08`.  The statuses 3, 7, and 9 print one constant message each.  The
  default prints "Unknown error in memory management".
  Evidence: the six bodies at `0x2edcc`, `0x2edec`, `0x2ee26`, `0x2ee3f`,
  `0x2ee58`, and `0x2ee6e`, and the strings at the object 3 offsets `0x1394`,
  `0x13c4`, `0x13f9`, `0x1415`, `0x1444`, `0x1475`, and `0x14a2`.
- Observation: every body calls the printer at `0x4bf64` with the stream
  `0x2fd2` and the format string.  After the body the function calls
  `exit(-1)` at `0x300be`.
  Evidence: the printer calls at `0x2eddf`, `0x2edff`, `0x2ee19`, `0x2ee32`,
  `0x2ee4b`, `0x2ee64`, and `0x2ee7a`, and the exit call at `0x2eec2`.
- Observation: the status values are sparse, so the compiler builds a
  comparison tree and not a jump table.  The function stores the status in the
  frame local at `[ebp-8]` and the argument in `[ebp-4]`.  Both values are
  volatile.  The original holds the EBP frame, so the module builds with
  `-of+`.
  Evidence: the two stores at `0x2edbb` and `0x2edc4` and the tree at `0x2ee84`.
- Observation: the rebuild with `-of+` holds 57 instructions and 172 bytes.  The
  identity check stays open at 13 of 57 instructions and 10 of 172 bytes.  The
  V2 toolchain saves the frame pointer first and uses a different case layout
  where the original saves the data registers first and uses a binary tree.  The
  store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2edad --rebuilt work/rebuild/mermod.exe --rebuilt-map
  work/rebuild/memory_error_report.map --rebuilt-symbol memory_error_report_`.
- Observation: the behavior check passes.  The harness prints the markers `1`,
  `2`, `3`, `7`, `9`, `0`, `4`, and `D`.  It runs the five named statuses, the
  default status 0 below the range, and the default status 4 in a case gap.
  Evidence: `python3 tools/rebuild/harness.py run --module memory_error_report`,
  `work/rebuild/mer_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x2edad`.

## The object_drive_step module at 0x1cee0

- Observation: the original body at `0x1cee0` holds 83 instructions and 297
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  300-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1cee0 --count 83` and
  `tools/rebuild/functions/0x1cee0.json`.
- Observation: the mode guard compares `DAT_0000ffa4` with the state at
  `+0x410`.  While the mode is below the state, the target is
  `(+0x2e4 + +0x2f0) * +0x3fc` and the bound is `+0x2e4 * +0x3fc`.
  Otherwise the target and the bound both equal `+0x2e4 * +0x3fc`.
  Evidence: the compare at `0x1cef3`, the two force equations at `0x1cefe`
  through `0x1cf2b`, and the single equation at `0x1cf33` through `0x1cf48`.
- Observation: below the target, the function clears the brake flag `+0x2c0`,
  adds `+0x2e8 * DAT_00118e04` to the speed `+0x418`, and clamps the speed down
  to the target.
  Evidence: the branch at `0x1cf5c` through `0x1cf98`.
- Observation: at or above the target, the function returns when the speed
  equals the target.  Above it, the function sets the brake flag `+0x2c0` to 1
  when the speed is above the bound, subtracts `+0x2ec * DAT_00118e04`, and
  clamps the speed up to the target.
  Evidence: the branch at `0x1cf9a` through `0x1cffe`.
- Observation: the frame delta comes from `DAT_00118e04`, and the mode comes
  from `DAT_0000ffa4`.  The throttle is the float at `+0x3fc`.
  Evidence: the loads at `0x1ceed`, `0x1cf72`, `0x1cfd2`, and `0x1cf10`.
- Observation: the original holds the EBP frame, so the module builds with
  `-of+`.
  Evidence: the frame at `0x1cee1` and `0x1cee4`.
- Observation: the rebuild with `-of+` holds 57 instructions and 212 bytes.  The
  identity check stays open at 7 of 57 instructions and 15 of 212 bytes.  The
  V2 toolchain pushes the frame pointer before the `edx` save and keeps the
  object pointer in `edx`, while the original pushes `edx` first and reloads
  `eax` from the frame for every access.  The V2 toolchain also keeps the new
  speed on the FPU stack with `fst` and compares it, while the original stores
  it with `fstp` and reloads it.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1cee0 --rebuilt work/rebuild/odrmod.exe --rebuilt-map
  work/rebuild/object_drive_step.map --rebuilt-symbol object_drive_step_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, `3`, `4`, `5`, `6`, `7`, and `D`.  It runs the accelerate branch
  with and without the clamp, the brake branch with and without the brake set,
  the deceleration clamp, the equal-speed path, the inactive mode, and the
  negative throttle bound.
  Evidence: `python3 tools/rebuild/harness.py run --module object_drive_step`,
  `work/rebuild/odr_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x1cee0`.

## The sound_update module at 0x81a70

- Observation: the plan row recorded 299 bytes, 68 instructions, and 11 calls.
  The merged store record holds the size 64.  The next entry
  `sound_effects_update` starts at `0x81ab0`, 64 bytes after `0x81a70`.
  Evidence: `docs/explanation/rebuild-plan.md`,
  `tools/rebuild/functions/0x81a70.json`, and
  `python3 tools/le.py disasm STARTH.EXE 0x81ab0 --count 1`.
- Observation: the real body of `sound_update` holds 16 instructions and 62
  bytes.  It starts at `0x81a70` and ends at the jump at `0x81aac`.  The two
  bytes at `0x81aae` are padding.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x81a70 --count 16`.
- Observation: the plan row was wrong because the body sweep of
  `le.function_body` does not stop at the jump.  The jump is not the first
  instruction, so the sweep follows the shared body to the first `ret` at
  `0x81b9a` and counts 299 bytes and 68 instructions.  The shared body belongs
  to `sound_effects_update` at `0x81ab0`.  The plan row now holds 62 bytes, 16
  instructions, and 1 call.
  Evidence: the `jmp` at `0x81aac`, the `ret` at `0x81b9a`, and the
  `le.function_body` sweep.
- Observation: the entry stores the driver pointer in the global `0x7451c`,
  saves all registers and the four segment registers, calls the reset helper at
  `0x82e0c`, clears the active count `0x72488`, loads the destination `0x40330`
  in `edi`, and jumps to the shared body at `0x81ac6`.
  Evidence: the instructions at `0x81a70` through `0x81aac`.
- Observation: the entry `sound_effects_update` at `0x81ab0` holds the shared
  body at `0x81ac6`.  This module does not absorb that entry.  It calls the
  shared body as `sound_service_body`.
  Evidence: `tools/rebuild/functions/0x81ab0.json` and the finding
  "The sound_effects_update module at 0x81ab0".
- Observation: the source update at `0x8a958` uses the destination pointer in
  `edi`.  The rebuild passes `0x40330` to `sound_service_body`.
  Evidence: the `push edi` and the `stosd` instructions from `0x8a958` through
  `0x8a9e7`.
- Inference: the shared body overwrites `ecx` and `eax` before it reads them, so
  the rebuild omits the `ecx` and `eax` setup of the original.
  Evidence: the `lodsd` at `0x8a9cf` and the `mov ecx` at `0x8aac0`.
- Observation: the record keeps the extent 64 and sets the count 16.  The
  identity check compares the 16-instruction entry only.
  Evidence: `tools/rebuild/functions/0x81a70.json`.
- Observation: the rebuilt module holds 6 instructions and 27 bytes up to the
  tail jump.  The identity check stays open at 3 of 16 instructions and 4 of 57
  bytes.  The store records the status `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x81a70 --count 16 --rebuilt work/rebuild/sudmod.exe
  --rebuilt-map work/rebuild/sound_update.map --rebuilt-symbol sound_update_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, and `D`.  The paths cover a set active count, a negative active
  count, two calls in one run, the call order, and the destination `0x40330`.
  Evidence: `python3 tools/rebuild/harness.py run --module sound_update`,
  `work/rebuild/sud_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x81a70`.

## The frame_timer module at 0x1fc38

- Observation: the original body at `0x1fc38` holds 76 instructions and 299
  bytes.  The plan row holds the same counts.  The merged store record keeps the
  312-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1fc38 --count 76` and
  `tools/rebuild/functions/0x1fc38.json`.
- Observation: the function waits until the frame counter `DAT_000007f9` is 2
  or more and then clears the counter.  The original loop reloads the counter
  on every turn.
  Evidence: the load at `0x1fc40`, the compare at `0x1fc46`, and the branch at
  `0x1fc49`.
- Observation: the old smooth delta goes to `DAT_00118e14`.  The raw delta is
  `counter * 0.01` in `DAT_00118e08`.  The smooth delta is
  `(old * 9.0 + raw) / 10.0` in `DAT_00118e04`.  The raw delta accumulates in
  `DAT_00118e10`.
  Evidence: the `fild` and `fmul` at `0x1fc53` and `0x1fc56`, the copy at
  `0x1fc5c` through `0x1fc61`, the smooth step at `0x1fc66` through
  `0x1fc80`, and the accumulate at `0x1fc86` through `0x1fc98`.
- Observation: the smooth delta clamps at 0.2 and 0.02.  The comparisons use
  the double constants, and the stores use the float immediates `0x3e4ccccd`
  and `0x3ca3d70a`.
  Evidence: the compares at `0x1fca4` and `0x1fcbf`, the stores at `0x1fcaf`
  and `0x1fcca`, and the constants at `0x100c68` and `0x100c70`.
- Observation: the frame rate is the truncated `1 / delta`.  The filter table
  pointer is `0x80e8 + 400 * rate`.  The original calls the Watcom helper at
  `0x4d9f4`, which truncates toward zero.
  Evidence: the divide at `0x1fcd6`, the helper call at `0x1fcea`, the scaled
  rate at `0x1fcf2` through `0x1fd19`, and the pointer store at `0x1fd23`.
- Observation: the phase `DAT_00118e0c` advances by `delta * 0.3` and wraps at
  1.0.  The word `DAT_0002153c` holds the phase scaled by 65535 and truncated.
  Evidence: the multiply at `0x1fce2`, the phase step at `0x1fd00` through
  `0x1fd40`, and the scale and store at `0x1fd46` through `0x1fd57`.
- Observation: the original constants sit in object 3 at `0x100c58` through
  `0x100c84`: the double 0.01, the floats 9.0 and 10.0, the doubles 0.2, 0.02,
  and 0.3, and the floats -1.0 and 65535.0.
  Evidence: the references at `0x1fc56`, `0x1fc6c`, `0x1fc80`, `0x1fca4`,
  `0x1fcbf`, `0x1fce2`, `0x1fd3a`, and `0x1fd4c`, and the data at
  `0x100c58`.
- Observation: the original has the EBP frame and the 0xc-byte local area, so
  the module builds with `-of+`.
  Evidence: the frame at `0x1fc3b` and the local area at `0x1fc3d`.
- Observation: the rebuilt exe keeps the numeric constants in the object-2
  data page at module offset `0x20000`.  At run time that page reads zero, so
  the literal constants in the module gave zero deltas and the behavior check
  failed.
  Evidence: the map segment `CONST` at `0002:00000000` and the runtime read of
  the relocated object-2 base.
- Observation: the module now takes the eight constants through the globals
  `frame_delta_scale`, `frame_delta_gain`, `frame_delta_norm`,
  `frame_delta_high`, `frame_delta_low`, `frame_phase_step`,
  `frame_phase_wrap`, and `frame_phase_scale`.  The harness sets the exact bit
  patterns.  This is the same pattern as `object_proximity_check`.
  Evidence: `tools/rebuild/frame_timer.c` and
  `tools/rebuild/frame_timer_main.c`.
- Observation: the rebuild holds 62 instructions and 278 bytes.  The identity
  check stays open at 27 of 62 instructions and 24 of 278 bytes.  The V2
  toolchain saves the frame pointer first, keeps the counter in `eax`, computes
  the raw delta before the smooth step, uses `imul` for the `400 * rate`
  product, compares the phase with an immediate, and reads the constants from
  the globals.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1fc38 --rebuilt work/rebuild/ftmmod.exe --rebuilt-map
  work/rebuild/frame_timer.map --rebuilt-symbol frame_timer_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `5` and `D`.  The paths cover the plain delta, the upper clamp with
  the truncated frame rate 4, the lower clamp with the frame rate 50, the phase
  wrap from 1.0, a phase step that stays below 1.0, and a phase step that
  crosses 1.0.
  Evidence: `python3 tools/rebuild/harness.py run --module frame_timer`,
  `work/rebuild/ftm_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x1fc38`.
- Observation: the two conversions truncate toward zero.  The shared
  `fpu_stub.asm` defines `__CHP` as a bare return, so the module uses the local
  `frame_timer_fpu.asm` stub with the truncating `__CHP`, the same stub as
  `object_proximity_check`.  Without the stub, a frame rate such as 6.896 would
  round to 7 and the phase word would change.
  Evidence: the helper at `0x4d9f4`, `tools/rebuild/fpu_stub.asm`, and
  `tools/rebuild/frame_timer_fpu.asm`.

## The object_update_all module at 0x2786c

- Observation: the original body at `0x2786c` holds 84 instructions and 309
  bytes up to the `ret` at `0x279a0`.  The plan row holds the same counts.  The
  merged store record keeps the 312-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2786c --count 84` and
  `tools/rebuild/functions/0x2786c.json`.
- Observation: the function runs five passes over the `DAT_0010f8ec` records at
  `DAT_00110ab8`.  The stride is 0x590 bytes, and the shared loop counter is
  `DAT_0010634c`.  The five passes call `object_transform` at `0x1cd44`,
  `object_player_input` at `0x1c760`, `object_physics` at `0x1c810`,
  `object_prepare` at `0x1c79c`, and `object_car_link` at `0x1c84c`.
  Evidence: the `imul eax,eax,0x590` at `0x27893`, `0x278c4`, `0x278f5`,
  `0x27926`, and `0x27956`, and the five loop bodies from `0x27884` through
  `0x27966`.
- Observation: the seven calls after the passes run `race_timeout` at
  `0x27f44`, `player_update` at `0x19ab0`, `ai_update` at `0x205c4`,
  `opponent_update` at `0x20954` with the value 1, `camera_update` at
  `0x8ac78` with the record `0x109a0`, `sound_effects_update` at `0x81ab0`
  with the driver `0x6350`, and `hud_update` at `0x27a34`.
  Evidence: the instructions from `0x27968` through `0x27995`.
- Observation: the old note recorded the stride as 0x164 bytes and four
  passes.  The disassembly shows the stride 0x590 and five passes.  This
  section replaces that note.
  Evidence: the `imul` immediates above and the five loop bodies.
- Observation: the rebuilt module holds 69 instructions and 285 bytes.  The
  identity check stays open at 14 of 69 instructions and 16 of 285 bytes.  The
  V2 toolchain saves the frame first, keeps the count in memory, uses `cmp`
  with a memory operand, and omits the jump-to-test form of the loop.  The
  store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2786c --rebuilt work/rebuild/oupdmod.exe --rebuilt-map
  work/rebuild/object_update_all.map --rebuilt-symbol object_update_all_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, and `D`.  The paths cover a zero count, one record, and three
  records.  The sequence proves the five passes, the record order, the stride,
  and the seven calls in order.
  Evidence: `python3 tools/rebuild/harness.py run --module object_update_all`,
  `work/rebuild/oupd_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x2786c`.

## The keyboard_hook module at 0x14e20

- Observation: the original body at `0x14e20` holds 100 instructions and 334
  bytes up to the early-exit block at `0x14f6e`.  The plan row holds the same
  counts.  The merged store record keeps the 408-byte extent, which includes
  the trailing NOP at `0x14fb7`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x14e20 --count 100` and
  `tools/rebuild/functions/0x14e20.json`.
- Observation: the install path sends the keyboard controller command `0xed`
  to port `0x60`, waits on the input buffer bit and the output buffer bit of
  port `0x64` with the timeout `0xffff`, reads the acknowledge from port
  `0x60`, and repeats the sequence up to four times while the acknowledge is
  not `0xfa`.  It then writes `0` to port `0x60`, clears bits 4, 5, and 6 of
  the BIOS keyboard flags at `0040:0017`, reads the real-mode interrupt 9
  vector with INT 21h AX=3509h, and installs the handler at CS:0xe4 with INT
  21h AX=2509h.
  Evidence: the port instructions at `0x14e4c` through `0x14ec5`, the GS
  access at `0x14ed1` and `0x14ee1`, and the two `dpmi_call` calls at
  `0x14f01` and `0x14f60`.
- Observation: the zero-argument path restores the saved real-mode interrupt 9
  vector with INT 21h AX=2509h.  It reads the saved segment from the word
  `0xcf0c` and the saved offset from the dword `0xcf10`.
  Evidence: the block at `0x14f6e` and the `dpmi_call` at `0x14fa9`.
- Inference: the handler at CS:0xe4 is the raw keyboard interrupt 9 service.
  It reads the scancode from port `0x60` and stores the make or break state in
  a 128-byte table at `DS:0x113`.  The handler is at linear `0x100e4`, which is
  object 1 offset `0xe4`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x100e4 --count 15` and the
  object base `0x10000` from `python3 tools/le.py info STARTH.EXE`.
- Observation: the rebuilt module holds 79 instructions and 302 bytes.  The
  identity check stays open at 25 of 79 instructions and 35 of 302 bytes.  The
  V2 toolchain saves the frame pointer first, keeps the loop counters in
  registers, calls the `inportb` and `outportb` helpers instead of the two
  inline port reads and the two inline port writes, and replaces the byte
  read-modify-write of the BIOS flags with one `and gs:[...],0x8f`.  The store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x14e20 --rebuilt work/rebuild/kbhmod.exe --rebuilt-map
  work/rebuild/keyboard_hook.map --rebuilt-symbol keyboard_hook_`.
- Observation: the rebuild calls `inportb` and `outportb` instead of the
  inline port instructions.  This is a deviation from the original.  The
  harness needs the calls to simulate the controller and to prove the
  acknowledge and the retry paths.  The two DPMI calls stay external, as in the
  original.
  Evidence: `tools/rebuild/keyboard_hook.c` and
  `tools/rebuild/keyboard_hook_main.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `4` and `D`.  The paths cover the first-acknowledge install, the
  one-retry install, the four-attempt install, the uninstall, and the BIOS flag
  mask.
  Evidence: `python3 tools/rebuild/harness.py run --module keyboard_hook`,
  `work/rebuild/kbh_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x14e20`.

## The object_integrate module at 0x1d6bc

- Observation: the original body at `0x1d6bc` holds 82 instructions and 339
  bytes up to the first `ret` at `0x1d80e`.  The plan row holds the same
  counts.  The merged store record keeps the 376-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1d6bc --count 82` and
  `tools/rebuild/functions/0x1d6bc.json`.
- Observation: the function adds the scaled velocity to the position at
  `+0x454` and `+0x45c`.  The scale is the velocity at `+0x460` and `+0x468`
  times the factor at `+0x450` times the frame delta `DAT_00118e04`.  Each sum
  truncates toward zero through the helper at `0x4d9f4`.
  Evidence: the `fld`, `fmul`, `fild`, `faddp`, `call 0x4d9f4`, and `fistp`
  blocks at `0x1d6c9` through `0x1d6f1` and `0x1d6fa` through `0x1d722`.
- Observation: the function multiplies the factor by the damping at
  `DAT_0010cf08 + 0x13c`.  It then clears the factor when the damped value
  stays below 10.0.  The constant sits at object 3 offset `0xb24`.
  Evidence: the `fmul` at `0x1d736`, the `fcomp` at `0x1d74b`, and the
  `fstp` of 0 at `0x1d759`.
- Observation: when both positions equal the target at `+0x160` and `+0x168`,
  the function adds the frame delta to the dwell timer `+0x46c`.  When the
  timer reaches 3.0, it multiplies both positions by the scale at
  `DAT_0010cf08 + 0x180` and truncates.  The threshold constant sits at object
  3 offset `0xb28`.  Otherwise the function clears the timer.
  Evidence: the two target compares at `0x1d76f` and `0x1d787`, the timer
  update at `0x1d79c`, the `fcomp` at `0x1d7b1`, the two snap blocks at
  `0x1d7c5` and `0x1d7ec`, and the timer clear at `0x1d812`.
- Observation: the rebuilt module holds 62 instructions and 267 bytes.  The
  identity check stays open at 9 of 62 instructions and 22 of 267 bytes.  The
  V2 toolchain saves the frame pointer first, keeps the object pointer in
  `edx`, reuses the local slot for the factor, uses a short jump, and stores
  the dwell timer with `fst` before the compare.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1d6bc --rebuilt work/rebuild/ointmod.exe --rebuilt-map
  work/rebuild/object_integrate.map --rebuilt-symbol object_integrate_`.
- Observation: the rebuild uses the module-local `object_integrate_fpu.asm`
  stub.  The stub makes `__CHP` truncate toward zero.  The shared
  `fpu_stub.asm` leaves the default round-to-nearest mode, so a negative sum
  would round away from zero.  This deviation keeps the four conversions
  correct for negative values.
  Evidence: `tools/rebuild/object_integrate_fpu.asm` and the two negative
  paths in `tools/rebuild/object_integrate_main.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `7` and `D`.  The paths cover the integration, the factor clamp, the
  dwell accumulation, the snap, both truncation directions, the threshold
  boundary, and the damping reduction.
  Evidence: `python3 tools/rebuild/harness.py run --module object_integrate`,
  `work/rebuild/oint_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x1d6bc`.

## The race_panel_0 module at 0x2e0b4

- Observation: the original body at `0x2e0b4` holds 103 instructions and 357
  bytes up to the `ret` at `0x2e218`.  The plan row holds the same counts.  The
  merged store record keeps the 360-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2e0b4 --count 103` and
  `tools/rebuild/functions/0x2e0b4.json`.
- Observation: each axis target centers half of an object field on the `0x140`
  screen width.  The x target uses the field at `+0x134` and the y target uses
  the field at `+0x120`.  The signed division by two uses the sign-correction
  pattern, and the second division uses a plain arithmetic shift.
  Evidence: the two blocks at `0x2e0bd` through `0x2e0dd` and `0x2e140`
  through `0x2e160`.
- Observation: the x position `DAT_00020824` moves up by the frame delta
  `DAT_0018e08` times the step at `0x12d4` while the target stays above it, and
  it snaps down to the target when the target stays below it.  The y position
  `DAT_00020828` uses the same pattern with the step at `0x12dc`.
  Evidence: the `fcomp`, `fld`, `fmul`, `fadd`, and `fstp` blocks at
  `0x2e0e0` through `0x2e117` and `0x2e163` through `0x2e19a`.
- Observation: the function adds the offset at `0x12d8` to the x position only.
  It then converts the x sum and the y position to an integer through the
  helper at `0x4d9f4`, which truncates toward zero.
  Evidence: the `fadd` at `0x2e128` and the two `call 0x4d9f4` sites at
  `0x2e130` and `0x2e1b0`.
- Observation: the function draws the two panels through `panel_show` at
  `0x2dfac` with the arguments `(x, 0, 0xf)` and `(y, 0x10, 0xe)`.
  Evidence: the calls at `0x2e13b` and `0x2e1bb` and the register sets
  `ebx=0xf`, `edx=0`, `ebx=0xe`, and `edx=0x10`.
- Observation: when the race state `DAT_00020988` is 1 or 2 and the low seven
  bits of the frame counter `DAT_000007fd` stay below `0x48`, the function
  draws the matching title through `panel_draw_centered` at `0x2e7cc`.  The
  title pointers sit at `DAT_0011fc58` and `DAT_0011fc5c`.  The call uses y
  `0xbe`, font 0, and color 2.
  Evidence: the state and blink tests at `0x2e1c0` through `0x2e20d`.
- Observation: the rebuild uses the module-local `race_panel_0_fpu.asm` stub.
  The stub makes `__CHP` truncate toward zero.  The shared `fpu_stub.asm`
  leaves the default round-to-nearest mode, so a negative sum would round away
  from zero.  This deviation keeps the two conversions correct for negative
  values.
  Evidence: `tools/rebuild/race_panel_0_fpu.asm` and the negative path in
  `tools/rebuild/race_panel_0_main.c`.
- Observation: the rebuilt module holds 110 instructions and 363 bytes.  The
  identity check stays open at 12 of 103 instructions and 69 of 357 bytes.
  The V2 toolchain saves the frame pointer first, keeps the object pointer in
  `eax`, uses `cdq`, keeps the y target in a second local slot, and reorders
  the title arguments.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2e0b4 --rebuilt work/rebuild/rp0mod.exe --rebuilt-map
  work/rebuild/race_panel_0.map --rebuilt-symbol race_panel_0_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `4` and `D`.  The paths cover the x increment, the y snap, the y
  increment, the x snap, both title states, the blink guard, and the negative
  truncation.
  Evidence: `python3 tools/rebuild/harness.py run --module race_panel_0`,
  `work/rebuild/rp0_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x2e0b4`.

## The net_player_add module at 0x2dddc

- Observation: the original body at `0x2dddc` holds 111 instructions and 394
  bytes up to the `ret` at `0x2df65`.  The plan row holds the same counts.  The
  merged store record keeps the 396-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2dddc --count 111` and
  `tools/rebuild/functions/0x2dddc.json`.
- Observation: the function has a frame.  It saves `ebx`, `ecx`, `edx`, `esi`,
  `edi`, and `ebp` before the frame, sets `ebp`, and subtracts 8 from `esp`.
  The local slot `[ebp-8]` holds the argument and the local slot `[ebp-4]`
  holds the index.  The rebuild uses the option `-of+`.
  Evidence: the prologue at `0x2dddc` through `0x2dde7` and the stores at
  `0x2dde7` and `0x2ddef`.
- Observation: the call at `0xc6fbc` returns the value at `DAT_0012229c`, and
  the function uses that value as the record index.  The record array sits at
  object-3 offset `0x2099c`, so the loader relocates it to linear `0x12099c`.
  The stride is 0x38 bytes: the code computes `index * 8 - index` and then
  multiplies the result by 8.
  Evidence: the call and the stride arithmetic at `0x2ddea` through `0x2ddfe`
  and the callee at `0xc6fbc`.
- Observation: the function stores the argument at `+0x04` and the index at
  `+0x08`.  It copies the name from `DAT_001203cc` to `+0x0c` with the inline
  two-byte copy loop.  It stores the cell `DAT_001203c4` at `+0x10` and the
  cell `DAT_001203c8` at `+0x14`.
  Evidence: the stores at `0x2de08`, `0x2de24`, and `0x2de74` through
  `0x2de8d`, and the copy loop at `0x2de49` through `0x2de5f`.
- Observation: the function reads the context pointer `DAT_00102dec` at
  object-3 offset `0x2dec`.  It stores the field at `+0x18` of that context at
  record `+0x1c`.  It uses the field at `+0x28` of that context as the index
  into the table at object-3 offset `0x21e4` and stores the table value at
  record `+0x18`.
  Evidence: the reads at `0x2de94` through `0x2dec6`.
- Observation: the function stores `DAT_0011d274` at `+0x20` and
  `DAT_0011d230` at `+0x24`.  It calls `0xc7100` with the address of the
  0x10-byte block at `+0x28`.  The callee copies 16 bytes from `0x21678` to
  that address.
  Evidence: the stores at `0x2decd` through `0x2def8`, the call at `0x2df17`,
  and the callee at `0xc7100`.
- Observation: the function calls `0xc7478` with the record base, the size
  `0xc`, the name at record `+0x0c`, the value `0x74`, and the count `9`.  The
  callee reads every argument from the stack, so the module declares the two
  calls `__cdecl`.
  Evidence: the pushes at `0x2df33` through `0x2df51` and the callee frame at
  `0xc7478`.
- Observation: the `-oi` option expands `strcpy` into the same two-byte copy
  loop as the original.
  Evidence: the copy loop at `0x2de49` through `0x2de5f` and the module
  recipe flags.
- Observation: the rebuilt module holds 69 instructions and 203 bytes.  The
  identity check stays open at 22 of 69 instructions and 18 of 203 bytes.  The
  V2 toolchain saves the frame pointer first, keeps the record pointer in
  `edx`, uses `imul edx, eax, 0x38` once, and stores every field through that
  register.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2dddc --rebuilt work/rebuild/npamod.exe --rebuilt-map
  work/rebuild/net_player_add.map --rebuilt-symbol net_player_add_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, and `D`.  The paths cover the slot 0, the slot 1 with the 0x38-byte
  stride, and the slot 2 with an 8-character name so the field stores must run
  after the copy.  Each path checks the record fields, the default block
  address, and the five register arguments.
  Evidence: `python3 tools/rebuild/harness.py run --module net_player_add`,
  `work/rebuild/npa_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x2dddc`.

## The object_init_from_file module at 0x4cc10

- Observation: the original body at `0x4cc10` holds 120 instructions and 396
  bytes up to the `ret` at `0x4cd9b`.  The plan row holds the same counts.  The
  store record keeps the 396-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4cc10 --count 120` and
  `tools/rebuild/functions/0x4cc10.json`.
- Observation: the original has the EBP frame.  It saves `ebx`, `ecx`, and
  `ebp`, reserves 0x1c bytes, and stores the object at `[ebp - 0x1c]` and the
  index at `[ebp - 0x18]`.  The rebuild uses the option `-of+`.
  Evidence: the prologue at `0x4cc10` through `0x4cc1b`.
- Observation: the function compares the index with `0x3e8`.  An index above
  the bound loads the value `0x16f0` and calls `0xc656e`, which is the message
  reporter, but the function continues because the original has no return on
  that path.  The value `0x16f0` is the data-segment offset of the string
  `error`.
  Evidence: the compare at `0x4cc1e`, the message call at `0x4cc2c`, and the
  data base `0xf6800`.
- Observation: the function reads the file record from the handle table at
  `0x21540` with the index as the word offset.
  Evidence: the two loads at `0x4cc31` through `0x4cc3c`.
- Observation: the function sets the object field at `+0x04` to 1 and the
  index at `+0x00`.  It clears the fields at `+0x08`, `+0x0c`, and `+0x10`,
  then the fields at `+0x14`, `+0x18`, and `+0x1c`.  It sets the field at
  `+0x20` to -1 and clears the six words at `+0x24` through `+0x38`.
  Evidence: the stores at `0x4cc3f` through `0x4ccd4`.
- Observation: the function fills the 0x20-byte records at object + 0x40.  The
  chunk count is the word at `file + 0x14` and the offset table is at
  `file + 0x18`.  A table word selects a chunk record at `file + offset`, and
  the word at `+0x0c` of that record selects the chunk header at
  `file + word`.  Each record holds the file, the count, and the six header
  words at `+0x04` through `+0x18`.  The record after the last chunk holds -1
  at `+0x04`.
  Evidence: the loop at `0x4ccdb` through `0x4cd8f`.
- Inference: the offset table points into the file record, and a chunk record
  holds the offset of its header.  The two offsets are file-relative.
  Evidence: the two additions at `0x4cd12` through `0x4cd28`.
- Observation: the rebuilt module holds 64 instructions and 218 bytes.  The
  identity check stays open at 46 of 64 instructions and 7 of 218 bytes.  The
  V2 toolchain saves the frame pointer first, keeps the object and file in
  `ebx` and `ecx`, folds the zero chains into single stores, and merges the
  loop increment.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x4cc10 --rebuilt work/rebuild/oifmod.exe --rebuilt-map
  work/rebuild/object_init_from_file.map --rebuilt-symbol
  object_init_from_file_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `3` and `D`.  The paths cover the two-chunk file, the zero-chunk
  file, the index 1001 above the bound, and the index 1000 at the bound.  The
  object starts filled with `0x5a`, so a field that the function does not
  write fails the check.
  Evidence: `python3 tools/rebuild/harness.py run --module
  object_init_from_file`, `work/rebuild/oif_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x4cc10`.

## The pcx_load module at 0x1b5a4

- Observation: the original body at `0x1b5a4` holds 108 instructions and 401
  bytes up to the `ret` at `0x1b734`.  The plan row holds the same counts.  The
  store record keeps the 404-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1b5a4 --count 108` and
  `tools/rebuild/functions/0x1b5a4.json`.
- Observation: the original has the EBP frame.  It reserves `0x1d4` bytes,
  subtracts `0x82` from `ebp`, and stores the four register arguments at
  `[ebp + 0x76]`, `[ebp + 0x6e]`, `[ebp + 0x62]`, and `[ebp + 0x6a]`.  The
  rebuild uses the option `-of+`.
  Evidence: the prologue at `0x1b5a4` through `0x1b5bc`.
- Observation: the function builds the container path with the `sprintf` call
  at `0x4d32c`.  The format is `%s\%s`, the first part is the base path at
  `DAT_00007edc`, and the second part is the first argument.  The `fopen` call
  at `0x4c08d` opens that path, uses the second argument as the member name,
  and passes `rb`.  A failed open passes the stream to `file_error_report` at
  `0x4bf88` and continues.
  Evidence: the call block at `0x1b5bf` through `0x1b5f9` and the strings at
  `0x1009ef` and `0x1009f5`.
- Observation: the function seeks 0x300 bytes before the end through the call
  at `0x4c5c7`, reads the 0x300-byte palette into the fourth argument through
  `stream_read`, and shifts every palette byte right by 2 with `shr`.  It then
  rewinds, reads the 0x80-byte header, and computes the pixel count as
  `(xmax - xmin + 1) * (ymax - ymin + 1)`.
  Evidence: the seek block at `0x1b5fe` through `0x1b621`, the shift loop at
  `0x1b62d` through `0x1b645`, and the header read at `0x1b656`.
- Observation: the function decodes the RLE runs into the third argument.  A
  byte with the mask `0xc0` equal to `0xc0` repeats the next byte
  `byte & 0x3f` times.  The other bytes are literals.  The loop stops when the
  pixel count reaches zero.  The function then calls `stream_close` at
  `0x4c38d` and returns 0.
  Evidence: the loop at `0x1b696` through `0x1b71b` and the epilogue at
  `0x1b720` through `0x1b734`.
- Observation: the rebuilt module holds 102 instructions and 340 bytes.  The
  identity check stays open at 8 of 102 instructions and 11 of 340 bytes.  The
  V2 toolchain saves `esi` and `edi`, keeps the arguments in registers, builds
  the frame without the `sub ebp` step, and folds the palette shift into a
  register move.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1b5a4 --rebuilt work/rebuild/pcxmod.exe --rebuilt-map
  work/rebuild/pcx_load.map --rebuilt-symbol pcx_load_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `3` and `D`.  The paths cover a 3x2 image with two RLE runs and one
  literal byte, a 63x1 image with one run of 0x3f bytes, a 1x1 image with the
  literal byte 0xbf, and a failed open that calls `file_error_report`.  The
  harness holds the input strings in the data segment, because this harness
  does not load the constant segment.
  Evidence: `python3 tools/rebuild/harness.py run --module pcx_load`,
  `work/rebuild/pcx_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x1b5a4`.

## The race_panel_3 module at 0x2e58c

- Observation: the original body at `0x2e58c` holds 120 instructions and 408
  bytes up to the `ret` at `0x2e723`.  The plan row holds the same counts.  The
  store record keeps the 408-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2e58c --count 120` and
  `tools/rebuild/functions/0x2e58c.json`.
- Observation: the function has a frame.  It saves `ebx`, `ecx`, `edx`, `esi`,
  `edi`, and `ebp` before the frame, sets `ebp`, and subtracts `0x94` from
  `esp`.  The rebuild uses the option `-of+`.
  Evidence: the prologue at `0x2e58c` through `0x2e594`.
- Observation: the function draws the title pointer `DAT_0011fc34` through
  `panel_draw_centered` at `0x2e7cc` with the y 2, the font 3, and the color 2.
  Evidence: the call at `0x2e5ab`.
- Observation: the index `DAT_001203c8` selects a 0x34-byte record from the
  table at object-1 offset `0x1d908`.  The text sits at `+0x14` of the record.
  The code computes the stride with `index * 4 - index`, two shifts, and an add.
  Evidence: the stride arithmetic at `0x2e5b0` through `0x2e5d1`.
- Observation: the function measures the text through `text_measure` at
  `0x254a8` with the font 2.  It halves the measured width and height with a
  signed division that truncates toward zero.  It sets x to
  `(0x140 - width / 2) / 2` and y to `0xc8 - 2 * (height / 2)`.  It draws the
  doubled position through `text_draw` at `0x25020` with the font 2 and the
  color 2.
  Evidence: the measure call at `0x2e5da`, the halving at `0x2e5df` through
  `0x2e5fc`, the centering at `0x2e5ff` through `0x2e618`, and the draw call at
  `0x2e64b`.
- Observation: when `DAT_0012081c` stays below `DAT_00120818` and the low six
  bits of the frame counter `DAT_000007fd` stay below 0x20, the function draws
  the title pointer `DAT_0011fc54` through `panel_draw_centered` with the y
  0x20, the font 1, and the color 2.
  Evidence: the test at `0x2e650` through `0x2e668` and the call at `0x2e67e`.
- Observation: a non-zero index draws the panel at (8, 0x64) through
  `panel_show` at `0x2dfac`.  The size is 2 when `DAT_0012039c` stays below
  `DAT_001203a8` and `DAT_0012037c` stays above 0.6.  Otherwise the size is 0.
  Evidence: the test at `0x2e683` through `0x2e6c7` and the call at `0x2e6c9`.
- Observation: the function draws the panel at (0xee, 0x64) when
  `DAT_00120818` plus 2 differs from the index.  The size is 3 when
  `DAT_0012039c` stays above `DAT_001203a8` and `DAT_0012037c` stays above 0.6.
  Otherwise the size is 1.
  Evidence: the test at `0x2e6ce` through `0x2e70c` and the call at `0x2e716`.
- Observation: the threshold 0.6 is the double at object-3 offset `0x12ef`,
  linear `0x1012ef`.  Both compares load the float `DAT_0012037c` and run
  `fcomp` against that qword.
  Evidence: the reads of `0x1012ef` and the compares at `0x2e69b` and
  `0x2e6ef`.
- Observation: the literal constant lands at DGROUP offset 0 in the rebuilt
  image.  With the literal constant, the low-threshold path failed, so the
  compare read a wrong value.  The rebuild reads the threshold through the
  global `race_panel_3_threshold`, and the harness sets the exact double bits
  `0x3fe3333333333333`.  The module uses the shared `fpu_stub.asm`, because it
  has no conversion to an integer.
  Evidence: the constant at object 2 offset 0 of `work/rebuild/rp3mod.exe`, the
  failed low-threshold path, and `tools/rebuild/race_panel_3.c`.
- Observation: the rebuilt module holds 98 instructions and 327 bytes.  The
  identity check stays open at 23 of 98 instructions and 30 of 327 bytes.  The
  V2 toolchain saves the frame pointer first, keeps the text in `esi`, folds the
  stride into `imul`, and uses `cdq`.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2e58c --rebuilt work/rebuild/rp3mod.exe --rebuilt-map
  work/rebuild/race_panel_3.map --rebuilt-symbol race_panel_3_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `5` and `D`.  The paths cover the zero index, the second title, the
  row cell above and below, the low threshold value, the skipped second panel,
  and the negative width that truncates toward zero.
  Evidence: `python3 tools/rebuild/harness.py run --module race_panel_3`,
  `work/rebuild/rp3_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x2e58c`.

## The overlay_queue_update module at 0x289bc

- Observation: the original body at `0x289bc` holds 117 instructions and 415
  bytes up to the `ret` at `0x28b5a`.  The plan row holds the same counts.  The
  store record keeps the 416-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x289bc --count 117` and
  `tools/rebuild/functions/0x289bc.json`.
- Observation: the function has a frame.  It saves `ebx`, `ecx`, `edx`, `esi`,
  and `ebp` before the frame, sets `ebp`, and subtracts `0xc` from `esp` for
  three local slots.
  Evidence: the prologue at `0x289bc` through `0x289c3`.
- Observation: the first loop walks the signed source list `DAT_00102108` and
  stops at the first negative entry.  An entry with the value 0 or with a clear
  byte at `DAT_00100002[i]` is ready.  A ready entry with the flag
  `DAT_00120884[i]` set to 1 clears the flag and moves into the queue.  The
  queue write uses the count `DAT_0012087c` into the table `DAT_0012082c`, and
  it clears the next slot.  The count stops at 4, so the table holds five
  entries.  A source entry above 0 with a set byte at `DAT_00100002[i]` sets the
  flag to 1 and waits.
  Evidence: the loop at `0x289c6` through `0x28a50`.
- Observation: the byte test at `0x28a39` uses the address `i + 2` against the
  data segment.  With the data segment at object 3, the byte is
  `DAT_00100002[i]`, linear `0x100002 + i`.
  Evidence: the compare at `0x28a39` and the source index at `0x28a36`.
- Observation: the second loop walks the nine records of the table at
  `DAT_00102e04`.  A record holds the five-byte key at `+0`, the value pointer
  at `+0xc`, the value at `+0x10`, and the xor flag at `+0x14`, with the 0x18
  stride.  The loop compares the low byte of each of the five queue entries
  with the key byte.  A zero queue entry stops the compare without a mismatch.
  A full queue that matches every key byte runs the record: a clear xor flag
  stores the value through the pointer, and a set xor flag xors the value
  through the pointer.  The code then calls `table_lookup` at `0x1f718` with 8
  and clears the count and the first queue entry.  When all nine records
  mismatch, the code clears the count and the first queue entry.
  Evidence: the loop at `0x28a52` through `0x28b4d` and the call at `0x28b10`.
- Observation: the recovered table holds the key `-----` in all nine records.
  The value pointers hold object-3 offsets such as `0x207f0`, and the xor flag
  is set in the records that hold it.
  Evidence: the table bytes at linear `0x102e04` through `0x102edb`.
- Observation: the rebuilt module holds 86 instructions and 272 bytes.  The
  identity check stays open at 19 of 86 instructions and 14 of 272 bytes.  The
  V2 toolchain folds the record stride into `imul` and loads the value pointer
  and the value directly.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x289bc --rebuilt work/rebuild/oqupd.exe --rebuilt-map
  work/rebuild/overlay_queue_update.map --rebuilt-symbol
  overlay_queue_update_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `6` and `D`.  The paths cover the empty source, the held entry and the
  deferred enqueue, the six-entry fill with the count at 4 and the overwritten
  fifth entry, the xor record, the full queue that mismatches every record, the
  two-entry key prefix, and the zero source entry with the flag set.
  Evidence: `python3 tools/rebuild/harness.py run --module overlay_queue_update`,
  `work/rebuild/oqup_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x289bc`.

## The opponent_update module at 0x20954

- Observation: the original body at `0x20954` holds 134 instructions and 454
  bytes up to the `ret` at `0x20b19`.  The plan row holds the same counts.  The
  store record keeps the 456-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x20954 --count 134` and
  `tools/rebuild/functions/0x20954.json`.
- Observation: the function has a frame.  It saves `edx`, `esi`, and `ebp`
  before the frame, sets `ebp`, subtracts `0x14` from `esp`, and stores the
  argument at `[ebp - 0x14]`.  The rebuild uses the option `-of+`.
  Evidence: the prologue at `0x20954` through `0x2095c`.
- Observation: a clear flag `DAT_0010cf58` returns at once.  A set flag starts
  the loop at the record base `DAT_0010f180` and ends it after
  `DAT_0010f178 * 0xbc` bytes.
  Evidence: the flag test at `0x2095f` and the end computation at `0x2096c`
  through `0x2098f`.
- Observation: a record runs when both absolute differences stay below 0xcb20:
  `DAT_00113ccc` minus the field at `+0x20`, and `DAT_00113cd0` minus the field
  at `+0x28`.  A record outside that range also runs when the argument is not
  zero.  The loop advances by `0xbc` bytes.
  Evidence: the distance code at `0x209aa` through `0x209dc` and the advance at
  `0x20994` and `0x20997`.
- Observation: a running record calls `0x22378` with `DAT_0010f174` and the
  body at `+0x44` only when the argument is not zero.  It then calls `0x20b1c`
  with the same two values and the `+0xc` float scaled by `DAT_00118e04`.
  Evidence: the call at `0x209f0` and the call at `0x20a12`.
- Observation: the function copies the float at `+0x78` to `+0x30`, copies
  `+0x54`, `+0x58`, and `+0x5c` to `+0x20`, `+0x24`, and `+0x28`, and writes
  the three copied values to the target record at `+8`, `+0xc`, and `+0x10`.
  Evidence: the copies at `0x20a17` through `0x20a81`.
- Observation: the target record comes from the pointer table at object-3
  offset `0x653c`.  The index is the field at `+0x18`, the entry stride is
  `0x1c`, and the first word of the entry holds the pointer.
  Evidence: the index code at `0x20a23` through `0x20a39`.
- Observation: the function scales the floats at `+0x2c`, `+0x30`, and `+0x34`
  by 1024.0 / pi, truncates toward zero, masks with `0x7ff`, and writes the
  three words at `+0x14`, `+0x18`, and `+0x1c`.  The constants are the float
  1024.0 at object-3 offset `0xcd9` and the float pi at object-3 offset
  `0xcdd`.
  Evidence: the three scale blocks at `0x20a84` through `0x20afa` and the
  constant bytes at file offset `0xf74d9`.
- Observation: the function calls `0x21b9c` with the record and with 1 when the
  argument is zero, and then it advances the loop.
  Evidence: the call at `0x20b0a` and the clear set by `sete`.
- Observation: the conversion helper at `0x4d9f4` truncates toward zero.  The
  module uses the local truncating FPU stub `opponent_update_fpu.asm`, because
  the shared `fpu_stub.asm` leaves the round-to-nearest mode.
  Evidence: the helper at `0x4d9f4` through `0x4da10` and
  `tools/rebuild/opponent_update_fpu.asm`.
- Observation: the rebuilt module holds 94 instructions and 327 bytes.  The
  identity check stays open at 14 of 94 instructions and 15 of 327 bytes.  The
  V2 toolchain keeps the argument in `ebx`, holds the record and the end in
  registers, folds the record stride into `imul`, and folds the table index
  into one `imul`.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x20954 --rebuilt work/rebuild/oppmod.exe --rebuilt-map
  work/rebuild/opponent_update.map --rebuilt-symbol opponent_update_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9` and `D`.  The paths cover the clear flag, the zero count, the
  near record with the zero and the one argument, the 0xcb20 boundary on x,
  the far record on z, the forced far record, the two-record loop, the
  truncating negative scale with the 0x7ff wrap, and the negative distance.
  Evidence: `python3 tools/rebuild/harness.py run --module opponent_update`,
  `work/rebuild/opp_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x20954`.

## The loading_screen module at 0x2a364

- Observation: the original body at `0x2a364` holds 127 instructions and 498
  bytes up to the `ret` at `0x2a555`.  The plan row holds the same counts.  The
  store record keeps the 500-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2a364 --count 127` and
  `tools/rebuild/functions/0x2a364.json`.
- Observation: the function has a frame.  It saves `ecx`, `esi`, `edi`, and
  `ebp` before the frame, sets `ebp`, subtracts `0x320` from `esp`, and stores
  the three arguments at `[ebp - 0x1c]`, `[ebp - 0x18]`, and `[ebp - 0x20]`.
  The rebuild uses the option `-of+`.
  Evidence: the prologue at `0x2a364` through `0x2a379`.
- Observation: the first call at `0x2f326` receives the pointer
  `DAT_000062ac` and the name at object-3 offset `0x100a`, which the executable
  holds as `NET_SPACE`.
  Evidence: the call at `0x2a383` and the string at file offset `0xf780a`.
- Observation: a second argument above 6 drops by 6 and sets the bit `0x08` of
  `DAT_0001d274`.  The function saves the full dword first and restores it at
  the end.  A value of 6 or less leaves the flag unchanged.
  Evidence: the compare at `0x2a388` and the set and restore blocks at
  `0x2a38e` through `0x2a3b5` and `0x2a53c` through `0x2a545`.
- Observation: the function sets `DAT_000203bc` to the adjusted argument minus
  1, calls `0x8bd54` with the table value `DAT_000021d8[context +8]`, copies the
  same context word to `DAT_0001d2b8`, clears 0x3e800 bytes at
  `DAT_00021528` through `0x8c160`, and fills the screen through `0x2aa98`.
  Evidence: the block at `0x2a3b8` through `0x2a3f3`.
- Observation: the function builds `load%d%c.pcx` with the adjusted argument
  and the letter `H` through `sprintf` at `0x4d32c`, loads the member from
  `gfx\graf.dat` into `DAT_0001fad0` with the palette at `DAT_0000cf5c`
  through `pcx_load` at `0x1b5a4`, and runs the cleanup at `0x15150` when the
  result is not zero.
  Evidence: the formats at object-3 offsets `0x1014`, `0x1021`, and `0x102e`,
  and the calls at `0x2a415`, `0x2a428`, and `0x2a431`.
- Observation: the function sets the palette through `0x81a05`, builds the
  palette shade tables through `0x258d0`, copies the pixels through `0x8c150`, runs
  the swaps at `0x1075f` and `0x107a1` behind the flags `0x10` and `0x20`, and
  draws `DAT_0001fc3c` at y `0x50` with font 3 and color 0 through `0x2e7cc`.
  Evidence: the calls at `0x2a43b` through `0x2a48c`.
- Observation: the function fills the screen again, builds `map%d` at
  `DAT_0001d294`, publishes `DAT_000203c4` at `DAT_0001d2a0`, the first two
  arguments at `DAT_0001d2b0` and `DAT_0001d2b4`, and the context words at
  `+0x38` and `+0x48` at `DAT_0001d2c8` and `DAT_0001d2cc`.
  Evidence: the block at `0x2a491` through `0x2a4e2`.
- Observation: the function reads the 0x50-byte record selected by the
  adjusted argument minus 1 from `DAT_00020430` into `DAT_0001d26c` when the
  low byte of `DAT_0001d260` holds none of the bits `0x64`, and stores 0
  otherwise.  The stride computation is `index * 5 * 16`.
  Evidence: the test at `0x2a4e2` through `0x2a509`.
- Observation: the function runs `race_cleanup` at `0x2a1b0`, runs `0x1fd70`,
  and on a non-zero result calls `0x108e4` with 3, `0x10978`, and `0x300be`.
  The tail then restores the saved flag when the argument was above 6 and calls
  `0x1be6c`.
  Evidence: the block at `0x2a50f` through `0x2a54a`.
- Observation: the rebuilt module holds 118 instructions and 459 bytes.  The
  identity check stays open at 40 of 118 instructions and 36 of 459 bytes.  The
  V2 toolchain keeps the arguments in registers, shares the error epilogue, and
  orders the flag restore and the tail call in another way.  The store records
  `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2a364 --rebuilt work/rebuild/ldsmod.exe --rebuilt-map
  work/rebuild/loading_screen.map --rebuilt-symbol loading_screen_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `8` and `D`.  The paths cover the normal load, the argument 9 that
  drops to 3 and sets and restores the flag, the failed load, the two swap
  flags, the mode 0x40 that skips the table, the error report, the argument 7
  that drops to 1, and the argument 6 that stays.
  Evidence: `python3 tools/rebuild/harness.py run --module loading_screen`,
  `work/rebuild/lds_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x2a364`.
- Observation: the harness cannot read the two literal strings of the module,
  because this minimal image does not load the constant segment.  The harness
  therefore checks the member name that `sprintf` writes into the local buffer
  and the map name, but not the resource name or the container name.
  Evidence: the note at `tools/rebuild/loading_screen_main.c` and the zero
  bytes at the low `CONST` offsets in `work/rebuild/loading_screen.map`.

## The race_heading_update module at 0x29610

- Observation: the original body at `0x29610` holds 136 instructions and 519
  bytes up to the `ret` at `0x29816`.  The plan row holds the same counts.  The
  store record keeps the 520-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x29610 --count 136` and
  `tools/rebuild/functions/0x29610.json`.
- Observation: the function has a frame.  It saves `ebx`, `ecx`, `edx`, `esi`,
  `edi`, and `ebp` before the frame, sets `ebp`, and subtracts `0x10` from
  `esp` for four local slots.  The rebuild uses the option `-of+`.
  Evidence: the prologue at `0x29610` through `0x29618`.
- Observation: the function converts `DAT_00120388` and `DAT_00120394` to the
  headings with the float 1024.0 at object-3 offset `0xf33` and the float pi at
  `0xf37`.  Each value is truncated toward zero through `0x4d9f4` and masked
  with `0x7ff`.  The heading pair wraps by `0x800` when the difference is at
  least `0x400`, in both directions.
  Evidence: the two conversion blocks at `0x2961b` through `0x29662` and the
  two wrap blocks at `0x29665` through `0x2969c`.
- Observation: the flag `DAT_0011d240` selects the interpolation factor.  A set
  flag calls the power routine at `0xc21e2` with the base `DAT_0012037c` and the
  double 2.0, and a clear flag copies `DAT_0012037c`.
  Evidence: the test at `0x2969d` and the calls at `0x296b9` and `0x296c1`.
- Observation: the function stores the scaled `DAT_00120384` at `DAT_00109a0`
  and the scaled `DAT_0012038c` at `DAT_00109a8`.  It interpolates the heading
  pair into `DAT_00109a4` and the three integer pairs `0x2039c`/`0x203a8`,
  `0x203a0`/`0x203ac`, and `0x203a4`/`0x203b0` into `DAT_00109ac`,
  `DAT_00109b0`, and `DAT_00109b4`.
  Evidence: the store and interpolation blocks at `0x296cb` through `0x297b4`.
- Observation: a zero `DAT_0012037c` returns after the outputs.  Otherwise the
  function sets `DAT_0012037c` to
  `DAT_0012037c - DAT_00118e08 * DAT_00120380`.  A negative result copies
  `DAT_00120394` into `DAT_00120388`, copies the three second integers into the
  first ones, and clears `DAT_0012037c`.
  Evidence: the sign test at `0x297ba`, the advance at `0x297c1` through
  `0x297d7`, and the reset at `0x297e6` through `0x29808`.
- Observation: the eight conversions to an integer use the truncating helper at
  `0x4d9f4`.  The module uses the local truncating FPU stub
  `race_heading_update_fpu.asm`, because the shared `fpu_stub.asm` leaves the
  round-to-nearest mode.
  Evidence: the helper at `0x4d9f4` through `0x4da10` and
  `tools/rebuild/race_heading_update_fpu.asm`.
- Observation: the rebuilt module holds 130 instructions and 502 bytes.  The
  identity check stays open at 25 of 130 instructions and 35 of 502 bytes.  The
  V2 toolchain saves `ebp` first, holds the headings in registers, keeps the
  pair differences and the interpolation values in local slots, and orders the
  power arguments in registers.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x29610 --rebuilt work/rebuild/rhupd.exe --rebuilt-map
  work/rebuild/race_heading_update.map --rebuilt-symbol
  race_heading_update_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `8` and `D`.  The paths cover the zero interpolation value, the power
  flag with the base 0.5 and the exponent 2.0, the two 0x400 wrap boundaries,
  the 0x3d1 difference without a wrap, the 2*pi mask to 0, the negative angles
  and the -1.5 truncation, the reset path, the positive frame step, and the
  negative zero early return.
  Evidence: `python3 tools/rebuild/harness.py run --module race_heading_update`,
  `work/rebuild/rhu_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x29610`.

## The entry probe tolerates repeated log lines

- Observation: DOSBox-X logs one instruction more than once, and the number of
  repeats changes between runs.  The entry detector skipped only the repeats of
  the entry jump itself.  When the log repeated the `sti` three times, the
  `mov ebx,esp` line fell outside the two-line window.  The fallback then
  selected the startup instruction as the entry, and stages 5 and 7 failed.
  Evidence: the pipeline run on 2026-11-03 reported five failures, and
  `work/entry/run/LOGCPU.TXT` held the entry at `0150:1F20F8` followed by three
  `sti` lines and `mov ebx,esp`.
- Observation: the detector now skips each run of equal log lines in the
  primary scan and in the fallback scan.  A unit test uses a log with repeated
  entry and `sti` lines.
  Evidence: `python3 -m unittest tools.rebuild.tests.test_entry_probe` and
  `python3 tools/entry_probe.py --game-dir
  ~/Documents/Screamer.app/Contents/Resources/game` reports the entry
  `0150:1F20F8` and the candidate `entry jump over the Watcom banner`.
- Observation: the full pipeline then reports 231 checks and 0 failures.
  Evidence: `python3 tools/pipeline_check.py --game-dir
  ~/Documents/Screamer.app/Contents/Resources/game`.
## The rebuild checks use le.py, not Ghidra or radare2

- No document states a decision to use `tools/le.py` in the rebuild checks
  instead of Ghidra or radare2.  The role split is visible in the code and the
  docs, but the project records no comparison.
  Evidence: a search of `docs/` and `tools/rebuild/` finds no such statement.
- The rebuild identity engine is the existing `tools/le.py`.  The framework
  design keeps it unchanged.
  Evidence: `docs/explanation/design-rebuild-instrumentation.md:262-263` maps
  `diff_module.py` to `le.py`, and lines 523-525 say the framework does not
  change the identity engine.
- The module diff needs the raw machine bytes.  `diff_module.py` reads both
  images with `le.py`, joins `Insn.raw` for each function body, and reports the
  byte compare.  A decompiler returns C text, and a disassembler returns a text
  listing, so neither gives a byte-for-byte verdict directly.
  Evidence: `tools/rebuild/diff_module.py:1-7`, `:121-163`, and `:217-222`.
- `le.py` is the only third-party disassembly dependency.  It loads capstone
  through `ctypes`, and the LE parser itself uses the standard library.
  Evidence: `tools/le.py:1-7` and `AGENTS.md:250-251`.
- The input fingerprint treats the analysis code as a versioned input.
  `TOOL_FILES` includes `le.py`, and `compute_fingerprint` hashes each tool
  source.  A Ghidra project under `work/ghidra/proj`, the loader extension
  outside the Nix store, and the radare2 analysis heuristics are not in the
  fingerprint.
  Evidence: `tools/rebuild/store.py:48-54` and `:349-391`, and
  `AGENTS.md:301-312`.
- A rebuild tool runs and exits and holds no state.  `le.py` is an importable
  library.  Ghidra needs a project directory and a headless run.  radare2 needs
  a subprocess and text parsing.
  Evidence: `AGENTS.md:221-222`, `tools/rebuild/diff_module.py:121-143`, and
  `tools/ghidra/decompile.sh:32-41`.
- The dependency rule allows the standard library and one `ctypes` binding to
  capstone.  Ghidra needs a second nixpkgs input and a manual extension
  install.
  Evidence: `AGENTS.md:250-251, 331-336`, `flake.nix:43-55`, and
  `tools/ghidra/install_loader.sh:1-13`.
- At the time of this finding, the radare2 cross-check was optional.  Stage 8
  was skipped when `r2` was not on `PATH`.  The core rebuild checks then ran
  on a host without `r2` or Ghidra.
  Evidence: the stage-8 radare2 check at the time of this finding.  The
  current gate is the mandatory three-tool cross-check.
- The docs do not reject Ghidra or radare2 for LE parsing.  radare2 has a
  native LE parser, and Ghidra reads LE through the LX/LE loader.
  Evidence: `docs/reference/tooling.md:637-639` and `:653-656`.
- The oracles supply what `le.py` lacks.  Ghidra applies the fixups, decompiles
  to C, and lists references.  radare2 recovers more functions: 923 against
  642.  `le.py` applies no fixup and does not decompress.
  Evidence: `docs/reference/tooling.md:691-695` and `:745`, and the radare2
  cross-check table in this page.
- At the time of this finding, stage 8 used radare2 as the independent
  cross-check of `le.py`.  It compared the entry point, the page 0 mapping,
  `main`, and the `main` caller.
  Evidence: the stage-8 radare2 check at the time of this finding.  The
  current gate is the mandatory three-tool cross-check.
- The records do not cite Ghidra or radare2.  The names and roles came from
  `program-map.md` in a one-time import.
  Evidence: a search of `tools/rebuild/functions/*.json` and
  `tools/rebuild/modules/*.json` finds no such reference, and the import
  finding in this page records the one-time import.
- Unknown: the project holds no measurement of what the rebuild would gain or
  lose if the identity engine used radare2 or Ghidra.  It has no such
  comparison.
- Note: the pipeline stage is now mandatory.  The bullets above record the
  earlier state.  See
  [The three-tool cross-check is mandatory](#the-three-tool-cross-check-is-mandatory).

## The three-tool cross-check is mandatory

- The full pipeline compares four identity facts from `tools/le.py`, radare2
  6.2.4, and Ghidra 12.0.4.  Stage 8 runs the compare on every full run, and
  the preflight refuses a missing capstone library, radare2, or Ghidra setup
  before stage 1.
  Evidence: `tools/cross_check.py`, `tools/pipeline_check.py`, and
  `tools/ghidra/identity.sh`.
- The three tools agree on the shipped `STARTH.EXE`:

  ```text
  entry: agree le=0x300f8 r2=0x300f8 ghidra=0x300f8
  page0: agree le=0x1a800->0x10000 r2=0x1a800->0x10000 ghidra=0x1a800->0x10000
  main: agree le=0x10020 r2=0x10020 ghidra=0x10020
  caller: agree le=0xd4364@0xd4324->0x10020 r2=0xd4364@0xd4324->0x10020 ghidra=0xd4364@0xd4324->0x10020
  ```

  Evidence: `nix develop .#cross-check --command python3 tools/cross_check.py
  --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE
  --main 0x10020 --call-site 0xd4364 --work-root work`.
- The identity values are the entry `0x300f8`, page 0 at file `0x1a800` and
  module `0x10000`, `main` at `0x10020`, and the caller `0xd4324` at call site
  `0xd4364` to `0x10020`.
  Evidence: the four `agree` lines above.
- The check counts are 231 checks for the full run, 227 checks with
  `--skip-cross`, 216 checks with `--skip-runtime`, and 212 checks with both
  flags.  `--skip-cross` skips stage 8 and the tool preflight.
  `--skip-runtime` skips stages 5, 6, 7, and 9 and still runs stage 8.
  Evidence: `nix develop .#cross-check --command python3 tools/pipeline_check.py
  --game-dir ~/Documents/Screamer.app/Contents/Resources/game` and the same
  command with `--skip-cross`, with `--skip-runtime`, and with both flags.
- The Ghidra loader creates the object blocks from a byte array, so the
  source info has no file bytes offset.  The page-0 file offset comes from the
  LE page table in the `.image` block.  Ghidra 12.0.4 appends
  ` (GhidraScript)` to each logged script line, and `identity.sh` strips the
  optional suffix.
  Evidence: `tools/ghidra/LeIdentity.java` and `tools/ghidra/identity.sh`.
- `tools/le.py` is a fingerprint input (`store.TOOL_FILES`).  The helper
  change updates every computed fingerprint, so the next verification run
  re-verifies every record once.  The record files stay unchanged.
  Evidence: `tools/rebuild/store.py` and the unchanged records under
  `tools/rebuild/functions/`.

## The reachable unknown candidates

- The verified and rebuilt call graph starts at ten functions: the five
  verified (`free`, `exit`, `printf`, `memory_available`, and `memcpy`), the
  four rebuilt (`bios_video`, `engine_shutdown`, `frame_step`, and
  `list_destroy`), and the matching `error_report`.  The graph holds 131 nodes.
  Of these nodes, 32 were stored `unknown` records.
  Evidence: the call graph from `python3 tools/le.py calls STARTH.EXE <root>`
  for each root and the records under `tools/rebuild/functions/`.
- The 32 records now carry a name, a role, a section, and a confidence.  The
  new sections are `Subsystem shutdown` and `The HMI sound driver`.  The other
  records join `The runtime memory manager`, `The resource layer`, `The sound
  loaders`, and `Program functions`.
  Evidence: `python3 tools/rebuild/docs.py render`,
  `python3 tools/rebuild/docs.py check` (23 sections, no drift), and
  `python3 tools/rebuild/store.py check` (683 records, 0 violations).
- The shutdown path holds five mapped functions.  `engine_interface_free` at
  `0x108c8` releases the engine interface list.  `video_text_mode` at `0x4c928`
  sets the text mode 3.  `music_bank_free` at `0x2fc90` stops the song and
  removes the two bank resources.  `sound_system_shutdown` at `0x1f28c` stops
  the timer and releases the song.  `dos_clock_sync` at `0x1fa2c` copies the
  real-time clock into the DOS date and time.
  Evidence: the decompiled `FUN_000108c8`, `FUN_0004c928`, `FUN_0002fc90`,
  `FUN_0001f28c`, and `FUN_0001fa2c`.
- The memory probe path holds four mapped functions.  `heap_block_largest` at
  `0x2f5ae` and `dos_block_largest` at `0x2f66c` find the largest single block.
  `heap_memory_available` at `0x2f758` and `dos_memory_available` at `0x2f82e`
  allocate blocks until the largest block falls to 0x100 and return the total.
  `memory_available` at `0x2f8ca` sums the two totals.
  Evidence: the decompiled `FUN_0002f5ae`, `FUN_0002f66c`, `FUN_0002f758`,
  `FUN_0002f82e`, and `FUN_0002f8ca`.
- The HMI sound driver path holds nineteen mapped functions.  The timer path is
  `music_timer_update` at `0xc86dc`, `music_timer_set` at `0xc88b2`, and
  `music_timer_shutdown` at `0xc8321`.  The DPMI path is `music_driver_lock` at
  `0xc8cbb`, `sound_driver_lock` at `0xd233c`, and the two reset functions at
  `0xc8ffa` and `0xd2743`.  The voice path is `sound_channel_setup`,
  `sound_channel_free`, `sound_voice_release`, `sound_notes_apply`, and the
  varint reader at `0xd3c8a`.
  Evidence: the decompiled `FUN_000c86dc`, `FUN_000c88b2`, `FUN_000c8321`,
  `FUN_000c8cbb`, `FUN_000d233c`, `FUN_000c8ffa`, `FUN_000d2743`,
  `FUN_000d39ec`, `FUN_000d38e6`, `FUN_000df4f8`, `FUN_000e3adb`, and
  `FUN_000d3c8a`.
- The earlier note that `engine_shutdown` stops the music with
  `FUN_00014e20(0)` is corrected.  The call `0x14e20(0)` restores the saved
  interrupt 9 handler.  The music stop is `music_bank_free` at `0x2fc90` and
  `sound_system_shutdown` at `0x1f28c`.
  Evidence: the decompiled `FUN_00014e20`, `FUN_0002fc90`, and `FUN_0001f28c`.
- 70 call targets of the graph were not store records.  Many start with
  `push ebp; mov ebp,esp` encoded as `55 8b ec`, and some have no frame at all.
  The scan now matches both prologue encodings, and the seed adds the direct
  call targets of the verified and rebuilt roots.  All 70 are now candidates.
  Evidence: `python3 tools/rebuild/store.py seed --original STARTH.EXE`
  (added 105, lost 0, overlaps 0) and `python3 tools/rebuild/store.py seed
  --check --original STARTH.EXE` (no difference).
- The decompilation command for the batch:

  ```sh
  python3 tools/ghidra/decompile.sh 0x108c8 0x1f28c 0x1fa2c 0x2f758 \
    0x2f82e 0x2fc90 0x300d6 0x4c928 0x2f326 0x2f40b 0x2f5ae 0x2f66c \
    0x2fe68 0xc8321 0xc86dc 0xc8ffa 0xc98f9 0xd2743 0xd2cab 0xc88b2 \
    0xc8cbb 0xc9d0d 0xd233c 0xd380d 0xd38e6 0xdf4f8 0xdf7aa 0xe2723 \
    0xd39ec 0xe3adb 0xd3c8a 0xe2f41
  ```

## The second prologue encoding and the call-target candidates

- The Watcom prologue has two encodings: `55 89 e5` and `55 8b ec`.  Both are
  `push ebp; mov ebp,esp`.  The `function_starts`, `nearest_function_start`,
  and the store entry check use both patterns.
  Evidence: the byte dump at `0xde7fd`, `0xde9be`, and `0xdf00c` (`55 8b ec`),
  `python3 tools/le.py funcs STARTH.EXE --limit 10000` (688 starts), and
  `python3 tools/le.py near STARTH.EXE 0xde800`.
- The `targets` command walks the direct call graph from the given roots.  It
  uses the same `function_body` and `call_sites` as the `calls` command and
  stops at the same terminators.  An indirect call has no target and is not
  added.
  Evidence: `python3 tools/le.py targets STARTH.EXE 0x108e4 0x10978 0x128ac
  0x2aa84 0x2ed77 0x2f8ca 0x3009b 0x300be 0x300ec 0x8c150` (`call targets:
  123`).
- The store seed unions four sources: the prologue scan, the direct call
  targets of the verified and rebuilt roots, the entry jump at `0x300f8`, and
  the exit helper at `0x300d6`.  A call target is a function entry even when
  the function has no frame, so the graph is the only source for the frameless
  functions, for example `0x101f6`, `0x25f14`, `0x264ec`, and `0x4d9ba`.
  Evidence: `store.scan_candidates` and the seed output.
- The seed added 105 records: 46 new prologue starts and 59 call targets that
  the prologue scan does not find.  11 of the 70 call targets are in the 46, so
  all 70 are now records.  The store holds 788 records and every node of the
  reachable graph is a record.
  Evidence: `python3 tools/rebuild/store.py seed --original STARTH.EXE`
  (added 105, lost 0, overlaps 0), `python3 tools/rebuild/store.py check
  --original STARTH.EXE` (788 records, 0 violations), and
  `python3 tools/rebuild/store.py seed --check --original STARTH.EXE` (no
  difference).
- The 105 new records have the status `unknown` and no section, so the
  generated program-map tables do not change.  The `size` of 15 existing
  records shrinks because a new entry divides their reach.
  Evidence: `python3 tools/rebuild/docs.py check` (no drift) and the record
  diff of the seed run.
- The `le.py` change is a fingerprint input change.  Every stored result now
  holds an old fingerprint, so the next verification run re-verifies the 11
  records with a result once.  The records themselves stay unchanged.
  Evidence: `store.TOOL_FILES` and `store.compute_fingerprint`.
- The merge with master added 145 new records from the module rebuilds.  The
  new rebuilt and verified roots add 158 further call-target candidates, and
  the seed writes them.  The store holds 1091 records.
  Evidence: `git merge origin/master`, `python3 tools/rebuild/store.py seed
  --original STARTH.EXE` (added 158, lost 0, overlaps 0), and `python3
  tools/rebuild/store.py check --original STARTH.EXE` (1091 records, 0
  violations).

## The menu_frame module at 0x29f80

- Observation: the original body at `0x29f80` holds 151 instructions and 560
  bytes.  It holds the six register saves, the frame with the 0x30c-byte local
  area, 10 calls, and 4 FPU instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x29f80 --count 151`.
- Observation: the first call is `pcx_load` at `0x1b5a4`.  The file name is
  `gfx\graf.dat` at object-3 offset `0xfe0`, and the member is `menu1h.pcx` at
  `0xfd5`.  The pixel buffer is `DAT_001fad0`, and the palette is `0xcf5c`.  A
  non-zero result calls the cleanup at `0x15150`.
  Evidence: the block at `0x29f8e` through `0x29fb1`, the bytes at linear
  `0x100fe0` and `0x100fd5`, and the `menu_frame` row of
  `docs/reference/program-map.md`.
- Observation: the function calls `video_present` at `0x27ec8` with the scale 0
  and the offset 0.  It copies `0x3e800` bytes from `DAT_001fad0` to
  `DAT_0021528` through `memcpy` at `0x8c150`.  It runs `video_block_swap_a` at
  `0x1075f` behind the flag `0x10` and `video_block_swap_b` at `0x107a1` behind
  the flag `0x20` of `DAT_001d274`.  It calls `video_fill` at `0x2aa98` and then
  `video_present` again with the same arguments.
  Evidence: the block at `0x29fb1` through `0x2a000`.
- Observation: the function sets the menu constants.  The frame flag
  `DAT_001d234` and the overlay words `DAT_001fac0` and `DAT_001fac4` and the
  game state `DAT_00203b8` become 0.  The frame step `DAT_0018e08` becomes 0.08,
  and the counter `DAT_000007f9` becomes 8.  The limit `DAT_001fab8` becomes
  `0x35e`, and the flag `DAT_001d240` becomes 1.  The factors `DAT_0020378` and
  `DAT_0020380` become 0.6, and the fade state `DAT_001fac8` becomes 1.  The menu
  value `DAT_001fb64` becomes -1.  The function clears `DAT_001fb58`,
  `DAT_001d224`, `DAT_001d248`, `DAT_00203a8`, `DAT_002039c`, `DAT_00203ac`,
  `DAT_00203a0`, `DAT_00109a0`, `DAT_0020384`, `DAT_00109a4`, `DAT_0020388`,
  `DAT_0020394`, `DAT_00109a8`, `DAT_002038c`, and `DAT_002037c`.  The stores at
  `DAT_00203b0` and `DAT_00203a4` become `0x1004`.
  Evidence: the block at `0x2a000` through `0x2a12f`.
- Observation: the two screen offsets use the fields at `DAT_0015ccc + 0x134`
  and `+0x120`.  The function divides the field by 2, subtracts the result from
  `0x140`, divides by 2, and subtracts `0x190` for x or `0x320` for y.  The
  results land at `DAT_0020824` and `DAT_0020828` as floats.
  Evidence: the block at `0x2a02e` through `0x2a053` and the block at
  `0x2a059` through `0x2a129`.
- Observation: the function keeps the longest of the three strings at
  `DAT_001fc1c`, `DAT_001fc20`, and `DAT_001fc24` in `DAT_00203b4`.  It calls
  the run init at `0x2dff4` and `table_lookup` at `0x1f718` with 0.
  Evidence: the block at `0x2a12f` through `0x2a1a6`.
- Observation: the two float conversions use `fild` and `fstp`.  The module
  uses the shared `fpu_stub.asm` and needs no truncating stub.
  Evidence: `tools/rebuild/modules/menu_frame.json`.
- Observation: the rebuilt module holds 116 instructions and 490 bytes.  The
  identity check stays open at 45 of 116 instructions and 45 of 490 bytes.  The
  V2 toolchain saves the registers in another order, omits the 0x30c-byte local
  area, and replaces the inline `repne scasb` string length with a helper.  The
  store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x29f80 --rebuilt work/rebuild/mnfmod.exe --rebuilt-map
  work/rebuild/menu_frame.map --rebuilt-symbol menu_frame_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `4` and `D`.  The paths cover both swap flags, the screen offsets with
  odd field values, the failed load, the three string lengths, the two present
  calls, the copy, and every constant.
  Evidence: `python3 tools/rebuild/harness.py run --module menu_frame`,
  `work/rebuild/mnf_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x29f80`.

## The list_file_load module at 0x15460

- Observation: the original body at `0x15460` holds 161 instructions and 581
  bytes.  It holds the three register saves, the frame with the 0x5a0-byte local
  area, 15 calls, and 1 FPU instruction.  The plan row holds the same counts.
  The merged store record keeps the 584-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x15460 --count 161`,
  `docs/explanation/rebuild-plan.md`, and `tools/rebuild/functions/0x15460.json`.
- Observation: the four register arguments are the type in `eax`, the name in
  `edx`, the list name in `ebx`, and the destination table in `ecx`.  The two
  stack arguments are the record stride and the record width.  `engine_init`
  passes `GEN`, `MENU`, `CD_TRACKS`, the table `0x1f00`, and the two values 4.
  Evidence: the prologue at `0x1546e` through `0x15477`, the call at `0x127ca`,
  and the strings at object-3 offsets `0x437`, `0x432`, and `0x428`.
- Observation: the function builds `%s\data\%s.dat` from the base path at
  `DAT_00007edc` and the type through `sprintf` at `0x4d32c`.  It builds
  `%s.dtr` from the name.  It opens the path as the member with the category
  `rt` through the open at `0x4c08d`.  A failed open reports the file error at
  `0x4bf88` and calls `exit(1)` at `0x300be`.
  Evidence: the block at `0x1547a` through `0x154d3`, the strings at object-3
  offsets `0x63f`, `0x64e`, and `0x655`, and `docs/reference/program-map.md`.
- Observation: the function reads whole lines with the mode 1 until the list
  name, and then reads tokens with the mode 0 until `END_LIST`.  A token `*`
  reads the field line and the parse continues.  The type byte selects `d` for
  `atoi` at `0xc2045`, `f` for `atof` at `0xc209c`, `x` for `sscanf` with `%x`
  at `0xc2024`, and `s` for an inline string copy of 0xc bytes.  The other
  bytes are separators.  The store index is `row * stride + column`.  A full
  record advances the row and clears the column.
  Evidence: the block at `0x154e9` through `0x15667` and the reader at
  `0x4c695`.
- Observation: the reader at `0x4c695` takes the stream in `eax`, the buffer in
  `edx`, and the mode in `ebx`.  The mode 1 stops at the newline.  The mode 0
  stops at the tab, the space, or the newline.  Both modes end the text with 0.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4c695 --count 106`.
- Observation: the function closes the stream through `0x4c38d` and returns the
  row count.  The `ret 8` removes the two stack arguments.
  Evidence: the block at `0x1568e` through `0x156a2`.
- Observation: DOS/4G 2.01a under DOSBox-X leaves the initialized data pages
  zero.  The temporary probe shows that an initialized array reads 0 until the
  program writes it.  The harness build defines `LIST_FILE_LOAD_HARNESS`, so the
  module builds the constant text in local buffers.  The default build keeps
  the literals of the original.  The harness also builds every text in writable
  data at run time.  The shared harness needs no change.
  Evidence: `tools/rebuild/modules/list_file_load.json`,
  `tools/rebuild/list_file_load.c`, and `tools/rebuild/list_file_load_main.c`.
- Observation: the rebuilt module holds 208 instructions and 533 bytes.  The
  identity check stays open at 24 of 161 instructions and 33 of 581 bytes.  The
  V2 toolchain builds the constant text at run time for the harness, uses other
  registers, and reorders the field dispatch.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x15460 --rebuilt work/rebuild/lflmod.exe --rebuilt-map
  work/rebuild/list_file_load.map --rebuilt-symbol list_file_load_`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, and `D`.  The normal path covers the two formatted names, the open, the
  line search, the `*` skip, the `d`, `f`, `x`, and `s` fields, the ignored
  token, two full records, and the close.  The failed open covers the report,
  the exit 1, and the empty read.
  Evidence: `python3 tools/rebuild/harness.py run --module list_file_load`,
  `work/rebuild/lfl_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x15460`.

## The object_revolution_step module at 0x1d30c

- Observation: the original body at `0x1d30c` holds 151 instructions and 589
  bytes.  It holds the six register saves, the frame with the 0x10-byte local
  area, 9 calls, and 38 FPU instructions.  The plan row holds the same counts.
  The record keeps the 592-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1d30c --count 151`,
  `docs/explanation/rebuild-plan.md`, and `tools/rebuild/functions/0x1d30c.json`.
- Observation: the function multiplies the speed at `+0x418` by the frame delta
  `DAT_00118e04`, divides the product by the axis call at `0x222e0`, and adds
  the result to the fraction at `+0x250`.  A fraction below 1.0 leaves the loop.
  At 1.0 and above the function clears the fraction, toggles the gear at
  `+0x254`, and advances the index at `+0x24c` from the entry at `+0x10`.
  Evidence: the block at `0x1d314` through `0x1d3df`.
- Observation: each loop iteration calls the matrix helper at `0x1d55c` and then
  truncates the two positions at `+0x2a8` and `+0x2ac` into `+0x258` and
  `+0x260` with the helper at `0x4d9f4`.  The loop also subtracts the distance
  from the local step, but the next iteration overwrites the local step, so the
  subtraction has no effect on the loop control.  The two truncation stores
  stay.
  Evidence: the block at `0x1d3e2` through `0x1d450`.
- Observation: the exit path calls the matrix helper once, truncates the two
  positions, sets the output at `+0x25c` from the entry at `+0xc`, copies the
  five state fields, and stores the angle from `0x226bc` at `+0x27c`.  The
  angle call takes the x, y, and z values at `+0x264`, `+0x268`, and `+0x26c`.
  Evidence: the block at `0x1d455` through `0x1d558`.
- Observation: the table lookup uses the base pointer at `DAT_0010f8e4`.  The
  gear index at `+0x248` selects a 36-byte record, and its first dword points to
  an entry array with a 56-byte stride.  The entry dword sits at offset `+0xc`
  for the output and at offset `+0x10` for the next index.
  Evidence: the block at `0x1d39a` through `0x1d3d9` and the block at `0x1d48b`
  through `0x1d4c4`.
- Observation: the rebuilt module holds 89 instructions and 423 bytes.  The
  identity check stays open at 14 of 89 instructions and 40 of 423 bytes.  The
  V2 toolchain omits the six register saves, keeps the object in `edx`, uses an
  integer compare for the 1.0 boundary, and uses multiply immediates for the two
  table strides.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1d30c --rebuilt work/rebuild/orsmod.exe --rebuilt-map
  work/rebuild/object_revolution_step.map --rebuilt-symbol
  object_revolution_step_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `5` and `D`.  The paths cover the no-wrap path, the wrap with gear 0
  and gear 1, the index update, the negative truncation, the exact 1.0 boundary,
  and the distance argument 18.  The module uses a local truncating FPU stub,
  because the shared stub rounds to nearest.
  Evidence: `python3 tools/rebuild/harness.py run --module
  object_revolution_step`, `work/rebuild/ors_out.txt`, and `python3
  tools/rebuild/run.py run --function 0x1d30c`.

## The startup module at 0x30170

- Observation: the original body at `0x30170` holds 196 instructions and 605
  bytes and ends at the `ret` at `0x303cc`.  The record keeps the 114032-byte
  extent to the next entry at `0x4bee0`, so the identity check uses
  `--count 196`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x30170 --count 196`,
  `docs/explanation/rebuild-plan.md`, and
  `tools/rebuild/functions/0x30170.json`.
- Observation: the body is the Watcom C/C++32 DOS-extender start routine from
  `bld/clib/startup/a/cstrt386.asm` of OpenWatcom V2.  It holds the interrupt
  enable, the stack alignment to four bytes, the stack top store at `0x4294`,
  the break store at `0x4280`, the DOS version call, the Phar Lap, CauseWay,
  Rational DOS/4G, and unknown extender paths, the PSP words, the command-line
  copy from `PSP:0x81`, the `no87=` environment scan, the program-name copy,
  the BSS clear with the `0x1000` limit for the Rational extender, the
  initializer call at `0xd4374`, the main invoker call at `0xd4324`, the
  message writer at `0x30389`, the finalizer call at `0xd43bf`, the DOS exit,
  and the DS restore.
  Evidence: the block at `0x30170` through `0x303cc` and the OpenWatcom V2
  source file `bld/clib/startup/a/cstrt386.asm`.
- Observation: the body holds three calls, at `0x3037b` to `0xd4374`, at
  `0x30382` to `0xd4324`, and at `0x303b7` to `0xd43bf`.  The two data
  boundaries are `_edata` at `0x62ac` and `_end` at `0x27eb8`, so the BSS
  spans `0x21c0c` bytes and the Rational path clears the first `0x1000` bytes.
  Evidence: `python3 tools/le.py calls STARTH.EXE 0x30170` and the block at
  `0x3032c` through `0x3035e`.
- Observation: the rebuilt module holds 40 of 196 instructions and 22 of 605
  bytes.  The C compiler does not produce the inline interrupt forms, the
  segment moves, or the exact register allocation, so the identity check stays
  open and the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x30170 --count 196 --rebuilt work/rebuild/stumod.exe
  --rebuilt-map work/rebuild/startup.map --rebuilt-symbol startup_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `5` and `D`.  The paths cover the Phar Lap version word, the PSP
  high address and the resize, the command-line copy, the `no87=` scan, the
  program name, the full BSS clear, the Rational zero-base and non-zero-base
  subtypes with the `0x1000` BSS limit, the CauseWay heap break and command
  offset, the unknown extender, and the message and exit chain.  The harness
  builds every text at run time, because DOS/4G 2.01a under DOSBox-X leaves the
  initialized data pages zero.
  Evidence: `python3 tools/rebuild/harness.py run --module startup`,
  `work/rebuild/stu_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x30170`.

## The joystick_update module at 0x24da0

- Observation: the original body at `0x24da0` holds 201 instructions and 630
  bytes.  The plan row holds the same counts.  The record keeps the 2176-byte
  extent to the next entry, so the identity check uses `--count 201`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x24da0 --count 201`,
  `docs/explanation/rebuild-plan.md`, and
  `tools/rebuild/functions/0x24da0.json`.
- Observation: the function walks the six 28-byte control records at
  `0x197d8`.  It stores one dword for each record at `0x19880`.  The control
  record holds the type at `+0x00`, the kind at `+0x04`, the mode at `+0x08`,
  the axis index at `+0x0c`, the device index at `+0x10`, and the button mask
  or the state pointer at `+0x18`.
  Evidence: the block at `0x24faf` through `0x25015` and the `controls_load`
  read of six 28-byte records into `0x197d8` and ten 4-byte records into
  `0x19704`.
- Observation: a record of type 1 and mode 0 or 2 is an axis.  A kind 1 record
  first searches the earlier records for the same device and the same axis.
  When it finds one, it copies the earlier output and makes no read.  This
  search stops at the current record, so it cannot match itself.
  Evidence: the block at `0x24dd7` through `0x24e2b`.
- Observation: otherwise the function reads the axis through the helper at
  `0x10263` with the mask `1 << device` and the port from `+0x10` of the
  28-byte axis record at `0x192a4`.  The axis address is
  `0x192a4 + device * 0x280 + axis * 0x1c`.  The function keeps the low 12
  bits, divides by the divisor at `+0x0c`, subtracts the center at `+0x04`,
  and multiplies by the scale at `+0x14` for a positive difference and at
  `+0x18` for a difference of zero or less.  The helper at `0x4d9f4` truncates
  the product toward zero.
  Evidence: the block at `0x24e38` through `0x24ee2` and the helper at
  `0x4d9f4`.
- Observation: the value is clamped to the range -1023 through 1023.  A mode 2
  record sets the full scale 1023 or -1023 when the value passes 511 or -511.
  A value below 102 in magnitude becomes zero.
  Evidence: the block at `0x24ee2` through `0x24f49`.
- Observation: a record of kind other than 1 reads the port from the 10-dword
  table at `0x19704` through the helper at `0x10257` and tests the mask at
  `+0x18`.  A set bit stores 1023 and a clear bit stores 0.  A mode other than
  0 or 2 tests the byte at `+2` of the state pointer at `+0x18` and stores
  1023 or 0.
  Evidence: the block at `0x24f50` through `0x24f9b`.
- Observation: the rebuilt module holds 48 of 201 instructions and 13 of 630
  bytes.  The identity check stays open.  The V2 toolchain saves fewer
  registers, uses a different loop shape, and forms the record index with a
  multiply.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x24da0 --count 201 --rebuilt work/rebuild/joymod.exe
  --rebuilt-map work/rebuild/joystick_update.map --rebuilt-symbol
  joystick_update_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9`, `A`, `B`, and `D`.  The paths cover the positive scale, the
  negative scale with the truncation toward zero, the dead zone, the high and
  low clamps, the mode 2 high clamp, the duplicate copy with one read, the raw
  button mask set and clear, the state pointer set and clear, and an unchanged
  record of another type.
  Evidence: `python3 tools/rebuild/harness.py run --module joystick_update`,
  `work/rebuild/joy_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x24da0`.

## The menu_space_init module at 0x29d08

- Observation: the original body at `0x29d08` holds 156 instructions and 632
  bytes.  The plan row holds the same counts.  The record keeps the 632-byte
  extent, so the identity check uses the whole body.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x29d08 --count 156`,
  `docs/explanation/rebuild-plan.md`, and
  `tools/rebuild/functions/0x29d08.json`.
- Observation: the function requests the `NET_SPACE` resource with the size
  `0x2000` and the `OPT_PCX` resource with the size `0x3e800`.  A zero result
  runs the memory error report at `0x2edad`, and the function continues.
  Evidence: the calls at `0x29d2a` and `0x29e37` and the two error branches at
  `0x29d2f` and `0x29e41`.
- Observation: the function builds `%s\gfx\graf.dat` and `%s\gfx\menu.dat` from
  the base path at `0x7edc`.  It builds the sprite member `menuH.spr` and the
  font member `menuH.fnt`, with `0x48` as the character.  A non-zero sprite or
  font result runs the cleanup at `0x15150`.
  Evidence: the `sprintf` calls at `0x29d93`, `0x29da6`, `0x29dcd`, and
  `0x29dfa`, the string table at `0x100f4d`, and the two branches at
  `0x29db9` and `0x29de0`.
- Observation: the function stores the rectangle `0`, `0`, `0x280`, and
  `0x190` at `DAT_00015ce4` through `DAT_00015cf0` and sets the screen with
  `0x140`, `0x190`, and `0xc8` at `0x4c850`, which doubles the first value to
  the width 640 and holds the height 400.
  Evidence: the stores at `0x29d78` through `0x29d8d` and the call at
  `0x29e23`.
- Observation: the function loads four member lists with the base pointers at
  `DAT_0001fb70`, `DAT_0001fb74`, `DAT_0001fb78`, and `DAT_0001fb7c` and the
  pointer arrays at `0x1fb80`, `0x1fd10`, `0x1fea0`, and `0x20030`.  It then
  loads the two list files `GEN`/`MENU`/`CHEATS` and `GEN`/`MENU`/`SOUND_FX`
  with the strides and counts `0x18`/`0xc` and `0x24`/`0x24`.
  Evidence: the four calls at `0x29e6f`, `0x29e83`, `0x29e9c`, and
  `0x29eb5`, and the two calls at `0x29ee0` and `0x29efd`.
- Observation: the music path runs only when `DAT_0001d2cc` is not zero.  It
  applies the input state `DAT_00001f04 + 1`, builds `music%d` from
  `DAT_00001f04`, loads the track into the record `0x62b0`, and selects it
  with 1.
  Evidence: the flag test at `0x29f27`, the input apply at `0x29f3b`, the
  `sprintf` call at `0x29f50`, and the two music calls at `0x29f60` and
  `0x29f6f`.
- Observation: the rebuilt module holds 151 instructions and 637 bytes.  The
  identity check stays open at 72 of 632 bytes and 46 of 151 instructions.
  OpenWatcom V2 saves the registers after the frame, uses other local offsets,
  and forms the calls in another order.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x29d08 --rebuilt work/rebuild/msimenu.exe --rebuilt-map
  work/rebuild/menu_space_init.map --rebuilt-symbol menu_space_init_`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `3` and `D`.  The paths cover the normal path, the two load
  failures, the music path, and the two zero resources.
  Evidence: `python3 tools/rebuild/harness.py run --module menu_space_init`,
  `work/rebuild/msimenu.txt`, and `python3 tools/rebuild/run.py run --function
  0x29d08`.

## The object_gear_step module at 0x1d834

- Observation: the original body at `0x1d834` holds 133 instructions and 635
  bytes up to the first `ret`.  The plan row holds the same counts.  The record
  keeps the 772-byte extent to the next entry at `0x1db38`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1d834 --count 133`,
  `docs/explanation/rebuild-plan.md`, and
  `tools/rebuild/functions/0x1d834.json`.
- Observation: the function branches on the state at `+0x2f8` through the jump
  table at `0x1d824`.  The table holds the CS-relative offsets `0xd8a8`,
  `0xd85a`, `0xdaaf`, and `0xdac9` for the states 1 through 4.
  Evidence: the `jmp dword ptr cs:[eax*4 + 0xd824]` at `0x1d852` and the
  16 bytes at linear `0x1d824`.
- Observation: the four constants live in object 3.  The float at
  `0x100b2c` is 6000.0, the double at `0x100b30` is 0.4, the double at
  `0x100b38` is 0.6, and the float at `0x100b40` is 12.566371.  The harness
  supplies the four values through globals with the exact bit patterns.
  Evidence: the reads at `0x1d97e`, `0x1d9d1`, `0x1da32`, and `0x1db0a`.
- Observation: state 2 sets the state to 1, the timer `+0x2fc` to 3000.0, the
  counter `+0x300` to 0, the velocities `+0x304` and `+0x308` to 12.0, and the
  velocity `+0x30c` to 0.  State 1 integrates the position `+0x49c` and the
  three velocities with the damping at `DAT_0010cf08 + 0x184`, and it
  subtracts 6000.0 times the frame delta from the timer.
  Evidence: the block at `0x1d85a` through `0x1d98d`.
- Observation: when the position stays at or below the target at `+0x238`,
  the function increments the counter, snaps the position to the target,
  reflects the timer by 0.4, clears the three integrated values, sets the
  velocity `+0x30c` from the counter parity and 0.6, sets the velocity
  `+0x304` to -1.0, and clears the velocity `+0x308`.  When the counter passes
  3, it clears the three integrated values and the state.
  Evidence: the block at `0x1d993` through `0x1da65`.
- Observation: state 3 sets the state to 4 and the velocity `+0x308` to 24.0.
  State 4 damps that velocity, integrates it into `+0x318`, and clears the
  value and the state when the value reaches 12.566371.
  Evidence: the block at `0x1daaf` through `0x1db25`.
- Observation: the rebuild uses an if-else chain, because the V2 switch jump
  table at the start of the code object faults in the harness.  The same fault
  shows in the earlier switch module `error_report`, whose stored run now
  faults at linear 1.  The rebuild also uses a local truncating FPU stub,
  because the shared stub rounds to nearest and the original helper at
  `0x4d9f4` truncates toward zero.
  Evidence: `python3 tools/rebuild/harness.py run --module error_report` and
  `work/rebuild/err_out.txt`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `7` and `D`.  The paths cover the state 1 and state 2 integration,
  the two wrap branches, the counter reset, the negative truncation, the two
  state 3 and state 4 branches, and the default path.
  Evidence: `python3 tools/rebuild/harness.py run --module object_gear_step`,
  `work/rebuild/ogst_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x1d834`.
- Observation: the rebuilt module holds 109 instructions and 591 bytes.  The
  identity check stays open at 73 of 591 bytes and 26 of 109 instructions.  The
  if-else chain and the V2 register use change the code shape.  The store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1d834 --rebuilt work/rebuild/ogstmod.exe --rebuilt-map
  work/rebuild/object_gear_step.map --rebuilt-symbol object_gear_step_`.

## The object_collision module at 0x1e00c

- Observation: the original body at `0x1e00c` holds 193 instructions and 765
  bytes up to the first `ret`.  The plan row holds the same counts.  The record
  keeps the 768-byte extent to the next entry at `0x1c84c`.  The function
  returns while `DAT_00108058` is 0.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1e00c --count 193`,
  `docs/explanation/rebuild-plan.md`, and
  `tools/rebuild/functions/0x1e00c.json`.
- Observation: when the state at `+0x414` is set and the target at `+0x4ec`
  is -1, the function scans the 0x590-byte records at object-3 offset 0x10ab8
  while the counter `DAT_00113cc8` is 3.  It skips the own record, a record
  with the target -1, and a record whose distance at `+0x4e4` is not larger
  than the object distance.  The first other record with a larger distance
  supplies the target, clears its own target, and decrements the counter.
  Evidence: the block at `0x1e081` through `0x1e0f8`.
- Observation: when the counter stays below 3, the function truncates the
  distance into `+0x4f0`, takes the default id from the two-entry table at
  object-3 offset 0x1e30 for the type at `+0x2e0` against 0xc, calls
  `table_lookup` at 0x1f718, stores the id, and increments the counter.  The
  table holds 2 for the other types and 10 for type 0xc.
  Evidence: the block at `0x1e0fe` through `0x1e150` and the 8 bytes at
  object-3 offset 0x1e30.
- Observation: the body copies the angle at `+0x418` to `+0x4f4` while the low
  byte of `DAT_00118e1c` holds bit 0 or bit 2.  It rotates the deltas at
  `+0x498` and `+0x4a0` against `DAT_00113ccc` and `DAT_00113cd0` with the
  angle `DAT_00113cd4`.  It forms the values `((+0x4f0 - +0x4e4) * 9.0) +
  (+0x4f4 * 2.4) + 13568.0`, `((35000.0 - +0x4e4) / 35000.0)^2 * 23552.0`,
  and `sqrt(abs(rotated_x / +0x4e4)) * 32767.0 * sign + 0x8000`.  It then
  stores the truncated distance at `+0x4f0`.
  Evidence: the block at `0x1e177` through `0x1e28e`.
- Observation: the constants live in object 3.  The float at offset 0xb64 is
  9.0, the double at 0xb68 is 2.4, the double at 0xb70 is 13568.0, the float
  at 0xb78 is 35000.0, the float at 0xb7c is 23552.0, and the double at 0xb80
  is 32767.0.  The harness supplies the six values through globals with the
  exact bit patterns.
  Evidence: the reads at `0x1e1dd`, `0x1e1ec`, `0x1e1f4`, `0x1e205`,
  `0x1e219`, and `0x1e25c`.
- Observation: the three stores call `0x1f7e4` with the two distance values
  and `0xc667d` and `0xc6920` with the object-3 pointer, the target id, and
  the second distance and angle values.  The low bit of `DAT_0011d261` and
  the flag at `+0x564` gate the three stores.  A clear state releases the
  target through 0x1f880 and decrements the counter.
  Evidence: the block at `0x1e28f` through `0x1e2fa`.
- Observation: the original truncates with the Watcom helper at `0x4d9f4`,
  so the module uses a local truncating FPU stub.  The harness supplies the
  stubs for `table_lookup` at 0x1f718, `0x1f7e4`, `0x1f880`, `0x2218c`,
  `0x22bec`, `0xc667d`, and `0xc6920`.
  Evidence: `tools/rebuild/object_collision_fpu.asm`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9` and `D`.  The paths cover the disabled gate, the release path,
  the scan hit and miss, both default ids, the distance and type compares, the
  four state flags, the copy at `+0x418`, the negative angle, and the three
  collision value stores.
  Evidence: `python3 tools/rebuild/harness.py run --module object_collision`,
  `work/rebuild/ocl_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x1e00c`.
- Observation: the rebuilt module holds 160 instructions and 635 bytes.  The
  identity check stays open at 45 of 635 bytes and 31 of 160 instructions.
  The V2 register use and the if-else form change the code shape.  The store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1e00c --rebuilt work/rebuild/oclmod.exe --rebuilt-map
  work/rebuild/object_collision.map --rebuilt-symbol object_collision_`.

## The object_car_link module at 0x1c84c

- Observation: the original body at `0x1c84c` holds 238 instructions and 863
  bytes up to the first `ret`.  The plan row holds the same counts.  The record
  keeps the 864-byte extent to the next entry at `0x1cbac`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1c84c --count 238`,
  `docs/explanation/rebuild-plan.md`, and
  `tools/rebuild/functions/0x1c84c.json`.
- Observation: the 28-byte entry at `DAT_0010653c + index * 28` supplies the
  link pointer at field 0 and the render pointer at field 4.  The index is the
  object field `+0x4dc`.  The object field `+4` selects a second entry, and its
  field 0 is the target pointer.
  Evidence: the reads at `0x1c85a` through `0x1c8a5`.
- Observation: the function copies the position `+0x498`, `+0x49c`, and
  `+0x4a0` into link offsets 8, 0xc, and 0x10.  It scales the floats `+0x4a4`,
  `+0x4bc`, and `+0x4ac` by 1024.0 and divides by pi, then truncates and masks
  the low 11 bits into link offsets 0x14, 0x18, and 0x1c.
  Evidence: the block at `0x1c8a8` through `0x1c953`.
- Observation: the floats `+0x4b0` and `+0x4b8` form two masked values.  The
  render pointer indexes the arrays at `+0x2dc`, `+0x2d4`, `+0x2cc`, and
  `+0x4e0` with a 32-byte stride.  The first value goes to offset 8 of each
  array.  The second value goes to offset 8 of the first array and to offset
  0x10 of the other three arrays.
  Evidence: the block at `0x1c956` through `0x1ca40`.
- Observation: the render array at `+0x58` supplies an integer at offset 8.
  The function subtracts the angle `+0x418` divided by 15.0, truncates the
  result, masks the low 11 bits, and clears bit 0.  The value goes to offset 8
  of the arrays at `+0x58`, `+0x100`, `+0xac`, and `+0x154`.
  Evidence: the block at `0x1ca43` through `0x1cae8`.
- Observation: the target pointer receives the position at offsets 8 and 0x10,
  the float `+0x2c4` at offset 0xc, and the first masked value at offset 0x18.
  When the state `+0x2f8` is 0, it also receives the values at 0x14 and 0x1c.
  The link pointer receives `+0x4e8` at offset 0x3c.
  Evidence: the block at `0x1caeb` through `0x1cb54`.
- Observation: the link flags at offset 0x20 come from the table at
  `DAT_0010d268` indexed by the type `+0x2e0` with a 64-byte stride.  When the
  field `+0x2c0` is 0, the function sets bit 0x4000.  When the angle `+0x418`
  is at most 4000.0, it sets bit 0x2000.
  Evidence: the block at `0x1cb57` through `0x1cba2`.
- Observation: the four constants live in object 3.  The float at offset 0xaf4
  is 1024.0, the float at 0xaf8 is pi, the float at 0xafc is 15.0, and the
  float at 0xb00 is 4000.0.  The harness supplies the four values through
  globals with the exact bit patterns.  The original truncates with the Watcom
  helper at 0x4d9f4, so the module uses a local truncating FPU stub, because
  the shared stub rounds to nearest.
  Evidence: the reads at `0x1c8de`, `0x1c909`, `0x1c934`, `0x1c95f`,
  `0x1c9d6`, `0x1ca58`, `0x1cb89`, and `tools/rebuild/object_car_link_fpu.asm`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `6` and `D`.  The paths cover the state copy, the two flag bits, the
  4000.0 threshold, the negative truncation, the 11-bit mask, and the table
  selection.
  Evidence: `python3 tools/rebuild/harness.py run --module object_car_link`,
  `work/rebuild/ocr_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x1c84c`.
- Observation: the rebuilt module holds 152 instructions and 578 bytes.  The
  identity check stays open at 29 of 578 bytes and 51 of 152 instructions.  The
  V2 register use and the shared pointer form change the code shape.  The store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1c84c --rebuilt work/rebuild/ocrmod.exe --rebuilt-map
  work/rebuild/object_car_link.map --rebuilt-symbol object_car_link_`.

## The race_panel_2 module at 0x2e21c

- Observation: the original body at `0x2e21c` holds 265 instructions and 879
  bytes up to the `ret` at `0x2e58a`.  The plan row holds the same counts.  The
  store record keeps the 880-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2e21c --count 265`,
  `docs/explanation/rebuild-plan.md`, and
  `tools/rebuild/functions/0x2e21c.json`.
- Observation: the function has a frame.  It saves `ebx`, `ecx`, `edx`, `edi`,
  and `ebp` before the frame, sets `ebp`, and subtracts `0x94` from the stack.
  The rebuild uses the option `-of+`.
  Evidence: the prologue at `0x2e21c` through `0x2e223`.
- Observation: the function draws the title pointer `DAT_0011fc30` through
  `panel_draw_centered` at `0x2e7cc` with the y 2, the font 3, and the color 2.
  Evidence: the call at `0x2e240`.
- Observation: the index `DAT_001203c4` selects a 0x44-byte record from the
  table at object-3 offset `0x1d550`.  The text at `+0x14` is an inline
  character array.  The field at `+0x30` selects a text pointer from the table
  at `0x1fc14`.
  Evidence: the stride arithmetic at `0x2e245` through `0x2e267` and the
  indirect reads at `0x2e2d3` through `0x2e2f6`.
- Observation: both table texts are measured through `text_measure` at
  `0x254a8` with the font 0.  The function halves the width and the height with
  a signed division and centers the half width on the 0x140 screen width.  It
  draws the doubled position through `text_draw` at `0x25020` with the font 0
  and the color 2.  The first y is three half heights.  The second y is 0xc8
  minus five half heights.
  Evidence: the blocks at `0x2e26c` through `0x2e2ce` and at `0x2e2fb` through
  `0x2e367`.
- Observation: the function builds a string of `DAT_001203b4` letters `A`,
  measures it with the font 1, saves the half width, and centers the half width
  plus 0x40.  It draws the title pointer `DAT_0011fc1c` at y 0xd2 minus five
  half heights.  The field at `+0x34` goes through `sprintf` at `0x4d32c` with
  the format at object-3 offset 0x12e0, linear 0x1012e0, `%d KMH`.  The function
  draws that buffer at `(x + saved) * 2`.  The titles at `DAT_0011fc20` and
  `DAT_0011fc24` follow one half height apart.  Each title has a panel at
  `(x + saved, y + 1)` with the size `field + 4` and `field + 9` through
  `panel_show` at `0x2dfac`.
  Evidence: the blocks at `0x2e36c` through `0x2e4e2`.
- Observation: a non-zero index draws the panel at (8, 0x64) with the size 2
  when `DAT_0012039c` stays below `DAT_001203a8` and `DAT_0012037c` stays above
  0.6, and with the size 0 otherwise.  The function draws the panel at
  (0xee, 0x64) with the size 3 when `DAT_001207f0` plus 11 differs from the
  index and the two cell tests pass, and with the size 1 otherwise.
  Evidence: the blocks at `0x2e4e7` through `0x2e57a`.
- Observation: the threshold 0.6 is the double at object-3 offset 0x12e7,
  linear 0x1012e7, bits `0x3fe3333333333333`.  The rebuild reads it through the
  global `race_panel_2_threshold`, and the harness sets the exact bits.  The
  module uses the shared `fpu_stub.asm`, because it has no conversion to an
  integer.
  Evidence: the compares at `0x2e505` and `0x2e559` and
  `tools/rebuild/race_panel_2.c`.
- Observation: DOS/4G 2.01a leaves the initialized data pages zero in the
  harness run, so the harness builds the expected strings in the data segment
  at run time.  The module builds the `A` buffer and the `sprintf` stub builds
  the ` KMH` suffix at run time.
  Evidence: `tools/rebuild/race_panel_2_main.c` and the earlier constant-text
  finding.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `3` and `D`.  The paths cover the two table texts, the indirect text,
  the `A` buffer, the formatted value, the four text panels, the two state
  panels, the skipped state panels, and the negative width that truncates
  toward zero.
  Evidence: `python3 tools/rebuild/harness.py run --module race_panel_2`,
  `work/rebuild/rp2_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x2e21c`.
- Observation: the rebuilt module holds 220 instructions and 696 bytes.  The
  identity check stays open at 58 of 220 instructions and 30 of 696 bytes.  The
  V2 toolchain uses a different register allocation and local layout.  The store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2e21c --rebuilt work/rebuild/rp2mod.exe --rebuilt-map
  work/rebuild/race_panel_2.map --rebuilt-symbol race_panel_2_`.

## The ai_update module at 0x205c4

- Observation: the original body at `0x205c4` holds 260 instructions and 911
  bytes up to the `ret` at `0x20952`.  The plan row holds the same counts.  The
  store record keeps the 912-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x205c4 --count 260`,
  `docs/explanation/rebuild-plan.md`, and
  `tools/rebuild/functions/0x205c4.json`.
- Observation: the function saves `edx`, `esi`, and `ebp`, sets `ebp`, and
  subtracts `0x40` from the stack.  The rebuild uses the option `-of+`.
  Evidence: the prologue at `0x205c4` through `0x205c9`.
- Observation: the loop walks `DAT_0010d6f8` records of 0x40 bytes at
  `DAT_0010d700`.  The flag at `+0x38` makes a record run.  A running record
  sets the flag at `+0x20`, saves the position at `+0` and `+8`, and advances
  the position by the speed at `+0x30` times the direction at `+0x24` and
  `+0x2c` times the frame delta `DAT_00118e04`.
  Evidence: the loop at `0x205d3` through `0x20721` and the advances at
  `0x20750` through `0x20795`.
- Observation: the same product turns the angles at `+0xc`, `+0x10`, and
  `+0x14` by the doubles at object-3 offsets 0xc8d, 0xc95, and 0xc9d, which
  hold 0.008, 0.004, and 0.0015.  The speed falls by the frame delta times
  2000.0, and a negative speed becomes zero.
  Evidence: the blocks at `0x20798` through `0x20829`.
- Observation: the collision check at `0x11c00` reads the position through
  `DAT_00108038` and `DAT_00108040`.  A zero return marks a collision.  The
  function saves the height at `+4` in `DAT_0010c72c`, restores the saved
  position, flips the sign bit of the high byte of the direction floats at
  `+0x24` and `+0x2c`, and scales the speed by the double 0.3 at object-3
  offset 0xca9.
  Evidence: the calls and the blocks at `0x20830` through `0x208a6`.
- Observation: the vertical speed at `+0x34` falls by the frame delta times
  500.0.  The height takes the truncated sum of the integer height and the
  vertical speed.  When `DAT_0010c72c` is not below the height, the height
  clamps to it, the vertical speed reverses through the double 0.8 at offset
  0xcb5, and a magnitude below 50.0 at offset 0xcbd clears the vertical speed.
  Evidence: the blocks at `0x208a9` through `0x2091f`.
- Observation: when both speeds are zero, the record stops.  A collision sets
  the height to -0x4e20.  Without a collision the function rotates the pair
  `DAT_0010c734` and `DAT_0010c73c` through `0x2218c` with the angle at `+0x10`
  plus the float -pi / 2 at offset 0xcc5, turns the result into the angle at
  `+0xc` with the float pi at 0xcc9, 46080.0 at 0xccd, and pi / 2 at 0xcd1, and
  clears the run flag.  A non-zero speed keeps the run flag.
  Evidence: the blocks at `0x205e8` through `0x20646` and `0x20922` through
  `0x20948`.
- Observation: a record with the flag at `+0x20` publishes its position and
  height into the target record from the pointer table `DAT_0010653c` at the
  0x1c-byte entry named by `+0x18`.  It scales the three angles by the float
  1024.0 at offset 0xcd5 divided by pi, truncates toward zero, and masks with
  0x7ff into the words at `+0x14`, `+0x18`, and `+0x1c`.
  Evidence: the publish block at `0x20649` through `0x20711`.
- Observation: the original calls the truncating helper at `0x4d9f4` six
  times.  The rebuild uses the local `ai_update_fpu.asm` truncating `__CHP`
  stub, because the shared `fpu_stub.asm` returns without the truncate mode.
  Evidence: the calls at `0x206ae`, `0x206d6`, `0x206fe`, `0x2076c`, `0x20790`,
  and `0x208d1`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9` and `D`.  The paths cover the zero count, the stopped record with
  and without the publish flag, the running record with and without a
  collision, the two stop paths, the height clamp, the small vertical speed,
  the negative fractional truncation, and the negative angle mask.
  Evidence: `python3 tools/rebuild/harness.py run --module ai_update`,
  `work/rebuild/ai_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x205c4`.
- Observation: the rebuilt module holds 181 instructions and 652 bytes.  The
  identity check stays open at 21 of 181 instructions and 27 of 652 bytes.  The
  V2 toolchain uses a different register allocation and local layout.  The store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x205c4 --rebuilt work/rebuild/aimod.exe --rebuilt-map
  work/rebuild/ai_update.map --rebuilt-symbol ai_update_`.

## The sound_bank_load module at 0x1f2e0

- Observation: the original body at `0x1f2e0` holds 268 instructions and 1046
  bytes up to the `ret` at `0x1f6f5`.  The plan row holds the same counts.  The
  store record keeps the 1048-byte extent.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1f2e0 --count 268`,
  `docs/explanation/rebuild-plan.md`, and
  `tools/rebuild/functions/0x1f2e0.json`.
- Observation: the function returns at once when the setup word at
  `DAT_00008058` is zero.  Otherwise it builds `%s\data\sound.dat` from the
  base path at `DAT_00007f44` through `sprintf` at `0x4d32c`.
  Evidence: the gate at `0x1f2f4` through `0x1f2fb` and the build at
  `0x1f301` through `0x1f316`.
- Observation: the loop walks the `DAT_00018d84` records of 0x24 bytes at
  `DAT_0015224`.  For each record it builds the member `%s.RAW`, opens the path
  with the category `rb` through `0x4c08d`, and copies the 0x74-byte template at
  object-3 offset 0x1e8c into the record at `DAT_0016034`.  A failed open
  reports the file error through `0x4bf88` and runs the exit at `0x300be` with
  the value 0.
  Evidence: the loop at `0x1f420` through `0x1f4b5` and the error path at
  `0x1f46b` through `0x1f47c`.
- Observation: the function reads the size through `0x4c7e0`, gets the resource
  `SOUND_FX` through `0x2eed0`, reads the member through `0x4c4e5`, stores the
  size at `+0x08`, and closes the stream through `0x4c38d`.  It stores
  `index + 1` at `+0x18`.
  Evidence: the blocks at `0x1f4b6` through `0x1f58f`.
- Observation: the record fields `+0x0c`, `+0x10`, `+0x14`, `+0x18`, `+0x1c`,
  and `+0x20` go to the result fields `+0x14`, `+0x0c`, `+0x30`, `+0x34`,
  `+0x38`, and `+0x40`.  The high nibble of `+0x20` selects the type: 0 gives
  0, 1 gives 2, 2 gives 1, and 3 gives 3.  The other values keep the template
  value 2.
  Evidence: the stores at `0x1f5a8` through `0x1f6e2` and the jump table at
  object-1 offset `0xf2d0`.
- Observation: the flags at `+0x28` hold 0x40 for a non-zero field `+0x18`,
  0x4000 for a non-zero field `+0x10`, and 0x100, 0x400, and 0x200 for bit 0x10
  of the bytes `+0x0f`, `+0x1f`, and `+0x23`.  These bytes are the high bytes of
  the dwords `+0x0c`, `+0x1c`, and `+0x20`.
  Evidence: the flag block at `0x1f33b` through `0x1f41a`.
- Observation: the original stores a far pointer into the pair at `+0x00` and
  `+0x04` with the `ds` value.  The rebuild stores a near pointer and leaves
  the segment at zero, because the flat target needs no far pointer.  The zero
  check for the pair still runs the memory error report at `0x2edad`.
  Evidence: the store at `0x1f4e8`, the check at `0x1f500` through `0x1f50e`,
  and `tools/rebuild/sound_bank_load.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `4` and `D`.  The paths cover the two records, the four status
  values, the zero setup word, the two failed opens, and the zero resource.
  Evidence: `python3 tools/rebuild/harness.py run --module sound_bank_load`,
  `work/rebuild/sbl_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x1f2e0`.
- Observation: the rebuilt module holds 223 instructions and 726 bytes.  The
  identity check stays open at 35 of 223 instructions and 37 of 726 bytes.  The
  V2 toolchain uses a different register allocation and local layout.  The store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1f2e0 --rebuilt work/rebuild/sblmod.exe --rebuilt-map
  work/rebuild/sound_bank_load.map --rebuilt-symbol sound_bank_load_`.

## The car_setup module at 0x142ec

- Observation: the plan row holds the first 65 instructions and 226 bytes up to
  the early `ret` at `0x143cd`.  The full body continues to the `ret` at
  `0x1496c`, so the store record keeps the 1668-byte extent and sets no size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x142ec --count 65` and
  `tools/rebuild/functions/0x142ec.json`.
- Observation: the two callers at `0x12c0c` and `0x12f50` walk the car count
  `DAT_0000f8ec` and call the function for each 0x590-byte record at
  `0x10ab8 + index * 0x590`.  The first callee at `0x11060` searches the
  0x78-record table at `0x6530` by name through `stricmp` at `0x4d9ba` and
  loads the object through `0x10bfc`.  The first call loads the record named
  `SHADOW` from object-3 offset 0x5dc into `+4`.  The second call loads the
  record named at `0xd25c + index * 0x40` into `+0x4dc`.  The record table at
  object-3 offset 0x1d5c holds the wheel names `flx`, `fli`, `blx`, `rli`,
  `frx`, `fri`, `brx`, and `rri`.  The module therefore names the function
  `car_setup`.
  Evidence: the calls at `0x12c64` and `0x13006`, the strings at `0x1005dc` and
  `0x101d5c`, and the callee at `0x11060`.
- Observation: the function stores the caller index at `+0`.  When the load
  flag is zero, it returns 0.  Otherwise it clamps the counter
  `DAT_0000f8f0` against the limit `DAT_0000d6dc`, advances the counter past
  the reserved slot `DAT_0011d2a0`, and clamps it again.  It selects the model
  index 0xc when the player count `DAT_0011d2b4` is above 2 and the counter
  otherwise, stores the index at `+0x2e0`, and gives the second call the name
  offset `0xd25c + index * 0x40`.
  Evidence: the blocks at `0x142f5` through `0x143b5`.
- Observation: a non-zero result from either `0x11060` call returns at once.
  The first error returns the result through the far epilogue at `0x14966`.
  The second error returns through the local epilogue at `0x143cd`.
  Evidence: the tests at `0x14369` through `0x14372` and `0x143c0` through
  `0x143cd`.
- Observation: the rebuild covers the 65-instruction prefix.  The load flag
  zero path returns 0 and the success path returns 0 after the two loads.  The
  full state reset after `0x1476e` is not in the module.
  Evidence: `tools/rebuild/car_setup.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `6` and `D`.  The paths cover the normal slot, the two clamp paths,
  the forced model 0xc, the two load errors, and the zero load flag.
  Evidence: `python3 tools/rebuild/harness.py run --module car_setup`,
  `work/rebuild/car_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x142ec`.
- Observation: the rebuilt module holds 51 instructions and 171 bytes.  The
  identity check stays open at 10 of 51 instructions and 13 of 171 bytes.  The
  original spills the three arguments to locals and saves the registers in a
  different order.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x142ec --rebuilt work/rebuild/carmod.exe --rebuilt-map
  work/rebuild/car_setup.map --rebuilt-symbol car_setup_`.

## The file_error_report module at 0x4bf88

- Observation: the plan row records 261 bytes, 88 instructions, and 7 calls.
  The function reads the status at `[arg+0xac]`, selects one of five messages
  through the jump table at `0x4c04c`, and prints the default interface message
  for the other values.  The name at `+0x78` is the argument for the messages
  with `%s`.  The function then calls `exit(-2)` at `0x300be`.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x4bf88 --count 88`.
- Observation: the six format strings sit at object-3 offsets 0x1608, 0x1622,
  0x1644, 0x1653, 0x1667, and 0x1685.  They hold `Cannot open main file: %s`,
  `File %s not present in main file.`, `Out of memory.`, `Out of file domain.`,
  `Out of memory to decode file.`, and `Unknown error in Engine interface
  <%s>.\n`.  The immediates in the code hold these object offsets, and the
  loader relocates them.
  Evidence: the object-3 base 0x100000 in `python3 tools/le.py info STARTH.EXE`
  and the reads at linear 0x101608 through 0x101685.
- Observation: the V2 build of a dense `switch` emits a data-segment jump table
  that the link does not relocate.  The run stops with a general-protection
  fault at the first call.  The module uses an if-chain, so the dispatch is a
  comparison chain and the behavior check passes.  The identity check stays
  open at 10 of 43 instructions and 7 of 125 bytes; the store records
  `rebuilt`.
  Evidence: `python3 tools/rebuild/harness.py run --module file_error_report`
  and `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x4bf88 --rebuilt work/rebuild/fermod.exe --rebuilt-map
  work/rebuild/file_error_report.map --rebuilt-symbol file_error_report_`.
- Observation: the behavior check passes.  The harness prints the markers `1`
  through `6`, `0`, and `D`.  The paths cover the five status values, the two
  default values, and the exit status -2.
  Evidence: `python3 tools/rebuild/harness.py run --module file_error_report`,
  `work/rebuild/fer_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x4bf88`.

## The player_car_init module at 0x136cc

- Observation: the plan row holds the first 87 instructions and 277 bytes up to
  the early `ret` at `0x137e0`.  The full body continues to the `ret` at
  `0x142ea`, so the store record keeps the 3104-byte extent and sets no size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x136cc --count 87` and
  `tools/rebuild/functions/0x136cc.json`.
- Observation: the six callers at `0x12c83`, `0x12d2c`, `0x12e2e`, `0x12f2a`,
  `0x130fa`, and `0x13181` pass the player object at object-3 offset `0xf91c`
  and a load flag.  The function copies the selected 0x40-byte `CARS_DATA`
  record from `0xd25c + DAT_0011d2a0 * 0x40` to the player object at `+0x470`.
  The format argument at `+0x474` points four bytes into the file name, so
  `AUTO00` gives `00`.  The animation field at `+0x0c` is a flag: zero
  selects the letter `A` and a non-zero value selects the letter `B`.
  Evidence: the section "The car record and its consumer" and the disassembly
  of `0x136e6` through `0x13728`.
- Observation: the function builds `COCK%s%c` from the file name tail and the
  letter, loads the cockpit resource through `0x1b23c`, and builds the same
  name with the letter `H` through `0x15460("GEN", "CARS", ...)`.  It then
  loads the `SHADOW` object at `+0x468` and the animation object named at
  `+0x470` at `+0x460` through `0x11060`.  The module therefore names the
  function `player_car_init`, because it initializes the player car from the
  selected car record.
  Evidence: the calls at `0x1373b` through `0x137c8`, the strings
  `COCK%s%c` at object-3 offset 0x564, `CARS` at 0x576, `GEN` at 0x57b, and
  `SHADOW` at 0x57f.
- Observation: the rebuild covers the 87-instruction prefix.  The zero load
  flag returns 0 and the success path returns 0 after the two object loads.
  The state reset after `0x13dcf` is not in the module.
  Evidence: `tools/rebuild/player_car_init.c`.
- Observation: the behavior check passes.  The harness prints the markers `1`
  through `4`, `0`, and `D`.  The paths cover the normal record, the two
  animation letters, the two object load errors, and the zero load flag.
  Evidence: `python3 tools/rebuild/harness.py run --module player_car_init`,
  `work/rebuild/pci_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x136cc`.
- Observation: the rebuilt module holds 70 instructions and 223 bytes.  The
  identity check stays open at 12 of 70 instructions and 9 of 223 bytes.  The
  V2 toolchain saves the registers in a different order and uses a different
  local layout.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x136cc --rebuilt work/rebuild/pcimod.exe --rebuilt-map
  work/rebuild/player_car_init.map --rebuilt-symbol player_car_init_`.

## The net_race_lobby module at 0x2d7c4

- Observation: the original body at `0x2d7c4` holds 198 instructions and 690
  bytes up to the first `ret` at `0x2da75`.  The case blocks continue to the
  `ret` at `0x2ddda`, so the store record keeps the 1560-byte extent and sets
  no function size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2d7c4 --count 198` and
  `tools/rebuild/functions/0x2d7c4.json`.
- Observation: the function has a frame.  It saves `ebx`, `ecx`, `edx`, `esi`,
  `edi`, and `ebp` before the frame.  It subtracts `0xdc` from `esp` and
  `0x82` from `ebp`.  The rebuild uses the option `-of+`.
  Evidence: the prologue at `0x2d7c4` through `0x2d7d2`.
- Observation: the race state dispatcher `race_state_update` at `0x29564`
  reaches this function through the entry at `0x29605`, which calls `0x2d7c4`.
  The dispatcher jump table at `cs:[eax*4 + 0x19540]` and the case blocks are
  the evidence.
  Evidence: the call at `0x29605` and the disassembly of `0x29564`.
- Observation: the function reads the 0x38-byte player records at object-3
  offset `0x2099c`.  The record layout is `+0x04` event, `+0x08` player index,
  `+0x0c` name, `+0x10` icon cell, `+0x14` car index, `+0x24` player bit, and
  `+0x28` the 0x10-byte node id.
  Evidence: the loads at `0x2d89c`, `0x2d8ba`, `0x2dad3`, `0x2db5f`, and
  `0x2dd76`, and the record layout in `tools/rebuild/net_player_add.c`.
- Observation: the receive call at `0xc739c` reads the 0xc-byte header into
  `[ebp + 0x26]` and returns the body pointer.  The copy at `0xc8208` copies
  the 0x2c-byte body into `[ebp + 0x32]`.  The header and the body form one
  0x38-byte record, so the node id at `[ebp + 0x4e]` is the record field at
  `+0x28`.
  Evidence: the receive block at `0x2d823` through `0x2d84c` and the callees
  at `0xc739c` and `0xc8208`.
- Observation: the event code at record `+0x04` selects the paths 0 (join),
  1 (leave), 2 (start), 3 (exit), and 0x1234 (quit).  The join path adds the
  node through `0xc712c`, sorts through `0xc6fc8`, finds the index through
  `0xc7248`, and copies the record.  The leave path removes the index through
  `0xc71dc`.
  Evidence: the dispatch at `0x2d84f` through `0x2d87a` and the blocks at
  `0x2d87c` and `0x2da7e`.
- Observation: the start path picks the chosen player from the frame counter
  `DAT_000007fd` on the host.  It stores the choice in `DAT_00120b5c` and calls
  `net_player_add` at `0x2dddc` with 0x8844 while it holds the cell
  `DAT_001203c8`.  A client waits for the 0x8844 event and reads the choice
  from record `+0x14`.
  Evidence: the block at `0x2d904` through `0x2d9b8`.
- Observation: when the local player is not the chosen player, the function
  runs the loading transition.  It calls `video_present` at `0x27ec8` with the
  scale 0 and the offset 0, calls `video_fill` at `0x2aa98`, loads
  `menu3h.pcx` from `gfx\graf.dat` into `DAT_0011fad0` with the palette at
  0xcf5c through `pcx_load` at `0x1b5a4`, copies 0x3e800 bytes to
  `DAT_0021528` through `memcpy` at `0x8c150`, sets the palette through
  `0x81a05`, and sets the active flag `DAT_00120994`.
  Evidence: the block at `0x2d9c5` through `0x2da39`, the strings at 0x12a6,
  0x12b1, and the data at 0xcf5c.
- Observation: the active mode calls `net_player_add` at `0x2dddc` with 0x8844
  and reads the events that target the chosen player.  The event 3 copies the
  record.  The event 4 calls `0x2a214` and then draws the chosen player and the
  car name from the table at `0x1d908` with the stride 0x34.
  Evidence: the block at `0x2dc45` through `0x2ddce`.
- Observation: the no-message path builds the player bit at `DAT_00120b60`
  from the field `+0x24`, draws the local player at (0xc, 0x34), and draws the
  remote players in two columns.  The local name uses the format at 0x12be and
  the remote names use the format at 0x12c1.
  Evidence: the block at `0x2daa8` through `0x2dc40` and the strings at 0x12be
  and 0x12c1.
- Inference: the function is the network race lobby.  The player record array,
  the join, leave, and start events, the chosen player, the `net_player_add`
  call, and the `menu3h.pcx` loading screen support the name.  The module
  records the name `net_race_lobby` with the confidence `derived`.
  Evidence: the call graph and the data flow above.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9` and `D`.  The paths cover the empty message, the join, the leave,
  the local start, the remote start with the loading transition, the client
  wait, the active record copy, the active menu transition, the exit event, and
  the quit event.
  Evidence: `python3 tools/rebuild/harness.py run --module net_race_lobby`,
  `work/rebuild/nrl_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x2d7c4`.
- Observation: the rebuilt module holds 154 instructions and 600 bytes.  The
  identity check stays open at 32 of 154 instructions and 61 of 600 bytes.  The
  V2 toolchain uses another register order and another local layout.  The store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2d7c4 --rebuilt work/rebuild/nrlmod.exe --rebuilt-map
  work/rebuild/net_race_lobby.map --rebuilt-symbol net_race_lobby_`.

## The player_view_setup module at 0x19ae8

- Observation: the original body at `0x19ae8` holds 223 instructions and 869
  bytes up to the `ret` at `0x19e4c`.  The plan row holds the same counts.  The
  store record keeps the 912-byte extent and sets no function size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x19ae8 --count 223` and
  `tools/rebuild/functions/0x19ae8.json`.
- Observation: the player update at `0x19ab0` calls this function as its first
  step on the player structure `DAT_00110a20`.  The car camera at `0x1a930`
  calls it at `0x1ae8b` on the view block at `+0x4f8` of the 0x590-byte car
  records at `DAT_00110ab8`.
  Evidence: the calls at `0x19ab8` and `0x1ae8b`, the `add eax,0x4f8` at
  `0x1ae86`, and `python3 tools/le.py disasm STARTH.EXE 0x19ab0 --count 14`.
- Observation: the seven 0x14-byte slots at `DAT_00101da4` hold the view kind,
  the render mode, the view distance, the video mode, and the chosen flag.  The
  static values are the kinds 0, 1, 2, 9, 9, 9, and 3, and the mode 9 marks a
  special slot.  The byte `DAT_000f003d[kind]` gates the kind, and the input
  bits `DAT_00118e1c` and `DAT_00118e1d` gate the change.
  Evidence: the table at object-3 offset 0x1da4, the reads at `0x19b32` through
  `0x19b8a`, and the decompiled `FUN_00019ae8`.
- Observation: the function copies the slot distance to `+0x90`, the slot video
  mode to `+0x8c`, and the render mode to `+0x88`.  It sets the screen mode
  through `0x4c850` with the width 0x140, the height 0x190, and the value
  0x104, or with the view distance and the truncated 0.65 product.  It copies
  the car position `+0x00` through `+0x08` to `+0x18` through `+0x20` and the
  angle `+0x30` through `+0x38` to `+0x48` through `+0x50`.  It starts the
  renderer at `0x19e78` or the car camera at `0x1a930`.
  Evidence: the block at `0x19bcf` through `0x19d13`, the constant `0.65` at
  object-3 offset 0x8dc, and the calls at `0x19c23`, `0x19c76`, `0x19c80`,
  `0x19cdd`, `0x19cf6`, and `0x19e3f`.
- Observation: the chosen slot 4 runs the camera scan over the counter
  `DAT_00110a1c`, the wrap value `DAT_0010f8ec`, and the car field at
  `DAT_0011100c` with the 0x590-byte stride.  The slot chosen flag is cleared,
  and the first free slot is recorded in `DAT_0011d2a8`.
  Evidence: the block at `0x19d45` through `0x19e2b`.
- Inference: the function selects and applies a view.  The seven-slot table,
  the chosen flag, the view distance, the mode copy, the car position copy, and
  the render and camera calls support the name.  The module records the name
  `player_view_setup` with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9` and `D`.  The paths cover the chosen slot, the kind gate, the
  special slot 1 fallback, the car-only path, the slot 4 scan, the scan wrap,
  the distance branch, the early return, the video mode 1 to 2 change, and the
  blocked input bit.
  Evidence: `python3 tools/rebuild/harness.py run --module player_view_setup`,
  `work/rebuild/pvs_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x19ae8`.
- Observation: the rebuilt module holds 149 instructions and 571 bytes.  The
  identity check stays open at 43 of 149 instructions and 34 of 571 bytes.  The
  V2 toolchain uses the register calling convention, keeps the view pointer in
  `ecx`, and folds the slot address.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x19ae8 --rebuilt work/rebuild/pvsmod.exe --rebuilt-map
  work/rebuild/player_view_setup.map --rebuilt-symbol player_view_setup_`.
- Observation: the original converts the view distance with the Watcom helper
  at `0x4d9f4`, which truncates toward zero.  The module uses the local
  truncating FPU stub `player_view_setup_fpu.asm`, because the shared stub
  rounds to nearest.  The harness shows the truncated value 171 for the
  distance 264.
  Evidence: the `fild` and `fmul` at `0x19cd1` and the helper at `0x4d9f4`;
  `tools/rebuild/modules/player_view_setup.json`.

## The race_data_load module at 0x131a8

- Observation: the original body at `0x131a8` holds 236 instructions and 919
  bytes up to the `ret` at `0x1353e`.  The plan row holds the same counts.  The
  store record keeps the 920-byte extent and sets no function size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x131a8 --count 236` and
  `tools/rebuild/functions/0x131a8.json`.
- Observation: the caller `race_session_start` at `0x128e0` runs this function
  at `0x128fb` as its first step after the flag store `_DAT_00118e1c = 0x102`.
  The caller returns a non-zero result at once.  The record of the caller
  already names the target `race_data_load`.
  Evidence: the calls at `0x128fb` and the record
  `tools/rebuild/functions/0x128e0.json`.
- Observation: the letter `H` comes from `DAT_0011d2b8 >= 1`, and the letter
  `L` comes from a smaller value.  The function builds
  `%s\gfx\%s%c.dat` from the config base `DAT_00007f44`, the track name
  `DAT_0011d294`, and the letter.  The flag `DAT_0011d274 & 8` adds the suffix
  `i` to the track name.  The level `DAT_0011d2b4` gives the member letter
  `A + level`.
  Evidence: the block at `0x131ce` through `0x132aa` and the decompiled
  `FUN_000131a8`.
- Observation: the function loads five lists.  The lists and their sizes are
  `OSTACLES` 0x24 of 0x24, `TRACK_DATA` 0x10 of 0x10, `OPPONENTS` 0x590 of
  0x14, `ANIMATIONS` 0xbc of 0x18, and `CARS_DATA` 0x40 of 0x40.  The tables
  are `DAT_0010f000`, `DAT_0010cf44`, `DAT_00110d9c`, `DAT_0010f180`, and
  `DAT_0010d25c`.  The count results go to `DAT_0010f8ec`, `DAT_0010f17c`, and
  `DAT_0010d6dc`.
  Evidence: the calls at `0x132aa`, `0x132c6`, `0x132e5`, `0x1330c`, and
  `0x1332e`.
- Observation: the function loads the graphics path, the track material, and
  the `.smg` stream.  It sets the screen mode through `0x4c850` with the width
  0x140, the height 0x190, and the value 0x104.  It copies the first
  `TRACK_DATA` value, the track color, to all four bytes of the word at
  `PTR_DAT_0008a844 + 0x10`.  It then builds the track names and loads the track
  resources.
  Evidence: the block at `0x13338` through `0x133ba`, the color block at
  `0x13381` through `0x133a6`, and the calls at `0x133af` and `0x133ba`.
- Observation: the game mode `DAT_0011d260 == 0x100` loads `attr%c.fnt` and
  `logo%c.spr` from `%s\gfx\graf.dat`.  Every other mode loads `game%c.fnt`
  and `game%c.spr` from `%s\gfx\game.dat`, and it also loads the three
  resources `impres_`, `implet_`, and `cdfront`.  The flag `DAT_0011d2cc`
  selects the music track `DAT_00001f00` or
  `DAT_00001f08[DAT_000203bc]`.
  Evidence: the two branches at `0x133c9` through `0x1352c` and the strings at
  object-3 offsets 0x4e6 through 0x55c.
- Inference: the function builds the race file names, loads the race lists and
  assets, and starts the race music.  The caller name, the list names, and the
  data flow support the name `race_data_load`.  The module records the name with
  the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `4` and `D`.  The paths cover the level zero return, the high and low
  letters, the replay flag, the `i` suffix, the non-0x100 mode, the mode 0x100,
  the music flag on and off, the five list loads, the color copy, and the two
  load branches.
  Evidence: `python3 tools/rebuild/harness.py run --module race_data_load`,
  `work/rebuild/rdl_out.txt`, and `python3 tools/rebuild/run.py run --function
  0x131a8`.
- Observation: the rebuilt module holds 233 instructions and 961 bytes.  The
  identity check stays open at 57 of 233 instructions and 90 of 919 bytes.  The
  V2 toolchain uses the register calling convention and a different local
  layout.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x131a8 --rebuilt work/rebuild/rdlmod.exe --rebuilt-map
  work/rebuild/race_data_load.map --rebuilt-symbol race_data_load_`.

## The track_progress_update module at 0x1db94

- Observation: the original body at `0x1db94` holds 259 instructions and 926
  bytes up to the `ret` at `0x1df31`.  The plan row holds the same counts.  The
  store record keeps the 928-byte extent and sets no function size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1db94 --count 259` and
  `tools/rebuild/functions/0x1db94.json`.
- Observation: the only caller is `object_prepare` at `0x1c79c`, which calls
  this function first at `0x1c7a8` on each object.  The record therefore takes
  the section `object_prepare callees`.
  Evidence: the sweep of every candidate function start for a call to
  `0x1db94`; the block at `0x1c79c` through `0x1c7b7`.
- Observation: the function reads the 0x20-byte node table at the global
  `0x7250` and the count at `0x7ed0`.  The loader at `0x150f4` calls
  `0x1b198` with the destination `0x7250`, and `0x1b198` builds
  `%s\data\%s.dat`, reads a 4-byte count, and reads `count * 0x20` bytes into
  the destination.  `race_start` builds the name `MAP%d` into `0x1d294`, so the
  table holds the track nodes of the current level.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x150f4 --count 29`,
  `python3 tools/le.py disasm STARTH.EXE 0x1b198 --count 60`, and the strings
  at object-3 offsets 0x972 and 0x981.
- Observation: the function examines the node before the current index, the
  current node, and the next node.  It accepts a node when each absolute axis
  delta stays below the node radius and the truncated distance does not exceed
  the radius.  It advances the current index at `+0x320` and the target index
  at `+0x3f0`, copies the node value and data to `+0x3f4` and `+0x3fc`, and
  marks the node in the chain at `+0x324` and `+0x328`.  A node after the first
  is marked only when the previous node is marked.
  Evidence: the blocks at `0x1dc23` through `0x1dda6`.
- Observation: a visit to node 0 with every node marked clears the chain, sets
  the count to one, and increments the lap counter at `+0x40c`.  The function
  then stores the ratio of the object-to-node distance to the node-to-next-node
  segment at `+0x404`, and the progress `100 * laps + index + ratio` at
  `+0x408`.  Both distances scale by the double 0.05 at object-3 offset
  `0xb44` and truncate to an integer.  A call with no hit sets the target index
  to -1.
  Evidence: the blocks at `0x1ddb1` through `0x1df23` and the double at object-3
  offset 0xb44.
- Inference: the function advances the track node index, tracks the visited
  chain and the lap count, and writes the track progress.  The caller, the
  track table, and the progress formula support the name
  `track_progress_update`.  The module records the name with the confidence
  `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the C literal `0.05` and the C `sqrt` body did not work in the
  harness.  The linker emitted `fmul qword ptr [0]` for the literal and
  `fmul qword ptr [8]` for the 0.5 constant of the Newton step, so the first
  run produced NaN at `+0x404` and `+0x408`.  The module now reads the global
  `track_scale`, the harness sets its exact bits, and the local FPU stub
  provides `sqrt_` through `fsqrt`.
  Evidence: the first build at `work/rebuild/tpumod.exe`, the disassembly of
  the rebuilt function, and `tools/rebuild/track_progress_update_fpu.asm`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9` and `D`.  The paths cover the first node, the forward advance, the
  lap reset, the segment fraction, the target copy, the no-hit target -1, the
  unmarked previous node, and the backward wrap to the last node.
  Evidence: `python3 tools/rebuild/harness.py run --module
  track_progress_update`, `work/rebuild/tpu_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x1db94`.
- Observation: the rebuilt module holds 168 instructions and 619 bytes.  The
  identity check stays open at 34 of 168 instructions and 27 of 619 bytes.  The
  V2 toolchain uses a different instruction order and local layout.  The store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1db94 --rebuilt work/rebuild/tpumod.exe --rebuilt-map
  work/rebuild/track_progress_update.map --rebuilt-symbol
  track_progress_update_`.

## The player_steering_update module at 0x1629c

- Observation: the original body at `0x1629c` holds 267 instructions and 997
  bytes up to the `ret` at `0x16680`.  The plan row holds the same counts.  The
  store record keeps the 1036-byte extent and sets no function size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1629c --count 267` and
  `tools/rebuild/functions/0x1629c.json`.
- Observation: the only caller is the player update group at `0x15a80`, which
  calls this function at `0x15aac` after the brake step at `0x16150`.  The
  group runs on the player object at object-3 offset `0xf91c`, the same object
  that `player_car_init` fills.  The record therefore takes the section
  `The vehicle state chain`.
  Evidence: the sweep of every candidate function start for a call to
  `0x1629c`; the block at `0x15a80` through `0x15ab4`; and the
  `player_car_init` finding above.
- Observation: the function selects the filter index `0x50`, `0x55`, or `0x58`
  from the state at `+0x148` and the flags at `+0x110`, `+0x4f4`, and
  `+0x170`.  It forms the speed ratio `+0x50c / +0x480` and the limit
  `(1 - sqrt(ratio)) * 0x100720 + 0x100728`.  The doubles at object-3 offsets
  0x720 and 0x728 hold 0.296706 and 0.0174533, that is 17 degrees and 1 degree
  in radians.  The float at object-3 offset 0x730 holds 1/1024, so the six
  control values at `0x119880` scale to a steering angle.  The mode at
  `0x119818` selects the direct axis, the two-axis sum, or the filtered path.
  Evidence: the blocks at `0x162a2` through `0x16465`, the filter call at
  `0x22b80`, and the constants at object-3 offsets 0x720, 0x728, and 0x730.
- Observation: the keyboard ramp at `+0x2f0`, `+0x4b0`, and `+0x4b8` adds
  `(value & 0x3f) * 0x10073c * (1 - sqrt(ratio))` with the sign of the low
  parity bit, and the clamp sets the limit at `+0x10c`.  The float at object-3
  offset 0x73c holds 0.0008.  The flags at `+0x6c`, `+0xc4`, and the state at
  `+0x148` reset the angle to zero.  The double at object-3 offset 0x734 holds
  1e-5 and guards the two filter paths.
  Evidence: the blocks at `0x16465` through `0x16593` and the constants at
  object-3 offsets 0x734 and 0x73c.
- Observation: a non-zero angle gives the turn radius
  `wheelbase / tan(angle)` at `+0x2f8`, the yaw step
  `+0x50c * 0x118e04 / radius` at `+0x2f4`, and the heading sum at `+0x4cc`.
  The function rotates the point `(radius, +0x2b0)` by the old and the new
  heading through `0x2218c` and stores the two differences at `+0x1c` and
  `+0x24`.  The rotation routine uses `cos` and `sin`, and the division uses
  `tan`, so the data flow is a bicycle-model steering step.
  Evidence: the blocks at `0x16593` through `0x16680` and the disassembly of
  `0x2218c`, `0x4da66`, `0x4da72`, and `0x4da83`.
- Inference: the function computes the player steering angle from the joystick
  and keyboard controls, limits the angle by the speed ratio, and writes the
  turn radius, the yaw step, the heading, and the position delta.  The caller,
  the control values, and the steering geometry support the name
  `player_steering_update`.  The module records the name with the confidence
  `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the harness links no math library.  The local FPU stub provides
  `sqrt`, `tan`, `sin`, and `cos` through `fsqrt`, `fptan`, `fsin`, and `fcos`,
  and it makes `__CHP` truncate toward zero.  The harness `filter_update` stub
  applies the one-pole filter with the table value of the index.
  Evidence: `tools/rebuild/player_steering_update_fpu.asm` and
  `tools/rebuild/player_steering_update_main.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `8` and `D`.  The paths cover the direct mode, the two-axis mode, the
  direct assignment, the keyboard clamp, the three resets, the filtered
  assignment, and the epsilon guard.
  Evidence: `python3 tools/rebuild/harness.py run --module
  player_steering_update`, `work/rebuild/psu_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x1629c`.
- Observation: the rebuilt module holds 213 instructions and 713 bytes.  The
  identity check stays open at 19 of 213 instructions and 37 of 713 bytes.  The
  V2 toolchain uses a different instruction order and local layout.  The store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1629c --rebuilt work/rebuild/psumod.exe --rebuilt-map
  work/rebuild/player_steering_update.map --rebuilt-symbol
  player_steering_update_`.

## The race_record_save module at 0x2e82c

- Observation: the original body at `0x2e82c` holds 347 instructions and 1185
  bytes.  The plan row holds the same counts.  The store record keeps the
  1228-byte extent and sets no function size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2e82c --count 347` and
  `tools/rebuild/functions/0x2e82c.json`.
- Observation: the three callers at `0x1fecf`, `0x29022`, and `0x2a2a8` pass
  `DAT_001203bc` in `eax`.  The callers sit in the loading-screen path at
  `0x1fd70`, in `net_race_screen` at `0x28ba4`, and in the race-menu handler
  at `0x2a214`.  The two latter functions belong to the `race_frame` group, so
  the record takes the section `race_frame callees`.
  Evidence: the direct call sweep for the target `0x2e82c`; the call sites at
  `0x1fecf`, `0x29022`, and `0x2a2a8`; and the records of `0x28ba4` and
  `0x2a214`.
- Observation: `choice_load` at `0x1bbc8` fills six 0x50-byte records at
  object-3 offset `0x20430` with the default lap time 180.0 and the name `GRA`.
  It fills six records at offset `0x20610` with the default race time 900.0.
  Each record holds five floats at `+0x00`, five 4-byte names at `+0x14`, five
  dwords at `+0x28`, and five dwords at `+0x3c`.
  Evidence: the blocks at `0x1bcfc` through `0x1be60` and the constants
  `0x43340000` and `0x44610000`.
- Observation: the function reads the lap time `DAT_0011d26c` and the race time
  `DAT_0011d268`.  It counts the entries that are smaller than the new time and
  inserts the new time at that slot, so both tables keep the smallest time
  first.  The first insert writes the lap table.  The second insert writes the
  race table and runs only when `DAT_0011d2b0` is 2.
  Evidence: the comparison loops at `0x2e85c` and `0x2ea0c`, the shift loops at
  `0x2e8a7` and `0x2ea57`, and the gate at `0x2e9f5`.
- Observation: the records screen at `0x2d4b8` reads the same two tables.  It
  formats the race entries as `\001%d. %3s %02d.%02d.%02d R` when the `+0x28`
  flag is non-zero and as `\001%d. %3s %02d.%02d.%02d` otherwise.  It formats
  the lap entries with the second pattern.  The `+0x28` flag therefore marks a
  saved replay.
  Evidence: the loops at `0x2d555` and `0x2d6f0`, the formats at object-3
  offsets `0x1254`, `0x126e`, and `0x1288`, and the flag test at `0x2d64c`.
- Observation: after a successful race insert, the function builds
  `%s\replays\temp.rpl` from the base path `DAT_00007edc`.  It checks that the
  file exists through `0xd1d44`, which runs the DOS find-first service.  It
  scans the five `+0x28` flags for the first free positive number, builds
  `%s\replays\REPLAY%d%d.RPL` from the track index and that number, deletes the
  target through `0xd1ed8` (DOS function 0x41), and renames the temporary file
  through `0xd1edd` (DOS function 0x56).
  Evidence: the blocks at `0x2ec00` through `0x2eccb`, the formats at object-3
  offsets `0x12fe` and `0x1312`, and the disassembly of `0xd1d44`, `0xd1ed8`,
  and `0xd1edd`.
- Observation: the input word `DAT_00118e1c` gates the save.  The bits `0x1020`
  return before any table write.
  Evidence: the test at `0x2e843` and the branch at `0x2e84c`.
- Inference: the function saves the current lap time and race time into the
  best-time tables and stores the replay file in the next free slot.  The two
  tables, the record screen, and the DOS file calls support the name
  `race_record_save`.  The module records the name with the confidence
  `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `8` and `D`.  The paths cover the input gate, the lap-only insert,
  the full lap and race insert, the two full-table cases, the missing temporary
  file, the equal-time insert, the track index 4, and the fastest lap insert.
  Evidence: `python3 tools/rebuild/harness.py run --module race_record_save`,
  `work/rebuild/rrs_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x2e82c`.
- Observation: the rebuilt module holds 159 instructions and 494 bytes.  The
  identity check stays open at 33 of 159 instructions and 16 of 494 bytes.  The
  V2 toolchain uses a different instruction order and local layout.  The store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2e82c --rebuilt work/rebuild/rrsmod.exe --rebuilt-map
  work/rebuild/race_record_save.map --rebuilt-symbol race_record_save_`.

## The player_car_view_update module at 0x16f34

- Observation: the original body at `0x16f34` holds 318 instructions and 1236
  bytes up to the `ret` at `0x17407`.  The plan row holds the same counts.  The
  store record keeps the 1236-byte extent and sets no function size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x16f34 --count 318` and
  `tools/rebuild/functions/0x16f34.json`.
- Observation: the five callers at `0x2003c`, `0x201d0`, `0x202c0`, `0x20360`,
  and `0x203c4` pass `0xf91c` in `eax`.  Each caller calls this function first
  and then the player physics group at `0x15a80`.  The five callers are the view
  groups that the frame update at `0x1ff70` runs in order.  The function
  therefore runs on the player object at object-3 offset `0xf91c` before the
  vehicle state chain, so the record takes the section
  `The vehicle state chain`.
  Evidence: the direct call sweep for the target `0x16f34`; the call sites at
  `0x2006a`, `0x201f5`, `0x202e8`, `0x20384`, and `0x203e8`; and the dispatch
  at `0x1ff70`.
- Observation: the function normalizes the three body angles at `+0x4c8`,
  `+0x4cc`, and `+0x4d0` to the range 0 to 2 pi.  The constants sit at
  object-3 offsets 0x790 and 0x794 and hold 2 pi and -2 pi.
  Evidence: the blocks at `0x16f3f` through `0x16fd7` and the data at object-3
  offsets 0x790 and 0x794.
- Observation: when the input bit 0x10 of `DAT_0011d260` is set, the function
  filters the view scale `+0x300` through `filter_update` at index `0x58`.  The
  target is `(+0x688 / DAT_0011d254) * DAT_0010d6e0 + DAT_0010d6e4`.
  Evidence: the block at `0x16fef` through `0x1702d` and the call to `0x22b80`.
- Observation: the function copies the speed `+0x50c` to `+0x588` through the
  helper at `0x15bfc`.  It projects the forward motion into the screen offsets
  `+0x4b0` and `+0x4b8` with the cosines and the sines at `0x4da66` and
  `0x4da72`, the speed, the frame delta `DAT_00118e04`, and the view scale
  `+0x300`.  The state at `+0x148` selects the full pitch projection or the flat
  yaw projection.  The flag at `+0x108` selects the pitch angle or the roll angle
  at `+0x4d0`.
  Evidence: the blocks at `0x17032` through `0x17208` and the helper at
  `0x15bfc`.
- Observation: the function adds the lateral rate `+0x54c` with the offsets
  `+0x550` and `+0x558`, applies the table factors at `DAT_0010cf08 + 0x140`
  and `+0x16c`, adds the heading rate `+0x548` to `+0x4cc`, clears `+0x54c`
  below the double 250.0 at object-3 offset 0x798, and adds the shake terms from
  `+0x4fc`, `+0x500`, and `+0x508`.  The shake scale is the double 2.0 at
  object-3 offset 0x7a0.
  Evidence: the blocks at `0x1720e` through `0x1739b` and the data at object-3
  offsets 0x798 and 0x7a0.
- Observation: the function writes the render flags at `+0x7b4`.  It copies
  `+0x47c`, sets bit 0x4000 when `+0x114` is zero, and sets bit 0x2000 when the
  speed stays at most the float 4000.0 at object-3 offset 0x7a8.  It then clears
  `+0x18` and `+0x10`.
  Evidence: the block at `0x173a1` through `0x17407`.
- Inference: the function prepares the player car for the five view groups.  It
  normalizes the body angles, filters the view scale, projects the motion into
  the screen offsets, applies the view rates and the shake, and sets the render
  flags.  The callers, the player object, and the screen-offset data support the
  name `player_car_view_update`.  The module records the name with the
  confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the harness links no math library.  The local FPU stub provides
  `sin`, `cos`, and `sqrt` through `fsin`, `fcos`, and `fsqrt`, and it makes
  `__CHP` truncate toward zero.  The original uses the routines at `0x4da72`,
  `0x4da66`, `0x22bec`, and `0x4d9f4`.
  Evidence: `tools/rebuild/player_car_view_update_fpu.asm` and
  `tools/rebuild/player_car_view_update_main.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `7` and `D`.  The paths cover the zero path, the filter path, the roll
  path, the flat yaw path, the angle normalization, the rate and shake path, the
  lateral offset path, and the render-flag path.
  Evidence: `python3 tools/rebuild/harness.py run --module
  player_car_view_update`, `work/rebuild/pcvw_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x16f34`.
- Observation: the rebuilt module holds 247 instructions and 898 bytes.  The
  identity check stays open at 23 of 247 instructions and 26 of 898 bytes.  The
  V2 toolchain uses a different frame, instruction order, and local layout.  The
  store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x16f34 --rebuilt work/rebuild/pcvwmod.exe --rebuilt-map
  work/rebuild/player_car_view_update.map --rebuilt-symbol
  player_car_view_update_`.

## The race_results_screen module at 0x2a558

- Observation: the original body at `0x2a558` holds 343 instructions and 1323
  bytes up to the `ret` at `0x2aa82`.  The plan row holds the same counts.  The
  store record keeps the 1324-byte extent and sets no function size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2a558 --count 343` and
  `tools/rebuild/functions/0x2a558.json`.
- Observation: the only direct caller is at `0x2a335`, inside the race-menu
  handler at `0x2a214`.  The handler is called by `net_race_screen` at
  `0x2908b` and `net_race_lobby` at `0x2dcf9`.  The named ancestors belong to
  the race-frame group, so the record takes the section `race_frame callees`.
  Evidence: the call sweep for the target `0x2a558` and the call sites at
  `0x2a335`, `0x2908b`, and `0x2dcf9`.
- Observation: the function clears the video buffer, loads `load<N>H.pcx` from
  `gfx\graf.dat`, sets the palette, builds the palette shade tables, builds the `map<N>` name,
  and copies the pixels.  The screen index comes from the chosen player record
  at object-3 offset `0x2099c`, field `+0x14`.  A value above 5 drops by 6 and
  sets the flag 0x08 in the saved word at `0x1d274`.
  Evidence: the blocks at `0x2a56c` through `0x2a6fe` and the string table at
  module linear `0x101034` through `0x10108b`.
- Observation: the function runs `race_cleanup`, the session init at `0x1fd70`,
  the music step, and `menu_space_init`.  The input word `DAT_00118e1c` gates
  the results: the bits 0x1020 return after `menu_frame`.
  Evidence: the calls at `0x2a74e` through `0x2a75d` and the test at `0x2a762`.
- Observation: when the gate is clear, the function loads `champ5h.pcx`, marks
  the local result record at `0x13cd8` as finished (`-3`) with the race time
  `DAT_0011d268` at `+0x2c`, collects the records whose status is `-3`, and
  sorts them in ascending time order.  It then formats each line as
  `\001%d. %s %s \002%s` with the position, the net player name at `+0x0c`, the
  time `%02d.%02d.%02d`, and the car text at `0x1d550` record `+0x14`.  It draws
  the lines through `text_draw` at `0x25020`.
  Evidence: the blocks at `0x2a771` through `0x2aa25`, the sort at `0x2a8bd`
  through `0x2a914`, and the formats at module linear `0x10106d` and `0x10107c`.
- Observation: the wait loop runs `frame_step` and exits on the byte at 3 or
  the byte at 0x1e.  Otherwise it polls the input through `0x26448` only when
  `DAT_0011d2c0` and `DAT_0011d2cc` are both non-zero.  It then runs
  `menu_frame` and restores the saved word at `0x1d274`.
  Evidence: the loop at `0x2aa39` through `0x2aa67` and the epilogue at
  `0x2aa69`.
- Inference: the function is the post-race results screen.  The `champ5h.pcx`
  background, the finished-result records, the time sort, and the result lines
  support the name `race_results_screen`.  The module records the name with the
  confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the module uses the FPU and reads the 100.0 constant from
  `DAT_0010108b`, because the harness does not load the constant pool.  The
  local FPU stub makes `__CHP` truncate toward zero, because the original uses
  the helper at `0x4d9f4`.
  Evidence: `tools/rebuild/race_results_screen_fpu.asm` and
  `tools/rebuild/race_results_screen_main.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `5` and `D`.  The paths cover the full two-finisher sort, the input
  gate, the failed first load, the first video swap, the track adjust, and the
  three-finisher hundredths sort.
  Evidence: `python3 tools/rebuild/harness.py run --module
  race_results_screen`, `work/rebuild/rrsc_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x2a558`.
- Observation: the rebuilt module holds 256 instructions and 970 bytes.  The
  identity check stays open at 47 of 256 instructions and 56 of 970 bytes.  The
  V2 toolchain uses a different frame and instruction order.  The store records
  `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2a558 --rebuilt work/rebuild/rrscmod.exe --rebuilt-map
  work/rebuild/race_results_screen.map --rebuilt-symbol race_results_screen_`.

## The championship_screen module at 0x2cf68

- Observation: the original body at `0x2cf68` holds 370 instructions and 1357
  bytes up to the `ret` at `0x2d4b4`.  The record keeps the 1360-byte extent
  and sets no function size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2cf68 --count 370` and
  `tools/rebuild/functions/0x2cf68.json`.
- Observation: the only caller is the call at `0x295f1` inside
  `race_state_update` at `0x29564`, case 5.  The dispatcher is a callee of
  `race_frame` at `0x2807c`, so the record takes the section
  `race_frame callees`.
  Evidence: the direct call sweep for `0x2cf68` and
  `tools/rebuild/race_state_update.c`.
- Observation: the selected index `DAT_001203dc` picks one 0x2c-byte record
  from the table at object-3 offset `0x22bc`.  The records hold the background
  names `champ0` through `champ7` at `+0x18`.  The loader at `0x2c7a8` builds
  `%s%c.pcx` from that name and reads the background for the state 5 screen.
  Evidence: the table at module linear `0x1022bc` and the block at `0x2c8b4`
  through `0x2c8d0`.
- Observation: the text list `DAT_00120030` is the `champ` list that
  `menu_space_init` loads at `0x29d08`.  The function reads the group starts
  from `DAT_001201c0` for the four heads, the two summary labels, and the
  title.
  Evidence: `tools/rebuild/menu_space_init.c` and `0x2c7f9` through `0x2c863`.
- Observation: a record type of 3 selects the standings table.  The function
  draws the two rectangles, the title, the four heads, and six rows.  Each row
  holds the position `-%d-`, the car number `DAT_00120400[i]`, the time
  `DAT_001203e8[i]` as minutes.seconds.hundredths, and the points
  `DAT_00120418[i]`.  The function sums the points into `DAT_001203e4` and
  draws the two summary lines.
  Evidence: the blocks at `0x2cfa8` through `0x2d399` and the formats at module
  linear `0x1011ff` through `0x101246`.
- Observation: the flag `DAT_00120804` and the record field `+0x24` select a
  placeholder row with the texts `10`, `00.00.00`, and `0`.  A car number of
  -1 selects the empty row.
  Evidence: the tests at `0x2d0ab`, `0x2d0c8`, and `0x2d263`.
- Observation: a record type other than 3 selects the centred list.  The
  record field `+0x0c` picks the group of the `champ` list, and the record
  field `+0x28` picks the font.  The function calls `net_race_screen` at
  `0x28ba4` when the frame timer `DAT_0011d24c` is above 10.0.
  Evidence: the loop at `0x2d39e` through `0x2d46a` and the compare at
  `0x2d46f`.
- Observation: the footer panel at `0x2d485` appears only when the low seven
  bits of `DAT_000007fd` are below 0x48.
  Evidence: the test at `0x2d485` and the call at `0x2d4a3`.
- Inference: the function is the championship screen for state 5.  The `champ`
  text list, the `champ<N>` backgrounds, the standings table, and the point
  sum support the name `championship_screen`.  The module records the name with
  the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `3` and `D`.  The paths cover the standings table with a normal, an
  empty, and a placeholder row, the point sum, the centred list, the
  `net_race_screen` gate, the timer gate, and the footer gate.
  Evidence: `python3 tools/rebuild/harness.py run --module championship_screen`,
  `work/rebuild/cscr_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x2cf68`.
- Observation: the rebuilt module holds 321 instructions and 1122 bytes.  The
  identity check stays open at 78 of 321 instructions and 77 of 1122 bytes.
  The V2 toolchain uses a different frame and instruction order.  The store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2cf68 --rebuilt work/rebuild/champmod.exe
  --rebuilt-map work/rebuild/championship_screen.map --rebuilt-symbol
  championship_screen_`.

## The engine_rpm_update module at 0x18db0

- Observation: the original body at `0x18db0` holds 448 instructions and 1734
  bytes up to the `ret` at `0x19475`.  The record keeps the 1736-byte extent and
  sets no function size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x18db0 --count 448` and
  `tools/rebuild/functions/0x18db0.json`.
- Observation: the only direct caller is the player update group at `0x15a80`,
  which runs this function at `0x15a94` after the player input step at
  `0x18a14` and before `0x160a4`, `0x16150`, and `player_steering_update` at
  `0x1629c`.  The group runs on the player object, so the record takes the
  section `The vehicle state chain`.
  Evidence: the direct call sweep for `0x18db0`; the block at `0x15a80`
  through `0x15ab4`; and the `player_steering_update` finding above.
- Observation: the function reads the input flags `0x18e1c` and `0x18e1d` and
  the six joystick axes `0x19880` through `0x19894`.  The flag bit `0x40`
  selects the axis store at `0x19884` and the filter index `0x3d`.  The input
  bit `0` gates the main path.  The manual path compares the axes 4 and 5 with
  `0x1985c` and `0x19878`.
  Evidence: the blocks at `0x18dca` through `0x18efb`.
- Observation: the function stores the gear at `+0x7ac` and the shift flag at
  `+0x140`.  The manual path increments or decrements the gear when the shift
  flag is zero.  The automatic path uses the thresholds `0x2134` and `0xfa0`
  against the engine speed at `+0x7a8`, the gear count at `+0x494`, and the
  state `+0x148`.
  Evidence: the blocks at `0x18e09` through `0x18f92`.
- Observation: the function forms the speed limit at `+0x510` as
  `+0x480 - (sin(+0x4c8) * 4.0 + 1.0) * 1000.0`.  It forms the engine-speed
  target from the throttle `+0x144`, the maximum engine speed `+0x484`, the
  wheel factor `+0x174`, and the gear ratio at `+0x494 + gear * 4`.  The gear
  count `1` selects the square-root torque path, and a larger count selects the
  sine torque path.  The function updates the road speed at `+0x50c`, sets
  `+0x110` on a strong deceleration, and calls `0x19478`, `filter_update` at
  `0x22b80`, `sin` at `0x4da72`, `sqrt` at `0x22bec`, and `0x1f7e4` with the
  value `14 * +0x7a8 + 32768`.
  Evidence: the blocks at `0x18f92` through `0x19468`; the constants at
  object-3 offsets 0x858 through 0x8a8; and the gear table at object-3 offset
  0x1d88.
- Inference: the function updates the player engine speed and the selected
  gear from the throttle, the road speed, and the pitch.  The engine-speed
  store at `+0x7a8`, the gear store at `+0x7ac`, the two shift thresholds, the
  gear-ratio table, and the sound value support the name `engine_rpm_update`.
  The module records the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the harness links no math library.  The local FPU stub provides
  `sin` and `sqrt` through `fsin` and `fsqrt`, and it makes `__CHP` truncate
  toward zero, because the original uses the helper at `0x4d9f4`.  The harness
  holds the constants in globals, because it does not load the constant
  segment.
  Evidence: `tools/rebuild/engine_rpm_update_fpu.asm` and
  `tools/rebuild/engine_rpm_update_main.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9`, `a`, and `D`.  The paths cover the manual upshift, the manual
  downshift, the manual neutral path, the automatic upshift, the automatic
  downshift, the state 5 reset, the input bit `0x40`, the clear input bit `0`,
  the neutral speed decay, the one-gear square-root path, and the over-speed
  clamp.
  Evidence: `python3 tools/rebuild/harness.py run --module engine_rpm_update`,
  `work/rebuild/eru_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x18db0`.
- Observation: the rebuilt module holds 339 instructions and 1117 bytes.  The
  identity check stays open at 44 of 339 instructions and 64 of 1117 bytes.
  The V2 toolchain uses a different instruction order and local layout.  The
  store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x18db0 --count 448 --rebuilt work/rebuild/erumod.exe
  --rebuilt-map work/rebuild/engine_rpm_update.map --rebuilt-symbol
  engine_rpm_update_`.

## The options_screen module at 0x21338

- Observation: the plan row records a 1715-byte body of 464 instructions, 38
  calls, and 47 FPU instructions.  The body starts at `0x21338` and ends at the
  `ret` at `0x219ea`.  The store record keeps the 1716-byte extent and sets no
  function size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x21338 --count 464` and
  `tools/rebuild/functions/0x21338.json`.
- Observation: the only direct caller is `input_map` at `0x2124f`.  It calls
  the function when the input flag `0x1b` is set and the mode `0x1d260` is not
  `0x10`, and it clears the dword `0x7f9` after the call.
  Evidence: the call site at `0x2124f` and the earlier input_map section.
- Observation: the label table at object-3 offset `0x1f74` holds the three
  five-byte strings `CD`, `FX`, and `MU`.  The pointer table at `0x1f80` holds
  the three data offsets `0x1d2c0`, `0x1d2bc`, and `0x1d2c4`.  The table at
  `0x1f20` holds seven records of three dwords: the title x, the static y, and
  the moving y.
  Evidence: the byte dump at object-3 offsets `0x1f20`, `0x1f74`, and `0x1f80`,
  and the reads at `0x217f5` through `0x21959`.
- Observation: the function clears the screen twice through `0x25d28` with the
  size pair `0x280` and `0x190`, fades the three volume locals by the frame
  delta times `16384.0`, draws the list rows with the format `03 25 64 00`, and
  draws the selected `CD`, `FX`, or `MU` bar through `0x23dfc`.  State 5 rises
  the selected volume by the frame delta times `48000.0` and clips it at
  `0x7fff`.  State 6 advances the volume index from 0 through 2.  The loop
  leaves when the key byte `0x03` is set.
  Evidence: the blocks at `0x2134c` through `0x2199a`.
- Inference: the caller, the three audio labels, the three audio slots, and the
  seven menu positions identify the function as the options screen.  The
  module records the name `options_screen` with the confidence `inferred`.
  Evidence: the call graph and the data references above.
- Observation: the harness sets the fade rate, the rise rate, the minimum
  delta, and the time scale from their bit patterns, because the minimal image
  does not load the constant segment.  The three labels are extern in the
  module and defined in the harness, so the harness can check the selected
  label.  The local FPU stub makes `__CHP` truncate toward zero, because the
  original uses the helper at `0x4d9f4`.
  Evidence: `tools/rebuild/options_screen.c`,
  `tools/rebuild/options_screen_main.c`, and
  `tools/rebuild/options_screen_fpu.asm`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9`, `a`, and `D`.  The paths cover the first frame, the volume fade,
  the state 4 fall, the state 5 rise and clip, the state 6 index cycle, the
  state 3 and state 2 selection, the left and right keys, the list rows, and
  the empty list.
  Evidence: `python3 tools/rebuild/harness.py run --module options_screen`,
  `work/rebuild/opts_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x21338`.
- Observation: the rebuilt module holds 464 decoded instructions and 1629
  bytes.  The identity check stays open at 69 of 464 instructions and 88 of
  1629 bytes.  The V2 toolchain uses a different instruction order, local
  layout, and helper form.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x21338 --count 464 --rebuilt work/rebuild/optsmod.exe
  --rebuilt-map work/rebuild/options_screen.map --rebuilt-symbol
  options_screen_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The car_camera module at 0x1a930

- Observation: the plan row records a 1425-byte body of 396 instructions, 18
  calls, and 126 FPU instructions.  The body starts at `0x1a930` and ends at
  the `ret` at `0x1aec0`.  The store record keeps the 1428-byte extent and
  sets no function size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1a930 --count 396` and
  `tools/rebuild/functions/0x1a930.json`.
- Observation: the direct callers are `player_view_setup` at `0x19ae8`, which
  calls this function at `0x19c76` and `0x19e3f`, and the camera group at
  `0x1aec4`, which calls it at `0x1aefc` on every 0x590-byte car record at
  `DAT_00110ab8` and then copies the selected view block at `+0x4f8`.
  Evidence: the three call sites, the loop at `0x1aed2` through `0x1af01`,
  and the copy at `0x1af45` through `0x1af57`.
- Observation: the function walks the 0x38-byte route records below the
  pointer `DAT_0010f90c`.  The count is at `+0x1c`, and each record holds the
  position at `+0x04` (x), `+0x08` (z), and `+0x0c` (y).  The loop picks the
  nearest record by the horizontal distance from the car position at `+0x498`
  and `+0x4a0`.  A zero distance does not take part.
  Evidence: the loop at `0x1a961` through `0x1aa05`.
- Observation: the function forms the yaw at `+0x52c` and the pitch at
  `+0x528`.  The float constants are 2pi at object-3 offset `0x92c`, -pi/2 at
  `0x930`, and -1500.0 at `0x934`.  The original calls the routine at
  `0xc5e42`, which computes pi/2 minus `acos(x)`, so the routine is `asin`.
  Evidence: the reads at `0x1aabe`, `0x1aad0`, `0x1ab0c`, and `0x1ad71`, the
  disassembly of `0xc5e42`, and the data at object-3 offsets `0x92c`, `0x930`,
  and `0x934`.
- Observation: the function copies the chosen record position into the view at
  `+0x4f8`, `+0x4fc`, and `+0x500`.  When the chosen record changes, it counts
  the distance from the saved target at `+0x16c` and `+0x174` into `+0x55c`.
  It sets the view flag at `+0x568` from the absolute height modulo 10 and the
  saved distance.  When the flag changes while the view is idle, it holds the
  old flag and runs the setup path once.  When the total distance exceeds 1500
  units, it moves the view along the yaw and the pitch.
  Evidence: the blocks at `0x1ab2b` through `0x1ae53`.
- Inference: the call graph, the per-car loop, and the view position, angle,
  and movement data identify the function as the car camera.  The existing
  `player_view_setup` module already names the callee `car_camera`, so the
  module records the name `car_camera` with the confidence `inferred`.
  Evidence: the call graph, the data flow above, and
  `tools/rebuild/player_view_setup.c`.
- Observation: the harness sets the three float constants and the far limit
  from their bit patterns, because the minimal image does not load the constant
  segment.  The local FPU stub supplies `asin`, `sqrt`, `sin`, and `cos` from
  the hardware, because this harness links no math library.  The stub also
  makes `__CHP` truncate toward zero, because the original uses the helper at
  `0x4d9f4`.
  Evidence: `tools/rebuild/car_camera.c`,
  `tools/rebuild/car_camera_main.c`, and
  `tools/rebuild/car_camera_fpu.asm`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `8` and `D`.  The paths cover the nearest nonzero node, the zero
  distance skip, the move above and below the 1500-unit threshold, the
  unchanged record, and the four height modulo 10 flag cases.
  Evidence: `python3 tools/rebuild/harness.py run --module car_camera`,
  `work/rebuild/ccam_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x1a930`.
- Observation: the rebuilt module holds 396 decoded instructions and 1383
  bytes.  The identity check stays open at 67 of 396 instructions and 71 of
  1383 bytes.  The V2 toolchain uses a different register order, local layout,
  and helper form.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1a930 --count 396 --rebuilt work/rebuild/carcam.exe
  --rebuilt-map work/rebuild/car_camera.map --rebuilt-symbol car_camera_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The menu_objects_init module at 0x2aab8

- Observation: the plan row records a 1420-byte body of 395 instructions, 32
  calls, and 0 FPU instructions.  The body starts at `0x2aab8` and ends at the
  `ret` at `0x2b043`.  The store record keeps the 1420-byte extent and sets no
  function size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2aab8 --count 395` and
  `tools/rebuild/functions/0x2aab8.json`.
- Observation: the only caller is `menu_space_init` at `0x29d08`, which calls
  this function at `0x29f0c`.  The nearest prologue below the call site is
  `0x29d08`.  The `menu_space_init` module already names the callee
  `menu_objects_init`.
  Evidence: the call at `0x29f0c`, the nearest prologue at `0x29d08`, and
  `tools/rebuild/menu_space_init.c`.
- Observation: the data at object-3 offset `0x3f0` holds `OBJ_LIST`, at
  `0x3ca` `3D_BLKOBJ`, at `0x108f` `ALETTER`, at `0x2edc` `AAUTO00`, and at
  `0x2ef0` `AMAP1`.
  Evidence: the data dump at the object-3 offsets and the immediate operands
  `0x3f0`, `0x3ca`, `0x108f`, `0x2edc`, and `0x2ef0`.
- Observation: the function copies the two names to local buffers and loads
  the `OBJ_LIST` resource through `0x119cc`.  It looks `ALETTER` up in the
  object list at `0x6530` through `0x11060`.  Each failure runs
  `bios_video(3)`, `error_report`, and `exit`.
  Evidence: the block at `0x2aabf` through `0x2ab17`.
- Observation: the loop at `0x2ab18` loads 79 objects into the 8-byte slots at
  `DAT_001d2d0` through `0x10ed4`.  The loop at `0x2ab4a` fills the 0xe slots
  at `DAT_001d550` with the names `AAUTO00` through `AAUTO06`.  The byte at
  offset 6 of the local name comes from the counter, so each pair of slots
  takes the next car name.  The first load sets the object field `+0x20` to
  0x6000 and `+0x3c` to 0.  The second load sets `+0x20` to 0xe000 and `+0x3c`
  to 0.
  Evidence: the loops at `0x2ab18` through `0x2ac95`.
- Observation: the loops at `0x2ac96`, `0x2ad62`, and `0x2ad6f` fill the 0xc
  slots at `DAT_001d908` with `AMAP1` through `AMAP6` twice and set the same
  object fields.  The cardat list at `DAT_001fd10` supplies 14 names and five
  numbers each.  The trackdat list at `DAT_001fea0` supplies 12 names and one
  number each.  The 26 slots bind through `0x11850`, the list links through
  `0x11568`, and the last call at `0x2b037` runs the init at `0x2dff4`.
  Evidence: the blocks at `0x2ac96` through `0x2b043`.
- Inference: the call graph, the menu object names, and the cardat and trackdat
  tables identify the function as the menu object setup.  The caller already
  names it `menu_objects_init`, so the module records that name with the
  confidence `derived`.
  Evidence: the call graph, the data flow above, and
  `tools/rebuild/menu_space_init.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, and `D`.  The paths cover the normal path, the `ALETTER` failure,
  and the first car name failure.  Each failure path continues after the
  `exit` stub, because the stub returns.
  Evidence: `python3 tools/rebuild/harness.py run --module menu_objects_init`,
  `work/rebuild/mo_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x2aab8`.
- Observation: the rebuilt module holds 395 decoded instructions and 1244
  bytes.  The identity check stays open at 67 of 395 instructions and 53 of
  1244 bytes.  The V2 toolchain uses a different register order, local layout,
  and helper form.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2aab8 --count 395 --rebuilt work/rebuild/menuobj.exe
  --rebuilt-map work/rebuild/menu_objects_init.map --rebuilt-symbol
  menu_objects_init_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The player_car_link module at 0x15c1c

- Observation: the original body at `0x15c1c` holds 306 instructions and 1160
  bytes up to the `ret` at `0x160a3`.  The plan row holds the same counts.  The
  store record keeps the 1160-byte extent and sets no function size.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x15c1c --count 306` and
  `tools/rebuild/functions/0x15c1c.json`.
- Observation: the five callers at `0x20195`, `0x20279`, `0x2034e`, `0x203bb`,
  and `0x2041f` pass the player object at object-3 offset `0xf91c` in `eax`.
  The callers are the five view groups.  Each group calls
  `player_car_view_update` at `0x16f34` and the player physics group at
  `0x15a80` first.  The function therefore runs on the player object after the
  vehicle state chain, so the record takes the section
  `The vehicle state chain`.
  Evidence: the direct call sweep for the target `0x15c1c`, the call sites at
  `0x20195`, `0x20279`, `0x2034e`, `0x203bb`, and `0x2041f`, and the records
  of `0x16f34` and `0x15a80`.
- Observation: the 28-byte entries at `DAT_0010653c` hold the link pointer at
  field 0 and the render pointer at field 4.  The object field `+0x460` selects
  the link entry.  The object field `+0x468` selects the target entry, and its
  field 0 is the target pointer.
  Evidence: the blocks at `0x15c27` through `0x15c78`.
- Observation: the call at `0x15c7e` targets `0x18ccc`.  That function forms
  the horizontal distance from the world position `DAT_00110a20` and
  `DAT_00110a28` to the object position `+0x4b0` and `+0x4b8`.  It writes the
  screen offsets `+0x118`, `+0x11c`, and `+0x120` with the limit 20.0 at
  object-3 offset 0x854.
  Evidence: the disassembly of `0x18ccc` through `0x18dad`.
- Observation: the link pointer receives the position sum from `+0x4b0`,
  `+0x4b4`, and `+0x4b8` with the offsets `+0x130`, `+0x134`, and `+0x138`.
  It also receives the angle sum from `+0x4c8`, `+0x4cc`, and `+0x4d0` with
  the offsets `+0x124`, `+0x128`, and `+0x12c`.  The scale 1024.0 at
  object-3 offset 0x6ec and the pi at 0x6f0 give the three 11-bit values.
  Evidence: the blocks at `0x15c83` through `0x15d30` and the data at
  object-3 offsets 0x6ec and 0x6f0.
- Observation: the render arrays take the value from `+0x4e0` times 1024.0
  times 2.0 at offset 8.  They take the value from `+0x4e8` times 1024.0 times
  4.0 at offset 0x10.  The constants 2.0 and 4.0 sit at object-3 offsets 0x6f4
  and 0x6f8.  The first masked pitch is `+0x108` divided by `+0x10c` times
  1.5 and by the double pi at 0x704.  The speed `+0x50c` divided by 15.0 at
  0x70c minus the stored value masks with 0x7fe.
  Evidence: the blocks at `0x15d91` through `0x15fc1` and the data at
  object-3 offsets 0x6f4, 0x6f8, 0x704, and 0x70c.
- Observation: the target pointer receives the screen offset `+0x118` at 8,
  the screen offset `+0x120` at 0x10, the link angle at 0x18, and `+0x4ec - 2`
  at 0xc.  When the state at `+0x148` is not 1, it also receives the two link
  angles at 0x14 and 0x1c.  The link pointer receives `+0x7b0` at 0x3c and
  `+0x7b4` at 0x20.
  Evidence: the blocks at `0x15d33` through `0x1609b`.
- Inference: the function links the player car object to the render arrays.
  It copies the screen positions, scales the three angles, writes the render
  values, and sets the link flags.  The player object, the five callers, and
  the sibling `object_car_link` at `0x1c84c` support the name
  `player_car_link`.  The module records the name with the confidence
  `derived`.
  Evidence: the call graph and the data flow above.
- Observation: the original truncates through the Watcom helper at `0x4d9f4`.
  The local FPU stub makes `__CHP` truncate toward zero.  The stub also holds
  `sqrt`, because the harness links no math library and the screen projection
  stub uses the routine at `0x22bec`.
  Evidence: `tools/rebuild/player_car_link_fpu.asm` and
  `tools/rebuild/player_car_link_main.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `7` and `D`.  The paths cover the full path, the state skip, the
  second target, the pitch divisor, the speed mask, the angle mask, the second
  axis, and the second link pair.
  Evidence: `python3 tools/rebuild/harness.py run --module player_car_link`,
  `work/rebuild/pcl_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x15c1c`.
- Observation: the rebuilt module holds 306 decoded instructions and 1133
  bytes.  The identity check stays open at 76 of 306 instructions and 67 of
  1133 bytes.  The V2 toolchain uses a different instruction order and local
  layout.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x15c1c --count 306 --rebuilt work/rebuild/pclink.exe
  --rebuilt-map work/rebuild/player_car_link.map --rebuilt-symbol
  player_car_link_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee name in the harness stay inferred.

## The map_object_place module at 0x2b044

- Observation: the original body at `0x2b044` holds 222 instructions and 708
  bytes up to the `ret` at `0x2b307`.  The next function starts at `0x2b308`, so
  the store record keeps the 708-byte extent and sets no function size.  The
  identity check uses 222 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2b044 --count 222` and
  `tools/rebuild/functions/0x2b044.json`.
- Observation: the direct call sweep finds one caller at `0x29a88` in the race
  state update at `0x29918`.  The caller walks the selected menu lines, reads
  the line descriptor at `DAT_0011fad4`, and takes the string from the pointer
  arrays at `DAT_0011fb80`.
  Evidence: the direct call sweep for `0x2b044`, the loop at `0x29a27` through
  `0x29ab4`, and the call at `0x29a88`.
- Observation: the `map_object_update` module at `0x2b9cc` reads the records
  that this function writes.  The update runs in the same caller, and the
  records hold the fields `+0x08`, `+0x34` through `+0x60` in the layout that
  the update reads.  The function fills the records from a menu string.
  Evidence: `tools/rebuild/map_object_update.c` and the disassembly of
  `0x2b109` through `0x2b27b`.
- Observation: the function computes the line start.  A centered line starts at
  `(length - 1) * 320`, and another line starts at `0x1200 - (+0x08) * 640`.
  The vertical position is `0x820 - (+0x04) * 384`.
  Evidence: the blocks at `0x2b053` through `0x2b0c0`.
- Observation: the function walks the string until the first code outside
  `0x20` through `0x7f`.  For each code it takes the first free record of the
  0x50 records at `DAT_001db78`, each 0x64 bytes long.  It stores the code minus
  0x20 at `+0x08`, the line id at `+0x04`, the target x and y at `+0x40` and
  `+0x44`, the start x and y at `+0x34` and `+0x38` or the target x and y, the
  start z at `+0x3c`, the state flag at `+0x0c`, the angle pi or 0 at `+0x10`,
  the amplitudes at `+0x4c` and `+0x50`, the progress 1.0 at `+0x58`, the
  direction 1.0 at `+0x5c`, the angle rate 1.0 at `+0x1c`, the zero fields, and
  the timer at `+0x60`.
  Evidence: the blocks at `0x2b0cd` through `0x2b27b`.
- Observation: the timer rises by 0.01 for each placed code.  The function then
  rotates the line origin by the angle at `+0x28` for the next code.  The new x
  is `y * sin + x * cos`, and the new y is `y * cos - x * sin`, truncated
  toward zero.
  Evidence: the blocks at `0x2b27b` through `0x2b2fc`.
- Inference: the function places one menu line into the map object records.  The
  caller, the record layout, and the update reader support the name
  `map_object_place`.  The module records the name with the confidence
  `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls the cosine helper at `0x4da66`, the sine
  helper at `0x4da72`, and the truncate helper at `0x4d9f4`.  The harness links
  no math library, so the local FPU stub supplies `cos` and `sin` from the
  hardware and makes `__CHP` truncate toward zero.  The timer step arrives
  through a global, because an FPU memory operand of a literal reads the
  constant pool at a low address in this toolchain.
  Evidence: `tools/rebuild/map_object_place_fpu.asm` and
  `tools/rebuild/map_object_place_main.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `4` and `D`.  The paths cover the centered line, the left line with
  the start override and the pi/2 rotation, the occupied record, the low code
  stop, and the high code stop.
  Evidence: `python3 tools/rebuild/harness.py run --module map_object_place`,
  `work/rebuild/mop_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x2b044`.
- Observation: the rebuilt module holds 222 decoded instructions and 671 bytes.
  The identity check stays open at 47 of 222 instructions and 15 of 671 bytes.
  The V2 toolchain uses a different prologue, a different instruction order, and
  a different local layout.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2b044 --count 222 --rebuilt work/rebuild/moplace.exe
  --rebuilt-map work/rebuild/map_object_place.map --rebuilt-symbol
  map_object_place_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The map_object_update module at 0x2b9cc

- Observation: the original body at `0x2b9cc` holds 310 instructions and 1121
  bytes up to the first `ret` at `0x2be2c`.  The body continues to the shared
  epilogue at `0x2be6f`, so the store record keeps the 1224-byte extent and
  sets no function size.  The identity check uses the first 310 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2b9cc --count 340` and
  `tools/rebuild/functions/0x2b9cc.json`.
- Observation: the only direct caller is at `0x29b86` in the race state update
  at `0x29918`.  The caller holds 0x50 records at `DAT_001db78`, each 0x64 bytes
  long, and runs this function on every record at `0x29b82`.
  Evidence: the direct call sweep for the target `0x2b9cc` and the loop at
  `0x29b5a` through `0x29b8d`.
- Observation: the map placement at `0x2b044` fills the same records from a map
  string.  It stores the character minus 0x20 at `+0x08`, the start positions at
  `+0x34` and `+0x38`, the target positions at `+0x40` and `+0x44`, the third
  position at `+0x3c`, the amplitudes at `+0x4c` and `+0x50`, the progress 1.0
  at `+0x58`, and the timer at `+0x60`.  The caller at `0x29a88` runs the
  placement before the update, so the records hold map objects.
  Evidence: the disassembly of `0x2b109` through `0x2b27b` and the call at
  `0x29a88`.
- Observation: a record with `+0x08` equal to -1 holds no object.  The function
  binds the list name at `0x6350` through the helper at `0x11850` and returns.
  Otherwise it calls the helper at `0x117fc`, reads the slot at
  `DAT_001d2d0[index * 2]`, and takes the object pointer from the 0x1c-byte
  entry table at `0x653c`.  The object frame `+0x20` is `DAT_001fab8` times the
  record id `+0x08` plus 0x226.
  Evidence: the blocks at `0x2b9e3` through `0x2ba7d` and the helpers at
  `0x117fc` and `0x11850`.
- Observation: the function interpolates the object positions `+0x08`, `+0x0c`,
  and `+0x10` from the record targets `+0x40`, `+0x44`, and `+0x48` back to
  `+0x34`, `+0x38`, and `+0x3c` with `1 - cos(progress * pi/2)`.  The progress
  is `+0x58`.  The cosine and the sine of the progress scaled by 2 pi times the
  amplitudes `+0x4c` and `+0x50` add the sway to the first two positions.
  Evidence: the blocks at `0x2ba80` through `0x2bbf5` and the data at
  object-3 offsets 0x10d3 and 0x10d7.
- Observation: the height `+0x10` also takes a travelling wave.  The phase is
  `(x + 4608) / 9216` times 2 pi plus `(y + 2080) / 4160` times pi plus
  `DAT_001d248`.  When the record flag `+0x0c` is zero, the x term is 0.5 and
  the phase takes the factor 0.25.  The sine of the phase times 150.0 adds to
  the height.
  Evidence: the blocks at `0x2bbf8` through `0x2bcc4` and the data at
  object-3 offsets 0x10db, 0x10df, 0x10e7, 0x10eb, 0x10f3, 0x10f7, and
  0x10ff.
- Observation: the record angles `+0x10`, `+0x14`, and `+0x18` scale by
  1024.0 / pi and mask with 0x7ff into the object angles `+0x14`, `+0x18`, and
  `+0x1c`.  The progress `+0x58` falls by the frame delta times `+0x5c`, and
  the timer `+0x60` falls by the frame delta.  Both clamp at zero.
  Evidence: the blocks at `0x2bcc7` through `0x2bdaa` and the data at
  object-3 offset 0x1103.
- Observation: the final block steps the record angle `+0x10`.  With `+0x0c`
  and `DAT_001fac0` both zero, the angle falls by the frame delta times 4 pi
  and clamps at 0.05.  Otherwise an angle above pi/2 sets `DAT_001fb64` to -2,
  the angle rises by the same rate, and an angle above pi clamps at pi.  An
  angle already at pi returns.
  Evidence: the blocks at `0x2bdaa` through `0x2be6f` and the data at
  object-3 offsets 0x1107 and 0x110b.
- Inference: the function advances the map object records.  It links the
  object, interpolates the position with the sway and wave offsets, and
  advances the object angles and timers.  The caller, the placement routine,
  and the record fields support the name `map_object_update`.  The module
  records the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls `sin` at `0x4da72`, `cos` at `0x4da66`, and
  the truncate helper at `0x4d9f4`.  The harness links no math library, so the
  local FPU stub supplies `sin` and `cos` from the hardware and makes `__CHP`
  truncate toward zero.
  Evidence: `tools/rebuild/map_object_update_fpu.asm`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `7` and `D`.  The paths cover the unused record, the full path, the
  two zero clamps, the pi return, the mode set, the interpolation, the overlay
  path, and the zero-angle return.
  Evidence: `python3 tools/rebuild/harness.py run --module map_object_update`,
  `work/rebuild/moupd.txt`, and
  `python3 tools/rebuild/run.py run --function 0x2b9cc`.
- Observation: the rebuilt module holds 310 decoded instructions and 1035
  bytes.  The identity check stays open at 37 of 310 instructions and 32 of
  1035 bytes.  The V2 toolchain uses a different instruction order and local
  layout.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2b9cc --count 310 --rebuilt work/rebuild/mapobj.exe
  --rebuilt-map work/rebuild/map_object_update.map --rebuilt-symbol
  map_object_update_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The spline_interpolate module at 0x22744

- Observation: the original body at `0x22744` holds 360 instructions and 1081
  bytes up to the `ret 4` at `0x22b7a`.  The store record keeps the 1084-byte
  extent and sets no function size.  The identity check uses the first 360
  instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x22744 --count 360` and
  `tools/rebuild/functions/0x22744.json`.
- Observation: the direct call sweep finds one caller at `0x231e0`.  The caller
  sits in the path playback at `0x23038`.  That function walks the 0x28-byte
  keyframe records of the table that `0x231f0` appends.  The keyframe append
  copies the view position, the view angles, and the other view fields of the
  player object at object-3 offset `0xf91c`.  The frame update at `0x1ff70`
  selects the view group at `0x202c0`, and that group calls both `0x231f0` and
  `0x23038`.
  Evidence: the direct call sweep for `0x22744`, the disassembly of `0x23038`
  and `0x231f0`, and the calls at `0x20306` and `0x20317`.
- Observation: the caller passes four point pointers and the fraction between
  the two keyframe timestamps.  The function reads the previous point `D` from
  `+0x00`, the first point `A` from `+0x04`, the second point `B` from `+0x08`,
  and the next point `C` from `+0x0c`.  The output pointer receives the three
  interpolated integers.  The caller copies the angles and the other fields of
  the current keyframe without interpolation.
  Evidence: the pointer loads at `0x22754` through `0x227bb` and the output
  stores at `0x22a43`, `0x22ad9`, and `0x22b70`.
- Observation: the function measures the chords `AB`, `BD`, and `AC` with the
  square root wrapper at `0x22bec`.  The chord `AB` divides by the double
  constant 3.0 at object-3 offset `0xda4`.  The tangents are
  `A' = A + (B - D) * (|AB| / (3 * |BD|))` and
  `B' = B + (A - C) * (|AB| / (3 * |AC|))`.  A zero `|BD|` or `|AC|` keeps the
  tangent at `A` or `B`.  The two tangents truncate to integers through the
  helper at `0x4d9f4`.
  Evidence: the chord blocks at `0x227be` through `0x2299b` and the constant at
  `0x100da4`.
- Observation: the result is the cubic Hermite form
  `A + t * 3(A' - A) + t^2 * (3B' - 6A' + 3A)
  + t^3 * (3A' + B - 3B' - A)`.  The fraction `t`, its square, and its cube
  stay in single precision.  The polynomial evaluates in double precision and
  truncates to an integer for each axis.
  Evidence: the polynomial blocks at `0x2299e` through `0x22b70`.
- Inference: the function interpolates a position along a recorded path from
  four keyframes.  The caller, the keyframe append, and the cubic form support
  the name `spline_interpolate`.  The module records the name with the
  confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls the square-root wrapper at `0x22bec` and the
  truncate helper at `0x4d9f4`.  The harness links no math library, so the local
  FPU stub supplies `sqrt` from the hardware, returns 0.0 for a negative
  argument as `0x22bec` does, and makes `__CHP` truncate toward zero.  The chord
  scale 3.0 arrives through a global.
  Evidence: `tools/rebuild/spline_interpolate_fpu.asm` and
  `tools/rebuild/spline_interpolate_main.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `8` and `D`.  The paths cover the collinear fraction 0.5, the two
  endpoints, the zero `|BD|`, the zero `|AC|`, the three-dimensional point, the
  two three-dimensional endpoints, and the negative coordinates.
  Evidence: `python3 tools/rebuild/harness.py run --module spline_interpolate`,
  `work/rebuild/spln_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x22744`.
- Observation: the rebuilt module holds 360 decoded instructions and 1030
  bytes.  The identity check stays open at 82 of 360 instructions and 55 of
  1030 bytes.  The V2 toolchain uses a different prologue, a different
  instruction order, and a different local layout.  The store records
  `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x22744 --count 360 --rebuilt work/rebuild/spline.exe
  --rebuilt-map work/rebuild/spline_interpolate.map --rebuilt-symbol
  spline_interpolate_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee name in the harness stay inferred.

## The player_car_skid_update module at 0x17d14

- Observation: the original body at `0x17d14` holds 290 instructions and 1078
  bytes up to the `ret` at `0x18149`.  The store record keeps the 1080-byte
  extent and sets no function size.  The identity check uses the first 290
  instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x17d14 --count 290` and
  `tools/rebuild/functions/0x17d14.json`.
- Observation: the direct call sweep finds one caller at `0x15bc0`.  The caller
  sits in the effect group at `0x15bac`.  The five view groups at `0x2003c`,
  `0x201d0`, `0x202c0`, `0x20360`, and `0x203c4` call that group on the player
  object at object-3 offset `0xf91c`, after the physics group at `0x15a80`.
  Evidence: the call sweep for `0x17d14`, the disassembly of `0x15bac`, and the
  calls at `0x200e0`, `0x20209`, `0x2032b`, `0x20398`, and `0x203fc`.
- Observation: the function forms the slip value from the speed `+0x50c`, the
  yaw step `+0x2f4`, the frame delta `DAT_00118e04`, and the table indices
  `+0x1c0`, `+0x268`, `+0x214`, and `+0x2bc`.  The deceleration path scales
  `+0x110` by the table entry `DAT_0010cf08 + 0x188`, filters the value into
  `+0x4f0`, and adds it to the heading `+0x4cc`.
  Evidence: the blocks at `0x17d26` through `0x17e3c`.
- Observation: the normal path compares the absolute slip with `+0x184` and
  the gate with `+0x188`.  A large slip stores the amplitude `+0x4f0`, the
  rate `+0x4f4`, and the sine and cosine `+0x500` and `+0x508`.  The amplitude
  clamps at 1.53938 and the rate is `+0x50c^2 / +0x2f8 * +0x670 * 0.2`.  A
  small slip clears the amplitude and the rate.  A gate above `+0x188` sets
  `+0x170`.
  Evidence: the blocks at `0x17e41` through `0x1803f` and the constants at
  object-3 offsets `0x7d0` through `0x810`.
- Observation: the tail forms the value from `cos(+0x4d0)`, the root of two,
  and `+0x4f0`, and filters it into `+0x128` through `0x2211c` with the index
  `0x5a`.  The filtered slip above 0.23 allocates a table entry through
  `table_lookup` at `0x1f718` and starts the four effect slots through
  `0x19828`.  At or below 0.23 it releases the entry through `0x1f880` when the
  state `+0x148` is below 3.
  Evidence: the tail block at `0x1803f` through `0x18141`, the tables at
  object-3 offsets `0x6350` and `0x6530`, and the call sites `0x180f9`,
  `0x1810a`, and `0x18134`.
- Inference: the function updates the shake terms and the skid effect of the
  player car.  The effect group caller, the shake terms that
  `player_car_view_update` applies, the four effect slots, and the effect gate
  support the name `player_car_skid_update`.  The module records the name with
  the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls the filter at `0x22b80`, the angle filter at
  `0x2211c`, the sine at `0x4da72`, the cosine at `0x4da66`, and the square
  root at `0x22bec`.  The harness links no math library, so the local FPU stub
  supplies `sin`, `cos`, and `sqrt`.  The constants arrive through globals.
  Evidence: `tools/rebuild/player_car_skid_update_fpu.asm` and
  `tools/rebuild/player_car_skid_update_main.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `8` and `D`.  The paths cover the deceleration path, the large and
  small slip, the two amplitude clamps, the gate above the limit, the two early
  flags, and the input bit 0x40.
  Evidence: `python3 tools/rebuild/harness.py run --module
  player_car_skid_update`, `work/rebuild/pcsk_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x17d14`.
- Observation: the rebuilt module holds 290 decoded instructions and 1075
  bytes.  The identity check stays open at 40 of 290 instructions and 63 of
  1075 bytes.  The V2 toolchain uses a different instruction order and local
  layout.  The store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x17d14 --count 290 --rebuilt work/rebuild/pcskmod.exe
  --rebuilt-map work/rebuild/player_car_skid_update.map --rebuilt-symbol
  player_car_skid_update_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The name_entry_screen module at 0x2c430

- Observation: the original body at `0x2c430` holds 230 instructions and 885
  bytes up to the `ret` at `0x2c7a4`.  The store record keeps the 888-byte
  extent and sets no function size.  The identity check uses the first 230
  instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2c430 --count 230` and
  `tools/rebuild/functions/0x2c430.json`.
- Observation: the direct call sweep finds one caller at `0x2bebe`.  The call
  is case 4 of the state init dispatcher at `0x2be94`.  The update case 4 at
  `0x295e7` calls `0x2ce84`.  The update dispatcher `race_state_update` at
  `0x29564` is a `race_frame` callee at `0x2807c`, so the record takes the
  section `race_frame callees`.
  Evidence: the call sweep for `0x2c430`, the jump tables at linear `0x2be70`
  and `0x29540`, and the calls at `0x295c9` and `0x2807c`.
- Observation: the init loads `menu2h.pcx` from `gfx\graf.dat` with the
  palette at object-3 offset `0xcf5c`.  It presents the buffer, copies
  `0x3e800` bytes to `DAT_00121528`, fills the screen, and presents it again.
  It clears `DAT_0011d240` and `DAT_0011d244`, sets `DAT_00120378` at 1.5,
  `DAT_00120380` at 8.0, and `DAT_0011fac8` at 1.
  Evidence: the calls at `0x2c48e` through `0x2c4c3` and the stores at
  `0x2c453` through `0x2c488`.
- Observation: the letter index is the first byte of the player name
  `DAT_001203cc` minus 0x41.  A non-zero index places the menu marker from the
  cosine and the sine of `index * -0.24166098` at the scale 4500.0 and stores
  the heading `index * 0.24166098` at `DAT_00120388` and `DAT_00120394`.  A
  zero index stores the default 0x1194 at `DAT_001203b0` and `DAT_001203a4`.
  Evidence: the reads and the FPU blocks at `0x2c4c8` through `0x2c586`.
- Observation: the input block at `0x2893a` increases `DAT_0011d23c`, wraps
  it above 0x19, and writes the letter `index + 0x41` into the name buffer at
  `DAT_001203cc` at the position `DAT_0011d244`.  The other users of
  `DAT_001203cc` are `net_player_add` at `0x2dddc` and `race_panel_4` at
  `0x2e724`, and both treat the buffer as the player name.
  Evidence: the block at `0x288f0` through `0x28976`, the wrap test at
  `0x28945`, and the copies at `0x2de49` and the format at `0x2e74c`.
- Observation: the loop builds 26 records at `DAT_001db78`, stride 0x64.  Each
  record holds the letters `0x41 + i` and `0x21 + i`, the ring position at the
  radius 3500.0, the delay `i * 0.04`, and the angle `i * 0.24166098`.  The
  `map_object_update` module at `0x2b9cc` reads the same record fields.
  Evidence: the loop at `0x2c610` through `0x2c77c` and
  `tools/rebuild/map_object_update.c`.
- Inference: the function is the state 4 player name entry screen.  The car
  selection at state 2 and the track selection at state 3 precede it.  The 26
  A to Z entries, the player name buffer, and the input that writes the chosen
  letter support the name `name_entry_screen`.  The module records the name
  with the confidence `inferred`.
  Evidence: the init table, the update table, the A to Z records, and the
  input block above.
- Observation: the original calls the cosine at `0x4da66`, the sine at
  `0x4da72`, and the truncate helper at `0x4d9f4`.  The harness links no math
  library, so the local FPU stub supplies `cos` and `sin` and makes `__CHP`
  truncate toward zero.  The constants arrive through globals.
  Evidence: `tools/rebuild/name_entry_screen_fpu.asm` and
  `tools/rebuild/name_entry_screen_main.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `4` and `D`.  The paths cover the letters C, Z, A, and B, the failed
  load, and the record bound.
  Evidence: `python3 tools/rebuild/harness.py run --module name_entry_screen`,
  `work/rebuild/nameent.txt`, and
  `python3 tools/rebuild/run.py run --function 0x2c430`.
- Observation: the rebuilt module holds 230 decoded instructions and 870
  bytes.  The identity check stays open at 45 of 230 instructions and 31 of
  870 bytes.  The V2 toolchain uses a different prologue, a different
  instruction order, and a different local layout.  The store records
  `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2c430 --count 230 --rebuilt work/rebuild/nameent.exe
  --rebuilt-map work/rebuild/name_entry_screen.map --rebuilt-symbol
  name_entry_screen_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The race_radar module at 0x23a94

- Observation: the original body at `0x23a94` holds 208 instructions and 870
  bytes up to the `ret` at `0x23df9`.  The store record keeps the 872-byte
  extent and sets no function size.  The identity check uses the first 208
  instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x23a94 --count 208` and
  `tools/rebuild/functions/0x23a94.json`.
- Observation: the direct call sweep finds one caller at `0x2368c`.  The call
  is the last step of the HUD draw at `0x23420`.  The frame update at
  `0x1ff70` calls the HUD draw at `0x2001a` after the five view groups.  No
  named function sits between the frame update and the target, so the record
  takes the section `Program functions`.
  Evidence: the call sweep for `0x23a94`, the call at `0x2001a`, and the
  dispatch at `0x1ff70`.
- Observation: the zoom counter `DAT_0011d2a4` selects the scale.  The value 0
  returns at once.  The values 1, 2, and 3 set the scale factor 0x30, 0x60, and
  0x90.  The input map at `0x2126d` advances the counter from 0 through 3.
  Evidence: the jump table at `0x23a84`, the blocks at `0x23aa9` through
  `0x23ac9`, and the input block at `0x2125a` through `0x2127f`.
- Observation: the function sets the 144x144 panel at (8, 248) in
  `DAT_00115ce4` through `DAT_00115cf0`, clears the panel through `0x25d28`
  with the size 0x90 and 0x90, and renders the world from the player position
  `DAT_0011fdcc` and `DAT_0011fdd4` and the heading `DAT_0011fde8` through
  `0x1057c`.  The panel bounds are x 8 through 0x98 and y 0xf8 through 0x188.
  Evidence: the stores at `0x23aca` through `0x23b25`, the call at `0x23b5b`,
  and the clear call at `0x25d28`.
- Observation: the mode byte `DAT_0011d260` selects the record source.  Bit
  0x10 scans the 0x2a8-byte traffic records at `DAT_00113cd8` while the
  counter stays below `DAT_00115218`, and a record with the first field -1 is
  skipped.  Otherwise bits 0 and 1 scan the 0x590-byte car records at
  `DAT_00110ab8` while the record stays below
  `DAT_00110ab8 + DAT_0010f8ec * 0x590`.  Otherwise bit 4 plots the single
  record at `DAT_00118ee8` and `DAT_00118ef0` when `DAT_0011d264` is set and
  the table entry at `DAT_00118e98` for the index `DAT_00118ed4 ^ 1` is
  non-zero.
  Evidence: the blocks at `0x23b7c` through `0x23da4`, and the mode 0x10
  handler at `0x12d0c` that fills the traffic table.
- Observation: a marker position is the rotated record delta times the zoom
  scale around the panel centre (39, 159), doubled and truncated.  The
  rotation goes through `0x2218c` with the heading `DAT_0011fde8`.  The
  function stores the position in `DAT_0011cb18` and `DAT_0011cb1c` and
  flushes the sprite list through `0x25af0`.  The final marker sits at
  (0x4c, 0x13c) with the mode 4, and the function restores the full-screen
  clip.
  Evidence: the FPU block at `0x23b60` through `0x23c28`, the calls at
  `0x23bdf`, `0x23c28`, and `0x23dcf`, and the stores at `0x23da9` through
  `0x23df0`.
- Inference: the 144x144 panel, the rotation around the player position, the
  three zoom levels, and the traffic, car, and ghost records support the name
  `race_radar`.  The module records the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original converts four marker values and the heading
  through the truncate helper at `0x4d9f4`.  The module uses a local truncating
  FPU stub.  The constants arrive through globals, because the minimal image
  does not load the constant segment.
  Evidence: `tools/rebuild/race_radar_fpu.asm` and
  `tools/rebuild/race_radar_main.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `8` and `D`.  The paths cover the zoom 0 return, the traffic path,
  the car path, the ghost path, the two ghost guards, the empty mode, the
  traffic precedence, and the empty traffic list.
  Evidence: `python3 tools/rebuild/harness.py run --module race_radar`,
  `work/rebuild/rrad_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x23a94`.
- Observation: the rebuilt module holds 208 decoded instructions and 874
  bytes.  The identity check stays open at 52 of 208 instructions and 69 of
  870 bytes.  The V2 toolchain uses a different prologue, an if chain instead
  of the compiler jump table, and a different local layout.  The store records
  `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x23a94 --count 208 --rebuilt work/rebuild/rradmod.exe
  --rebuilt-map work/rebuild/race_radar.map --rebuilt-symbol race_radar_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The car_sound_update module at 0x242bc

- Observation: the original body at `0x242bc` holds 215 instructions and 829
  bytes up to the `ret` at `0x245f8`.  The store record keeps the 836-byte
  extent and sets no function size.  The identity check uses the first 215
  instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x242bc --count 215` and
  `tools/rebuild/functions/0x242bc.json`.
- Observation: the direct call sweep finds one caller at `0x15bf1`.  The call
  is the last step of the effect group at `0x15bac`.  The group runs the player
  car steps on the player object and calls this function only when the byte
  `DAT_00118e1c` bit 0 is set.  The five view groups call the group on the
  player object at object-3 offset `0xf91c`, so the record takes the section
  `The vehicle state chain`.
  Evidence: the call sweep for `0x242bc`, the group at `0x15bac`, and the call
  at `0x15bf1`.
- Observation: the callees at `0x24c4c`, `0x24cc8`, and `0x24d30` are
  sound-effect timers.  They enqueue an entry through `0x247fc` into the queue
  at `0x19080`, and the queue resolves the table entry at `0x1f718`.  The
  helper at `0x22664` returns the angle test.
  Evidence: the enqueue at `0x247fc`, the queue at `0x19080`, and the
  `table_lookup` call at `0x24a61`.
- Observation: the angle test compares `+0x13c` plus the constant with the
  heading `+0x4cc` within pi/4.  The speed `+0x50c`, the engine speed `+0x688`,
  the latch `DAT_00119060`, the mode byte `DAT_0011d260`, the pitch term
  `+0x4f4`, the progress `+0x684`, the base height `+0x680`, and the value
  `+0x790` drive the timers `0x10`, `0x11`, `0x15`, `0x18`, `0x14`, `0x0a`,
  `0x0d`, and `0x0f` and the triggers `0x16`, `0x1a`, and `0x13`.
  Evidence: the blocks at `0x242c9` through `0x245ec`.
- Observation: the mode bits 0 and 1 scan the 0x590-byte car records at
  `DAT_00110ab8` up to `DAT_00110ab8 + DAT_0010f8ec * 0x590`.  A record near
  the player progress `+0x684` is found through the float compare at `+0x408`
  and `+0x40c` or through the progress difference, and the result goes to the
  timer `0x17`.
  Evidence: the loop at `0x244c4` through `0x245a5`.
- Inference: the sound-effect timers, the player car fields, the race-mode
  branches, and the car-record scan support the name `car_sound_update`.  The
  module records the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original converts the pitch term through the truncate
  helper at `0x4d9f4`.  The module uses a local truncating FPU stub.  The
  constants arrive through globals, because the minimal image does not load the
  constant segment.
  Evidence: `tools/rebuild/car_sound_update_fpu.asm` and
  `tools/rebuild/car_sound_update_main.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9`, `a`, `b`, `c`, and `D`.  The paths cover the gate, the near and
  far angle, the mode bits `0x10`, `0x20`, and `0x40`, the latch set and clear,
  the two record-scan hits, the record-scan miss, the timer threshold, and the
  fourteen callee arguments.
  Evidence: `python3 tools/rebuild/harness.py run --module car_sound_update`,
  `work/rebuild/csnd.txt`, and
  `python3 tools/rebuild/run.py run --function 0x242bc`.
- Observation: the rebuilt module holds 215 decoded instructions.  The
  identity check stays open at 19 of 215 instructions and 75 of 785 compared
  bytes.  The V2 toolchain uses a different epilogue, a different local layout,
  and a different call order for the float arguments.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x242bc --count 215 --rebuilt work/rebuild/csndupd.exe
  --rebuilt-map work/rebuild/car_sound_update.map --rebuilt-symbol
  car_sound_update_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The name_entry_letter_update module at 0x2b660

- Observation: the original body at `0x2b660` holds 219 instructions and 803
  bytes up to the first `ret` at `0x2b982`.  The second return block continues
  to the shared epilogue at `0x2b9cb`.  The store record keeps the 876-byte
  extent and sets no function size.  The identity check uses the first 219
  instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2b660 --count 219` and
  `tools/rebuild/functions/0x2b660.json`.
- Observation: the direct call sweep finds one caller at `0x2cec5`.  The call
  is inside the state 4 update at `0x2ce84`, which runs on each of the 0x50
  records at `DAT_001db78`.  The state init at `0x2c430` builds the 26 A to Z
  records, and the input at `0x2893a` selects the letter.  The update
  dispatcher `race_state_update` at `0x29564` calls the state 4 update at
  `0x295e7`.  The record takes the section `race_frame callees`.
  Evidence: the call sweep for `0x2b660`, the loop at `0x2ceac` through
  `0x2cec5`, and the calls at `0x295e7` and `0x2807c`.
- Observation: the function binds the list name at `0x6350` through `0x11850`
  when the record field `+0x08` is -1.  Otherwise it fells the timer `+0x60`
  by the frame delta `DAT_0018e08` and returns while the timer stays at or
  above zero.  After the timer reaches zero it clears the timer, refreshes the
  list through `0x117fc`, looks the object up in the slot table
  `DAT_001d2d0` and the 0x1c-byte entry table `DAT_0010653c`, and sets the
  object frame `+0x20` from `DAT_001fab8`.
  Evidence: the blocks at `0x2b670` through `0x2b714`.
- Observation: the object position `+0x08` and `+0x10` take the record fields
  `+0x40` and `+0x48`.  The height `+0x0c` interpolates from `+0x44` toward
  `+0x38` with the square of the progress `+0x58` through the Watcom pow
  helper at `0xc21e2`.  The record angles `+0x10`, `+0x14`, and `+0x18` scale
  by 1024.0 / pi into the object angles `+0x14`, `+0x18`, and `+0x1c` with
  the mask `0x7ff`.
  Evidence: the blocks at `0x2b717` through `0x2b850` and the call at
  `0x2b75b`.
- Observation: the progress `+0x58` oscillates between zero and one with the
  direction `+0x5c`.  Above one it clamps at one, clears the direction, scales
  the start `+0x44`, and turns to -1 when the scaled start falls below the
  target `+0x38`.  At zero it clamps at zero and turns to +1.  The angle
  `+0x10` rises when the flag `+0x0c` is set and resets at `0x400cbe4d` above
  the bound, or falls and resets at `0xbf20d97c` below the bound.
  Evidence: the blocks at `0x2b853` through `0x2b97a`.
- Inference: the only caller is the name entry update, the records hold the A
  to Z letters, and the fields match the letter record.  These facts support
  the name `name_entry_letter_update`.  The module records the name with the
  confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls the pow helper at `0xc21e2` and the truncate
  helper at `0x4d9f4`.  The module calls pow only with the exponent 2.0.  The
  local assembly stub squares the base and states that limit.  The constants
  arrive through globals, because the minimal image does not load the constant
  segment.
  Evidence: `tools/rebuild/name_entry_letter_update_fpu.asm` and
  `tools/rebuild/name_entry_letter_update_main.c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9`, `a`, `b`, and `D`.  The paths cover the unused record, the
  timer gate, the full path, the interpolation, both completion clamps, both
  zero-direction clamps, and both angle resets.
  Evidence: `python3 tools/rebuild/harness.py run --module
  name_entry_letter_update`, `work/rebuild/nelt.txt`, and
  `python3 tools/rebuild/run.py run --function 0x2b660`.
- Observation: the rebuilt module holds 219 decoded instructions and 764
  bytes.  The identity check stays open at 42 of 219 instructions and 29 of
  764 compared bytes.  The V2 toolchain uses a different prologue, a different
  local layout, and a different instruction order.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2b660 --count 219 --rebuilt work/rebuild/neltupd.exe
  --rebuilt-map work/rebuild/name_entry_letter_update.map --rebuilt-symbol
  name_entry_letter_update_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The race_records_screen module at 0x2d4b8

- Observation: the original body at `0x2d4b8` holds 224 instructions and 779
  bytes up to the `ret` at `0x2d7c2`.  The store record keeps the 780-byte
  extent and sets no function size.  The identity check uses 224 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2d4b8 --count 224` and
  `tools/rebuild/functions/0x2d4b8.json`.
- Observation: the direct call sweep finds one caller at `0x295fb`.  The call
  is case 6 of the state dispatcher `race_state_update` at `0x29564`.  The
  dispatcher is a `race_frame` callee at `0x2807c`, so the record takes the
  section `race_frame callees`.
  Evidence: the call at `0x295fb`, the jump table at `0x29579`, and the call
  at `0x2807c`.
- Observation: the function clears the screen area twice through `0x25d28`
  with the rectangle (0x60, 0x48, 0x1c4, 0x140).  It draws the selected track
  title from the six-entry pointer table `DAT_0011fbfc`, indexed by the
  selected track `DAT_0012098c`, and it draws the two headers
  `DAT_0011fc28` and `DAT_0011fc2c` at y 0x28 and y 0x78.
  Evidence: the blocks at `0x2d4cc` through `0x2d53c`.
- Observation: the first list holds five race records at
  `DAT_00120610 + index * 0x50`.  Each record holds five floats at +0x00, five
  4-byte names at +0x14, and five flags at +0x28.  A flag that is not zero
  selects the format `\001%d. %3s %02d.%02d.%02d R` at object-3 offset 0x1254.
  A zero flag selects the format `\001%d. %3s %02d.%02d.%02d  ` at offset
  0x126e.  The row that matches `DAT_00120990` uses the color 1, and the other
  rows use the color 0.  The rows start at y 0x3c and step by 0xa.
  Evidence: the blocks at `0x2d555` through `0x2d64e`, the formats at object-3
  offsets 0x1254 and 0x126e, and the flag test at `0x2d64c`.
- Observation: the second list holds five lap records at
  `DAT_00120430 + index * 0x50`.  The rows use the format at object-3 offset
  0x1288 and the color 0.  The rows start at y 0x8c and step by 0xa.
  Evidence: the blocks at `0x2d6d2` through `0x2d7b1` and the format at
  object-3 offset 0x1288.
- Observation: the minutes and the seconds come from the truncated time
  divided by 60.  The hundredths come from the fractional part times the float
  100.0 at object-3 offset 0x12a2.  The original truncates through the Watcom
  helper at `0x4d9f4`, so the module uses a local truncating `__CHP` stub.
  Evidence: the calls at `0x2d55b`, `0x2d570`, and `0x2d5a7`, the float
  100.0 at object-3 offset 0x12a2, and
  `tools/rebuild/race_records_screen_fpu.asm`.
- Inference: the only caller is the race state 6 update.  The two record
  tables at `DAT_00120430` and `DAT_00120610` are the tables that
  `race_record_save` at `0x2e82c` fills.  The function draws the best lap
  times and the best race times of the selected track.  These facts support
  the name `race_records_screen`.  The module records the name with the
  confidence `inferred`.
  Evidence: the call graph, the two tables, and the formats above.
- Observation: the behavior check passes.  The harness prints the markers `0`,
  `1`, `2`, and `D`.  The paths cover the replay suffix, the highlight row,
  the track offset, the two trailing spaces, and a lap time above 59 minutes.
  Evidence: `python3 tools/rebuild/harness.py run --module
  race_records_screen`, `work/rebuild/rcrec.txt`, and
  `python3 tools/rebuild/run.py run --function 0x2d4b8`.
- Observation: the rebuilt module holds 224 decoded instructions and 631
  bytes.  The identity check stays open at 34 of 224 instructions and 44 of
  631 compared bytes.  The V2 toolchain uses a different prologue, a different
  local layout, and a different instruction order.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2d4b8 --count 224 --rebuilt work/rebuild/rcrec.exe
  --rebuilt-map work/rebuild/race_records_screen.map --rebuilt-symbol
  race_records_screen_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.
## The strict byte gate and the whole-image oracle

- The default identity gate accepts three verdicts.  Only one of them, the
  byte-identical verdict, is an exact match.  The verdict with the link-time
  operands is exact after relocation, and the reordered verdict is a shape
  match.  `run.py --strict` accepts the first two verdicts only.
  Evidence: `tools/rebuild/run.py` and
  `python3 -m unittest tools/rebuild/tests/test_run.py`.
- `store.py check` now validates every function record against
  `contracts/record.schema.json` and every module record against
  `contracts/module.schema.json`.  The validator is the standard-library
  module `tools/rebuild/schema.py`.  The check found three legacy results
  without `result.fingerprint`: `0x10978`, `0x2ed77`, and `0x2f8ca`.  The
  records now carry a fingerprint.
  Evidence: `python3 tools/rebuild/store.py check` prints
  `683 records, 0 violations`.
- `tools/le.py` reads the fixup section size, checksum, page-table offset, and
  record-table offset from the LE header.  The accessor `fixup_section()`
  returns the raw fixup section.  The tool does not decode the individual
  fixup records and does not apply fixups.
  Evidence: `tools/le.py` and
  `python3 tools/le.py info ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`.
- `tools/rebuild/image_compare.py` is the whole-image oracle.  It compares the
  stub, the LE header, the resident tables, the fixup section, every object's
  pages, the non-resident name table, and the whole file.  The tool compared
  the original image with itself and reported 1034923 of 1034923 bytes equal.
  The region scores are stub 10832, header 196, loader 990, fixups 96278,
  object 1 901120, and object 3 25259.  The object 2 region is absent because
  the object has no pages.
  Evidence: `python3 tools/rebuild/image_compare.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --rebuilt
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`.
- Pipeline stage 11 runs the whole-image oracle when `--rebuilt-image` exists.
  The stage reports a skip when no whole image is built, so the documented
  check counts do not change.
  Evidence: `tools/pipeline_check.py`.

## The relocation-aware function oracle

- `tools/le.py` decodes the LE fixup page table and the LE fixup record table.
  It returns one `Relocation` per fixup slot with the slot address, the width,
  and the target.  The shipped `STARTH.EXE` holds 11259 slots, and every slot is
  an internal 4-byte reference.
  Evidence: `python3 tools/le.py info
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE` and
  `python3 -m unittest tools/rebuild/tests/test_oracle.py`.
- `tools/rebuild/oracle.py` masks every byte that a fixup slot controls and
  every byte that encodes a direct branch target.  It compares the remaining
  bytes in instruction order.  The instruction order is significant, so a
  reordered sequence does not pass.
- The oracle shows the split between the default gate and the strict gate:

  | Function | Match | Strict gate | Default gate |
  | --- | --- | --- | --- |
  | `memcpy` `0x8c150` | `exact-bytes` | pass | pass |
  | `free` `0x300ec` | `exact-relocations` | pass | pass |
  | `exit` `0x300be` | `exact-relocations` | pass | pass |
  | `list_destroy` `0x2ed77` | `exact-relocations` | pass | pass |
  | `printf` `0x3009b` | `same-shape` | fail | pass |
  | `memory_available` `0x2f8ca` | `same-shape` | fail | pass |
  | `error_report` `0x10978` | `same-shape` | fail | pass |
  | `frame_step` `0x2aa84` | `different` | fail | fail |
  | `engine_shutdown` `0x128ac` | `different` | fail | fail |
  | `bios_video` `0x108e4` | `different` | fail | fail |

  Evidence: `python3 tools/rebuild/diff_module.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE
  --original-address 0x300ec --rebuilt work/rebuild/free.exe --rebuilt-map
  work/rebuild/free.map --rebuilt-symbol free_` and the same command for the
  other addresses; `run.run_identity(..., strict=True)` returns the strict
  verdict.
- A direct relative branch needs no fixup, because the relative distance inside
  one object does not change when the loader relocates the object.  The oracle
  therefore identifies a direct branch target from the instruction encoding, not
  from the fixup table.  An absolute data reference has a fixup slot, because
  the absolute address does change at load time.
  Evidence: the `free` jump at `0x300ee` has no fixup slot, and the two `exit`
  hook-slot references at `0x300c3` and `0x300c9` each have one.
- The earlier `printf` finding records the previous verdict `the modules hold the
  same instructions in a different order`.  The oracle now classifies the same
  bytes as `same-shape`: the mnemonic order matches, and the differing bytes are
  the stack offsets and the register moves, not the link-time addresses.
  Evidence: the `printf` row in the table above.

## The first mismatch in the three open modules

The relocation-aware oracle compares the three open modules with the V2 build.
The first mismatch is a code byte, a mask, or an operand, in this order of
address:

| Function | First mismatch | Original | Rebuilt | Cause |
| --- | --- | --- | --- | --- |
| `frame_step` `0x2aa84` | instruction 5 | `0x2aa96 5d` `pop ebp` | `0x10012 89 ec` `mov esp,ebp` | the V2 epilogue adds `mov esp,ebp` |
| `engine_shutdown` `0x128ac` | instruction 6 | `0x128c0 b8 b0 62 00 00` `mov eax,0x62b0` with a fixup at `0x128c1` | `0x10014 b8 b0 62 00 00` `mov eax,0x62b0` without a fixup | the source passes the literal `0x62b0`; the original operand is a relocatable object-3 address |
| `bios_video` `0x108e4` | instruction 0 | `0x108e4 53` `push ebx` | `0x10000 55` `push ebp` | the recipe `-of+` puts the frame before the register saves |

- `list_destroy`: the V2 build first differed at the local slot order.  The
  hand-written assembly now matches (see "The hand-written list_destroy assembly
  matches the original").
- `frame_step`: the prologue and the three calls match.  The only difference is
  the extra `mov esp,ebp` before `pop ebp`.  The earlier compiler sweep shows
  the same epilogue in 8.5a through 11.0b, and no option removes it.
- `engine_shutdown`: the instruction bytes at the first mismatch are equal.  The
  original has a 4-byte internal fixup to object 3, offset `0x62b0`, so the
  operand is a link-time address in object 3.  The offset `0x62b0` is past the
  `0x62ab` bytes of initialized object-3 data, so it points into the zero-filled
  part.  The rebuilt source passes the literal `0x62b0`, so the operand is a
  constant.  The first code-shape mismatch is instruction 12: the original
  `pop ebp` against the rebuilt `mov esp,ebp`.
- `bios_video`: the original saves the used registers first (`ebx`, `ecx`,
  `edx`, `ebp`), then sets the frame.  The `-of+` recipe sets the frame first
  (`push ebp`, `mov ebp,esp`, then `push ebx`, `push ecx`, `push edx`).  A
  temporary `-od` recipe moved the frame but pushed `esi` and `edi` too, and it
  held 26 instructions and 60 bytes, so `-od` alone is not the fix.  The source
  also stores `regs.w.ax` as a 16-bit value, while the original stores the full
  32-bit mode to the register set.

Evidence: `python3 tools/rebuild/diff_module.py --original
~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address
<address> --rebuilt work/rebuild/<module>.exe --rebuilt-map
work/rebuild/<module>.map --rebuilt-symbol <symbol>` for the four addresses,
and `python3 tools/rebuild/harness.py build --module bios_video_od` for the
temporary `-od` recipe.

## The list_destroy local slot order under the period 10.0a compiler

- The period 10.0a `-od` build reaches the original instruction count and byte
  count: 25 of 25 instructions and 54 of 54 bytes.  The first difference is the
  local slot order.  The original stores the first parameter (`list`) at
  `[ebp-8]`, the second parameter (`unused`) at `[ebp-4]`, and the return
  temporary at `[ebp-0xc]`.  The 10.0a build stores them at `[ebp-0xc]`,
  `[ebp-8]`, and `[ebp-4]`.
  Evidence: a temporary module record with the current `list_destroy.c` and
  `period_flags: ["-od"]`, `python3 tools/rebuild/harness_dos.py build --module
  list_destroy10`, and `python3 tools/rebuild/diff_module.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE
  --original-address 0x2ed77 --rebuilt work/rebuild/ldmod10.exe --rebuilt-map
  work/rebuild/list_destroy10.map --rebuilt-symbol list_destroy_`.
- The source-form search did not change the order.  These forms kept the
  instruction count and the slot order: the current form, a `goto` to the end, an
  `if`/`else` with an empty else, and a `(void)unused` statement.  These forms
  added instructions and still kept the return temporary in the highest slot: an
  explicit local declared at the top, a nested block, and a declaration after the
  `if`.  A `return (void *)list` cast changed nothing.
  Evidence: the temporary `list_destroy_exp.c` with each form and the same
  build and diff commands.
- The cause is the compiler's local allocator.  The 10.0a compiler allocates the
  compiler-generated return temporary in the highest local slot, then places the
  two parameter spills below it.  The original compiler places the return
  temporary in the lowest slot, then the two parameter spills above it.  The
  source form does not control this order.
- The current source form stays the closest one.  The initial 10.0 compiler
  was tested next; it gives the same slot order (see "The initial 10.0 compiler
  and the list_destroy slot order").  The remaining candidates are a pre-10.0
  compiler or a hand-written assembly function that assigns the three slots
  directly.
- The period harness needs a clean artifact set.  An old `ldmod10.exe` and
  `list_destroy10.map` make `wait_for_artifacts` return before the new link, and
  a DOSBox-X process can stay alive after the link, so the harness can hang.
  Remove the old artifacts before a rebuild, and confirm the process exits.

## The initial 10.0 compiler and the list_destroy slot order

- The initial Watcom 10.0 Limited Availability compiler was extracted from the
  Internet Archive item `watcom-c-cpp-compilers-collection`, file
  `watcom-10.0-la/watcom10la.iso` (SHA-256
  `91d3a2698c9f907994cecfae7e571edb904d31f38651aaf996c296c0e2badd5a`).  The
  `PACK0022` archive holds `WCC386.EXE` (517764 bytes).  `WPACK` unpacks it.  The
  compiler banner is `WATCOM C32 Optimizing Compiler  Version 10.0 Limited
  Availability`.
  Evidence: `WPACK -l PACK0022` and `C:\WCC386.EXE` under DOSBox-X with the
  10.0a `W32RUN.EXE` in the PATH.
- The initial 10.0 `-od` build of `list_destroy.c` gives the same result as the
  10.0a build: 25 of 25 instructions, 54 bytes, and 44 of 54 bytes equal.  Its
  local slot order is `list` at `[ebp-0xc]`, `unused` at `[ebp-8]`, and the
  return temporary at `[ebp-4]`.  It does not match the original order, which is
  `list` at `[ebp-8]`, `unused` at `[ebp-4]`, and the return temporary at
  `[ebp-0xc]`.
  Evidence: `C:\WCC386.EXE -bt=dos -s -zq -od -fo=LD.OBJ LD.C`, the 10.0a
  `WLINK` with `system dos4g`, and `python3 tools/rebuild/diff_module.py
  --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE
  --original-address 0x2ed77 --rebuilt work/watcom10la/build/EXP.EXE
  --rebuilt-map work/watcom10la/build/EXP.MAP --rebuilt-symbol list_destroy_`.
- The initial 10.0 compiler is therefore not the cause of the local slot order.
  The remaining candidates are a pre-10.0 compiler or a hand-written assembly
  function that assigns the three slots directly.

## The Watcom 9.5 compiler and the list_destroy slot order

- The Watcom C/C++ 9.5 compiler was extracted from the Internet Archive item
  `watcom-c-cpp-compilers-collection`, file
  `watcom-9.5/discmaster/Sybase - Watcom C++ 9.5b.zip`.  The `32/06/WCC386.DOS`
  file is a WPACK archive.  It holds `wcc386.exe` (627702 bytes).  The compiler
  banner is `WATCOM C32 Optimizing Compiler  Version 9.5`.
  Evidence: `WPACK -l 32/06/WCC386.DOS` and `WPACK 32/06/WCC386.DOS wcc386.exe`.
- The 9.5 `-od` build of `list_destroy.c` gives 25 of 25 instructions, 54 bytes,
  and 44 of 54 bytes equal.  Its local slot order is `list` at `[ebp-0xc]`,
  `unused` at `[ebp-8]`, and the return temporary at `[ebp-4]`.  This is the same
  order as the 10.0 and 10.0a builds, and it does not match the original order:
  `list` at `[ebp-8]`, `unused` at `[ebp-4]`, and the return temporary at
  `[ebp-0xc]`.
  Evidence: `WCC386 -bt=dos -s -zq -od -fo=LD.OBJ LD.C`, the 10.0a `WLINK`
  with `system dos4g`, and `python3 tools/rebuild/diff_module.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE
  --original-address 0x2ed77 --rebuilt work/watcom95b/build95/EXP.EXE
  --rebuilt-map work/watcom95b/build95/EXP.MAP --rebuilt-symbol list_destroy_`.
- The 9.5 compiler is therefore not the cause of the local slot order.  The
  remaining candidates are an earlier compiler (8.5a or 9.01d) or a hand-written
  assembly function.  The earlier finding records that the 8.5a `-od` build is
  frameless, so it cannot reproduce the original `-od` prologue; the 9.01d slot
  order stays untested.

## The Watcom 9.01d compiler and the list_destroy slot order

- The Watcom C/C++ 9.01d compiler was extracted from the Internet Archive item
  `watcom-c-cpp-compilers-collection`, file
  `watcom-9.01d/Watcom - C++ 9.01d.zip`.  The `01/WCC386.DOS` file is a WPACK
  archive.  It holds `wcc386.exe` (200632 bytes).  The compiler needs the code
  generator `386WCGL.EXE` from `01/386WCGL.WPK` and the preprocessor
  `WCC386P.EXE` from `01/WCC386P.WPK`.  The banner is `WATCOM C 386 Optimizing
  Compiler  Version 9.01`.
  Evidence: `WPACK -l 01/WCC386.DOS`, `WPACK 01/WCC386.DOS wcc386.exe`,
  `WPACK 01/386WCGL.WPK 386wcgl.exe`, and `WPACK 01/WCC386P.WPK wcc386p.exe`.
- The 9.01d `-od` build of `list_destroy.c` gives 25 of 25 instructions, 54
  bytes, and 44 of 54 bytes equal.  Its local slot order is `list` at
  `[ebp-0xc]`, `unused` at `[ebp-8]`, and the return temporary at `[ebp-4]`.
  This is the same order as the 9.5, 10.0, and 10.0a builds, and it does not
  match the original order: `list` at `[ebp-8]`, `unused` at `[ebp-4]`, and the
  return temporary at `[ebp-0xc]`.
  Evidence: `WCC386 -bt=dos -s -zq -od -fo=LD.OBJ LD.C`, the 10.0a `WLINK`
  with `system dos4g`, and `python3 tools/rebuild/diff_module.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE
  --original-address 0x2ed77 --rebuilt work/watcom901d/build/EXP.EXE
  --rebuilt-map work/watcom901d/build/EXP.MAP --rebuilt-symbol list_destroy_`.
- The 9.01d compiler is therefore not the cause of the local slot order.  The
  remaining compiler candidate is 8.5a, but the earlier finding records that its
  `-od` build is frameless, so it cannot reproduce the original `-od` prologue.
  A hand-written assembly function is the remaining option; the next section
  records it.

## The hand-written list_destroy assembly matches the original

- The `list_destroy` module now uses the hand-written
  `tools/rebuild/list_destroy.asm` source with the `wasm` assembler.  The C
  source `list_destroy.c` stays alongside as the C reference version.  The
  module recipe `tools/rebuild/modules/list_destroy.json` lists the assembly
  source and the unchanged `list_destroy_main.c` harness.
  Evidence: `tools/rebuild/modules/list_destroy.json`.
- The original disassembly is the reference.  The command
  `python3 tools/le.py disasm
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE 0x2ed77 --count
  25` prints 25 instructions and 54 bytes, from `push ebx` to `ret`.  The
  assembly follows that sequence instruction by instruction.
  Evidence: `work/rebuild/list_destroy_original.disasm`.
- The assembler chooses the load encoding for `mov ebp,esp` and `mov esp,ebp`,
  and the short form for `sub esp,0ch`.  The original uses the store encoding
  (`89 e5`, `89 ec`) and the long immediate form (`81 ec 0c 00 00 00`).  The
  source pins those three encodings with `db`.
  Evidence: the first assembly build held `8b ec`, `83 ec 0c`, and `8b e5`;
  the pinned source holds the original bytes.
- The rebuilt body holds 25 of 25 instructions and 54 bytes.  51 of 54 bytes
  are equal.  The three differing bytes are the `call free_` displacement at
  offsets 33 through 35.  The opcode byte at offset 32 is equal, and the fourth
  displacement byte at offset 36 is zero in both bodies.  The `je` branch keeps
  the same `74 0a` bytes, because the relative distance does not change.  The
  only differing instruction operands are the two direct branch targets, which
  are link-time addresses.
  Evidence: `python3 tools/rebuild/diff_module.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE
  --original-address 0x2ed77 --rebuilt work/rebuild/ldmod.exe --rebuilt-map
  work/rebuild/list_destroy.map --rebuilt-symbol list_destroy_`.
- The oracle verdict is `exact-relocations`: "the instruction sequences match;
  the differing operands are the link-time addresses".  The strict gate passes.
  The behavior check prints the four markers `F`, `R`, `Z`, and `N`.  The
  record `0x2ed77` is `verified` with 25 of 25 instructions, 51 of 54 bytes, and
  a behavior pass.
  Evidence: `python3 tools/rebuild/run.py run --function 0x2ed77` and
  `work/rebuild/ld_out.txt`.
- The local slot order is the original order: `list` at `[ebp-8]`, `unused` at
  `[ebp-4]`, and the return temporary at `[ebp-0xc]`.  The C compilers (9.01d,
  9.5, 10.0, and 10.0a) give a different order, so the assembly removes the
  local slot limit for this function.
  Evidence: the rebuilt disassembly in
  `work/rebuild/list_destroy_rebuilt.disasm` and the C compiler sections above.
- The record marks the source language with `implementation: assembly`.  The
  status page shows `asm` in the table language column, on the function chip,
  and in the detail pane.
  Evidence: `python3 tools/rebuild/store.py set --function 0x2ed77
  --implementation assembly` and `python3 tools/rebuild/view.py html`.

## The player_car_reset_update module at 0x18528

- Observation: the original body at `0x18528` holds 168 instructions and 776
  bytes up to the first `ret` at `0x1882f`.  The state 5 block at `0x18830` and
  the state 6 block at `0x18896` continue to the shared `ret` at `0x188b7`, so
  the record keeps the 912-byte extent and sets no function size.  The identity
  check uses 168 instructions.
  Evidence: `python3 tools/le.py disasm
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE 0x18528 --count
  168` and `tools/rebuild/functions/0x18528.json`.
- Observation: the only direct caller is the effect group at `0x15bac`, which
  calls the function at `0x15bd0`.  The player physics group at `0x15a80` sets
  the state at `+0x148` to 2 at `0x16da4` when the heading error times the
  speed passes the threshold at `+0x180`.  The five view groups at `0x2003c`,
  `0x201d0`, `0x202c0`, `0x20360`, and `0x203c4` call the effect group on the
  player object at object-3 offset `0xf91c`.
  Evidence: the direct call sweep for `0x18528`; the blocks at `0x15a80` through
  `0x15ab4`, `0x15bac` through `0x15bf9`, and `0x16d80` through `0x16dae`.
- Observation: the state 2 init block looks the sound entry 8 up through
  `table_lookup` at `0x1f718`, enqueues the effect 9 with the value 3 and 0.5
  through `0x247fc`, sets the state to 1, copies the slot index
  `DAT_00110aa4` to `+0x160`, and sets `DAT_00001e18` when the replay flag
  `DAT_00118e6c` is zero.  It scales the speed `+0x50c` by 0.6, publishes the
  position `+0x4b0` and `+0x4b8` through `DAT_00108038` and `DAT_00108040`,
  calls `0x120c0` with 0, and copies the direction `DAT_00108048` to the
  heading `+0x4cc`.  It sets the timer `+0x14c` to 3000.0, clears the counter
  `+0x150`, sets the velocities `+0x154` and `+0x15c` to 12.0, clears the
  offsets `+0x124`, `+0x128`, and `+0x12c`, and clears `+0x134` and `+0x170`.
  Evidence: the blocks at `0x18550` through `0x1866f`.
- Observation: the state 1 update block adds the scaled timer to `+0x134`,
  subtracts 6000.0 times the frame delta `DAT_00118e04` from `+0x14c`, damps
  `+0x154` and `+0x15c` with the coefficient at `DAT_0010cf08 + 0x184`, and
  integrates them into `+0x124` and `+0x12c`.  A non-positive `+0x134` starts
  the four effect slots through `0x19828`, increments the counter, reflects the
  timer by 0.4, clears `+0x124` and `+0x12c`, selects -0.6 on an odd counter or
  0.6 on an even counter at `+0x15c`, and sets `+0x154` to -1.0.  When the
  counter passes 3, the state clears and, with `DAT_00118e1c` bit 0 set and the
  replay flag clear, the slot record at `DAT_00101db4 + 5 * +0x160` sets.
  Evidence: the blocks at `0x18670` through `0x1882f`.
- Observation: the state 5 block runs only with a non-zero gear `+0x7ac`.  It
  allocates `+0x7d0` through `table_lookup` when the handle is -1, toggles the
  low five bits of `+0x4bc`, advances the heading `+0x4cc` by 0.3 times
  `+0x108`, and starts the four effect slots.  The state 6 block clears
  `+0x4bc` and the state.
  Evidence: the blocks at `0x18830` through `0x188b7`.
- Observation: the jump table at `0x18510` maps the states 1 to 6 to `0x18670`,
  `0x18550`, `0x188b0`, `0x188b0`, `0x18830`, and `0x18896`.  The constants at
  object-3 offsets 0x820, 0x828, 0x82c, and 0x834 hold the double 0.6, the
  float 6000.0, the double 0.4, and the double 0.3.
  Evidence: the table bytes at `0x18510` through `0x18527` and the constants at
  object-3 offsets 0x820 through 0x837.
- Inference: the heading and speed test, the direction snap, the speed scale
  0.6, and the three-step timer support the name `player_car_reset_update`.
  The module records the name with the confidence `inferred`.
  Evidence: the trigger at `0x16da4`, the init block at `0x18550`, and the
  update block at `0x18670`.
- Observation: the harness holds one player object, the frame delta, the filter
  table, the slot index, the two flags, the position globals, the direction
  result, the slot record array, and the constants.  It covers the state 2
  init, the positive and negative accumulator, the odd and even counter, the
  slot mark, the flag gates, the state 5 gear gate, the state 5 handle and
  heading, the state 6 clear, and the default state.  The behavior check prints
  every marker.
  Evidence: `python3 tools/rebuild/harness.py run --module
  player_car_reset_update` and `work/rebuild/pcr_out.txt`.
- Observation: the identity check stays open at 31 of 168 instructions and 86
  of 776 bytes, because V2 uses the EBP frame after the register saves and
  keeps the object in EBX.  The build and behavior checks pass, so the record
  status is `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE
  --original-address 0x18528 --count 168 --rebuilt work/rebuild/pcrsmod.exe
  --rebuilt-map work/rebuild/player_car_reset_update.map --rebuilt-symbol
  player_car_reset_update_`.

## The player_route_camera module at 0x1a630

- Observation: the original body at `0x1a630` holds 243 instructions and 768
  bytes up to the `ret` at `0x1a92f`.  The record keeps the 768-byte extent,
  which ends at the next function `car_camera` at `0x1a930`, and sets no
  function size.  The identity check uses 243 instructions.
  Evidence: `python3 tools/le.py disasm
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE 0x1a630 --count
  243` and `tools/rebuild/functions/0x1a630.json`.
- Observation: the only direct caller at `0x19ec4` is the view mode dispatch at
  `0x19e78`.  The dispatch selects this function for the render mode 3 and
  passes the player object at object-3 offset `0xf91c` in `eax` and the player
  view object at `DAT_00110a20` in `edx`.  The player update at `0x19ab0` and
  `player_view_setup` at `0x19ae8` both call the dispatch.
  Evidence: the direct call sweep for `0x1a630`, the dispatch table at
  `0x19e50`, and the calls at `0x19ac2` and `0x19c80`.
- Observation: the function refreshes the two list slots at `+0x460` and
  `+0x468` through the helper at `0x117fc`.  It then walks the 0x38-byte route
  records below the pointer `DAT_0010f910`, the RPL route.  The count is at
  `+0x1c`, and each record holds the position at `+0x04` (x), `+0x08` (z), and
  `+0x0c` (y).  The loop picks the nearest record by the horizontal distance
  from the car position at `+0x4b0` and `+0x4b8`, and a zero distance does not
  take part.
  Evidence: the calls at `0x1a653` and `0x1a66b`, and the loop at `0x1a67c`
  through `0x1a71c`.
- Observation: the function forms the yaw at `+0x34` and the pitch at `+0x30`
  of the view from the chosen record.  It copies the record position into the
  view at `+0x00`, `+0x04`, and `+0x08` and the two saved copies at `+0x0c` and
  `+0x14`.  When the total distance exceeds 1500 units, it moves the view point
  along the yaw and the pitch by the excess.  The constants are 2pi at
  object-3 offset `0x91c`, -pi/2 at `0x920`, and -1500.0 at `0x924`.
  Evidence: the blocks at `0x1a71c` through `0x1a92f` and the data at
  object-3 offsets `0x91c`, `0x920`, and `0x924`.
- Observation: the route table is the `MAP1RPL.SPL` member.  The loader at
  `0x14fb8` builds the name with the suffix `%srpl` at object-3 offset `0x900`
  and stores the pointer at `DAT_0010f910`.  The loader also stores the
  `%scam` route at `DAT_0010f90c`, and the sibling `car_camera` at `0x1a930`
  walks that CAM route in the same way.
  Evidence: the format strings at object-3 offsets `0x60f` and `0x615`, the
  loader calls at `0x150b6` and `0x150cd`, and the `MAP1I.DAT` member list.
- Inference: the view mode dispatch, the `%srpl` route walk, and the view block
  write support the name `player_route_camera`.  The module records the name
  with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the harness holds the route table, the node array, the car
  object, the view object, the three constants, and the list refresh stub.  It
  covers the move, the reflected yaw, the distance below the limit, the exact
  limit, the zero-distance skip, the nearest of two nodes, and the car offset.
  The behavior check passes.
  Evidence: `python3 tools/rebuild/harness.py run --module
  player_route_camera` and `work/rebuild/prc_out.txt`.
- Observation: the identity check stays open at 46 of 243 instructions and 27
  of 768 bytes, because V2 saves the frame pointer first and uses another
  register layout and another FPU stub form.  The build and behavior checks
  pass, so the record status is `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE
  --original-address 0x1a630 --count 243 --rebuilt work/rebuild/prcam.exe
  --rebuilt-map work/rebuild/player_route_camera.map --rebuilt-symbol
  player_route_camera_`.

## The player_track_node_update module at 0x1821c

- Observation: the original body at `0x1821c` holds 220 instructions and 756
  bytes up to the `ret` at `0x1850f`.  The record keeps the 780-byte extent,
  which ends at the next function `player_car_reset_update` at `0x18528`, and
  sets no function size.  The identity check uses 220 instructions.
  Evidence: `python3 tools/le.py disasm
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE 0x1821c --count
  220` and `tools/rebuild/functions/0x1821c.json`.
- Observation: the only direct caller at `0x15ac4` is the second player step
  group at `0x15ab8`.  The group runs on the player object at object-3 offset
  `0xf91c` after the physics group at `0x15a80`.  The five view groups at
  `0x2003c`, `0x201d0`, `0x202c0`, `0x20360`, and `0x203c4` call both groups.
  Evidence: the sweep of every candidate function start for a call to
  `0x1821c` and the calls at `0x200a5` and `0x2011b`.
- Observation: the function reads the 0x20-byte track node table at the global
  `0x7250` and the count at `0x7ed0`.  The loader at `0x150f4` calls `0x1b198`
  with the destination `0x7250`, so the table holds the track nodes.  A node
  holds the radius at `+0x00`, the value at `+0x04`, the surface grip at
  `+0x0c`, the flags at `+0x10`, the x position at `+0x14`, and the y position
  at `+0x1c`.
  Evidence: the node reads at `0x18285` through `0x183a4` and the loader at
  `0x150f4`.
- Observation: the function examines the node before the current index at
  `+0x598`, the current node, and the next node.  It accepts a node when each
  absolute axis delta stays below the node radius and the truncated distance
  does not exceed the radius.  It then sets the current index and the target
  index at `+0x668`, saves the old target value and flags to `+0x58c` and
  `+0x590`, and copies the node value, the surface grip, and the flags to
  `+0x66c`, `+0x670`, and `+0x678`.
  Evidence: the blocks at `0x18256` through `0x183a4`.
- Observation: the visited chain at `+0x59c` and the second chain at `+0x5a0`
  select the status `+0x780`.  A hit on a visited node after the first, or on a
  node in the second chain, sets 2, and any other hit sets 1.  A call with no
  hit sets the target index to -1.  The next callee at `0x194a0` consumes the
  status, the chains, and the ratio to advance the lap `+0x684` and the
  progress `+0x680`.
  Evidence: the blocks at `0x183ad` through `0x18413` and the block at
  `0x194b0` through `0x195f0`.
- Observation: the function forms the segment from the current node to the
  next node and the distance from the object position at `+0x4b0` and `+0x4b8`
  to the current node.  Both values scale by the double 0.05 at object-3 offset
  `0x818` and truncate through the helper at `0x4d9f4`.  The function stores the
  ratio at `+0x67c`.
  Evidence: the blocks at `0x18413` through `0x18501` and the data at
  object-3 offset `0x818`.
- Inference: the track table, the node fields, the visited chains, and the lap
  consumer support the name `player_track_node_update`.  The module records the
  name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the harness holds the object record, four track nodes, the node
  count, and the scale 0.05.  It covers the current hit, the two status-2
  gates, the previous and next scan passes, the no-hit target -1, the
  next-index wrap, the ratio 1, and the truncated ratio 0.4.  The behavior check
  passes.
  Evidence: `python3 tools/rebuild/harness.py run --module
  player_track_node_update` and `work/rebuild/ptnu_out.txt`.
- Observation: the identity check stays open at 47 of 220 instructions and 52
  of 756 bytes, because V2 saves the frame pointer first and uses another
  register layout and another local slot order.  The build and behavior checks
  pass, so the record status is `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE
  --original-address 0x1821c --count 220 --rebuilt work/rebuild/ptnu.exe
  --rebuilt-map work/rebuild/player_track_node_update.map --rebuilt-symbol
  player_track_node_update_`.

## The player_car_suspension_update module at 0x168b4

- Observation: the original body at `0x168b4` holds 219 instructions and 740
  bytes up to the `ret` at `0x16b97`.  The next function starts at `0x16b98`,
  so the record keeps the 740-byte extent and sets no function size.  The
  identity check uses 219 instructions.
  Evidence: `python3 tools/le.py disasm
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE 0x168b4 --count
  219` and `tools/rebuild/functions/0x168b4.json`.
- Observation: the only direct caller at `0x15bb8` is the effect group at
  `0x15bac`.  The group runs on the player object at object-3 offset `0xf91c`
  after the player physics group at `0x15a80`.  The five view groups at
  `0x2003c`, `0x201d0`, `0x202c0`, `0x20360`, and `0x203c4` call the group.
  Evidence: the sweep of every candidate function start for a call to
  `0x168b4` and the call at `0x200e0`.
- Observation: the function walks the four 0x54-byte suspension units at
  `+0x194`.  A unit holds the phase at `+0x30`, the travel at `+0x34`, the
  lateral force at `+0x38`, and the longitudinal force at `+0x40`.  The
  distances `+0x2e4` and `+0x2ec` separate the wheels.
  Evidence: the loop at `0x16a96` through `0x16a8d` and the reads at
  `0x16948`, `0x1697f`, and `0x169c4`.
- Observation: the constants live in object 3.  The double at offset `0x744`
  is 0.00175, the double at `0x74c` is 1.5e-09, the float at `0x754` is 3.0,
  the float at `0x758` is 4 pi, the float at `0x75c` is pi, and the double at
  `0x760` is 2.0.  The pointer at `0xcf08` selects the filter table.
  Evidence: the fixups at `0x168d6`, `0x168ea`, `0x16acb`, `0x169cc`,
  `0x169d2`, `0x16a37`, and `0x16a1a`, and the values at `0x100744`,
  `0x10074c`, `0x100754`, `0x100758`, `0x10075c`, and `0x100760`.
- Observation: the function forms the speed change from `+0x50c` minus
  `+0x588` and the lateral load from the square of `+0x50c`, the constant at
  `0x74c`, and `+0x2f4` divided by the frame delta.  The odd units scale the
  speed change by 3.0, and the bit-2 units take the lateral load.
  Evidence: the blocks at `0x168c2` through `0x16934`.
- Observation: a new magnitude resets the phase and stores the lateral force,
  or filters the longitudinal force through `filter_update` at `0x22b80` with
  the index `0x3d`.  The function forms the travel from `cos` at `0x4da66` of
  the phase times 4 pi plus pi.  It clamps the travel to -18 through 18, damps
  both forces through the filter table, and clears a force below 2.0.
  Evidence: the blocks at `0x1693a` through `0x16a8a` and the `fcomp` against
  the double at `0x760`.
- Observation: the function writes the body height `+0x4c0` as the average of
  the four travel values.  It writes the body angles `+0x4e8` and `+0x4e0`
  from the travel differences and the distances `+0x2e4` and `+0x2ec` through
  `atan` at `0xc2254`.  The sibling function at `0x166a8` reads the same units
  and distances, and `player_car_link` at `0x15c1c` applies `+0x4e0` and
  `+0x4e8` to the render objects.
  Evidence: the blocks at `0x16ad4` through `0x16b97`, the function at
  `0x166a8`, and the reads at `0x15d94` and `0x15e11`.
- Inference: the four suspension units, the body height, and the two body
  angles support the name `player_car_suspension_update`.  The module records
  the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the harness holds the player object, four suspension units, the
  filter table, the frame delta, and the constants.  It covers the opposite
  left and right forces, both travel clamps, the no-reset path, the dead zone,
  the filtered longitudinal force, the negative average -7, and the two body
  angles.  The behavior check passes.
  Evidence: `python3 tools/rebuild/harness.py run --module
  player_car_suspension_update` and `work/rebuild/pcsu_out.txt`.
- Observation: the identity check stays open at 23 of 219 instructions and 32
  of 719 bytes, because V2 saves the frame pointer first and uses another
  register layout and another local slot order.  The build and behavior checks
  pass, so the record status is `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original
  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE
  --original-address 0x168b4 --count 219 --rebuilt work/rebuild/pcsu.exe
  --rebuilt-map work/rebuild/player_car_suspension_update.map --rebuilt-symbol
  player_car_suspension_update_`.

## The car_collision_response module at 0x17828

- Observation: the original body at `0x17828` holds 183 instructions and 708
  bytes up to the `ret` at `0x17aeb`.  The branch at `0x178ee` jumps over that
  `ret` to the second block at `0x17aec`, which ends at the `ret` at `0x17b52`.
  The next function starts at `0x17b54`, so the store record keeps the 812-byte
  extent and sets no function size.  The identity check uses 183 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x17828 --count 183`,
  `python3 tools/le.py disasm STARTH.EXE 0x17aec --count 40`, and
  `tools/rebuild/functions/0x17828.json`.
- Observation: the direct call sweep finds one caller at `0x17523` in the car
  proximity pass at `0x17408`.  The player effect group at `0x15b08` calls that
  pass at `0x15b30`.  The pass walks the 0x590-byte car records at
  `DAT_00110ab8` and calls this function when the distance falls below
  `self + 0x544 + record + 0x44c`, the record physics gate at `+0x414` is set,
  and the record radius at `+0x2f8` is zero.
  Evidence: the byte sweep for a relative call to `0x17828`, the loop at
  `0x17408` through `0x1755a`, and the call at `0x17523`.
- Observation: the caller passes the player object in `eax` and the nearby car
  record in `edx`.  It stores the two axis deltas at `self + 0x564` and
  `self + 0x56c` and the truncated distance at `self + 0x55c`.  This function
  forms the unit vector from those fields and projects the overlap through
  `record + 0x498` and `record + 0x4a0` into `DAT_00108038` and
  `DAT_00108040`.  The map collision check at `0x11c00` reads the two globals.
  Evidence: the blocks at `0x1745d` through `0x17523` and `0x17841` through
  `0x178e7`.
- Observation: when the map cell is clear, the function moves the record.  It
  scales the record rotation `+0x418` by the double 0.7 at object-3 offset
  `0x7ac`, or it sets the speed factor `+0x450` from the double 0.5 at `0x7b4`,
  the turn rate `+0x54c` from the double 0.21 at `0x7c8`, the yaw response
  `+0x4c8`, and the rate `+0x548` from the double 0.0002.  It reads the record
  heading `+0x4a8` and moves the record positions `+0x454` and `+0x45c`.
  When the map cell is blocked, it moves the player positions `+0x4b0` and
  `+0x4b8` instead.
  Evidence: the blocks at `0x178f4` through `0x17b52`.
- Inference: the car record scan, the overlap projection, and the two corrective
  moves support the name `car_collision_response`.  The module records the name
  with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original truncates through the helper at `0x4d9f4`, so the
  module uses the local `car_collision_response_fpu.asm` truncating `__CHP`
  stub.  The constants arrive through globals, because the minimal image does
  not load the constant segment.  The original has the EBP frame, so the module
  builds with `-of+`.
  Evidence: `tools/rebuild/car_collision_response.c`,
  `tools/rebuild/car_collision_response_fpu.asm`, and the calls at `0x178aa`,
  `0x178dc`, `0x17aaa`, `0x17ada`, `0x17b11`, and `0x17b41`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `7` and `D`.  The paths cover the clear cell with positive and
  negative deltas, the zero distance count, the first flag bit 1, the three
  second-flag combinations, and the pi/2 angle clamp.
  Evidence: `python3 tools/rebuild/harness.py run --module
  car_collision_response` and `work/rebuild/ccr_out.txt`.
- Observation: the rebuilt module holds 183 decoded instructions and 700 bytes.
  The identity check stays open at 18 of 183 instructions and 36 of 700 bytes,
  because V2 uses another prologue, another instruction order, and another local
  layout.  The build and behavior checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x17828 --count 183 --rebuilt work/rebuild/ccr.exe
  --rebuilt-map work/rebuild/car_collision_response.map --rebuilt-symbol
  car_collision_response_`.
- Limit: the original has no symbol name for this function.  The module name
  and the three callee names in the harness stay inferred.

## The track_nearest_node module at 0x120c0

- Observation: the original body at `0x120c0` holds 201 instructions and 695
  bytes up to the `ret` at `0x12376`.  The next function starts at `0x12378`,
  so the store record keeps the 696-byte extent and sets no function size.  The
  identity check uses 201 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x120c0 --count 201` and
  `tools/rebuild/functions/0x120c0.json`.
- Observation: the direct call sweep finds five call sites: `0x12048` in the
  player segment helper at `0x11f98`, `0x13ea5` in `player_car_init`,
  `0x1522b` in the AI route setup at `0x1517c`, `0x16ccb` in
  `player_track_node_update`, and `0x185d8` in `player_car_reset_update`.
  Each caller writes the position to `DAT_00108038` and `DAT_00108040` and
  reads the heading `DAT_00108048` back.  `player_car_init` and
  `player_car_reset_update` copy that heading to the car field `+0x4cc`.
  Evidence: the byte sweep for a relative call to `0x120c0`, the calls at
  `0x12048`, `0x13ea5`, `0x1522b`, `0x16ccb`, and `0x185d8`, and the heading
  stores at `0x13eb3` and `0x185e6`.
- Observation: the function scans the three-by-three cell neighborhood of the
  spatial grid at `DAT_00102fa4`.  The row index is
  `((y + 0x100000) >> 13) - 1` and the cell index is
  `((x + 0x100000) >> 13) - 1`.  A row pointer or a cell pointer of -1 is
  skipped.  The map collision check at `0x11c00` uses the same grid, the same
  0x100000 bias, the same 0x2000 cell size, and the same nine-entry offset
  table at `DAT_00101cf0` and `DAT_00101cf8`.
  Evidence: the loops at `0x12116` through `0x12239`, the grid reads at
  `0x12101` and `0x12120`, and the offsets at `0x101cf0` through `0x101d17`.
- Observation: a cell record holds the 12-byte node entries below the pointer
  at `+0x08` with the count at `+0x1c`, and the 28-byte segment entries below
  the pointer at `+0x0c` with the count at `+0x20`.  Each entry holds the angle
  at `+0x00`, the x position at `+0x04`, and the y position at `+0x08`.  The
  zero mode stores the nearest node at `DAT_00108054`, and the non-zero mode
  stores the nearest segment at `DAT_00108044`.
  Evidence: the mode branch at `0x12168`, the node loop at `0x1223e` through
  `0x12307`, the segment loop at `0x12170` through `0x12239`, and the grid
  relocation at `0x4cb4c`.
- Observation: the nearest test compares the squared distance with
  `DAT_00108020`, which starts at `0x7ffffff8`.  The zero mode copies the node
  x and y to `DAT_0010804c` and `DAT_00108050` and forms the heading at
  `DAT_00108048`.  The flag bit 3 of `DAT_0011d274` adds pi at
  `DAT_0010040c`, and a heading that reaches two pi at `DAT_00100410` receives
  minus two pi at `DAT_00100414`.
  Evidence: the blocks at `0x1230c` through `0x12376` and the constants at
  `0x10040c`, `0x100410`, and `0x100414`.
- Inference: the grid data, the node and segment entries, the heading output,
  and the five callers support the name `track_nearest_node`.  The module
  records the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the module reuses the shared `fpu_stub.asm`, because it adds
  and compares floats and does not convert a float to an integer.  The
  constants arrive through globals, because the minimal image does not load the
  constant segment.  The original has the EBP frame, so the module builds with
  `-of+`.
  Evidence: `tools/rebuild/track_nearest_node.c` and the float reads at
  `0x12334`, `0x12351`, and `0x1235c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9`, `A`, and `D`.  The paths cover the nearest node, the cross-row
  scan, the pi add, the two-pi wrap, the no-hit return 0, the last-cell hit,
  the nearest segment, the segment no-hit, and the exact two-pi boundary.
  Evidence: `python3 tools/rebuild/harness.py run --module track_nearest_node`
  and `work/rebuild/tnn_out.txt`.
- Observation: the rebuilt module holds 201 decoded instructions and 687
  bytes.  The identity check stays open at 39 of 201 instructions and 36 of 687
  bytes, because V2 saves the frame pointer first, uses another register and
  local layout, and uses `imul` for the entry stride.  The build and behavior
  checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x120c0 --count 201 --rebuilt work/rebuild/tnn.exe
  --rebuilt-map work/rebuild/track_nearest_node.map --rebuilt-symbol
  track_nearest_node_`.
- Limit: the original has no symbol name for this function.  The module name
  and the two entry names in the harness stay inferred.

## The car_select_screen module at 0x2bedc

- Observation: the original body at `0x2bedc` holds 171 instructions and 682
  bytes up to the `ret` at `0x2c185`.  The next function starts at `0x2c188`,
  so the store record keeps the 684-byte extent and sets no function size.  The
  identity check uses 171 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2bedc --count 171` and
  `tools/rebuild/functions/0x2bedc.json`.
- Observation: the only direct caller at `0x2beb0` is case 2 of the state init
  dispatcher at `0x2be94`.  The jump table at linear `0x2be70` maps the state 2
  to `0x2beb0`, the state 3 to `0x2beb7`, and the state 4 to `0x2bebe`.  The
  matching state update is case 2 of `race_state_update` at `0x29564`, which
  calls `0x2cb20` at `0x295d3`.  `race_state_update` is a `race_frame` callee at
  `0x2807c`, so the record takes the section `race_frame callees`.
  Evidence: the call sweep for `0x2bedc`, the jump table at linear `0x2be70`,
  and the calls at `0x29564` and `0x2807c`.
- Observation: the init loads `menu4h.pcx` from `gfx\graf.dat` with the palette
  at object-3 offset `0xcf5c`.  The file string sits at object-3 offset `0x111f`
  and the member string at `0x1114`.  The function presents the buffer, copies
  `0x3e800` bytes to `DAT_0021528`, fills the screen, and presents it again.  It
  sets `DAT_0011d240` at 1, `DAT_00120378` at 1.5, `DAT_00120380` at 0.95, and
  `DAT_0011fac8` at 1.
  Evidence: the calls at `0x2bf23` through `0x2bf58` and the bytes at linear
  `0x101114` and `0x10111f`.
- Observation: the loop builds 14 records at `DAT_0011d550`, stride 0x44.  Each
  record holds an object index at `+0x00`.  The object table at `0x653c` uses
  the index with stride 0x1c.  The function sets the record angles `+0x08`,
  `+0x0c`, and `+0x10` to 0, pi/2, and 0, and sets the object position `+0x08`,
  `+0x0c`, and `+0x10` to `x = -4000 * i`, `y = 0`, and `z = 0xb54`.  It
  converts the record angles to the object angles `+0x14`, `+0x18`, and `+0x1c`
  with the factor 1024.0 / pi and the mask 0x7ff, and it refreshes the object
  list at `0x6530` with the name `0x6350`.
  Evidence: the loop at `0x2c00d` through `0x2c15d` and the constants at
  object-3 offsets `0x112c` and `0x1130`.
- Observation: the selection index `DAT_001203c4` scales to
  `-4000 * index` at `DAT_00203a8` and `DAT_002039c`.  `race_panel_2` at
  `0x2e21c` reads the same table and index and draws the car name, the speed in
  km/h, and the car statistics.  `race_results_screen` at `0x2a558` also reads
  the table at `0x1d550`, and the state 2 update at `0x2cb20` walks the same 14
  records.
  Evidence: the scaling at `0x2bf5d` through `0x2bfa4`, the reads in
  `tools/rebuild/race_panel_2.c`, and the loop at `0x2cb36`.
- Inference: the state 2 init builds the car selection table, and the state 2
  overlay draws the selected car.  The table, the selection index, the
  `menu4h.pcx` member, and the `race_panel_2` reader support the name
  `car_select_screen`.  The module records the name with the confidence
  `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls the truncate helper at `0x4d9f4`.  The shared
  `fpu_stub.asm` defines `__CHP` as a bare return, which keeps the
  round-to-nearest mode, so a negative value rounds away from zero.  The module
  uses the local `car_select_screen_fpu.asm`, which makes `__CHP` truncate
  toward zero.  The float constants arrive through globals, because the minimal
  image does not load the constant segment.  The original has the EBP frame, so
  the module builds with `-of+`.
  Evidence: `tools/rebuild/car_select_screen_fpu.asm`,
  `tools/rebuild/car_select_screen.c`, and the FPU blocks at `0x2c096`,
  `0x2c0d1`, and `0x2c10c`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `4` and `D`.  The paths cover the index 0, 3, and 7 offsets, the
  failed load, and the permuted object table with the record 14 sentinel.
  Evidence: `python3 tools/rebuild/harness.py run --module car_select_screen`,
  `work/rebuild/csel.txt`, and
  `python3 tools/rebuild/run.py run --function 0x2bedc`.
- Observation: the rebuilt module holds 171 decoded instructions and 705
  bytes.  The identity check stays open at 53 of 171 instructions and 43 of 682
  bytes, because V2 saves the frame pointer first, uses another register and
  local layout, and uses `imul` for the record stride.  The build and behavior
  checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2bedc --count 171 --rebuilt work/rebuild/csel.exe
  --rebuilt-map work/rebuild/car_select_screen.map --rebuilt-symbol
  car_select_screen_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The track_select_screen module at 0x2c188

- Observation: the original body at `0x2c188` holds 164 instructions and 679
  bytes up to the `ret` at `0x2c42e`.  The next function starts at `0x2c430`,
  so the store record keeps the 680-byte extent and sets no function size.  The
  identity check uses 164 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2c188 --count 164` and
  `tools/rebuild/functions/0x2c188.json`.
- Observation: the only direct caller at `0x2beb7` is case 3 of the state init
  dispatcher at `0x2be94`.  The jump table at linear `0x2be70` maps the state 2
  to `0x2beb0`, the state 3 to `0x2beb7`, and the state 4 to `0x2bebe`.  The
  matching state update is case 3 of `race_state_update` at `0x29564`, which
  calls `0x2cc98` at `0x295dd`.  `race_state_update` is a `race_frame` callee at
  `0x2807c`, so the record takes the section `race_frame callees`.
  Evidence: the call sweep for `0x2c188`, the jump table at linear `0x2be70`,
  and the calls at `0x29564` and `0x2807c`.
- Observation: the init loads `menu3h.pcx` from `gfx\graf.dat` with the palette
  at object-3 offset `0xcf5c`.  The file string sits at object-3 offset `0x113f`
  and the member string at `0x1134`.  The function presents the buffer, copies
  `0x3e800` bytes to `DAT_0021528`, fills the screen, and presents it again.  It
  sets `DAT_0011d240` at 1, `DAT_00120378` at 1.5, `DAT_00120380` at 0.95, and
  `DAT_0011fac8` at 1.
  Evidence: the calls at `0x2c1cf` through `0x2c204` and the bytes at linear
  `0x101134` and `0x10113f`.
- Observation: the loop builds 12 records at `DAT_0011d908`, stride 0x34.  Each
  record holds an object index at `+0x00`.  The object table at `0x653c` uses
  the index with stride 0x1c.  The function sets the record angles `+0x08`,
  `+0x0c`, and `+0x10` to 0, pi/2, and 0, and sets the object position `+0x08`,
  `+0x0c`, and `+0x10` to `x = -40000 * i`, `y = 0`, and `z = 0xb54`.  It
  converts the record angles to the object angles `+0x14`, `+0x18`, and `+0x1c`
  with the factor 1024.0 / pi and the mask 0x7ff, and it refreshes the object
  list at `0x6530` with the name `0x6350`.
  Evidence: the loop at `0x2c29d` through `0x2c406` and the constants at
  object-3 offsets `0x114c` and `0x1150`.
- Observation: the selection index `DAT_001203c8` scales to
  `-40000 * index` at `DAT_00203a8` and `DAT_002039c`.  The state 3 update at
  `0x2cc98` walks the same 12 records, and `race_panel_3` at `0x2e58c` reads
  the same table and index and draws the row text.  The table holds `AMAP1`
  through `AMAP6` twice.
  Evidence: the scaling at `0x2c209` through `0x2c26c`, the reads in
  `tools/rebuild/race_panel_3.c`, and the loop at `0x2ccef`.
- Inference: the state 3 init builds the track selection table, and the state 3
  overlay draws the selected track.  The map names, the `menu3h.pcx` member, the
  `race_panel_3` reader, and the state 3 position after the state 2 car
  selection support the name `track_select_screen`.  The module records the
  name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls the truncate helper at `0x4d9f4`.  The shared
  `fpu_stub.asm` defines `__CHP` as a bare return, which keeps the
  round-to-nearest mode, so a negative value rounds away from zero.  The module
  uses the local `track_select_screen_fpu.asm`, which makes `__CHP` truncate
  toward zero.  The float constants arrive through globals, because the minimal
  image does not load the constant segment.  The original has the EBP frame, so
  the module builds with `-of+`.
  Evidence: `tools/rebuild/track_select_screen_fpu.asm`,
  `tools/rebuild/track_select_screen.c`, and the FPU blocks at `0x2c33f`,
  `0x2c37a`, and `0x2c3b5`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `4` and `D`.  The paths cover the index 0, 3, and 7 offsets, the
  failed load, and the permuted object table with the record 12 sentinel.
  Evidence: `python3 tools/rebuild/harness.py run --module track_select_screen`,
  `work/rebuild/tsel.txt`, and
  `python3 tools/rebuild/run.py run --function 0x2c188`.
- Observation: the rebuilt module holds 164 decoded instructions and 671
  bytes.  The identity check stays open at 50 of 164 instructions and 40 of 671
  bytes, because V2 saves the frame pointer first, uses another register and
  local layout, and uses `imul` for the record stride.  The build and behavior
  checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2c188 --count 164 --rebuilt work/rebuild/tracksel.exe
  --rebuilt-map work/rebuild/track_select_screen.map --rebuilt-symbol
  track_select_screen_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The choice_load module at 0x1bbc8

- Observation: the original body at `0x1bbc8` holds 189 instructions and 673
  bytes up to the `ret` at `0x1be68`.  The next function starts at `0x1be6c`,
  so the store record keeps the 676-byte extent and sets no function size.  The
  identity check uses 189 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1bbc8 --count 189` and
  `tools/rebuild/functions/0x1bbc8.json`.
- Observation: the function builds `%s\config\choice.dat` from the base path at
  `DAT_00007edc` through the `sprintf` call at `0x1bbe1`, opens the path with
  the mode `rb` through the `fopen` call at `0x1bbf1`, and tests the handle at
  `0x1bbfc`.  The format string sits at object-3 offset `0xa75` and the mode
  string at `0xa8a`.
  Evidence: the calls at `0x1bbe1` and `0x1bbf1`, the test at `0x1bbfc`, and
  the bytes at linear `0x100a75` and `0x100a8a`.
- Observation: on a successful open the function reads 0x460 bytes into the
  record at `0x203c4` through the `fread` call at `0x1bdff`, copies the fields
  at `0x203d0`, `0x203d4`, `0x203d8`, `0x207fc`, `0x20800`, and `0x20820` to
  the race context at `DAT_00002dec` offsets `+0x08`, `+0x18`, `+0x28`,
  `+0x38`, `+0x48`, and `+0x1c`, and closes the handle through the `fclose`
  call at `0x1be5b`.
  Evidence: the block at `0x1bded` through `0x1be5f`.
- Observation: on a failed open the function writes the default record.  It
  clears the version fields at `0x203c4`, `0x203c8`, `0x203dc`, `0x203e0`, and
  `0x203e4`.  It sets the three 0x6000 limits at `0x20808`, `0x2080c`, and
  `0x20810`, the three 1 flags at `0x207fc`, `0x20800`, and `0x20814`, and the
  two 2 flags at `0x207f4` and `0x20820`.  It sets the global `0x1d2a8` to 2
  and clears `0x20818` and `0x2081c`.  It marks the six index slots at
  `0x20400` with -1 and copies the tag `GRA` from object-3 offset `0xa8d` to
  `0x203cc`.
  Evidence: the block at `0x1bc04` through `0x1bcc2` and the bytes at linear
  `0x100a8d`.
- Observation: the function fills six lap records at `0x20430` and six race
  records at `0x20610`.  Each record is 0x50 bytes and holds five time slots at
  `+0x00`, five 4-byte tags at `+0x14`, and five flags at `+0x28`.  The lap time
  slots take the bit pattern 0x43340000 (180.0) and the race time slots take
  0x44610000 (900.0).  The tags take `GRA` from the object-3 offsets `0xa91`
  and `0xa95`, and the flags take zero.
  Evidence: the loop at `0x1bcc3` through `0x1be60` and the constants at
  `0x1bd0e` and `0x1bd84`.
- Observation: the direct callers are `race_start` at `0x2779e` and
  `menu_space_init` at `0x29d3d`.  Both call the function in the setup path.
  Three callers read the same record: `loading_screen` at `0x2a364` reads the
  0x50-byte entry at `0x20430`, `race_record_save` at `0x2e82c` reads the lap
  and race tables, and `race_records_screen` at `0x2d4b8` formats them.
  Evidence: the calls at `0x2779e`, `0x29d3d`, and the record reads in
  `tools/rebuild/loading_screen.c` and `tools/rebuild/race_record_save.c`.
- Inference: the function loads the choice file.  The `%s\config\choice.dat`
  path, the open, the read, the close, and the default record support the name
  `choice_load`.  The module records the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the time slots have the type `int`, because the original stores
  the immediate bit patterns and the rebuild links no FPU run-time library.  A
  `float` assignment would reference the undefined `_fltused_` and the emulator
  symbols.  The original has the EBP frame, so the module builds with `-of+`.
  Evidence: `tools/rebuild/choice_load.c` and the failed build without the
  `int` slots.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `2` and `D`.  The paths cover the success copy, the default record,
  and a second success handle.  The float checks compare the bit patterns.
  Evidence: `python3 tools/rebuild/harness.py run --module choice_load`,
  `work/rebuild/chl_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x1bbc8`.
- Observation: the rebuilt module holds 189 decoded instructions and 628
  bytes.  The identity check stays open at 73 of 189 instructions and 34 of 628
  bytes.  V2 saves the registers after the frame, places the success block
  first, uses other local offsets, and calls a helper for the tag copy.  The
  build and behavior checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1bbc8 --count 189 --rebuilt work/rebuild/chcload.exe
  --rebuilt-map work/rebuild/choice_load.map --rebuilt-symbol choice_load_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The player_car_cornering_update module at 0x16ca0

- Observation: the original body at `0x16ca0` holds 182 instructions and 657
  bytes up to the `ret` at `0x16f30`.  The next function starts at `0x16f34`,
  so the store record keeps the 660-byte extent and sets no function size.  The
  identity check uses 182 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x16ca0 --count 182` and
  `tools/rebuild/functions/0x16ca0.json`.
- Observation: the only direct caller is at `0x15af4`.  The call sits in the
  player physics group at `0x15ab8`.  The group runs on the player object at
  object-3 offset `0xf91c`, and the five view groups at `0x2003c`, `0x201d0`,
  `0x202c0`, `0x20360`, and `0x203c4` call the group.  The module therefore
  takes the section `The vehicle state chain`.
  Evidence: the direct call sweep for `0x16ca0`, the disassembly at `0x15ab8`
  through `0x15b04`, and the call sites at `0x200e0`, `0x20209`, `0x2032b`,
  `0x20398`, and `0x203fc`.
- Observation: the function publishes `+0x4b0` and `+0x4b8`, calls the nearest
  node search at `0x120c0`, and copies the track heading `DAT_00108048` to
  `+0x13c`.  It forms the heading error between the car heading `+0x4cc` and the
  track heading, folds the error into 0 through pi/2, and scales it to 0
  through 1.  The constants pi, -pi, and pi/2 sit at object-3 offsets `0x768`,
  `0x76c`, and `0x770`.
  Evidence: the blocks at `0x16caa` through `0x16d6e` and the nearest node
  store at `0x1234c`.
- Observation: a speed `+0x50c` times the error above `+0x180` sets the effect
  state `+0x148` to 2 through `0x19478` when the flag `+0x7b0` is clear.  The
  function normalizes the velocity `+0x44` and `+0x4c` into `+0x550` and
  `+0x558`.  It stores the lateral rate `+0x54c` from the sine at `0x4da72`,
  the speed, the arc tangent at `0xc21e2` with 4.0, and 2000.0, and it clamps
  the rate at 5000.0.  It removes the speed loss from `+0x50c` with the double
  0.9 at object-3 offset `0x784` and stores the heading rate `+0x548` from
  `+0xc4` minus `+0x6c` with 5.0 at offset `0x78c`.
  Evidence: the blocks at `0x16d71` through `0x16ec0`.
- Observation: the function clears `+0x110`, `+0x4f0`, and `+0x4f4`.  It clears
  the filtered slip `+0x128` when `+0x170` is clear.  It keeps the type-4 table
  entry `+0x7d4` through the acquire at `0x1f7ac` when the state did not change.
  Evidence: the blocks at `0x16ec0` through `0x16f30`.
- Inference: the caller position in the physics group, the heading error
  against the nearest track node, the lateral rate and the heading rate that
  `player_car_view_update` at `0x16f34` applies, the speed loss, the skid
  state, and the table entry support the name `player_car_cornering_update`.
  The module records the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls the sine at `0x4da72`, the arc tangent at
  `0xc21e2`, and the square root at `0x22bec`.  The harness links no math
  library, so the local FPU stub supplies `sin`, `atan2`, and `sqrt`.  The
  module converts no value to an integer, so `__CHP` stays a bare return.  The
  original has the EBP frame, so the module builds with `-of+`.
  Evidence: `tools/rebuild/player_car_cornering_update_fpu.asm` and
  `tools/rebuild/modules/player_car_cornering_update.json`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `7` and `D`.  The paths cover the two early flags, the normal path,
  the skid trigger, the blocked trigger, the heading wrap, the lateral clamp,
  the kept slip, and the speed clamp.
  Evidence: `python3 tools/rebuild/harness.py run --module
  player_car_cornering_update`, `work/rebuild/pccrn.txt`, and
  `python3 tools/rebuild/run.py run --function 0x16ca0`.
- Observation: the identity check stays open at 29 of 182 instructions and 38
  of 649 bytes.  The V2 toolchain saves the registers in another order and uses
  another local layout.  The build and behavior checks pass, so the store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x16ca0 --count 182 --rebuilt work/rebuild/pccrn.exe
  --rebuilt-map work/rebuild/player_car_cornering_update.map --rebuilt-symbol
  player_car_cornering_update_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The menu_text_update module at 0x29918

- Observation: the original body at `0x29918` holds 174 instructions and 657
  bytes up to the `ret` at `0x29ba8`.  The next function starts at `0x29bac`,
  so the store record keeps the 660-byte extent and sets no function size.  The
  identity check uses 174 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x29918 --count 174` and
  `tools/rebuild/functions/0x29918.json`.
- Observation: the only direct caller is at `0x29581`.  The call sits in case 0
  of the dispatcher `race_state_update` at `0x29564`, and `race_frame` at
  `0x2807c` calls the dispatcher every frame.  The jump table at linear
  `0x19540` maps the state 0 to `0x29918`.
  Evidence: the direct call sweep for `0x29918`, the disassembly at `0x29564`
  through `0x2958a`, and the jump table at `0x19540`.
- Observation: when the key `DAT_0011fb64` differs from the menu index
  `DAT_0011fb60`, the function resets the 0x50 records at `DAT_001db78` through
  0x2b450 and counts the active lines of the table `DAT_002de8[DAT_0011fb60]`
  into `DAT_0011fb6c`.  Each 16-byte line record holds an offset at `+0x00` and
  an offset at `+0x08`, and their sum indexes the string pointer table at
  `DAT_001fb80`.
  Evidence: the blocks at `0x29935` through `0x2999c` and the loop at `0x29a27`
  through `0x29aaf`.
- Observation: the line descriptor at `DAT_0011fad4` holds the line at `+0x00`,
  the row at `+0x04`, the centered flag 1 at `+0x0c`, the selection flag at
  `+0x10`, the five values of the 8-entry table at `0x221c` at `+0x14` through
  `+0x24`, the angle 0.09 at `+0x28`, and the timer 0 at `+0x2c`.  The row
  starts at `8 - DAT_0011fb6c`, the line rises by one per line, and the row
  rises by two per line.  The table index `DAT_0011fabc` wraps above 7.
  Evidence: the blocks at `0x2999e` through `0x29ac8` and the table at
  `0x221c`.
- Observation: the function marks the selected line through 0x2b308 and
  advances the phase `DAT_0011d248` by the frame delta `DAT_0018e08` times the
  constant at `0xf3b`.  The constant is 3 pi.  A phase above the constant at
  `0xf3f` resets to zero.  The constant is 8 pi.
  Evidence: the call at `0x29ada`, the FPU block at `0x29adf` through
  `0x29b04`, and the float values `0x4116cbe4` and `0x41c90fdb` at object-3
  offsets `0xf3b` and `0xf3f`.
- Observation: when the key `DAT_0011fb64` is -2, the function runs 0x2b494 on
  every record, restores the key to `DAT_0011fb60`, and clears the overlay word
  `DAT_0011fac0`.  Otherwise it runs `map_object_update` at `0x2b9cc` on every
  record and stores the current key when the update leaves the key unchanged.
  Evidence: the blocks at `0x29b0a` through `0x29b9b`.
- Inference: the function places and animates the text lines of the selected
  menu as map objects.  The caller, the string table, the line descriptor, and
  the two map object modules support the name `menu_text_update`.  The module
  records the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the constants at `0xf3b` and `0xf3f` arrive through globals,
  because the minimal image does not load the constant segment.  The module uses
  the shared `fpu_stub.asm`, because it performs no float-to-integer
  truncation.  The original has the EBP frame, so the module builds with
  `-of+`.
  Evidence: `tools/rebuild/menu_text_update.c` and
  `tools/rebuild/modules/menu_text_update.json`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `6` and `D`.  The paths cover the page change, the unchanged key, the
  finish path, the phase wrap, the counter wrap, the empty menu, and the kept
  -2 key.
  Evidence: `python3 tools/rebuild/harness.py run --module menu_text_update`,
  `work/rebuild/mtu_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x29918`.
- Observation: the rebuilt module holds 174 decoded instructions and 605
  bytes.  The identity check stays open at 36 of 174 instructions and 27 of 605
  bytes.  V2 saves the registers after the frame, folds the record stride into
  an immediate, and loads the table values in another order.  The build and
  behavior checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x29918 --count 174 --rebuilt work/rebuild/menutxt.exe
  --rebuilt-map work/rebuild/menu_text_update.map --rebuilt-symbol
  menu_text_update_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The race_hud_draw module at 0x23420

- Observation: the original body at `0x23420` holds 173 instructions and 633
  bytes up to the `ret` at `0x23698`.  The next function starts at `0x2369c`,
  so the store record keeps the 636-byte extent and sets no function size.  The
  identity check uses 173 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x23420 --count 173` and
  `tools/rebuild/functions/0x23420.json`.
- Observation: the only direct caller at `0x2001a` is the frame update at
  `0x1ff70`, which calls this function after the five view groups at `0x2003c`,
  `0x201d0`, `0x202c0`, `0x20360`, and `0x203c4`.  No named function sits
  between the frame update and the target, so the record takes the section
  Program functions.
  Evidence: the direct call sweep for `0x23420`, the disassembly of `0x1ff70`,
  and the call at `0x2001a`.
- Observation: the first block formats the race clock `DAT_001100a8` into the
  global text at `0x101f98`.  The format at object-3 offset `0xdd0` is
  `%02d:%02d:%02d`.  The minutes are `t / 60`, the seconds are `t % 60`, and
  the centiseconds are `(clock - t) * 100.0`, where `t` is the truncated clock
  and the scale is the float at object-3 offset `0xdfe`.
  Evidence: the block at `0x2342d` through `0x23487` and the bytes at linear
  `0x100dd0` and `0x100dfe`.
- Observation: a set replay flag `DAT_00118e6c` stops the draw after the clock
  format.  The frame counter `DAT_000007fd` bit 0x80 then draws the label `R`
  at (0x10, 0x154) with the font 0 and the color 6.
  Evidence: the block at `0x2348c` through `0x234c0` and the string at
  object-3 offset `0xddf`.
- Observation: otherwise the function draws the gauge panel at `0x2369c`.  It
  formats the lap value `DAT_001100ac` through the format `%d` at offset
  `0xde1` and centers it at y 0x10 with the font 6 and the color 2.  The value
  from `DAT_0010ffa0` rises by one when it is at or below `DAT_0011d2b0` and
  draws through the format `%d%c` at offset `0xde4` with the character `:`.
  The counter `DAT_0011d2b0` plus one follows at the measured width plus 0xa,
  and the offset `DAT_0010ffa4` draws at (0x228, 0x16).
  Evidence: the block at `0x234c5` through `0x235d1` and the strings at linear
  `0x100de1`, `0x100de4`, `0x100de9`, and `0x100dec`.
- Observation: the second time block formats the lap clock `DAT_001100b0`
  through the format at offset `0xdef`.  The function draws the first clock at
  (0x10, 0x70), the lap clock at (0x10, 0x90), and the static clock label at
  `0x101f8c` at (0x10, 0xb0), each with the font 3 and the color 2.  The last
  call draws the race radar at `0x23a94`.
  Evidence: the block at `0x235d6` through `0x2368c`, the string at linear
  `0x100def`, and the label at linear `0x101f8c`.
- Inference: the frame update caller, the race clock, the lap value, the two
  counters, the gauge panel, and the radar support the name `race_hud_draw`.
  The module records the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original truncates each float through the helper at
  `0x4d9f4`.  The shared `fpu_stub.asm` leaves round-to-nearest, so the module
  uses the local `__CHP` stub in `race_hud_draw_fpu.asm`.  The 100.0 scale
  arrives through a global, because the minimal image does not load the
  constant segment.  The original has the EBP frame, so the module builds with
  `-of+`.
  Evidence: `tools/rebuild/race_hud_draw_fpu.asm` and
  `tools/rebuild/modules/race_hud_draw.json`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `4` and `D`.  The paths cover the normal draw, the replay label, the
  replay return, the display value above the position, and the equal display
  and position.
  Evidence: `python3 tools/rebuild/harness.py run --module race_hud_draw`,
  `work/rebuild/racehud.txt`, and
  `python3 tools/rebuild/run.py run --function 0x23420`.
- Observation: the rebuilt module holds 173 decoded instructions and 593
  bytes.  The identity check stays open at 27 of 173 instructions and 41 of 593
  bytes.  V2 saves the registers after the frame, moves the time arithmetic to
  a helper, and uses another local layout.  The build and behavior checks pass,
  so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x23420 --count 173 --rebuilt work/rebuild/racehud.exe
  --rebuilt-map work/rebuild/race_hud_draw.map --rebuilt-symbol
  race_hud_draw_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The player_view_orbit module at 0x1a3b8

- Observation: the original body at `0x1a3b8` holds 179 instructions and 631
  bytes up to the `ret` at `0x1a62e`.  The next function starts at `0x1a630`,
  so the store record keeps the 632-byte extent and sets no function size.  The
  identity check uses 179 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1a3b8 --count 179` and
  `tools/rebuild/functions/0x1a3b8.json`.
- Observation: the three direct callers at `0x19eb5`, `0x19ed8`, and `0x19ef3`
  are the view mode dispatch at `0x19e78`.  The dispatch selects this function
  for the view modes 1, 2, and 0.  It passes the player object at object-3
  offset `0xf91c` in `eax`, the player view object at `DAT_00110a20` in `edx`,
  and the mode in `ebx`.  The player update at `0x19ab0` and
  `player_view_setup` at `0x19ae8` both call the dispatch, so the record takes
  the section `player_view_setup callees`.
  Evidence: the direct call sweep for `0x1a3b8`, the disassembly of `0x19e78`,
  and the call at `0x19ab0`.
- Observation: the function refreshes the two list slots at `+0x460` and
  `+0x468` through `0x117fc`.  It wraps the player yaw at `+0x4cc` into the
  range zero to 2 pi with the floats at object-3 offsets `0x904` and `0x908`,
  which hold 2 pi and -2 pi.
  Evidence: the block at `0x1a3c7` through `0x1a43b` and the bytes at linear
  `0x100904` and `0x100908`.
- Observation: the mode selects the height offsets `0x12c`, `0x12c`, and
  `0x64`, the orbit radii `0x6a4`, `0x514`, and `0x578`, and the pitch
  offsets at `+0x4d4` plus the double at `0x90c` or `0x914`.  The doubles hold
  `6.157521772384643` and `6.094689917564392`.
  Evidence: the blocks at `0x1a441` through `0x1a4b3` and the bytes at linear
  `0x10090c` and `0x100914`.
- Observation: the function forms the height at `+0x04` from the player height
  `+0x4b4` and the mode offset minus the radius times the sine of the old pitch
  at `+0x30`.  A set flag `+0x7c` writes the height, the yaw at `+0x34`, the
  second angle at `+0x38`, and the pitch at `+0x30`.  A clear flag approaches
  the values through `0x22bb4` and `0x22018` with the step index `0x51`.
  Evidence: the blocks at `0x1a4ba` through `0x1a58a`.
- Observation: the function places the view on the orbit.  `+0x00` is
  `+0x4b0` minus the radius times the sine of the yaw times the cosine of the
  pitch.  `+0x08` is `+0x4b8` plus the radius times the cosine of the yaw times
  the cosine of the pitch.  The two saved copies sit at `+0x0c` and `+0x14`.
  Evidence: the blocks at `0x1a58f` through `0x1a628`.
- Inference: the view mode dispatch, the player view object, the three orbit
  presets, and the sibling `player_route_camera` and `car_camera` support the
  name `player_view_orbit`.  The module records the name with the confidence
  `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls `sin` through `0x4da72`, `cos` through
  `0x4da66`, and the truncation helper at `0x4d9f4`.  This harness links no
  math library, so `player_view_orbit_fpu.asm` supplies `sin`, `cos`, and the
  truncating `__CHP`.  The four constants arrive through globals, because the
  minimal image does not load the constant segment.  The original has the EBP
  frame, so the module builds with `-of+`.
  Evidence: `tools/rebuild/player_view_orbit_fpu.asm` and
  `tools/rebuild/modules/player_view_orbit.json`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `7` and `D`.  The paths cover the three modes, the pitch below zero,
  the high and low yaw wrap, the approach branch, and the long mode-1 offset.
  Evidence: `python3 tools/rebuild/harness.py run --module player_view_orbit`,
  `work/rebuild/pvorbit.txt`, and
  `python3 tools/rebuild/run.py run --function 0x1a3b8`.
- Observation: the rebuilt module holds 179 decoded instructions and 566
  bytes.  The identity check stays open at 31 of 179 instructions and 34 of 566
  bytes.  V2 saves the registers after the frame, reorders the mode switch, and
  uses another local layout.  The build and behavior checks pass, so the store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1a3b8 --count 179 --rebuilt work/rebuild/pvorbit.exe
  --rebuilt-map work/rebuild/player_view_orbit.map --rebuilt-symbol
  player_view_orbit_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The opponent_avoid_player module at 0x1e414

- Observation: the original body at `0x1e414` holds 162 instructions and 590
  bytes up to the `ret` at `0x1e661`.  The next function starts at `0x1e670`,
  so the store record keeps the 604-byte extent and sets no function size.  The
  identity check uses 162 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1e414 --count 162` and
  `tools/rebuild/functions/0x1e414.json`.
- Observation: the only direct caller at `0x1c804` is the per-object step at
  `0x1c7d0`.  That step first marks the nearby objects at `0x126e4` and
  resolves the nearest opponent at `0x1d00c`.  The object update at `0x2003c`
  runs the step on each 0x590-byte car record at `0x10ab8` after
  `object_prepare`.  The record therefore takes the section
  `object update callees`.
  Evidence: the sweep of every candidate function start for a call to
  `0x1e414`, the block at `0x1c7d0` through `0x1c80d`, and the loop at
  `0x20168` through `0x20187`.
- Observation: the function forms the look-ahead point three car radii ahead of
  the opponent.  It uses the world position at `+0x498` and `+0x4a0`, the
  heading at `+0x4a8`, and the radius at `+0x44c`.  It returns when the point
  stays 5000 units or more from the player position.
  Evidence: the block at `0x1e421` through `0x1e4d7` and the immediate
  `0x1388`.
- Observation: the player position is the fixed player object at `0xf91c`.
  The field `+0x4b0` sits at `0xfdcc` and the field `+0x4b8` sits at `0xfdd4`.
  The race radar reads the same two fields as the player position, and the
  reference copy at `0x212d0` passes them to the proximity check.
  Evidence: the fixups at `0x1e4b0` and `0x1e4c9` to object 3 offsets `0xfdcc`
  and `0xfdd4`, the `race_radar` finding, and the block at `0x212ea` through
  `0x21305`.
- Observation: near the player, the function sets the avoidance radius at
  `+0x488` to twice the opponent radius `+0x44c` plus the player radius at
  `+0x544`.  It then checks the two player body points at `+0x52c` and `+0x534`,
  and at `+0x538` and `+0x540`.  The player update at `0x17c6c` rotates the body
  offsets `+0x514` and `+0x520` by the negative heading and adds the player
  position, so the four fields are the two body points in the world.
  Evidence: the block at `0x1e4dd` through `0x1e53f` and the block at `0x17c6c`
  through `0x17d10`.
- Observation: when a body point stays inside the avoidance radius and the
  opponent speed `+0x418` is above the player speed `+0x50c`, the function
  pushes the opponent local position `+0x454` and `+0x45c` to the avoidance
  radius along the offset from that point.  It forms the distance through the
  square-root helper at `0x22bec`.  The fields `+0x484`, `+0x48c`, and `+0x494`
  hold the distance, the offset x, and the offset z.
  Evidence: the block at `0x1e551` through `0x1e655`.
- Inference: the caller step, the player body points, the player speed gate, and
  the local position push support the name `opponent_avoid_player`.  The module
  records the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls `sin` at `0x4da72`, `cos` at `0x4da66`,
  `sqrt` at `0x22bec`, and the truncating helper at `0x4d9f4`.  This harness
  links no math library, so `opponent_avoid_player_fpu.asm` supplies `sin`,
  `cos`, and `sqrt` from the hardware and makes `__CHP` truncate toward zero.
  The original has the EBP frame, so the module builds with `-of+`.
  Evidence: `tools/rebuild/opponent_avoid_player_fpu.asm` and
  `tools/rebuild/modules/opponent_avoid_player.json`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9` and `D`.  The paths cover the two far guards, the two far body
  points, the speed gate, the first point, the second point, both points, the
  radius scaling, the 4999-unit guard boundary, and the distance equal to the
  avoidance radius.
  Evidence: `python3 tools/rebuild/harness.py run --module
  opponent_avoid_player`, `work/rebuild/oavoid.txt`, and
  `python3 tools/rebuild/run.py run --function 0x1e414`.
- Observation: the rebuilt module holds 513 bytes.  The identity check stays
  open at 20 of 162 instructions and 19 of 513 bytes.  V2 saves the registers
  after the frame and uses another instruction order and local layout.  The
  build and behavior checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1e414 --count 162 --rebuilt work/rebuild/oavoid.exe
  --rebuilt-map work/rebuild/opponent_avoid_player.map --rebuilt-symbol
  opponent_avoid_player_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The traffic_collision_pass module at 0x175dc

- Observation: the original body at `0x175dc` holds 165 instructions and 587
  bytes up to the `ret` at `0x17826`.  The next function starts at `0x17828`,
  so the store record keeps the 588-byte extent and sets no function size.  The
  identity check uses 165 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x175dc --count 165` and
  `tools/rebuild/functions/0x175dc.json`.
- Observation: the only direct caller at `0x15b6c` is the traffic effect group
  at `0x15b44`.  That group first calls the nearby-record marker at `0x12424`
  and then this function, and it calls the clear step at `0x124c0` afterwards.
  The traffic view group at `0x201d0` calls the group at `0x20268`, and the
  frame update at `0x1ff70` selects the group through the mode byte
  `DAT_0011d260` bit 0x10.  The car path has the same shape at `0x15b08` with
  the car proximity pass at `0x17408`.  The record therefore takes the section
  `The vehicle state chain`.
  Evidence: the sweep of every candidate function start for a call to
  `0x175dc`, the block at `0x15b44` through `0x15b7c`, the block at `0x201d0`
  through `0x202bc`, and the `car_collision_response` finding.
- Observation: the function clears the contact mask at `+0x2fc`.  It returns at
  once when the state field `+0x148` is not zero.  Otherwise it walks the
  0x2a8-byte traffic records at `DAT_00113cd8` while the index stays below the
  count `DAT_00115218`, and it skips a record whose first field is negative.
  Evidence: the block at `0x175e6` through `0x17615` and the loop at `0x177aa`
  through `0x177bc`.
- Observation: a record whose flag field `+0x14` holds the bit of the player
  index `DAT_0011d250` takes the velocity branch.  The function calls the push
  helper at `0x17b54` with the zero flag and copies the negated record velocity
  `+0x30` and `+0x38` to `+0x550` and `+0x558`.
  Evidence: the block at `0x177d9` through `0x17805`.
- Observation: otherwise the function acts only on a record in the nearby set
  `+0x104` and not flagged at `+0x16` bit 0.  The marker at `0x12424` fills the
  nearby set with every traffic record within 1500 units of the player position
  on each axis.  The function sets the contact distance at `+0x560` to the
  player radius `+0x544` plus the traffic radius `+0x2a4`.
  Evidence: the block at `0x12424` through `0x124be` and the block at
  `0x1761a` through `0x17657`.
- Observation: the function tests the two player body points at `+0x52c` and
  `+0x534` against the two traffic points at `+0x28c` and `+0x294` in 12-byte
  steps.  Each pair stores its deltas at `+0x564` and `+0x56c` and its
  truncated distance at `+0x55c`.  On a hit it calls the push helper with the
  set flag and marks the record bit in `+0x2fc`.  The last step filters
  `+0x7c4` toward the contact flag through `filter_update` at `0x22b80` with
  the rate 50.
  Evidence: the block at `0x1765d` through `0x17781` and the call at `0x1781a`.
- Inference: the caller chain, the traffic table, the two body-point pairs, and
  the push helper support the name `traffic_collision_pass`.  The module
  records the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls `sqrt` at `0x22bec` and the truncating helper
  at `0x4d9f4`.  This harness links no math library, so
  `traffic_collision_pass_fpu.asm` supplies `sqrt` from the hardware and makes
  `__CHP` truncate toward zero.  The original has the EBP frame, so the module
  builds with `-of+`.
  Evidence: `tools/rebuild/traffic_collision_pass_fpu.asm` and
  `tools/rebuild/modules/traffic_collision_pass.json`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9` and `D`.  The paths cover the state gate, the empty traffic list,
  the negative first field, the velocity branch, the nearby set, the record
  flag, the equal distance, the four body-pair pushes, the truncation of 2.828
  to 2, and the two-record mask.
  Evidence: `python3 tools/rebuild/harness.py run --module
  traffic_collision_pass`, `work/rebuild/tpass.txt`, and
  `python3 tools/rebuild/run.py run --function 0x175dc`.
- Observation: the rebuilt module holds 560 bytes.  The identity check stays
  open at 33 of 165 instructions and 35 of 560 bytes.  V2 saves the registers
  after the frame and uses another instruction order and local layout.  The
  build and behavior checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x175dc --count 165 --rebuilt work/rebuild/tpass.exe
  --rebuilt-map work/rebuild/traffic_collision_pass.map --rebuilt-symbol
  traffic_collision_pass_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The view_transition_update module at 0x19f00

- Observation: the original body at `0x19f00` holds 178 instructions and 577
  bytes up to the `ret` at `0x1a140`.  The next function starts at `0x1a144`, so
  the store record keeps the 580-byte extent and sets no function size.  The
  identity check uses 178 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x19f00 --count 178` and
  `tools/rebuild/functions/0x19f00.json`.
- Observation: the two direct callers are the player update at `0x19acc` and
  the car camera at `0x1aeb3`.  The player update runs this function as its
  third view step on the player view object `DAT_00110a20`.  The car camera
  runs it on the view block at `+0x4f8` of a 0x590-byte car record.  The view
  setup at `0x19ae8` sets the two transition flags `+0x54` and `+0x58`, so the
  record takes the section `player_update callees`.
  Evidence: the sweep of every candidate function start for a call to
  `0x19f00`, the call at `0x19acc`, the call at `0x1aeb3`, and the setup block
  at `0x19c40` through `0x19c50`.
- Observation: a set flag `+0x54` makes the function form the position deltas
  `+0x24`, `+0x28`, and `+0x2c` from the copy at `+0x18` minus the base at
  `+0x00`, and the angle deltas `+0x3c`, `+0x40`, and `+0x44` from the saved
  angles at `+0x48` minus the base angles at `+0x30`.  It then clears the flag.
  Evidence: the block at `0x19f08` through `0x19f86`.
- Observation: a set flag `+0x58` makes the function move the three angle
  deltas toward zero through the approach helper at `0x22018` with the step
  index `0x19`.  It scales the three position deltas by the frame factor at
  `DAT_0010cf08 + 0x168` and truncates toward zero.
  Evidence: the block at `0x19f89` through `0x1a023` and the helper at
  `0x22018`.
- Observation: the function then tests the cosine of each angle delta against
  0.99995 and the absolute value of each scaled delta against
  `DAT_00118e04 * 200.0`.  When every test passes, the view has settled.  The
  function sets the screen mode 320 by the view distance `+0x90` with the
  horizon at 65 percent, copies the view video mode `+0x8c` to
  `DAT_001100c0`, and clears the flag `+0x58`.
  Evidence: the blocks at `0x1a026` through `0x1a134`, the double at object-3
  offset `0x8e4`, and the float at object-3 offset `0x8ec`.
- Inference: the caller chain, the two transition flags, and the settle test
  support the name `view_transition_update`.  The module records the name with
  the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls `cos` at `0x4da66` and the truncating helper
  at `0x4d9f4`.  This harness links no math library, so
  `view_transition_update_fpu.asm` supplies `cos` from the hardware and makes
  `__CHP` truncate toward zero.  The three constants arrive through globals,
  because the minimal image does not load the constant segment.  The original
  has the EBP frame, so the module builds with `-of+`.
  Evidence: `tools/rebuild/view_transition_update_fpu.asm` and
  `tools/rebuild/modules/view_transition_update.json`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `8` and `D`.  The paths cover the idle path, the delta formation, the
  settled view, the first and third large angle delta, the position limit
  boundary, the negative truncation, the frame factor limit, and the combined
  flags.
  Evidence: `python3 tools/rebuild/harness.py run --module
  view_transition_update`, `work/rebuild/vtran.txt`, and
  `python3 tools/rebuild/run.py run --function 0x19f00`.
- Observation: the rebuilt module holds 579 bytes.  The identity check stays
  open at 22 of 178 instructions and 23 of 577 bytes.  V2 saves the registers
  after the frame, keeps the view pointer in `ecx`, and forms the absolute
  values with branches instead of the original `cdq` sequence.  The build and
  behavior checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x19f00 --count 178 --rebuilt work/rebuild/vtran.exe
  --rebuilt-map work/rebuild/view_transition_update.map --rebuilt-symbol
  view_transition_update_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The opponent_start module at 0x124c0

- Observation: the original body at `0x124c0` holds 164 instructions and 545
  bytes up to the `ret` at `0x126e0`.  The next function starts at `0x126e4`, so
  the store record keeps the 548-byte extent and sets no function size.  The
  body sweep stops earlier, so the record also sets the instruction count 164.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x124c0 --count 164` and
  `tools/rebuild/functions/0x124c0.json`.
- Observation: the three direct callers at `0x15b38`, `0x15b74`, and `0x15ba0`
  are the vehicle groups at `0x15b0c`, `0x15b44`, and `0x15b80`.  The car path
  runs the car proximity pass at `0x17408` first.  The traffic path runs the
  nearby marker at `0x12424` and the traffic collision pass at `0x175dc` first.
  All three groups run this function on the player object at object-3 offset
  `0xf91c`, so the record takes the section `The vehicle state chain`.
  Evidence: the sweep of every candidate function start for a call to
  `0x124c0`, the block at `0x15b25` through `0x15b38`, the block at `0x15b61`
  through `0x15b74`, and the block at `0x15b9d` through `0x15ba0`.
- Observation: the function walks the `DAT_0010d6f8` opponent records of 0x40
  bytes at `DAT_0010d700`.  A record with a set run flag `+0x38` is skipped.
  Otherwise the record position at `+0`, `+4`, and `+8` must stay within 1500
  units of the player position at `+0x4b0`, `+0x4b4`, and `+0x4b8` on every
  axis.  The setup at `0x1517c` writes the same record array from the route
  table `DAT_0010d6f4`, so the records are the opponent cars.
  Evidence: the block at `0x124ca` through `0x12551` and the setup at
  `0x1517c` through `0x152b0`.
- Observation: for a record in range the function sets the contact distance at
  `+0x560` to the player radius `+0x544` plus 100.  It then tests the two
  12-byte player body points at `+0x52c` and `+0x538` against the record
  position and stores the truncated distance at `+0x55c`.  A distance below the
  contact distance by less than 100 units starts the record.
  Evidence: the block at `0x12553` through `0x12641`, the square root at
  `0x22bec`, and the truncating helper at `0x4d9f4`.
- Observation: a started record takes the sine and the negated cosine of the
  player heading `+0x4cc` into `+0x24` and `+0x2c`.  The vertical speed `+0x34`
  takes the player speed `+0x50c` times the double 0.02 at object-3 offset
  `0x418`.  The speed `+0x30` takes the player speed times the double 1.4 at
  object-3 offset `0x420`.  The first start increments the player counter
  `+0x7b8`, sets the flags `+0x3c` and `+0x38`, and calls `table_lookup` at
  `0x1f718` with 5.
  Evidence: the block at `0x12641` through `0x126d4` and the doubles at
  object-3 offsets `0x418` and `0x420`.
- Inference: the caller chain, the opponent record array, the player body
  points, and the start of the nearby record support the name `opponent_start`.
  The module records the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls `sin` at `0x4da72`, `cos` at `0x4da66`,
  `sqrt` at `0x22bec`, and the truncating helper at `0x4d9f4`.  This harness
  links no math library, so `opponent_start_fpu.asm` supplies `sin`, `cos`, and
  `sqrt` from the hardware and makes `__CHP` truncate toward zero.  The two
  scales arrive through globals, because the minimal image does not load the
  constant segment.  The original has the EBP frame, so the module builds with
  `-of+`.
  Evidence: `tools/rebuild/opponent_start_fpu.asm` and
  `tools/rebuild/modules/opponent_start.json`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9` and `D`.  The paths cover the empty list, the three range guards,
  the contact-distance boundary, the first and the second body point, an
  already running record, a record that started before, and the truncation of
  the two distances sqrt(2) and sqrt(13) to 1 and 3.
  Evidence: `python3 tools/rebuild/harness.py run --module opponent_start`,
  `work/rebuild/ostart.txt`, and `python3 tools/rebuild/run.py run --function
  0x124c0`.
- Observation: the rebuilt module holds 509 bytes.  The identity check stays
  open at 18 of 164 instructions and 18 of 509 bytes.  V2 saves the frame
  pointer first, keeps the record pointer in `ecx` and the end pointer in
  `edi`, and forms the absolute values with `neg` instead of the original `cdq`
  sequence.  The build and behavior checks pass, so the store records
  `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x124c0 --count 164 --rebuilt work/rebuild/ostart.exe
  --rebuilt-map work/rebuild/opponent_start.map --rebuilt-symbol
  opponent_start_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The spline_load module at 0x1af70

- Observation: the original body at `0x1af70` holds 148 instructions and 527
  bytes up to the `ret` at `0x1b17e`.  The next function starts at `0x1b180`, so
  the store record keeps the 528-byte extent and sets no function size.  The
  record sets the instruction count 148.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1af70 --count 148` and
  `tools/rebuild/functions/0x1af70.json`.
- Observation: the five direct callers at `0x15088`, `0x1509f`, `0x150b6`,
  `0x150cd`, and `0x150e4` sit in the setup at `0x14fb8`.  The setup builds the
  data names `MAP1opp`, `MAP1anm`, `MAP1cam`, `MAP1rpl`, and `MAP1OST` from the
  prefix `DAT_0011d294` through the formats at object-3 offsets `0x603` through
  `0x627`.  It then calls this function once for each name.  The run-time member
  trace names the five members `MAP1opp.spl`, `MAP1anm.spl`, `MAP1cam.spl`,
  `MAP1rpl.spl`, and `MAP1OST.spl`.
  Evidence: the sweep of every candidate function start for a call to
  `0x1af70`, the block at `0x14fb8` through `0x150f3`, and the member sequence in
  this document.
- Observation: the first `sprintf` at `0x4d32c` builds `%s\data\%s.dat` from the
  base path `DAT_00007f44` and the first argument.  The second `sprintf` builds
  `%s.spl` from the second argument.  The member open at `0x4c08d` opens the
  path with the category `rb` at object-3 offset `0x94e`.  A failed open returns
  0.
  Evidence: the format strings at object-3 offsets `0x938` and `0x947`, the mode
  at `0x94e`, and the block at `0x1af8a` through `0x1afd5`.
- Observation: the first pass reads a four-byte point count, skips the four-byte
  type and the `count * 0x38` data bytes through the seek at `0x4c5c7`, and
  increments the spline count.  The pass ends at the first short read.
  Evidence: the block at `0x1afdd` through `0x1b01c`.
- Observation: the function gets the `GAMESPLINE` resource of `count * 0x24`
  bytes through `0x2eed0` into the third argument.  A zero result runs the
  memory error report at `0x2edad`.  It rewinds the stream and reads each
  0x24-byte record: the count at `+0x1c`, the type at `+0x20`, and the
  `count * 0x38` data bytes into the resource at `+0x00`.  A zero data resource
  runs the memory error report.  The function closes the stream at `0x4c38d` and
  returns the spline count.
  Evidence: the block at `0x1b01e` through `0x1b17e` and the list names at
  object-3 offsets `0x951` and `0x95c`.
- Inference: the five callers, the `.spl` member names, the `GAMESPLINE`
  resource, and the point records support the name `spline_load`.  The module
  records the name with the confidence `inferred`.
  Evidence: the call graph, the member trace, and the data flow above.
- Observation: the harness holds a simulated member, the resource handle, the
  base path, the two name arguments, and the spline records.  The five paths
  cover the two splines, the empty member, the failed open, the zero data
  resource, and the counts 1, 0, and 7.  The output holds the markers `0`
  through `4` and `D`.  The behavior check passes.
  Evidence: `tools/rebuild/spline_load_main.c`,
  `python3 tools/rebuild/harness.py run --module spline_load`, and
  `work/rebuild/spl_out.txt`.
- Observation: the rebuilt module holds 504 bytes.  The identity check stays
  open at 27 of 148 instructions and 29 of 504 bytes.  V2 saves the frame
  pointer first, keeps the record pointer in `esi`, uses `imul` for the record
  stride, and uses other local offsets.  The build and behavior checks pass, so
  the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1af70 --count 148 --rebuilt work/rebuild/splmod.exe
  --rebuilt-map work/rebuild/spline_load.map --rebuilt-symbol spline_load_`
  and `python3 tools/rebuild/run.py run --function 0x1af70`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The sound_emitter_update module at 0x21b9c

- Observation: the original body at `0x21b9c` holds 153 instructions and 524
  bytes up to the `ret` at `0x21da7`.  The next function starts at `0x21da8`, so
  the store record keeps the 524-byte extent and sets no function size.  The
  record sets the instruction count 153.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x21b9c --count 153` and
  `tools/rebuild/functions/0x21b9c.json`.
- Observation: the only direct caller at `0x20b0a` sits in the opponent update
  at `0x20954`.  The caller passes the record and 1 when its own argument is
  zero, so the sound update runs only on that path.
  Evidence: the sweep of every call site for `0x21b9c` and the block at
  `0x20afc` through `0x20b0f`.
- Observation: the function forms the distance from the camera position
  `DAT_0013ccc` and `DAT_0013cd0` and the record position at `+0x20` and
  `+0x28`.  It shifts each axis delta by 3, squares the two values, and scales
  the square root from `0x22bec` by the double 8.0 at object-3 offset `0xd21`.
  Evidence: the block at `0x21bc3` through `0x21c10`.
- Observation: a clear sound at `+0x38` starts below the float 40000.0 at
  object-3 offset `0xd29`.  It stores the truncated distance at `+0x3c` and
  gets the id from the table through `0x1f718` with `DAT_0018d88` plus the
  index at `+0x14`.  A set sound stops through `0x1f880` above 40000.0.
  Evidence: the block at `0x21c13` through `0x21c68`.
- Observation: a running sound copies the float at `+0xc` to `+0x40`, rotates
  the camera delta by `DAT_0013cd4` through `0x2218c`, forms the pitch with the
  float 9.0 at `0xd2d` and 65536.0 at `0xd31`, the squared volume with 32767.0
  at `0xd35`, and the pan with the double 32767.0 at `0xd39`, and sets the
  three values through `0x1f7e4`, `0xc667d`, and `0xc6920`.
  Evidence: the block at `0x21c6b` through `0x21d9b`.
- Observation: the low bit of `DAT_0011d261` and the flag `DAT_0010a8c`
  suppress the three set calls.  The function still rotates and stores the new
  distance.
  Evidence: the test at `0x21d5a` through `0x21d6a`.
- Inference: the caller, the distance gate, the sound id, the pitch, the
  volume, and the pan support the name `sound_emitter_update`.  The module
  records the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the conversions to an integer use the Watcom helper at
  `0x4d9f4`, which truncates toward zero.  The shared `fpu_stub.asm` leaves
  round-to-nearest, so the module uses the local truncating `__CHP` stub in
  `sound_emitter_update_fpu.asm`.
  Evidence: the five helper calls in the body and
  `tools/rebuild/sound_emitter_update_fpu.asm`.
- Observation: the harness holds the record, the camera position and angle, the
  table base and owner, the two flags, the six constants, and the seven callee
  stubs.  The twelve paths cover the two returns, the start, the stop, the far
  clear sound, the running sound, the store gate, the 40000.0 boundary on both
  sound states, the clear mute flag, and the negative pitch.  The output holds
  the markers `0` through `9`, `a`, and `D`.  The behavior check passes.
  Evidence: `tools/rebuild/sound_emitter_update_main.c`,
  `python3 tools/rebuild/harness.py run --module sound_emitter_update`, and
  `work/rebuild/sndemit.txt`.
- Observation: the rebuilt module holds 514 bytes.  The identity check stays
  open at 22 of 153 instructions and 17 of 514 bytes.  V2 saves the frame
  pointer first, keeps the record pointer in `ecx`, folds the two axis squares
  into `imul`, and uses other local offsets.  The build and behavior checks
  pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x21b9c --count 153 --rebuilt work/rebuild/sndemit.exe
  --rebuilt-map work/rebuild/sound_emitter_update.map --rebuilt-symbol
  sound_emitter_update_` and `python3 tools/rebuild/run.py run --function
  0x21b9c`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The dos_datetime_set module at 0x1fa2c

- Observation: the original body at `0x1fa2c` holds 166 instructions and 524
  bytes up to the `ret` at `0x1fc37`.  The next function starts at `0x1fc38`, so
  the store record keeps the 524-byte extent and sets no function size.  The
  record sets the instruction count 166.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1fa2c --count 166` and
  `tools/rebuild/functions/0x1fa2c.json`.
- Observation: the two direct callers at `0x128cf` and `0x15173` sit in
  `engine_shutdown` at `0x128ac` and the menu cleanup at `0x15150`.  Both call
  this function after `0x1f28c` in the teardown path.
  Evidence: the sweep of every call site for `0x1fa2c` and the blocks at
  `0x128ca` through `0x128d4` and `0x1516e` through `0x15178`.
- Observation: the function sets the PIT channel 0 divisor to zero through
  `0x101f6`.  It then reads the real-time clock date with INT 1A AX=0400 and the
  time with INT 1A AX=0200 through `dpmi_call` at `0x4bf4b`.
  Evidence: the blocks at `0x1fa35` through `0x1fb25`.
- Observation: the function converts the BCD month, day, century, year, hour,
  minute, and second to binary.  It forms the full year as `century * 100 +
  year`.
  Evidence: the conversion blocks at `0x1fa5f` through `0x1fb03` and
  `0x1fb2a` through `0x1fb9e`.
- Observation: the function sets the DOS date with INT 21 AH=2B from the year,
  the month, and the day, and the DOS time with INT 21 AH=2D from the hour, the
  minute, and the second.
  Evidence: the blocks at `0x1fba1` through `0x1fbd3` and `0x1fbe8` through
  `0x1fc1c`.
- Observation: a non-zero `AL` from the date call reports the message at
  object-3 offset `0xc08`, which reads `Invalid date specified. Check the
  date!`.  A non-zero `AL` from the time call reports the message at object-3
  offset `0xc30`, which reads `Invalid time specified. Check the time!`.  Both
  go through `0xc656e`.
  Evidence: the reads at `0x100c08` and `0x100c30`, and the tests at `0x1fbd8`
  and `0x1fc21`.
- Inference: the real-time clock read, the BCD conversion, the DOS date and time
  set, and the two messages support the name `dos_datetime_set`.  The module
  records the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the harness holds the real-time clock date and time, the two
  invalid flags, and the DPMI helper stubs.  The six paths cover the dates 2024,
  1999, and 2000, the zero time, the invalid date, the invalid time, and both
  errors.  The output holds the markers `0` through `5` and `D`.  The behavior
  check passes.
  Evidence: `tools/rebuild/dos_datetime_set_main.c`,
  `python3 tools/rebuild/harness.py run --module dos_datetime_set`, and
  `work/rebuild/ddt_out.txt`.
- Observation: the identity check stays open at 30 of 166 instructions and 29 of
  524 bytes.  V2 puts the frame first, saves `esi` and `edi`, uses `imul` for the
  BCD scale, and uses other local offsets.  The build and behavior checks pass,
  so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1fa2c --count 166 --rebuilt work/rebuild/ddtmod.exe
  --rebuilt-map work/rebuild/dos_datetime_set.map --rebuilt-symbol
  dos_datetime_set_` and `python3 tools/rebuild/run.py run --function 0x1fa2c`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The player_car_contact_update module at 0x166a8

- Observation: the original body at `0x166a8` holds 159 instructions and 524
  bytes up to the `ret` at `0x168b3`.  The next function starts at `0x168b4`, so
  the store record keeps the 524-byte extent and sets no function size.  The
  record sets the instruction count 159.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x166a8 --count 159` and
  `tools/rebuild/functions/0x166a8.json`.
- Observation: the only direct caller at `0x15afc` is the second player step
  group at `0x15ab8`.  The group runs on the player object at object-3 offset
  `0xf91c` after `player_car_cornering_update` at `0x16ca0`, and the five view
  groups at `0x2003c`, `0x201d0`, `0x202c0`, `0x20360`, and `0x203c4` call the
  group.
  Evidence: the sweep of every candidate function start for a call to
  `0x166a8`, the call at `0x15afc`, and the block at `0x15ab8` through
  `0x15b04`.
- Observation: the wheel contact step at `0x11a30` walks the same four
  0x54-byte wheel units at `+0x194`.  It stores the first ground contact height
  at `+0x24`, the second ground contact height at `+0x28`, the surface flag at
  `+0x2c`, and the two contact points at `+0x00`, `+0x08`, `+0x0c`, and
  `+0x14`.  The distances `+0x2e4` and `+0x2ec` separate the wheels on the two
  axes.
  Evidence: the loop at `0x11a92` through `0x11bf0`, the stores at `0x11b3f`,
  `0x11b61`, `0x11be7`, `0x11b02`, `0x11b1d`, `0x11ba8`, and `0x11bc5`, and the
  reads at `0x11aaf` and `0x11ac0`.
- Observation: the function copies the two contact heights to `+0x04` and
  `+0x10`.  When the surface flag is set, it adds the signed step from the low
  bits of the two contact coordinates to the first height.  It then writes the
  average raw height `+0x4ec`, the average adjusted height `+0x4b4`, and the
  four body angles `+0x4c8`, `+0x4d0`, `+0x4d4`, and `+0x4dc` through `atan` at
  `0xc2254`.
  Evidence: the loop at `0x166cc` through `0x16734`, the averages at `0x16736`
  through `0x167ad`, and the four angle blocks at `0x167b3` through `0x168b2`.
- Observation: the body height and the four angles feed the view orbit at
  `0x1a3b8` and the view update at `0x16f34`, and the wheel contact step reads
  `+0x4c8` and `+0x4d0` back at `0x11a43` and `0x11a5c`.
  Evidence: the reads at `0x1a459`, `0x1a4bd`, `0x1a517`, and `0x1a568`, and the
  reads at `0x16f44`, `0x16f8e`, and `0x16f9c`.
- Inference: the four wheel contact heights, the two adjusted body heights, the
  four body angles, and the contact loop support the name
  `player_car_contact_update`.  The module records the name with the confidence
  `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the harness holds the player object and the four wheel units.
  The five paths cover the surface clear path, the positive dither, the
  negative dither, the surface gate, and the negative averages.  The output
  holds the markers `0` through `4` and `D`.  The behavior check passes.
  Evidence: `tools/rebuild/player_car_contact_update_main.c`,
  `python3 tools/rebuild/harness.py run --module player_car_contact_update`, and
  `work/rebuild/pccu_out.txt`.
- Observation: the rebuilt module holds 506 bytes.  The identity check stays
  open at 46 of 159 instructions and 12 of 506 bytes.  V2 puts the frame first,
  keeps the object in `ecx`, builds a local helper for the unit pointer,
  divides in extended precision, and uses another block order.  The build and
  behavior checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x166a8 --count 159 --rebuilt work/rebuild/pccu.exe
  --rebuilt-map work/rebuild/player_car_contact_update.map --rebuilt-symbol
  player_car_contact_update_` and `python3 tools/rebuild/run.py run --function
  0x166a8`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The opponent_path_update module at 0x20b1c

- Observation: the original body at `0x20b1c` holds 166 instructions and 516
  bytes up to the `ret 4` at `0x20d1d`.  The next function starts at `0x20d20`,
  so the record keeps the 516-byte extent and sets no function size.  The
  identity check uses 166 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x20b1c --count 166` and
  `tools/rebuild/functions/0x20b1c.json`.
- Observation: the only direct caller at `0x20a12` is `opponent_update` at
  `0x20954`.  It passes the state base `DAT_0010f174`, the path record at
  `+0x44`, and the float at `+0xc` scaled by `DAT_00118e04`.  The caller is in
  the object_update_all group, so the module takes the section
  object_update_all callees.
  Evidence: the call at `0x20a12` and `tools/rebuild/functions/0x20954.json`.
- Observation: the path record holds the track index at `+0`, the node index at
  `+4`, the segment phase at `+8`, and the direction flag at `+0xc`.  The callee
  at `0x222e0` returns the curve speed from the control block at `+0x3c` and the
  phase.  The phase advances by the frame step divided by the speed.
  Evidence: the speed call at `0x20b35`, the divide at `0x20b3a`, and the add at
  `0x20b40`.
- Observation: when the phase reaches one, the function clears the phase.  A
  clear direction flag sets the flag and holds the node.  A set direction flag
  clears the flag and moves the node index through the next field at `+0x10` of
  the 56-byte node.
  Evidence: the gate at `0x20b49` through `0x20b51`, the flag block at
  `0x20b64` through `0x20b71`, and the node block at `0x20b73` through
  `0x20bab`.
- Observation: the callee at `0x22378` evaluates the spline segment and writes
  the target position and angle at `+0x60`, `+0x64`, and `+0x68`, and the next
  control values at `+0x6c` and `+0x70`.  The function subtracts the distance
  from the integer position `+0x10` and `+0x18` to the target from the frame
  step, and it truncates the target into `+0x10`, `+0x14`, and `+0x18` through
  the helper at `0x4d9f4`.
  Evidence: the evaluation calls at `0x20bb6` and `0x20c19`, the distance block
  at `0x20bbe` through `0x20be7`, and the truncation blocks at `0x20bea`
  through `0x20c0c` and `0x20c1e` through `0x20c4e`.
- Observation: the segment path copies the history values, takes the arc sine
  from `0xc5e42` of the new lateral value at `+0x1c`, and forms the heading at
  `+0x34` with pi/2 at object-3 offset `0xce1` or 2.5 pi at `0xce5`.  It wraps
  the heading into one turn with 2 pi at `0xce9` and -2 pi at `0xced`.
  Evidence: the copy block at `0x20c51` through `0x20c8a`, the arc sine at
  `0x20c99`, the two heading branches at `0x20cb1` through `0x20cca`, and the
  two wrap loops at `0x20cd9` through `0x20d17`.
- Inference: the phase advance, the node walk, the spline evaluation, and the
  heading form a path follower for the opponent records.  The caller and the
  data flow support the name `opponent_path_update`.  The module records the
  name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the harness holds the path record, the state base, the node
  block, the four constants, and the two curve stubs.  The five paths cover the
  in-segment phase, the segment end with a clear direction flag, the segment
  end with a set direction flag, the negative lateral branch, and the exact
  phase one.  The output holds the markers `0` through `4` and `D`.  The behavior
  check passes.
  Evidence: `tools/rebuild/opponent_path_update_main.c`,
  `python3 tools/rebuild/harness.py run --module opponent_path_update`, and
  `work/rebuild/opppath.txt`.
- Observation: the rebuilt module holds 483 bytes and 166 instructions.  The
  identity check stays open at 17 of 166 instructions and 18 of 483 bytes.  V2
  puts the frame before the register saves, keeps the state in `ebx` and the
  path in `ecx`, folds the node strides into `imul`, keeps the distance parts in
  local floats, and calls the absolute value as a helper.  The build and
  behavior checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x20b1c --count 166 --rebuilt work/rebuild/opppath.exe
  --rebuilt-map work/rebuild/opponent_path_update.map --rebuilt-symbol
  opponent_path_update_` and `python3 tools/rebuild/run.py run --function
  0x20b1c`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The member_list_load module at 0x1b394

- Observation: the original body at `0x1b394` holds 166 instructions and 502
  bytes up to the `ret` at `0x1b589`.  The next function starts at `0x1b58c`,
  so the record keeps the 504-byte extent and sets no function size.  The
  identity check uses 166 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1b394 --count 166` and
  `tools/rebuild/functions/0x1b394.json`.
- Observation: the four direct callers at `0x29e6f`, `0x29e83`, `0x29e9c`, and
  `0x29eb5` sit in `menu_space_init` at `0x29d08`.  They pass the names `menu`,
  `cardat`, `trackdat`, and `champ` and the pointer arrays at `0x1fb80`,
  `0x1fd10`, `0x1fea0`, and `0x20030`.  The two further callers at `0x13435`
  and `0x134c5` in `race_data_load` at `0x131a8` pass the `game` list and the
  array at `DAT_00118f94`.  `menu_space_init` is in the frame_update callees
  group, so the module takes the section frame_update callees.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x29e40 --count 45`, the
  block at `0x13420`, and `work/ghidra/phaseF_131a8.txt`.
- Observation: the function builds `%s\data\local.dat` from the base path at
  `DAT_00007f44` and the member name `%s.txt` from the first argument.  It
  opens the member with the category `rt` through `0x4c08d`.  A failed open
  runs the file error report at `0x4bf88`.
  Evidence: the `sprintf` calls at `0x1b3c3` and `0x1b3e0`, the `fopen` call at
  `0x1b3f4`, and the strings at object-3 offsets `0x9c9`, `0x9db`, and `0x9e2`.
- Observation: the first pass reads the text with the line mode 1 through
  `0x4c695` and sums the byte size of every line that does not start with `*`.
  It gets the `TEXT` resource of that size through `0x2eed0` into
  `DAT_000062ac`.  A zero result runs the memory error report at `0x2edad`.
  Evidence: the size loop at `0x1b408` through `0x1b446`, the `resource_get`
  call at `0x1b455`, the string at object-3 offset `0x9e5`, and the null test
  at `0x1b45d`.
- Observation: the function rewinds the stream through `0x4c5c7`, copies every
  line that does not start with `*` into the resource, and stores its pointer in
  the second argument.  It terminates the pointer array with -1, closes the
  stream through `0x4c38d`, and returns the resource pointer.
  Evidence: the rewind call at `0x1b477`, the copy blocks at `0x1b4b7` through
  `0x1b55c`, the sentinel store at `0x1b56a`, and the close call at `0x1b576`.
- Inference: the five text lists, the `local.dat` member, and the `TEXT`
  resource support the name `member_list_load`.  The function loads a named text
  list from `local.dat` and fills a pointer array.  The module records the name
  with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the harness holds the base path, the resource handle, the
  resource buffer, the simulated member text, and the pointer array.  The five
  paths cover the normal path with a skipped `*` line and a copied `#` line,
  the failed open, the zero resource, the single copied line with the `game`
  name, and the empty member.  The output holds the markers `0` through `4` and
  `D`.  The behavior check passes.
  Evidence: `tools/rebuild/member_list_load_main.c`,
  `python3 tools/rebuild/harness.py run --module member_list_load`, and
  `work/rebuild/mlist.txt`.
- Observation: the rebuilt module holds 158 instructions and 535 bytes.  The
  identity check stays open at 26 of 166 instructions and 24 of 502 bytes.  V2
  saves the registers after the frame, keeps the name in `esi` and the array in
  local memory, builds the constant text in local buffers for the harness, and
  uses different local offsets.  The build and behavior checks pass, so the
  store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1b394 --count 166 --rebuilt work/rebuild/memlist.exe
  --rebuilt-map work/rebuild/member_list_load.map --rebuilt-symbol
  member_list_load_` and `python3 tools/rebuild/run.py run --function 0x1b394`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The opponent_route_setup module at 0x1517c

- Observation: the original body at `0x1517c` holds 137 instructions and 499
  bytes up to the `ret` at `0x1536e`.  The function continues through the `ret`
  at `0x15377`, so the record keeps the 508-byte extent to the next function at
  `0x15378` and sets no function size.  A branch at `0x152f8` jumps over the
  first `ret` to the common block at `0x151ba`, so the identity check uses 137
  instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1517c --count 137` and
  `tools/rebuild/functions/0x1517c.json`.
- Observation: the only direct caller at `0x1354c` is `race_track_load` at
  `0x13540`.  `race_session_start` at `0x128e0` calls `0x13540` at `0x1294e`
  and drops a non-zero result.  The level loader at `0x14fb8` fills the spline
  table `DAT_0010d6f4` through `spline_load` at `0x1af70` and stores the spline
  count at `DAT_0010d6fc`.
  Evidence: the byte sweep for a relative call to `0x1517c`, the call at
  `0x1294e`, and the loader calls at `0x150e4` and `0x150e9`.
- Observation: the first spline record holds the point count at `+0x1c`, the
  first node index at `+0x20`, and the point array pointer at `+0x00`.  The
  function copies the count to `DAT_0010d6f8` and clears it when
  `DAT_0010d6fc` is zero.
  Evidence: the loads at `0x15188`, `0x15190`, and `0x151a5`, and the store at
  `0x1519f`.
- Observation: the function walks the 0x38-byte point records from the first
  node index.  A point holds x at `+0x04`, z at `+0x08`, the model index at
  `+0x0c`, and the next node index at `+0x10`.
  Evidence: the point loads at `0x151d4`, `0x15201`, `0x1531e`, and `0x152c7`.
- Observation: a non-zero load flag forms the model name at
  `0xf000 + model * 0x24 + (DAT_0011d274 & 3) * 0xc` and loads it from the
  object table at `0x6530` through `0x11060`.  The load stores the object slot
  at `+0x18` of the record.  A non-zero result returns at once.
  Evidence: the block at `0x152fe` through `0x15365`.
- Observation: the function fills one 0x40-byte record at
  `DAT_0010d700 + index * 0x40`.  The record takes x at `+0x00` and z at
  `+0x08` from the point, the ground height at `+0x04` from the map collision
  check at `0x11c00` through `DAT_0010c72c`, the track heading at `+0x10` from
  `track_nearest_node` at `0x120c0` through `DAT_00108048`, and the angles at
  `+0x0c` and `+0x14` from the rotation of `DAT_0010c734` and `DAT_0010c73c`
  through `0x2218c` by the heading minus pi/2, scaled by pi/46080.  The function
  sets the flags `+0x20` and `+0x38` and clears `+0x3c`.
  Evidence: the block at `0x151ba` through `0x152b0` and the constants at
  object-3 offsets `0x633`, `0x637`, and `0x63b`.
- Observation: the function converts an int to a float only.  It does not
  convert a float to an int, so it needs no truncating `__CHP` stub.  The shared
  `fpu_stub.asm` supplies the FPU run-time symbols and the float-used marker.
  Evidence: the `fild` and `fstp` instructions at `0x15269` through `0x15290`
  and `tools/rebuild/fpu_stub.asm`.
- Inference: the opponent record array, the route spline, and the object slot
  support the name `opponent_route_setup`.  The module records the name with the
  confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the harness holds the spline table, four point records, the
  record array, the height and heading outputs, the rotation pair, and the four
  callee stubs.  The nine paths cover the zero spline count, the no-load path,
  the load success, the load error, the next chain order, the state mask 3, the
  negative positions and angles, and the error on the second record.  The output
  holds the markers `0` through `7` and `D`.  The behavior check passes.
  Evidence: `tools/rebuild/opponent_route_setup_main.c`,
  `python3 tools/rebuild/harness.py run --module opponent_route_setup`, and
  `work/rebuild/orset.txt`.
- Observation: the rebuilt module holds 137 decoded instructions and 421 bytes.
  The identity check stays open at 29 of 137 instructions and 24 of 421 bytes.
  V2 puts the frame before the register saves, uses another local layout, and
  forms the point and name offsets with another instruction order.  The build
  and behavior checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1517c --count 137 --rebuilt work/rebuild/orset.exe
  --rebuilt-map work/rebuild/opponent_route_setup.map --rebuilt-symbol
  opponent_route_setup_` and `python3 tools/rebuild/run.py run --function
  0x1517c`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The car_collision_pass module at 0x1d00c

- Observation: the original body at `0x1d00c` holds 144 instructions and 497
  bytes up to the `ret` at `0x1d1fc`.  The next function starts at `0x1d200`, so
  the store record keeps the 500-byte extent and sets no function size.  The
  identity check uses 144 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1d00c --count 144` and
  `tools/rebuild/functions/0x1d00c.json`.
- Observation: the only direct caller at `0x1c7fc` is the per-object step at
  `0x1c7d0`.  That step first marks the nearby objects at `0x126e4`, then runs
  this function, and then runs `opponent_avoid_player` at `0x1e414`.  The object
  update at `0x2003c` runs the step on each 0x590-byte car record at
  `DAT_00110ab8` after `object_prepare`.  The record therefore takes the section
  `object update callees`.
  Evidence: the byte sweep for a relative call to `0x1d00c`, the block at
  `0x1c7d0` through `0x1c80c`, and the loop at `0x20168` through `0x20187`.
- Observation: the function reads the record number at `+0x00` and scans every
  later car record up to the count `DAT_0010f8ec`.  For each candidate it stores
  the contact distance at `+0x488` as the two radii at `+0x44c`.  The bit of the
  candidate index in the mask at `+0x0c` selects the record, and the physics
  gate at `+0x414` must be set.
  Evidence: the block at `0x1d016` through `0x1d192`.
- Observation: the function tests the two body points of the record against the
  two body points of the candidate.  A point holds x at `+0x00`, y at `+0x04`,
  and z at `+0x08`, and the point array starts at `+0x434`.  The function
  stores the two deltas at `+0x48c` and `+0x494`, squares and adds them, takes
  the square root through `0x22bec`, truncates it through `0x4d9f4`, and stores
  the distance at `+0x484`.  A distance below the contact distance calls the
  collision response at `0x1d200` with the record in `eax` and the candidate in
  `edx`.
  Evidence: the block at `0x1d05b` through `0x1d135`.
- Observation: after the scan, a drop from the saved speed at `+0x178` to the
  current speed at `+0x418` above 5000.0 sets the crash state at `+0x2f8` to 2
  or 3 when the state is zero.  The low bit of the quarter of the x and z
  positions at `+0x498` and `+0x4a0` selects the state.  The threshold is the
  float 5000.0 at object-3 offset `0xb18`.
  Evidence: the block at `0x1d1a5` through `0x1d1ef` and the constant at
  object-3 offset `0xb18`.
- Inference: the record scan, the body-point overlap test, the radius gate, and
  the crash state support the name `car_collision_pass`.  The module records the
  name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original takes the square root at `0x22bec` and truncates
  through the helper at `0x4d9f4`.  This harness links no math library, so
  `car_collision_pass_fpu.asm` supplies `sqrt` from the hardware and makes
  `__CHP` truncate toward zero.  The original has the EBP frame, so the module
  builds with `-of+`.
  Evidence: `tools/rebuild/car_collision_pass_fpu.asm` and
  `tools/rebuild/modules/car_collision_pass.json`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9`, `A`, `B`, and `D`.  The paths cover the empty scan, the mask
  skip, the clear physics gate, the first close pair, the truncation of 8 to 2,
  the equal distance, the start at the record number plus one with an inactive
  candidate, the crash state 2, the set state, the exact 5000.0 boundary, the
  positive parity, and the negative parity.
  Evidence: `python3 tools/rebuild/harness.py run --module car_collision_pass`,
  `work/rebuild/ccpass.txt`, and `python3 tools/rebuild/run.py run --function
  0x1d00c`.
- Observation: the rebuilt module holds 144 decoded instructions and 502 bytes.
  The identity check stays open at 25 of 144 instructions and 25 of 497 bytes.
  V2 saves the frame pointer first, uses another prologue order, and uses
  registers for the loops.  The build and behavior checks pass, so the store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1d00c --count 144 --rebuilt work/rebuild/ccpass.exe
  --rebuilt-map work/rebuild/car_collision_pass.map --rebuilt-symbol
  car_collision_pass_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The track_select_update module at 0x2cc98

- Observation: the original body at `0x2cc98` holds 124 instructions and 491
  bytes up to the `ret` at `0x2ce82`.  The next function starts at `0x2ce84`,
  so the store record keeps the 492-byte extent and sets no function size.  The
  identity check uses 124 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2cc98 --count 124` and
  `tools/rebuild/functions/0x2cc98.json`.
- Observation: the only direct caller at `0x295dd` is case 3 of
  `race_state_update` at `0x29564`.  The jump table at linear `0x29540` maps
  case 3 to `0x295dd`.  `race_state_update` is a `race_frame` callee at
  `0x2807c`, so the record takes the section `race_frame callees`.
  Evidence: the call sweep for `0x2cc98`, the jump table at linear `0x29540`,
  and the calls at `0x29564` and `0x2807c`.
- Observation: the matching state 3 init is `track_select_screen` at `0x2c188`,
  which builds the 12 map records at `DAT_0011d908` with stride 0x34.  The
  update walks the same records, and `race_panel_3` at `0x2e58c` reads the
  table.
  Evidence: the loop at `0x2ccdf` through `0x2ce77` and the init in
  `tools/rebuild/track_select_screen.c`.
- Observation: the function adds `DAT_0018e08 * pi/4` at object-3 offset
  `0x11e3` to the heading at `+0x0c` of each record.  It resets the heading to
  0 at or above 2*pi at object-3 offset `0x11e7`.  It converts the record
  angles at `+0x08`, `+0x0c`, and `+0x10` to the object angles at `+0x14`,
  `+0x18`, and `+0x1c` with the factor 1024.0 / pi and the mask 0x7ff.  The
  object pointer comes from `DAT_0010653c` with the record object index at
  `+0x00` and the stride 0x1c.
  Evidence: the loop at `0x2ccef` through `0x2ce77` and the constants at
  object-3 offsets `0x11e3`, `0x11e7`, `0x11eb`, and `0x11ef`.
- Observation: when the mode `DAT_0011d260` is 0x10, the function calls
  `net_player_add` with 3, swaps `DAT_001203c8` with `DAT_00120b5c`, calls
  `net_player_add` with 0x8844, and restores the cell.  `net_race_lobby` uses
  the same 0x8844 event and the same two selection cells.
  Evidence: the block at `0x2cca1` through `0x2ccd3`, the calls at `0x2ccaf`
  and `0x2cccb`, and `tools/rebuild/net_race_lobby.c`.
- Inference: the state 3 position after the state 2 car selection, the shared
  record table, the object angle writes, and the network selection handshake
  support the name `track_select_update`.  The module records the name with
  the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls the truncate helper at `0x4d9f4` before each
  `fistp`.  The module uses the local `track_select_update_fpu.asm`, because
  the shared `fpu_stub.asm` leaves the round-to-nearest mode.  The constants
  arrive through globals, because the minimal image does not load the constant
  segment.  The original has the EBP frame, so the module builds with `-of+`.
  Evidence: `tools/rebuild/track_select_update_fpu.asm`,
  `tools/rebuild/track_select_update.c`, and the FPU blocks at `0x2cd68`,
  `0x2cdc8`, and `0x2ce28`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `7` and `D`.  The paths cover the normal path with the record 12
  sentinel, the network handshake, the frame step, the 2*pi reset, the
  below-2*pi case, the positive and negative truncation, and the object index
  stride.
  Evidence: `python3 tools/rebuild/harness.py run --module track_select_update`,
  `work/rebuild/tsupd.txt`, and
  `python3 tools/rebuild/run.py run --function 0x2cc98`.
- Observation: the rebuilt module holds 412 bytes and 124 instructions.  The
  identity check stays open at 22 of 124 instructions and 38 of 412 bytes
  because V2 saves the frame pointer first, uses another prologue order, and
  uses a register loop counter.  The build and behavior checks pass, so the
  store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2cc98 --count 124 --rebuilt work/rebuild/tsupd.exe
  --rebuilt-map work/rebuild/track_select_update.map --rebuilt-symbol
  track_select_update_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The palette_shade_build module at 0x258d0

- Observation: the original body at `0x258d0` holds 151 instructions and 475
  bytes up to the `ret` at `0x25aaa`.  The next function starts at `0x25aac`, so
  the store record keeps the 476-byte extent and sets no function size.  The
  identity check uses 151 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x258d0 --count 151` and
  `tools/rebuild/functions/0x258d0.json`.
- Observation: the five direct callers pass the palette at `0xcf5c` in `eax`
  after a palette load.  The callers are `race_session_start` at `0x12a85`,
  `loading_screen` at `0x2a44a` and `0x2a638`, and `name_entry_screen` at
  `0x2c90b` and `0x2c9fe`.
  Evidence: the call sweep for `0x258d0` and the `mov eax, 0xcf5c` at
  `0x12a80`, `0x2a440`, `0x2a633`, `0x2c900`, and `0x2c9f3`.
- Observation: the function builds two 256-byte lookup tables.  For each of the
  two scales 0.75 and 0.6 and each of the 256 palette entries, it scales the red,
  green, and blue bytes, clamps each value at 255.0, finds the nearest palette
  entry by the squared distance, and stores the index at `0x1cc00 + plane *
  0x100`.  The local scale array also holds 0.5 at `0x3f000000`, but the loop
  bound 2 leaves the third value unused.  The best-distance sentinel is
  22100000.0 at `0x4ba89c10`.
  Evidence: the loops at `0x258d8` through `0x25aa0` and the constants at
  `0x258db`, `0x258e2`, `0x258e9`, and `0x25a0d`.
- Observation: the function sets the pointer cell `DAT_0015d1c` to `0x1cc00`
  and the pointer cell `DAT_000bcddc` to `0x1cd00`.  The pixel remap at
  `0xccd9a` reads the `DAT_000bcddc` table and the block at `0x25d53` reads the
  `DAT_0015d1c` table, so the tables are the 0.6 and 0.75 shade maps of the
  framebuffer.
  Evidence: the stores at `0x258f7` and `0x25910`, the reads at `0xccd99` and
  `0x25d53`, and the remap at `0xccda0` through `0xccda4`.
- Inference: the callers, the palette argument, the two dim scales, and the two
  shade maps support the name `palette_shade_build`.  The module records the
  name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the function converts only an integer to a float, so it needs no
  truncating `__CHP` stub.  The shared `fpu_stub.asm` supplies the FPU run-time
  symbols and the float-used marker.  The clamp constant `255.0` compiles to the
  constant pool at linear 0, which the minimal image does not load, so the
  module reads the constant through the global `palette_clamp`.  The original
  has the EBP frame, so the module builds with `-of+`.
  Evidence: `tools/rebuild/palette_shade_build.c`,
  `tools/rebuild/palette_shade_build_main.c`, and the `fcomp` operand at
  `0x1008d` of `work/rebuild/psbmod.exe`.
- Observation: the behavior check passes.  The harness holds the 0x300-byte
  palette, the 0x200-byte table region, and the clamp 255.0.  The paths cover
  the sampled 0.75 and 0.6 indices, the colored exact-match mapping, the
  non-decreasing 0.75 table, and the 0.6 table at or below the 0.75 table.  The
  output holds the markers `0` through `4` and `D`.
  Evidence: `python3 tools/rebuild/harness.py run --module palette_shade_build`,
  `work/rebuild/psb_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x258d0`.
- Observation: the rebuilt function body holds 117 instructions and 360 bytes.
  The identity window uses the 151 original instructions, so the check stays
  open at 18 of 151 instructions and 19 of 462 bytes because V2 saves five
  registers, uses another prologue order, and uses registers for the two loops.
  The build and behavior checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x258d0 --count 151 --rebuilt work/rebuild/psbmod.exe
  --rebuilt-map work/rebuild/palette_shade_build.map --rebuilt-symbol
  palette_shade_build_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The hud_panel_show module at 0x27a5c

- Observation: the original body at `0x27a5c` holds 135 instructions and 471
  bytes up to the `ret` at `0x27c32`.  The next function starts at `0x27c34`,
  so the store record keeps the 472-byte extent and sets no function size.  The
  identity check uses 135 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x27a5c --count 135` and
  `tools/rebuild/functions/0x27a5c.json`.
- Observation: the only direct caller at `0x27a40` is `hud_update` at `0x27a34`.
  The `object_update_all` frame step at `0x2786c` calls `hud_update` at the end
  of the object pass, so the record takes the section
  `object_update_all callees`.
  Evidence: the byte sweep for a relative call to `0x27a5c` and the block at
  `0x27968` through `0x27995`.
- Observation: the function reads the state `DAT_0011ce54` and the toggle
  `DAT_0011ce40`.  A state below 2 centers the viewport record
  `DAT_0015ccc + state * 0x14` on its width at `+8`.  The state 0x12 with the
  toggle set centers the record 2 width at `+0x30`.  Every other state centers
  the record 2 width at `+0x30`.  The center is
  `(0x140 - width / 2) / 2`.
  Evidence: the block at `0x27a67` through `0x27ac2` and the block at `0x27b1b`
  through `0x27bc4`.
- Observation: the filter at `0x22b80` moves `DAT_0011ce4c` toward the target at
  the index 0x50 for a state below 2 and for the state 0x12 with the toggle set.
  The target is `-(DAT_0011ce40 * 0x140)` for a state below 2 and -340.0 for
  the state 0x12.  Every other state moves toward 0.0 at the index 0x5a.
  Evidence: the calls at `0x27af1`, `0x27b64`, and `0x27bcf`, and the constants
  at `0x27b4c` and `0x27b54`.
- Observation: the function stores the doubled rounded center at
  `DAT_0011cb18`, the message row at `DAT_0011cb1c`, and the state or 2 at
  `DAT_0011cb20`.  It sets `DAT_0011cb14` to 1 and `DAT_0011cb24` to 0, and
  calls `panel_update` at `0x25af0`.
  Evidence: the stores at `0x27b0c` through `0x27c20` and the call at
  `0x27c25`.
- Inference: the caller `hud_update`, the panel fields `DAT_0011cb14` through
  `DAT_0011cb24`, and the `panel_update` call support the name
  `hud_panel_show`.  The module records the name with the confidence
  `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls the truncate helper at `0x4d9f4` before each
  `fistp`.  This harness links no math library, so
  `hud_panel_show_fpu.asm` makes `__CHP` truncate toward zero.  The float
  constants arrive through globals, because the minimal image does not load the
  constant segment.  The original has the EBP frame, so the module builds with
  `-of+`.
  Evidence: `tools/rebuild/hud_panel_show_fpu.asm` and
  `tools/rebuild/modules/hud_panel_show.json`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `5` and `D`.  The paths cover the state 0, the state 1, the state
  0x12 with the toggle set, the state 5, the fractional truncation, and the
  state 0x12 with the toggle clear.
  Evidence: `python3 tools/rebuild/harness.py run --module hud_panel_show`,
  `work/rebuild/hps_out.txt`, and
  `python3 tools/rebuild/run.py run --function 0x27a5c`.
- Observation: the rebuilt function body holds 110 instructions and 376 bytes.
  The identity window uses the 135 original instructions, so the check stays
  open at 22 of 135 instructions and 14 of 440 bytes because V2 saves a
  different register set, uses another prologue order, uses `cdq` for the sign
  mask, and uses `imul` for the viewport record.  The build and behavior checks
  pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x27a5c --count 135 --rebuilt work/rebuild/hpshow.exe
  --rebuilt-map work/rebuild/hud_panel_show.map --rebuilt-symbol
  hud_panel_show_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The credits_scroll module at 0x27c34

- Observation: the original body at `0x27c34` holds 133 instructions and 466
  bytes up to the `ret` at `0x27e05`.  The next function starts at `0x27e08`,
  so the store record keeps the 468-byte extent and sets no function size.  The
  identity check uses 133 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x27c34 --count 133` and
  `tools/rebuild/functions/0x27c34.json`.
- Observation: the only direct caller at `0x27a45` is `hud_update` at
  `0x27a34`.  The `object_update_all` frame step at `0x2786c` calls
  `hud_update` at the end of the object pass, so the record takes the section
  `object_update_all callees`.
  Evidence: the byte sweep for a relative call to `0x27c34` and the block at
  `0x27a34`.
- Observation: the pointer table `DAT_0011cef8` holds the pages of the credits
  text.  `engine_init` calls `member_list_load` at `0x1b394` with the member
  name `attr` at object-3 offset `0x47b`, and `member_list_load` reads that
  member from `data\local.dat`.  The index `DAT_0011ce3c` holds the first line
  of the page.  A line that starts with `#` ends the page.  The function walks
  up to 0x14 lines from the index.
  Evidence: the call at `0x1302e`, the string at object-3 offset `0x47b`, and
  `python3 tools/gfx_container.py lzw DATA/LOCAL.DAT`.
- Observation: the integer filter at `0x22bb4` moves the position
  `DAT_0011ce58[i]` toward `-(DAT_0011ce40 * 0x190)` at the coefficient
  `DAT_0010cf08[0x5a]`, unless the position sits at or below -0x168, which sets
  the timer `DAT_0011ce48` to 4.0.  The function measures each line through
  `text_measure` at `0x254a8`, centers it on the 0x140 width, and draws it
  through `text_draw` at `0x25020` at the doubled position and the row
  `i * 0x28` with the font 0 and the color 2.
  Evidence: the calls at `0x27c95`, `0x27cb8`, and `0x27d0f`, and the filter
  body at `0x22bb4`.
- Observation: after the page the timer grows by the frame delta
  `DAT_00118e04`.  A timer below 4.0 returns.  Otherwise the timer clears and
  the toggle `DAT_0011ce40` flips.  The flip from 1 to 0 advances
  `DAT_0011ce3c` past the `#` line, increments the page state
  `DAT_0011ce54`, and resets the 0x14 positions to 0x15e.  The -1 sentinel sets
  the session state `DAT_0011ce44` to 3, clears the page state and the index,
  and resets the positions to 0x15e and the cells `DAT_0011cea8` to 0x64.
  Evidence: the block at `0x27d2f` through `0x27dfd`.
- Inference: the caller `hud_update`, the credits text from `local.dat`, and
  the page walk support the name `credits_scroll`.  The module records the name
  with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the target DOS/4G build leaves the initialized data pages zero
  under DOSBox-X, so the harness fills the line table in the BSS at run time.
  The seven paths cover the first page below the timer, the low line on the
  second page, the toggle 1 to 0 advance, the toggle 0 to 1 hold, the -1
  sentinel, the width 40 center, the coefficient 0.5 filter with the timer
  boundary, and the timer 2.0 below the bound.  The output holds the markers
  `0` through `6` and `D`.
  Evidence: `tools/rebuild/credits_scroll_main.c` and
  `work/rebuild/credits.txt`.
- Observation: the behavior check passes.  The rebuilt function body holds 98
  instructions and 370 bytes.  The identity
  window uses the 133 original instructions, so the check stays open at 30 of
  133 instructions and 22 of 466 bytes because V2 saves the frame pointer
  first, uses another prologue order, and uses integer immediates for the
  float constants.  The build and behavior checks pass, so the store records
  `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x27c34 --count 133 --rebuilt work/rebuild/credits.exe
  --rebuilt-map work/rebuild/credits.map --rebuilt-symbol credits_scroll_`
  and `python3 tools/rebuild/run.py run --function 0x27c34`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The car_proximity_pass module at 0x17408

- Observation: the original body at `0x17408` holds 133 instructions and 466
  bytes up to the `ret` at `0x175d9`.  The next function starts at `0x175dc`, so
  the store record keeps the 468-byte extent and sets no function size.  The
  identity check uses 133 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x17408 --count 133` and
  `tools/rebuild/functions/0x17408.json`.
- Observation: the only direct caller at `0x15b30` is the player effect group
  at `0x15b08`.  That group calls the nearby marker at `0x12378`, then this
  function, and then the opponent start at `0x124c0`.  The traffic path has the
  same shape at `0x15b44` with the traffic collision pass at `0x175dc`.
  Evidence: the byte sweep for a relative call to `0x17408` and the block at
  `0x15b08` through `0x15b40`.
- Observation: the function returns at once when the state field `+0x148` is
  not zero.  Otherwise it walks the 0x590-byte car records at `DAT_00110ab8`
  while the index stays below the count `DAT_0010f8ec`.  For each record it
  sets the contact distance at `+0x560` to the player radius `+0x544` plus the
  record radius `+0x44c`.
  Evidence: the block at `0x1741a` through `0x17571`.
- Observation: the bit of the index in the nearby mask at `+0x104` selects the
  record, the physics gate at `+0x414` must be set, and the record state at
  `+0x2f8` must be zero.  The function then tests the two 12-byte player body
  points at `+0x52c` and `+0x534` against the two record body points at
  `+0x434` and `+0x43c`.
  Evidence: the block at `0x17577` through `0x174ae`.
- Observation: each pair stores its deltas at `+0x564` and `+0x56c`, squares
  and adds them, takes the square root through `0x22bec`, truncates it through
  `0x4d9f4`, and stores the distance at `+0x55c`.  A distance below the contact
  distance sets the contact flag, calls the collision response at `0x17828`
  with the player in `eax` and the record in `edx`, and ends the second-point
  pass for that first point.
  Evidence: the block at `0x174c3` through `0x17528`.
- Observation: the last step filters `+0x7c4` toward the contact flag through
  `filter_update` at `0x22b80` with the rate 50.
  Evidence: the call at `0x175cd`.
- Inference: the caller player effect group, the car record scan, the two
  body-point pairs, and the collision response support the name
  `car_proximity_pass`.  The module records the name with the confidence
  `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original calls `sqrt` at `0x22bec` and the truncating helper
  at `0x4d9f4`.  This harness links no math library, so
  `car_proximity_pass_fpu.asm` supplies `sqrt` from the hardware and makes
  `__CHP` truncate toward zero.  The original has the EBP frame, so the module
  builds with `-of+`.
  Evidence: `tools/rebuild/car_proximity_pass_fpu.asm` and
  `tools/rebuild/modules/car_proximity_pass.json`.
- Observation: the behavior check passes.  The harness prints the markers `0`
  through `9`, `A`, `B`, and `D`.  The paths cover the state gate, the empty
  count, the mask skip, the clear physics gate, the set record state, the first
  close pair, the response on each player point, the equal distance, the
  truncation of 2.828 to 2, the second record, and the filter move to 100 and
  100.5.
  Evidence: `python3 tools/rebuild/harness.py run --module
  car_proximity_pass`, `work/rebuild/cprox.txt`, and
  `python3 tools/rebuild/run.py run --function 0x17408`.
- Observation: the rebuilt module holds 488 bytes and 133 decoded instructions.
  The identity check stays open at 19 of 133 instructions and 29 of 466 bytes
  because V2 saves the frame pointer first, uses another prologue order, and
  keeps the loop counters in registers.  The build and behavior checks pass, so
  the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x17408 --count 133 --rebuilt work/rebuild/cprox.exe
  --rebuilt-map work/rebuild/car_proximity_pass.map --rebuilt-symbol
  car_proximity_pass_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The player_state_snapshot module at 0x1ed04

- Observation: the original body at `0x1ed04` holds 115 instructions and 465
  bytes up to the `ret` at `0x1eed4`.  The next function starts at `0x1eed8`, so
  the store record keeps the 468-byte extent and sets no function size.  The
  identity check uses 115 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1ed04 --count 115` and
  `tools/rebuild/functions/0x1ed04.json`.
- Observation: the only direct caller at `0x1ec88` is the mode-10 step body at
  `0x1ec7c`.  That body loads the player object at object-3 offset `0xf91c` in
  `eax` and then calls this function.  The mode-10 step at `0x1eae0` calls the
  body at `0x1eb27` and `0x1eb64`, and the race session start at `0x12d0c`
  calls the step.
  Evidence: the direct call sweep for `0x1ed04`, the body at `0x1ec7c` through
  `0x1ed01`, and the calls at `0x1eb27` and `0x1eb64`.
- Observation: the function copies the current player fields to the snapshot
  block.  The dword at `+0x7e8` takes the global at `0x11521c`, `+0x7ec` takes
  the global at `0x11d250`, `+0x7f4` takes the field at `+0x7b0`, `+0x7f8`
  takes the global at `0x11d2a0`, `+0x7fc` takes the field at `+0x684`, `+0x800`
  takes the field at `+0x688`, `+0x804` takes the field at `+0x680`, and
  `+0x808` takes the field at `+0x78c`.
  Evidence: the block at `0x1ed0f` through `0x1ede2`.
- Observation: the three view-offset sums store `+0x4b0 + +0x130`,
  `+0x4b4 + +0x134`, and `+0x4b8 + +0x138` at `+0x818`, `+0x81c`, and `+0x820`.
  The three angle sums store `+0x4c8 + +0x124`, `+0x4cc + +0x128`, and
  `+0x4d0 + +0x12c` at `+0x824`, `+0x828`, and `+0x82c`.  The height at `+0x4ec`
  minus 2 lands at `+0x830`.
  Evidence: the block at `0x1ede8` through `0x1ee99`.
- Observation: the state flag at `+0x148` adds `0x10000` when it is not zero.
  The input byte at `0x118e1c` bit 4 adds `0x20000` unless the input byte at
  `0x118e1d` bit 4 is also set.  The combined value of `+0x2fc`, `+0x7b4`, and
  the extra bits lands at `+0x7f0`.  When the state is not 1, the function also
  copies `+0x824` and `+0x82c` to `+0x834` and `+0x838`.
  Evidence: the block at `0x1ed2c` through `0x1ed79` and the block at
  `0x1ee9f` through `0x1eeca`.
- Inference: the player object, the caller mode-10 step, the snapshot block, and
  the conditional copies support the name `player_state_snapshot`.  The module
  records the name with the confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the original has the EBP frame, so the module builds with
  `-of+`.  The module uses the FPU loads, stores, and adds, and the local
  `fpu_stub.asm` supplies the FPU run-time symbols.
  Evidence: `tools/rebuild/modules/player_state_snapshot.json`.
- Observation: the harness holds the player object, the five globals, and the
  two input bytes.  It covers the four simple copies, the reordered field
  copies, the float copies, the three view-offset sums, the three angle sums,
  the height minus 2, the two conditional copies, the state flag, the input
  flag, the state 1 copy skip, the control bit, and the clear input bit.  The
  output holds the markers `0` through `8` and `D`, and the behavior check
  passes.
  Evidence: `python3 tools/rebuild/harness.py run --module
  player_state_snapshot`, `work/rebuild/pssnap.txt`, and
  `python3 tools/rebuild/run.py run --function 0x1ed04`.
- Observation: the rebuilt module holds 474 bytes and 115 decoded instructions.
  The identity check stays open at 34 of 115 instructions and 20 of 465 bytes
  because V2 saves the frame pointer first, keeps the object in a register, and
  uses near data offsets.  The build and behavior checks pass, so the store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1ed04 --count 115 --rebuilt work/rebuild/pssnap.exe
  --rebuilt-map work/rebuild/player_state_snapshot.map --rebuilt-symbol
  player_state_snapshot_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The wheel_contact_update module at 0x11a30

- Observation: the original body at `0x11a30` holds 141 instructions and 461
  bytes up to the `ret` at `0x11bfc`.  The next function starts at `0x11c00`, so
  the store record keeps the 464-byte extent and sets no function size.  The
  record sets the instruction count 141.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x11a30 --count 141` and
  `tools/rebuild/functions/0x11a30.json`.
- Observation: the two direct callers are the second player step group at
  `0x15ab8` through the call at `0x15aec` and the cornering body at `0x16b98`
  through the call at `0x16c92`.  The step group runs on the player object at
  object-3 offset `0xf91c` after the physics group at `0x15a80` and before
  `player_car_cornering_update` at `0x16ca0` and `player_car_contact_update` at
  `0x166a8`.  The five view groups call the step group, so the record takes the
  section `The vehicle state chain`.
  Evidence: the direct call sweep for `0x11a30`, the call at `0x15aec` in the
  group at `0x15ab8`, and the call at `0x16c92` in the body at `0x16b98`.
- Observation: the function forms the absolute cosine of `+0x4d0` and the
  absolute cosine of `+0x4c8` through `0x4da66`.  It clears the wheel flag
  `+0x2f0` and walks the four 0x54-byte wheel units at `+0x194`.
  Evidence: the block at `0x11a3d` through `0x11a8f` and the loop at `0x11a92`
  through `0x11bf0`.
- Observation: a unit holds the two local offsets at `+0x18` and `+0x20`, the
  two contact pairs at `+0x00`, `+0x08`, `+0x0c`, and `+0x14`, the two ground
  heights at `+0x24` and `+0x28`, and the surface flag at `+0x2c`.
  Evidence: the reads at `0x11aaf` and `0x11ac0` and the stores at `0x11b02`,
  `0x11b1d`, `0x11b3f`, `0x11b61`, `0x11ba8`, `0x11bc5`, and `0x11be7`.
- Observation: for each unit the function scales the two offsets by the two
  cosines, truncates them through `0x4d9f4`, and rotates the pair through
  `0x2218c` by the negative yaw `+0x4cc` plus the delta `+0x128`.  It adds
  `+0x4b0` and `+0x4b8` and stores the first contact pair through the globals
  `0x8038` and `0x8040`.
  Evidence: the block at `0x11aac` through `0x11b26`.
- Observation: on a hit from the map collision check at `0x11c00` the first
  ground height `+0x24` takes the global `0xc72c`, the surface flag `+0x2c`
  takes the compare of the global `0x8024` with `0x65`, and the wheel flag
  `+0x2f0` takes the even-unit surface bit.  The function then rotates the same
  pair by the negative yaw alone and stores the second contact pair through the
  same two globals, and a hit sets the second ground height `+0x28`.
  Evidence: the block at `0x11b2b` through `0x11bea`.
- Inference: the four wheel units, the two contact pairs, the two ground
  heights, the surface flag, and the two callers support the name
  `wheel_contact_update`.  The module records the name with the confidence
  `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the conversions to an integer use the Watcom helper at
  `0x4d9f4`, which truncates toward zero.  The cosine uses the routine at
  `0x4da66`.  The harness links no math library, so the local
  `wheel_contact_update_fpu.asm` supplies `cos` and `fabs` and makes `__CHP`
  truncate toward zero.  The original has the EBP frame, so the module builds
  with `-of+`.
  Evidence: `tools/rebuild/wheel_contact_update_fpu.asm` and the block at
  `0x11aac` through `0x11acb`.
- Observation: the harness holds the player object, the four wheel units, the
  two contact globals, the ground height, the surface value, and the two callee
  stubs.  The six paths cover the no-collision path, the collision with the
  surface value `0x65`, the collision with another surface value, the clear of
  a preset wheel flag, the two rotation angles, and the absolute cosine of pi.
  The output holds the markers `0` through `5` and `D`, and the behavior check
  passes.
  Evidence: `tools/rebuild/wheel_contact_update_main.c`,
  `python3 tools/rebuild/harness.py run --module wheel_contact_update`,
  `work/rebuild/wcu.txt`, and `python3 tools/rebuild/run.py run --function
  0x11a30`.
- Observation: the rebuilt module holds 455 bytes and 141 decoded instructions.
  The identity check stays open at 31 of 141 instructions and 15 of 455 bytes
  because V2 saves the frame first, keeps the object in `ebx` and the unit in
  `ecx`, and uses other local offsets.  The build and behavior checks pass, so
  the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x11a30 --count 141 --rebuilt work/rebuild/wcu.exe
  --rebuilt-map work/rebuild/wheel_contact_update.map --rebuilt-symbol
  wheel_contact_update_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The map_object_finish module at 0x2b494

- Observation: the original body at `0x2b494` holds 118 instructions and 457
  bytes up to the `ret` at `0x2b65c`.  The next function starts at `0x2b660`, so
  the store record keeps the 460-byte extent and sets no function size.  The
  record sets the instruction count 118.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x2b494 --count 118` and
  `tools/rebuild/functions/0x2b494.json`.
- Observation: the only direct caller at `0x29b86` is the key -2 branch of
  `menu_text_update` at `0x29918`.  That branch runs this function on each of
  the 0x50 map object records at `DAT_001db78` and then restores the menu key,
  so the record takes the section `race_frame callees`.
  Evidence: the call at `0x29b86` in the body at `0x29918` and the record loop
  at `0x29b6a` through `0x29b9e`.
- Observation: a record with the field `+0x08` equal to -1 holds no object.
  The function binds the list name at `0x6350` to the slot
  `DAT_001d2d0 + record->0x00 * 8` through the helper at `0x11850` and returns.
  Evidence: the compare at `0x2b4a3` and the call at `0x2b4c1`.
- Observation: the normal path reads the slot `DAT_001d2d0[index * 2]`, binds
  the slot through the helper at `0x117fc`, and takes the object pointer from
  the 0x1c-byte entry table `DAT_0010653c`.  It sets the object id at `+0x20`
  from `DAT_001fab8 * record->0x08 + 0x226` and copies the record positions
  `+0x40`, `+0x44`, and `+0x48` to the object fields `+0x08`, `+0x0c`, and
  `+0x10`.
  Evidence: the block at `0x2b4cb` through `0x2b571`.
- Observation: when the record index `+0x04` equals the selected line
  `DAT_001fb08[DAT_001fb60]`, the record angle `+0x10` takes pi/2.  The three
  record angles `+0x10`, `+0x14`, and `+0x18` become the object angles `+0x14`,
  `+0x18`, and `+0x1c`, scaled by 1024.0 / pi and masked with 0x7ff.  The
  progress at `+0x58` and the timer at `+0x60` then reset to zero.
  Evidence: the compare at `0x2b587`, the three scale blocks at `0x2b595`
  through `0x2b63d`, and the reset at `0x2b640` through `0x2b653`.
- Inference: the caller `menu_text_update`, the shared 0x64-byte map object
  records, the slot table, and the matching `map_object_update` writer support
  the name `map_object_finish`.  The module records the name with the
  confidence `inferred`.
  Evidence: the call graph and the data flow above.
- Observation: the scale constants sit at object-3 offsets `0x109b` and
  `0x109f` and hold 1024.0 and pi.  The original calls the truncate helper at
  `0x4d9f4` before each `fistp`.  This harness links no math library, so the
  local `map_object_finish_fpu.asm` makes `__CHP` truncate toward zero.  The
  constants arrive through globals, because the minimal image does not load the
  constant segment.  The original has the EBP frame, so the module builds with
  `-of+`.
  Evidence: the fixup targets at `0x2b59d` and `0x2b5a3`, the float values at
  object-3 offsets `0x109b` and `0x109f`, and
  `tools/rebuild/map_object_finish_fpu.asm`.
- Observation: the harness holds one 0x64-byte record, four objects, the entry
  table, the slot table, the selected-line table, and the two float constants.
  The six paths cover the full path, the selected line, the bind path, the
  negative truncation, the large angle mask, and a different slot.  The output
  holds the markers `0` through `5` and `D`, and the behavior check passes.
  Evidence: `tools/rebuild/map_object_finish_main.c`,
  `python3 tools/rebuild/harness.py run --module map_object_finish`,
  `work/rebuild/mofin.txt`, and `python3 tools/rebuild/run.py run --function
  0x2b494`.
- Observation: the rebuilt module holds 400 bytes and 118 decoded instructions.
  The identity check stays open at 32 of 118 instructions and 21 of 400 bytes
  because V2 saves a different register set, uses another prologue order, keeps
  the record in `ecx`, and places the three float constants in the local data.
  The build and behavior checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x2b494 --count 118 --rebuilt work/rebuild/mofin.exe
  --rebuilt-map work/rebuild/map_object_finish.map --rebuilt-symbol
  map_object_finish_`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The func_1e914 module at 0x1e914

- Observation: the original body at `0x1e914` holds 160 instructions and 452
  bytes up to the `ret 4` at `0x1ead5`.  The next function starts at `0x1eae0`,
  so the store record keeps the 460-byte extent and sets no function size.  The
  record sets the instruction count 160.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x1e914 --count 160` and
  `tools/rebuild/functions/0x1e914.json`.
- Observation: no direct call, no data reference, and no LE relocation target
  reaches `0x1e914`.  The function has no known caller.
  Evidence: the scan of every `E8` call in object 1, the scan of the literal
  `0x1e914` and `0xe914` in the image, and the scan of the fixup targets.
- Observation: the sibling at `0x1e7b4` evaluates the same uniform quadratic
  basis in two dimensions.  The opponent path update at `0x20b1c` calls the
  body at `0x22378`, which calls the sibling at `0x22499`.  This function is the
  three-dimensional variant of the same curve sampler.
  Evidence: the constant pairs `0xbac`/`0xbb4` and `0xbb8`/`0xbc0`, the call at
  `0x22499`, and the callers at `0x20954` and `0x20b1c`.
- Observation: the function takes the output pointer in `eax`, the three
  control points in `edx`, and the fraction on the stack.  It computes the
  basis weights `w0 = 0.5 * (1 - t)^2`, `w2 = 0.5 * t^2`, and
  `w1 = 1 - (w0 + w2)` over the points at offsets 0, `0xc`, and `0x18`.
  Evidence: the block at `0x1e920` through `0x1e951`.
- Observation: the output at `+0x00`, `+0x04`, and `+0x08` holds the sampled
  position.  The function then samples the same basis at `t + 0.002`, subtracts
  the first sample, divides the difference by its length through the
  square-root helper at `0xc6cbe`, and writes the three tangent components to
  `+0x0c`, `+0x10`, and `+0x14`.
  Evidence: the blocks at `0x1e954` through `0x1e9be` and `0x1ea66` through
  `0x1eacf`.
- Observation: the double 0.5 sits at object-3 offset `0xbb8` and the float
  0.002 sits at object-3 offset `0xbc0`.  The original calls the Watcom
  square-root helper at `0xc6cbe`.  This harness links no math library, so the
  local `func_1e914_fpu.asm` supplies `sqrt_` from the hardware and the FPU
  run-time symbols.  The constants arrive through globals, because the minimal
  image does not load the constant segment.  The original has the EBP frame, so
  the module builds with `-of+`.
  Evidence: the fixup targets at `0x1e930`, `0x1e93e`, `0x1e9c4`, and
  `0x1e9dd`, the values at object-3 offsets `0xbb8` and `0xbc0`, and
  `tools/rebuild/func_1e914_fpu.asm`.
- Inference: the name stays `func_1e914`, because no caller and no data
  reference support a stronger name.  The sibling 2D sampler and the
  three-dimensional tangent output are the only evidence, so the relation is
  weak.  The record takes the section `The vehicle state chain` with the spline
  family, because the sibling sampler serves the opponent path update and the
  function holds the same curve role.
  Evidence: the call graph and the data flow above.
- Observation: the harness holds the three control points, the six-float
  output, and the two basis constants.  The seven paths cover the x, y, and z
  control axes, the mid fraction, the end fraction, and the two negative
  tangent signs.  The output holds the markers `0` through `6` and `D`, and the
  behavior check passes.
  Evidence: `tools/rebuild/func_1e914_main.c`,
  `python3 tools/rebuild/harness.py run --module func_1e914`, and
  `work/rebuild/func1e9.txt`.
- Observation: the rebuilt module holds 431 bytes and 160 decoded instructions.
  The identity check stays open at 24 of 160 instructions and 21 of 431 bytes
  because V2 keeps the output pointer in `ecx` and the input pointer in `edx`,
  uses a shorter frame, fuses the two `(1 - t)` factors into one `fmul st(0)`,
  and places the two basis constants in the local data.  The build and behavior
  checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x1e914 --count 160 --rebuilt work/rebuild/func1e9.exe
  --rebuilt-map work/rebuild/func_1e914.map --rebuilt-symbol func_1e914_` and
  `python3 tools/rebuild/run.py run --function 0x1e914`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The path_playback_update module at 0x23038

- Observation: the original body at `0x23038` holds 148 instructions and 437
  bytes up to the `ret` at `0x231ec`.  The next function starts at `0x231f0`, so
  the store record keeps the 440-byte extent and sets no function size.  The
  identity check uses 148 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x23038 --count 148` and
  `tools/rebuild/functions/0x23038.json`.
- Observation: the only direct caller at `0x20317` is the view group at
  `0x202c0`.  That group appends a keyframe through `0x231f0` and then calls
  this function with the playback block at object-3 offset `0x18ed8` in `eax`
  and zero in `edx`.  The frame update at `0x1ff70` selects the view group
  through the mode byte `DAT_0011d260`.
  Evidence: the calls at `0x2030b` and `0x20317`, the relocation targets at
  `0x20311`, and the disassembly of `0x202c0` and `0x231f0`.
- Observation: the first argument points to a playback block.  The field at
  `+0x08` points to a state record.  The state holds the index at `+0x04`, the
  count at `+0x08`, the active flag at `+0x0c`, and the keyframe array at
  `+0x20`.  A keyframe holds 0x28 bytes.  The position is at `+0x00`, `+0x04`,
  and `+0x08`.  The three angles are at `+0x0c`, `+0x10`, and `+0x14`.  The two
  integer fields are at `+0x18` and `+0x1c`.  The float field is at `+0x20`.
  The timestamp is at `+0x24`.
  Evidence: the loads at `0x23055` through `0x2317d` and the append at
  `0x231f0`.
- Observation: the function returns at once when the global `DAT_0011d264` is
  zero or when the state is not active.  With a zero second argument it
  advances the state index while the playback time `DAT_00118e10` is above the
  timestamp of the current keyframe.  It sets the active flag to -1 when the
  index reaches the count.  It then copies the head keyframe (the keyframe
  before the index) to the playback block at `+0x10` through `+0x30`.  A set
  second argument keeps the head at zero and stops after the copy.
  Evidence: the blocks at `0x23048` through `0x23151`.
- Observation: on the normal path the function forms the fraction between the
  head stamp and the next stamp.  It takes the keyframes before, at, after, and
  two after the head.  It calls `spline_interpolate` at `0x22744` with the
  fraction.  It calls the spline only when the head index is above one and the
  count minus the head index is above two, because the cubic form needs the two
  neighbours.
  Evidence: the fraction block at `0x23199` through `0x231aa`, the pointer
  frame at `0x231ad` through `0x231cb`, and the call at `0x231e0`.
- Observation: the playback time sits at object-3 offset `0x18e10` and the
  enable flag at object-3 offset `0x1d264`.  The frame timer at `0x1fc38` adds
  the frame delta to the playback time and the init at `0x12bc0` clears it, so
  the first is a global clock and not a constant.  The two globals arrive from
  the harness, because the minimal image does not load that part of the segment.
  The original has the EBP frame, so the module builds with `-of+`.
  Evidence: the fixup targets at `0x2304a`, `0x23083`, and `0x2319b`, the
  disassembly of `0x1fc38` and `0x12bc0`, and
  `tools/rebuild/path_playback_update.c`.
- Inference: the name `path_playback_update` follows from the caller position
  in the view group, the keyframe append at `0x231f0`, the playback block at
  `0x18ed8`, the index advance, and the spline call.  The original has no
  symbol name, so the record keeps the name with the confidence inferred.
  Evidence: the call graph and the data flow above.
- Observation: the harness holds the playback block, the playback state, and
  ten keyframes.  It covers the zero enable flag, the inactive state, the set
  second argument, the interpolation with the fraction 0.5, the index advance
  over five stamps, the run past the last stamp, the two head guards, and the
  zero index with an equal time.  The output holds the markers `0` through `8`
  and `D`, and the behavior check passes.
  Evidence: `tools/rebuild/path_playback_update_main.c`,
  `python3 tools/rebuild/harness.py run --module path_playback_update`, and
  `work/rebuild/pplay.txt`.
- Observation: the rebuilt module holds 428 bytes and 148 decoded instructions.
  The identity check stays open at 46 of 148 instructions and 21 of 428 bytes.
  V2 saves the frame pointer first, keeps the playback block in `ebx`, uses a
  shorter frame, multiplies the index by 40, and reads the enable flag through
  a near data address.  The build and behavior checks pass, so the store
  records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x23038 --count 148 --rebuilt work/rebuild/pplay.exe
  --rebuilt-map work/rebuild/path_playback_update.map --rebuilt-symbol
  path_playback_update_` and `python3 tools/rebuild/run.py run --function
  0x23038`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.

## The quit_confirm_screen module at 0x219ec

- Observation: the original body at `0x219ec` holds 122 instructions and 431
  bytes up to the `ret` at `0x21b9a`.  The next function starts at `0x21b9c`,
  so the store record keeps the 432-byte extent and sets no function size.  The
  identity check uses 122 instructions.
  Evidence: `python3 tools/le.py disasm STARTH.EXE 0x219ec --count 122` and
  `tools/rebuild/functions/0x219ec.json`.
- Observation: the only direct caller at `0x1feb3` is the race frame loop
  `0x1fd70`.  The loop calls this function when the mode byte `DAT_001d260` bit
  `0x10` is clear and the mode is not 2.  `loading_screen` at `0x2a514` and
  `race_results_screen` at `0x2a753` call the loop, and the loading path exits
  the game with the return value when it is not zero.
  Evidence: the call at `0x1feb3`, the two loop calls, the branch at `0x1feaa`,
  and the exit path at `0x2a51c`.
- Observation: the function sets the screen mode `0x140` by `0x190` with the
  depth `0x104` through `0x4c850`, starts the sound service with the driver at
  object-3 offset `0x6350` through `0x81ab0`, runs the two video swaps that
  `DAT_001d274` bits `0x10` and `0x20` gate, clears the screen through
  `0x2aa98`, and fills the screen three times through `0x25d28`.
  Evidence: the blocks at `0x21a08` through `0x21a6d`.
- Observation: the three text pointers at object-3 offsets `0x18fe0`,
  `0x18fe4`, and `0x18fe8` hold the title and the two choices.  The title draws
  at y `0x34` with the color 2 and the choices draw at y `0x5c` and `0x7a`
  through `0x2e7cc`.  The selected choice takes the color 2 and the other
  choice takes `0xe`, and the exit flag selects the choice.  The highlight
  phase is bit `0x10` of `DAT_000007f9`.
  Evidence: the blocks at `0x21a6f` through `0x21b04`.
- Observation: the loop polls the input through `0x26448` when `DAT_001d2c0`
  and `DAT_001d2cc` are not zero and copies the default data through `0x10193`.
  The up keys `DAT_0000004a` and `DAT_0000004d` clear the exit flag and the
  down keys `DAT_00000052` and `DAT_0000004f` set it.  Each key group holds a
  latch that clears on release, and a key change calls `0x1f718` with 6.  The
  enter key `DAT_0000001e` returns the exit flag.  The escape key
  `DAT_00000003` sets the exit flag to 1 and returns it.
  Evidence: the blocks at `0x21a85` through `0x21b9a`.
- Observation: the object-2 key bytes arrive from `default_data_copy`, which
  copies 32 dwords from object 1 into object 2 at `0xf0002`.  The harness holds
  the key bytes as globals, so the behavior check can set them.
  Evidence: the copy at `0x10193`, the object table, and
  `tools/rebuild/quit_confirm_screen_main.c`.
- Inference: the two choices, the quit answer, and the exit in the loading path
  support the name `quit_confirm_screen`.  The original holds no symbol for this
  function, so the record keeps the name with the confidence inferred.
  Evidence: the call graph and the data flow above.
- Observation: the harness holds the three text pointers, the mode flags, the
  two input gates, the highlight phase, and the six key bytes.  It runs nine
  paths: the enter key, the up key, the down key with a second poll, the escape
  key, the two input gates, the two video swaps, a held up key latch, and an up
  key release and press.  The output holds the markers `0` through `8` and `D`,
  and the behavior check passes.
  Evidence: `tools/rebuild/quit_confirm_screen_main.c`,
  `python3 tools/rebuild/harness.py run --module quit_confirm_screen`, and
  `work/rebuild/qconf.txt`.
- Observation: the rebuilt module holds 436 bytes and 122 decoded instructions.
  The identity check stays open at 25 of 122 instructions and 48 of 431 bytes,
  and 0 of 35 relocation slots match.  V2 saves the frame pointer first, keeps
  the exit flag in `esi` and the up latch in `edi`, uses a 12-byte frame, folds
  the fill index into `ecx`, and reads the globals through near data operands.
  The build and behavior checks pass, so the store records `rebuilt`.
  Evidence: `python3 tools/rebuild/diff_module.py --original STARTH.EXE
  --original-address 0x219ec --count 122 --rebuilt work/rebuild/qconf.exe
  --rebuilt-map work/rebuild/quit_confirm_screen.map --rebuilt-symbol
  quit_confirm_screen_` and `python3 tools/rebuild/run.py run --function
  0x219ec`.
- Limit: the original has no symbol name for this function.  The module name
  and the callee names in the harness stay inferred.
