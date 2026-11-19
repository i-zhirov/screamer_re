# Source Forms

- **Date**: 2026-11-15

This page records the known source forms of the original game.  Each section
names the source form, the compiler and flag condition, the effect on the
object code, the original census, the evidence, and the first-difference
signature.  `first_difference.py` classifies the first oracle difference of a
non-strict module with the difference signatures on this page.

## The registers-before-frame prologue with -od without -of+

- **Source form**: a C function with a frame and saved registers, compiled with
  `-od` and without `-of+`.
- **Compiler and flag condition**: the `-od` flag alone gives the
  registers-before-frame layout.  The 9.01d, 9.5, 10.0, and 10.0a compilers
  give this layout with `-od`.  The tested 9.01d, 9.5, and 10.0a compilers
  give the frame-first layout with `-of+`.  The 8.5a `-od` build is frameless.
  A sweep of 35 option combinations, four calling conventions, and five
  `#pragma aux` variants kept the frame-first prologue only when the
  combination also passed `-of+`.
- **Effect on the object code**: the function saves its registers before it
  sets up the frame.  The original prologue is `push ebx; push ecx; push esi;
  push edi; push ebp; mov ebp,esp; sub esp,0xc`.
- **Original census**: the original text holds 205 prologues of the form `push
  ebx; push ecx; [push edx]; push esi; push edi; push ebp; mov ebp,esp`.  Two
  examples are at `0x11c00` and `0x1821c`.
- **Evidence**: the findings sections "The period Watcom 10.0a toolchain" and
  "The initial 10.0 compiler and the list_destroy slot order", the byte census
  of `STARTH.EXE` object 1, and the disassembly at `0x11c00`, `0x1821c`,
  `0x2aa84`, and `0x2ed77`.  The command `python3
  tools/rebuild/harness_dos.py build --module list_destroy` with `-od` gives 25
  of 25 instructions equal.
- **First-difference signature**: a `-of+` rebuild inverts the save order, so
  the first difference is a `byte 0:` difference on two `push` instructions of
  a 32-bit register.  That difference classifies as `register save order`.

## The frame-only epilogue against the conservative mov esp,ebp

- **Source form**: a C function with a frame and no locals.
- **Compiler and flag condition**: the `-of+` flag.  The 8.5a, 9.01d, 9.5,
  10.0 Limited Availability, 10.0a, 10.5, 10.6, and 11.0b compilers all emit
  `mov esp,ebp; pop ebp; ret` for `frame_step`.  The original emits `pop ebp;
  ret`.
- **Effect on the object code**: the conservative epilogue adds one `mov
  esp,ebp` before the frame teardown.
- **Original census**: the original text holds 116 frame functions without
  locals that end in `pop ebp; ret`.
- **Evidence**: the findings section "The period Watcom 10.0a toolchain" and
  the command `python3 tools/rebuild/harness_dos.py build --module frame_step`.
- **First-difference signature**: the rebuild holds one instruction the
  original does not, so the instruction counts differ.  The first-difference
  report shows the measured pair at the insertion point.  The `instruction
  count` idiom is reserved for the equal-prefix count case.

## The local slot order

- **Source form**: a C function with two parameters and a compiler-generated
  return temporary, for example `list_destroy`.
- **Compiler and flag condition**: the 10.0a compiler with `-od`.  The findings
  infer that the compiler's local allocator places the return temporary in the
  highest local slot.  The tested source forms did not control the order.
- **Effect on the object code**: the stack displacements of the parameter
  spills and the return temporary change.  The original stores `list` at
  `[ebp-8]`, `unused` at `[ebp-4]`, and the return temporary at `[ebp-0xc]`.
  The 10.0a build stores them at `[ebp-0xc]`, `[ebp-8]`, and `[ebp-4]`.
- **Original census**: the `list_destroy` slot layout is the measured original
  layout.  The source-form search kept the instruction count and the slot order
  for the existing source form.  The same search kept them for a `goto` to the
  end, an `if`/`else` with an empty else, and a `(void)unused` statement.
