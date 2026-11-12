# Rebuild Quality Improvement Design

- **Date**: 2026-11-11
- **Status**: Approved
- **Authority**: `docs/explanation/findings.md` holds the evidence.

## Problem

The verified share of the rebuild store is low. The store holds 935 function
records. It holds 30 `verified` records. Only 23 of the 30 pass the strict
gate. The report is also not trustworthy, because the pipeline can pass a
record whose stored result is `fail`.

## Observations

- The store holds 652 `unknown`, 31 `named`, 221 `rebuilt`, 1 `matching`, and
  30 `verified` records.
- The 23 strict records are 20 hand-written assembly modules and 3 Watcom
  library C functions (`exit`, `exit_flush`, and `free`).
- No game function written in C is a strict match.
- The median rebuilt module matches 21.4% of instructions and 5.9% of bytes.
- Only 119 of 935 records declare a `count`. The other records stop at the
  first `ret`.
- The period Watcom 10.0a compiler and OpenWatcom V2 produce nearly the same
  code for six sampled high-match modules when they use the same flags.
- The period compiler improves some modules. `lzw_bit_read` moves from 27 to
  36 of 119 instructions. `list_destroy` in C moves from 19 to 25 of 25
  instructions with `-od`. `error_report` places its jump table and reaches
  16 of 16 instructions.
- The original `malloc` at `0xd1f25` is byte-identical to the `nmalloc` module
  in the 10.0a `CLIB3R.LIB`. The library listing maps `malloc_`, `free_`,
  `printf_`, `memcpy_`, and `memset_` to named modules.
- `run.py --skip-unchanged` prints `PASS` for a record with a stored `fail`.
- `pipeline_check.py` accepts that line when the fingerprint matches.
- A failed build does not demote a `verified` record.
- A failed behavior check does not demote a behavior-only `verified` record.
- The fingerprint does not include the strict mode or the toolchain.
- `oracle.py` masks every direct branch target, including intra-function
  branches.
- `oracle.py` ignores the object, target, and addend of an internal fixup.
- `le.py` stops a function body at the first `ret`.
- `le.py` does not stitch LE pages.
- The behavior check does not delete the old output. It accepts a line that
  ends with the expected text.
- The whole-image oracle is a skip while no rebuilt `STARTH.EXE` exists.

## Root Causes

| ID | Cause | Evidence |
| --- | --- | --- |
| RC-1 | The reconstruction is decompiled C, not the original C source. | The period and V2 compilers agree on six sampled modules with the same flags. |
| RC-2 | The record scope mixes game functions, library functions, and untriaged candidates. | 652 records are `unknown`. 13 named records match library symbols. |
| RC-3 | The verification pipeline has false accepts. | A stored `fail` prints `PASS` under `--skip-unchanged`. A failed build keeps `verified`. |
| RC-4 | The default identity gate accepts shape matches. | 7 of 30 `verified` records are shape matches. |
| RC-5 | The oracle does not verify branch targets, internal fixup targets, or full function extents. | `oracle.py:90`, `oracle.py:127`, `le.py:607`. |
| RC-6 | The period toolchain is not in the verification loop. | `run.py` always calls `harness.py`. No module holds `period_flags`. |
| RC-7 | Module recipes encode per-module flag guesses, mostly `-of+`. | 191 C sources use `-of+`. The original prologue census shows a large registers-first family. |
| RC-8 | Runtime library functions are reimplemented. | `malloc` at `0xd1f25` is byte-identical to `CLIB3R.LIB` module `nmalloc`. |
| RC-9 | The whole-image gate is not exercised. | No rebuilt `STARTH.EXE` exists. Stage 11 reports a skip. |

## Requirements

