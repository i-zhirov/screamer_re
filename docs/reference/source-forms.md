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
  the first difference is a `byte N:` difference on the save bytes.  That
  difference classifies as `other`.

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
  original does not, so the instruction counts differ and the idiom is `other`.

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
  the argument in a register, so the instruction counts differ and the idiom is
  `other`.  When both bodies store at different frame slots, the idiom is
  `local slot order`.

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
  so the instruction counts differ and the idiom is `other`.  A changed
  instruction mnemonic gives `instruction selection`.

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
  into a shared tail, so the instruction counts differ and the idiom is
  `other`.  A `branch target:` difference that keeps the count gives the idiom
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
  omits `inc eax`, so the instruction counts differ and the idiom is `other`.

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
| no difference after the optional `instruction N: ` prefix | `match` |
| every other difference, including a different instruction count | `other` |

A `mask:` difference is `constant operand`, and a relocation-slot difference
also produces a `mask:` difference.  This overlap is a limitation of the
classification.  A `byte 2:` difference gives `local slot order` only when both
operands hold a stack frame displacement.

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