- **Evidence**: the findings section "The list_destroy local slot order under
  the period 10.0a compiler".
- **First-difference signature**: a `byte 2:` difference whose original and
  rebuilt operands both hold `[ebp - ...]` classifies as `local slot order`.
  One example is `byte 2: 0xf8 against 0xf4`.

## The frame size

- **Source form**: a C function whose local declaration set fixes the frame
  size.  The declaration set is the local variables and the compiler-generated
  locals.
- **Compiler and flag condition**: the 10.0a compiler.  It allocates the frame
  with one `sub esp, imm`, where the immediate is the rounded frame size.
- **Effect on the object code**: a different declaration set gives a different
  `sub esp, imm` immediate.  The instruction shape stays the same, so the
  first difference is the immediate byte of the frame allocation.
- **Original census**: the first-difference report measures three rows:
  `race_lap_complete` at `0x19678`, `loading_screen` at `0x2a364`, and
  `stream_read` at `0x4c4e5`.
- **Evidence**: the first-difference report of the three rows, with the command
  `python3 tools/rebuild/first_difference.py report --module race_lap_complete
  --module loading_screen --module stream_read`, and the measured first
  difference `byte 2:` on two `sub esp, imm` instructions.
- **First-difference signature**: a `byte 2:` difference whose original and
  rebuilt mnemonics are both `sub` and whose operands both start `esp, 0x`
  classifies as `frame size`.

## The volatile parameter local

- **Source form**: a C function whose parameter is `volatile int`.
- **Compiler and flag condition**: the 10.0a compiler.  A plain parameter stays
  in `eax`.  Neither `-of+`, `-ot`, `-d1`, nor `-d1+` produces the frame store
  for a plain parameter.  The `volatile` parameter reproduces the store and the
  reload.
- **Effect on the object code**: the compiler stores the argument to the frame
  and reloads it before the switch.  The byte sequence is `83 ec 04 89 45 fc
  8b 45 fc`: `sub esp,4`, `mov [ebp-4],eax`, and `mov eax,[ebp-4]`.
- **Original census**: the frame-local pattern appears 32 times in the original
  text.
- **Evidence**: the findings section "The error_report register saves, the body
  expansion, and the frame local", `work/opt/S2.C` against `S1.C`, and the
  parameter census of `STARTH.EXE` object 1 for the byte sequence `83 ec 04 89
  45 fc 8b 45 fc`.
- **First-difference signature**: without the `volatile` form the rebuilt keeps
  the argument in a register, so the instruction counts differ.  The
  first-difference report shows the measured pair at the removal point of the
  frame store and the reload.  The `instruction count` idiom is reserved for the
  equal-prefix count case.  When both bodies store at different frame slots, the
  idiom is `local slot order`.

## The #pragma aux modify exact [regs] register contract

- **Source form**: a printer declaration with the auxiliary pragma `#pragma aux
  report "_*" modify exact [eax];`.
- **Compiler and flag condition**: the 10.0a compiler.  The name string `"_*"`
  is required, because a pragma without it maps the symbol to `report_` instead
  of `_report`, and the link fails.
- **Effect on the object code**: the pragma reports that only `eax` changes, so
  the compiler saves no register around the call.  A plain `extern void
  __cdecl report(char *table, ...)` makes the compiler save `ebx`, `ecx`, and
  `edx` for the whole function.  The three saves and their epilogue pops
  disappear with the pragma.
- **Original census**: `error_report` is the measured module.  `work/opt/V1.C`
  (no pragma) holds `push ebx; push ecx; push edx`, `VEAX.C` (`modify exact
  [eax]`) and `VEMPTY.C` (`[]`) hold no save.
- **Evidence**: the findings section "The error_report register saves, the body
  expansion, and the frame local" and the builds of `work/opt/V1.C`, `V8.C`,
  `V9.C`, `VEAX.C`, and `VEMPTY.C` with the 10.0a compiler.