| ID | Requirement |
| --- | --- |
| FR-001 | `--skip-unchanged` must print the stored verdict. It must not print `PASS` for a stored `fail`. |
| FR-002 | A failed build or a failed behavior check must demote the record. A `verified` record must not keep `verified` after a failed check. |
| FR-003 | The input fingerprint must cover the strict mode and the toolchain. The result mode records the runtime, but the runtime must not invalidate a stored result when a behavior check is skipped. |
| FR-004 | Strict must be the default identity gate. The report must separate exact, relocation-only, shape, and behavior-only results. |
| FR-005 | A behavior run must delete the old output first. A marker must match a complete output line. |
| FR-006 | The oracle must compare intra-function branch targets. |
| FR-007 | The oracle must compare the symbol identity of an internal relocation target, not only the relocation kind. |
| FR-008 | A function extent must cover all reachable code. It must not stop at the first `ret` when more code belongs to the function. |
| FR-009 | `le.read` must stitch LE pages. |
| FR-010 | Every record must hold a provenance value: `game`, `library`, `compiler_helper`, `not_a_function`, or `data`. |
| FR-011 | The project must triage the 652 `unknown` records. |
| FR-012 | The project must report byte coverage and counts by provenance. |
| FR-013 | The project must index the 10.0a libraries and verify a library function against its original object. |
| FR-014 | The whole-image build must link the original library objects. |
| FR-015 | `run.py` must select the toolchain. Each module must record its best compiler and flag set. |
| FR-016 | The project must keep a source-form idiom catalogue and a first-difference report. |
| FR-017 | A rebuilt `STARTH.EXE` must exist and the whole-image oracle must be a mandatory stage. |
| FR-018 | The documentation and the check counts must match each change. |

## Success Measures

| ID | Measure |
| --- | --- |
| SM-1 | A record with a stored `fail` cannot produce a `PASS` line. |
| SM-2 | The strict and default verified counts are separate and reproducible. |
| SM-3 | Every record has a provenance value. |
| SM-4 | `malloc`, `fopen`, and `__fprtf` are exact. The seven library shape matches become byte-identical. |
| SM-5 | A rebuilt `STARTH.EXE` exists and `image_compare.py` reports a byte score. |

## Constraints

- C-1: Python 3.14.7. Use the standard library and the `ctypes` capstone binding only.
- C-2: Tests use `unittest` under `tools/rebuild/tests/`.
- C-3: A tool takes its configuration from the command line. It reads no environment variable.
- C-4: A tool writes only under `work/` or to an explicit output path.
- C-5: Prefer C. Use assembly only when C cannot reproduce the original bytes, and record the reason.
- C-6: A harness output name stays within the 8.3 limit.
- C-7: The 10.0a toolchain runs alongside OpenWatcom V2. Do not replace V2.
- C-8: The status ladder stays `unknown`, `not_a_function`, `named`, `rebuilt`, `matching`, `verified`.
- C-9: The identity verdict strings stay unchanged.
- C-10: The full check reports 303 checks today. `--skip-cross` reports 299, `--skip-runtime` reports 288, and both flags report 284. Update these counts when a change adds or removes a check.
- C-11: Do not commit. The user commits.

## Non-Goals

- No modern-platform port.
- No new third-party dependency.
- No reimplementation of a runtime library function that the original library holds.

## Design Decisions

- The result of an identity check gains a `match` field with one value:
  `exact-bytes`, `exact-relocations`, `same-shape`, `same-mnemonics`, or
  `different`.
- The result of a run gains a `mode` object with `strict`, `toolchain`, and
  `runtime`.
- The fingerprint hashes the strict mode and the toolchain. It does not hash
  the runtime mode, because a `--skip-runtime` run must not invalidate the
  stored build and identity results.
- Strict is the default. `--allow-shape` restores the old default gate.
- A failed build sets the status to `named` when the record has a name, and to
  `unknown` when it has no name. The failed result stays on the record.
- A failed behavior check sets the status to `matching` when identity passes,
  and to `named` or `unknown` when identity is not declared.
- Stage 11 remains a drift check. It accepts a stored `fail` as the expected
  result, but the progress line and the run summary must show `FAIL`.