- **First-difference signature**: the extra register saves change the prologue,
  so the instruction counts differ.  The first-difference report shows the
  measured pair at the insertion or removal point of the saves.  The
  `instruction count` idiom is reserved for the equal-prefix count case.  A
  changed instruction mnemonic gives `instruction selection`.

## The -ot body expansion

- **Source form**: a switch whose every case holds a full body.
- **Compiler and flag condition**: the 10.0a compiler with `-of+ -ot`.  The
  default optimization and `-os` merge the common call tails.  `-oc` alone
  expands only the first body.  `-od` recovers no jump table.
- **Effect on the object code**: the compiler keeps one full body per code.
  Each body holds its own `push`s, its own `call` to the printer, and its own
  epilogue.  Without `-ot`, the bodies after the first push their constants and
  jump into the first body.
- **Original census**: `error_report` holds 21 bodies.  `work/opt/E1.C` with
  `-of+ -ot` keeps every body full.  `E2.C` (`-os`) and `E3.C` (`-ou`) keep the
  jumps.
- **Evidence**: the findings section "The error_report register saves, the body
  expansion, and the frame local".
- **First-difference signature**: without `-ot` the rebuilt folds the bodies
  into a shared tail, so the instruction counts differ.  The first-difference
  report shows the measured pair at the insertion or removal point.  The
  `instruction count` idiom is reserved for the equal-prefix count case.  A
  `branch target:` difference that keeps the count gives the idiom
  `basic-block order`.  A `mnemonic:` difference gives the idiom `instruction
  selection`.

## The jump-table bias with inc eax

- **Source form**: a switch whose lower bound is -1, so the codes run from -1
  through 22.
- **Compiler and flag condition**: the 10.0a compiler with `-of+`.  A source
  with the cases 0 through 23 emits `cmp eax,0x17` and no `inc`.  A source with
  the cases -1 through 22 emits `inc eax` and `cmp eax,0x17`.
- **Effect on the object code**: the compiler adds the bias instruction `inc
  eax`, the bound `cmp eax,0x17`, and a 24-entry jump table.
- **Original census**: the original dispatch holds `inc eax` and `cmp eax,0x17`,
  so its table holds 24 entries.  The entries map the code `c` to the entry
  `c+1`.
- **Evidence**: the findings section "The error_report switch range and the
  missing case", `work/opt/T24.C`, and `work/opt/TM1.C`, built with the 10.0a
  `wcc386` and linked with `wlink system dos4g`.
- **First-difference signature**: without the negative lower bound the rebuild
  omits `inc eax`, so the instruction counts differ.  The first-difference
  report shows the measured pair at the removal point.  The `instruction count`
  idiom is reserved for the equal-prefix count case.

## The short push imm8 against the long push imm32 with a relocation

- **Source form**: a call that passes a format-string address, which is a
  relocatable symbol, instead of an integer constant.
- **Compiler and flag condition**: the 10.0a compiler shortens an integer
  constant that fits in a signed byte to `push imm8` (`6a ib`), and no flag
  disables it.  `-ot`, `-os`, `-or`, `-oa`, `-oc`, `-od`, `-d1`, and `-d1+`
  all keep `6a 44`.  A relocatable operand is unknown at compile time, so the
  compiler emits `push imm32` (`68 id`) with a relocation even when the value is
  small.
- **Effect on the object code**: `push imm8` is two bytes.  `push imm32` is
  five bytes and holds a four-byte fixup slot.
- **Original census**: only `error_report` pushes immediate arguments.  The
  printer at `0x4bf64` has 35 calls, and 21 of them are inside `error_report`.
  The other nine measured modules push registers only or nothing.
- **Evidence**: the findings sections "The short push encoding and the
  format-string arguments" and "The format-string audit of the other modules",
  `work/opt/SL.C`, and `python3 tools/le.py disasm work/opt/SL.EXE 0x10030
  --count 8`.
- **First-difference signature**: a `size: 5 against 2` difference gives
  `instruction encoding`.  A `relocations: 1 against 0` difference gives
  `relocation target`.

## The period compiler jump-table placement

- **Source form**: a switch compiled by the period toolchain.
- **Compiler and flag condition**: the 10.0a compiler.  The rebuilt code holds
  `jmp dword ptr cs:[eax*4+0x10]`, which has the same shape as the original
  `jmp dword ptr cs:[eax*4+0x918]`.  The linker relocates the table address,
  and the program prints all four markers.  The V2 build emits the same
  reference without the table and crashes.
- **Effect on the object code**: the compiler places the table and the linker
  relocates its address.  The operand difference is a fixup slot, not a byte
  difference.
- **Original census**: the `error_report` table holds 24 entries.
- **Evidence**: the findings section "The period Watcom 10.0a toolchain",
  bullet "The period compiler places and relocates the jump table of
  `error_report`", and the command `python3 tools/rebuild/harness_dos.py build
  --module error_report`.
- **First-difference signature**: a `relocation target:` difference gives
  `relocation target`.

## The 8.3 source-name collision

- **Source form**: two source files whose names truncate to the same 8.3 name.
  `shutdown.c` and `shutdown_main.c` both become `SHUTDOWN.C`.
- **Compiler and flag condition**: the period DOS compiler under DOSBox-X.  The
  DOS tools truncate a long file name to eight characters.  The period harness
  copies each source to an indexed 8.3 name (`S0.C`, `S1.C`, and later names)
  before the compile.
- **Effect on the object code**: the collision does not change the object code.
  The compiler reads the first file twice, and the link fails.
- **Original census**: the collision hits six modules: `shutdown`,
  `bios_video`, `error_report`, `frame_step`, `list_destroy`, and
  `memory_available`.
- **Evidence**: the findings section "The period Watcom 10.0a toolchain" and the
  command `python3 tools/rebuild/harness_dos.py build --module shutdown`.
- **First-difference signature**: a failed build gives the report `build`
  `fail` and the idiom `other`.  The report writes no row for a module whose
  identity is strict.

## The difference signatures

| Signature form | Idiom |
| --- | --- |
| `byte 2:` with `[ebp - ...]` on both operands | `local slot order` |
| `mask:` | `constant operand` |
| `size:` | `instruction encoding` |
| `mnemonic:` | `instruction selection` |
| `branch target:` | `basic-block order` |
| `relocations:`, `relocation offset:`, `relocation width:`, or `relocation target:` | `relocation target` |
| `byte 0:` with a `push` of a 32-bit register on both operands | `register save order` |
| `byte 2:` with `sub esp, 0x...` on both operands | `frame size` |
| `instruction count:` | `instruction count` |
| no difference after the optional `instruction N: ` prefix | `match` |
| every other difference | `other` |

A `mask:` difference is `constant operand`, and a relocation-slot difference
also produces a `mask:` difference.  This overlap is a limitation of the
classification.  A `byte 2:` difference gives `local slot order` only when both
operands hold a stack frame displacement and neither mnemonic is `sub`.  The
frame-size rule runs before the local-slot rule.

## The body census of the image rows

The build `python3 tools/rebuild/whole_image.py --all` writes the image
`work/rebuild/STARTH.EXE` and the report `work/rebuild/whole-image.json`.  The
report holds 347 module items: 13 `write` rows, 12 `reuse` rows, and 322
skipped rows.  The summary is `13 patched, 12 reused, 322 skipped, 16 slots
relocated, 1034754 of 1034923 bytes equal`.  The score is a byte score, not a
correctness measure.

A `build` `pass` row whose `rebuilt_size` differs from the record `size` is a
size mismatch.  It is `smaller` when `rebuilt_size` is below `original_size`,
`larger` when it is above, and `padding` when it equals `original_size` and
lies below `size`.  The 319 size-mismatch rows split into 273 `smaller`, 31
`larger`, and 15 `padding`.  The 13 `write` rows, the 4 shape-misaligned rows
(`race_data_release`, `bios_tick_reset`, `race_cleanup`, and `video_buf_load`),
and the 11 library rows stay out of this census.  The 12 `reuse` rows are
`padding`; the other 3 `padding` rows are non-strict and stay skipped.

The first-difference report `python3 tools/rebuild/first_difference.py report
--all --out work/rebuild/first-differences.json` selects the 221 non-strict
records with an identity match.  It excludes the 30 strict module rows and the
96 records whose identity check holds no match.  The report rows split into 193
`smaller`, 20 `larger`, and 8 `aligned`.  The 96 records with no match add 80
`smaller`, 11 `larger`, 3 `padding`, and the 2 shape-misaligned rows
`race_data_release` and `bios_tick_reset`, so the image census is 273
`smaller`, 31 `larger`, and 15 `padding`.  The 12 strict padding rows are
absent from the report.

The table holds one row per report family and idiom.  The count is the number
of rows with the family and the idiom.  The first index, the instruction count,
and the byte count are the values of the example row.  The instruction and byte
counts are the oracle's equal and total values.  The `other` idiom splits by the
differing byte index.  The 6 `aligned` `local slot order` rows are `write`
rows; the 2 `aligned` `register save order` rows are shape-misaligned.

| Family | Idiom | Count | First index | Instructions | Bytes | Example |
| --- | --- | --- | --- | --- | --- | --- |
| `smaller` | `register save order` | 132 | 0 | 0/56 | 38/194 | `game_main` 0x10020 |
| `smaller` | `instruction selection` | 53 | 4 | 0/218 | 31/728 | `object_load` 0x10bfc |
| `smaller` | `frame size` | 1 | 7 | 0/124 | 41/431 | `race_lap_complete` 0x19678 |
| `smaller` | `local slot order` | 2 | 6 | 0/127 | 106/498 | `loading_screen` 0x2a364 |
| `smaller` | `other` (`byte 1`) | 4 | 6 | 0/111 | 25/394 | `net_player_add` 0x2dddc |
| `smaller` | `other` (`byte 0`) | 1 | 1 | 0/273 | 21/993 | `text_draw` 0x25020 |
| `larger` | `instruction selection` | 10 | 0 | 0/153 | 17/513 | `radar_world_draw` 0x1057c |
| `larger` | `register save order` | 8 | 0 | 0/71 | 86/271 | `engine_init` 0x127a0 |
| `larger` | `frame size` | 1 | 4 | 0/50 | 113/161 | `fopen` 0x4c08d |
| `larger` | `instruction encoding` | 1 | 21 | 0/104 | 78/379 | `dpmi_real_int` 0x25db0 |
| `aligned` | `local slot order` | 6 | 8 | 20/27 | 43/58 | `memory_available` 0x2f8ca |
| `aligned` | `register save order` | 2 | 0 | 0/25 | 34/100 | `race_cleanup` 0x2a1b0 |

The 132 `smaller` `register save order` rows are the registers-before-frame
prologue.  The 53 `smaller` `instruction selection` rows are the general
mnemonic difference.  The one `smaller` `frame size` row is
`race_lap_complete`.  The 2 `smaller` `local slot order` rows are
`loading_screen` and `stream_read`; the frame-size source batch moved them from
the frame-size idiom, and their final first differences are instruction 6.  The
4 `byte 1` `smaller` `other` rows are `net_player_add`, `resource_read`,
`resource_find`, and `lzw_bit_read`.  The one `byte 0` `smaller` `other` row is
`text_draw`.  The 6 `aligned` `local slot order` rows are `dos_memory_alloc`,
`list_create`, `memory_alloc_small`, `memory_available`, `memory_free`, and
`pool_init`.

## The measured outcomes

The table records the source-form experiment of the six near-miss modules.  The
first-difference column holds the difference before the source change.  The
final match, the instruction count, and the byte count hold the kept source.
The outcome column records the experiment result.  The `n/a` cells mean the
module is not a local-slot form.

| Module | Record | Source form | Final match | Instructions | Bytes | First difference | Idiom | Original stack slots | Rebuilt stack slots | Outcome |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `error_report` | 0x10978 | relocatable `printer_data` symbol in place of the integer literal `0x7fe8` | `exact-relocations` | 171/171 | 402/554 | instruction 23: `mask: [1, 2, 3, 4] against []` | constant operand | n/a | n/a | Kept.  The relocatable symbol makes the compiler emit the long `push imm32` with a relocation. |
| `dpmi_int31_entry` | 0xd4410 | explicit bytes `8e 5d 0c` in place of `mov ds, word ptr [ebp + 0xc]` | `exact-bytes` | 28/28 | 56/56 | instruction 10: `size: 3 against 4` | instruction encoding | n/a | n/a | Kept.  The explicit three bytes keep the 16-bit segment load without the operand-size prefix. |
| `memory_available` | 0x2f8ca | direct return `return probe_free(pool) + probe_size(pool);` in place of the `unsigned total;` local and the two statements | `same-shape` | 27/27 | 43/58 | instruction 8: `byte 2: 0xfc against 0xf4` | local slot order | `[ebp-4]` = the argument `pool`, `[ebp-8]` = the sum, `[ebp-0xc]` = the return temporary | `[ebp-4]` = the sum, `[ebp-8]` = the return temporary, `[ebp-0xc]` = the argument `pool` | Rejected.  It fell to the `different` match with 22 of 25 instructions and 30 of 52 bytes.  The two-statement form is kept. |
| `list_create` | 0x2ecf8 | named result `void *result; result = list; return result;` | `same-shape` | 46/46 | 102/127 | instruction 7: `byte 2: 0xf8 against 0xf4` | local slot order | `[ebp-8]` = the argument `list`, `[ebp-4]` = the argument `count`, `[ebp-0xc]` = the return temporary | `[ebp-0xc]` = the argument `list`, `[ebp-8]` = the argument `count`, `[ebp-4]` = the return temporary | Rejected.  It fell to the `different` match with 43 of 46 instructions and 94 of 127 bytes.  The source that returns `list` is kept. |
| `pool_init` | 0x2f904 | struct local `struct dpmi_block { unsigned int block[7]; unsigned int segments[3]; }; struct dpmi_block regs;` in place of the two arrays | `same-shape` | 44/44 | 107/129 | instruction 8: `byte 2: 0xfc against 0xf8` | local slot order | `[ebp-4]` = the argument `pool`, `[ebp-8]` = the return temporary, `[ebp-0xc]` = the `block[6]` status, `[ebp-0x10]` = `block[5]`, `[ebp-0x24]` = `block[0]`, `[ebp-0x30]` = the `segments` base | `[ebp-4]` = the return temporary, `[ebp-8]` = the argument `pool`, `[ebp-0x14]` = the `segments` base, `[ebp-0x18]` = the `block[6]` status, `[ebp-0x1c]` = `block[5]`, `[ebp-0x30]` = `block[0]` | Rejected.  It gave the same `same-shape` match with 44 of 44 instructions and 107 of 129 bytes, so the tie keeps the two-array form. |
| `memory_alloc_small` | 0x2f172 | struct local `struct dpmi_block { unsigned int block[7]; unsigned int segments[3]; }; struct dpmi_block regs;` in place of the two arrays, with the `unsigned int value;` local kept | `same-shape` | 49/49 | 120/147 | instruction 6: `byte 2: 0xf4 against 0xf0` | local slot order | `[ebp-4]` = the argument `address`, `[ebp-8]` = the argument `size`, `[ebp-0xc]` = the argument `handle`, `[ebp-0x10]` = the `value` local, `[ebp-0x14]` = the `block[6]` status, `[ebp-0x18]` = `block[5]`, `[ebp-0x1c]` = `block[4]`, `[ebp-0x24]` = `block[2]`, `[ebp-0x28]` = `block[1]`, `[ebp-0x2c]` = `block[0]`, `[ebp-0x38]` = the `segments` base, `[ebp-0x3c]` = the return temporary | `[ebp-4]` = the argument `address`, `[ebp-8]` = the `value` local, `[ebp-0xc]` = the return temporary, `[ebp-0x10]` = the argument `handle`, `[ebp-0x14]` = the argument `size`, `[ebp-0x20]` = the `segments` base, `[ebp-0x24]` = the `block[6]` status, `[ebp-0x28]` = `block[5]`, `[ebp-0x2c]` = `block[4]`, `[ebp-0x34]` = `block[2]`, `[ebp-0x38]` = `block[1]`, `[ebp-0x3c]` = `block[0]` | Kept.  It gave the `same-shape` match with 49 of 49 instructions and 120 of 147 bytes, against 118 of 147 bytes for the two-array form. |

The four local-slot modules keep the measured first difference and the idiom.
The cause is the compiler allocator local slot order, as an inference from the
measured original and rebuilt pair.  The before and after median byte match of
the six are in `docs/explanation/findings.md`, section "The six near-miss
source-form outcomes".

### The frame-size source batch

The batch changes the `loading_screen` and `stream_read` sources to the local
declaration set that gives the original `sub esp, imm`, and it reverts the
`race_lap_complete` source.  It keeps the C form, the bytes-first rank, and the
module recipes.  The three rows stay non-strict, so the batch adds no strict
form.  The store keeps 30 strict modules: 8 `exact-bytes` and 22
`exact-relocations`.  The before and after counts are the identity record
counts.  The first-difference column holds the first-difference report index
and text.

| Module | Record | Source change | Identity before | Identity after | First difference after (report) | Outcome |
| --- | --- | --- | --- | --- | --- | --- |
| `loading_screen` | 0x2a364 | `char name[0x308];` in place of `char name[0x320];` | 105/491 bytes, 55/124 instructions | 106/491 bytes, 55/124 instructions | instruction 6: `byte 2: 0xe4 against 0xf0` | Kept.  The frame is now `sub esp, 0x320`.  The remaining difference is the compiler allocator local slot order: the rebuilt argument slots sit at `[ebp - 0x10]`, `[ebp - 0x14]`, and `[ebp - 0x18]`, and the original slots sit at `[ebp - 0x1c]`, `[ebp - 0x18]`, and `[ebp - 0x20]`.  The higher byte count keeps the form. |
| `stream_read` | 0x4c4e5 | the `int read;` local for the raw `fread` result, with `result = read;` | 159/218 bytes, 72/77 instructions | 159/224 bytes, 76/79 instructions | instruction 6: `byte 2: 0xf4 against 0xe4` | Kept.  The frame is now `sub esp, 0x1c`, and the instruction match rises from 72/77 to 76/79.  The remaining difference is the local slot order: the rebuilt argument slots sit at `[ebp - 0x1c]` to `[ebp - 0x14]`, and the original slots sit at `[ebp - 0xc]` to `[ebp - 4]`.  The instruction match keeps the form on the tie in bytes. |
| `race_lap_complete` | 0x19678 | the `volatile int saved[3];` local and its three stores, then reverted | 41/314 bytes, 27/98 instructions | 41/314 bytes, 27/98 instructions | instruction 7: `byte 2: 0x1c against 0x10` | Reverted.  The stores wrote values that nothing reads, so the source is restored.  The frame stays `sub esp, 0x10`, and the first difference stays the `frame size` pair.  The behavior check fails on the markers 0 and 3, so the record stays `rebuilt`. |

The image build reports 13 patched, 12 reused, 322 skipped, 16 slots
relocated, and 1034754 of 1034923 bytes equal.  The batch does not change the
score, because each changed body stays below its record span and the build keeps
the original bytes.  The 4 `byte 1` `other` rows and the one `byte 0` `other`
row stay unchanged.  Their measured follow-ups are `net_player_add` at 0x2dddc
with instruction 6: `byte 1: 0xe5 against 0xc7`, `resource_read` at 0x2efc2
with instruction 4: `byte 1: 0xe5 against 0xc6`, `resource_find` at 0x2f25f
with instruction 5: `byte 1: 0xe5 against 0xc1`, `lzw_bit_read` at 0x4d844 with
instruction 6: `byte 1: 0xe5 against 0xc6`, and `text_draw` at 0x25020 with
instruction 1: `byte 0: 0x8b against 0x89`.  The batch does not convert them to
assembly.
