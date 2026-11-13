# Rebuild Quality Improvement Roadmap

> Each slice gets its own plan, written when the slice starts. Execute a
> slice with the `executing-plans` skill.

- **Date**: 2026-11-11
- **Status**: Approved
- **Design**: `docs/explanation/rebuild-quality-design.md`
- **Strategy**: Make the reported number true first. Then widen the scope and
  remove work that the original libraries already solve. Then add the period
  toolchain and source-form work. The whole-image gate comes last, because it
  depends on the library and toolchain slices.

## Slices

### Slice 1 — baseline

Deliver a truthful verification baseline. The runner must not print `PASS` for
a stored `fail`. A failed check must demote the record. The fingerprint must
cover the mode. Strict becomes the default gate. The behavior check must use
fresh output and exact marker lines. The report must separate exact,
relocation-only, shape, and behavior-only results. This slice stands alone,
because every later slice needs a trustworthy number.

- **Status:** Implemented
- **Depends on:** —
- **Requirements:** FR-001, FR-002, FR-003, FR-004, FR-005, FR-018
- **Manual prerequisites:** Docker for the V2 harness, DOSBox-X for the
  behavior checks, and the installed Ghidra loader for the full check.
- **Plan:** docs/explanation/rebuild-quality-plan-baseline.md
- **Done when:** A stored `fail` cannot produce a `PASS` line. A failed build
  demotes a `verified` record. The strict and default counts are separate. The
  unit suite passes. The full check runs and its counts and docs match.

### Slice 2 — oracle

Make the identity oracle compare what the report claims. Compare
intra-function branch targets. Compare the target identity of an internal
fixup. Cover the whole function extent. Stitch LE pages. Re-baseline the strict
count after the change.

- **Status:** Implemented
- **Depends on:** Slice 1
- **Requirements:** FR-006, FR-007, FR-008, FR-009, FR-018
- **Manual prerequisites:** —
- **Plan:** docs/explanation/rebuild-quality-plan-oracle.md
- **Done when:** The tests reject a wrong branch and a wrong global reference.
  The full function extent is compared. A body that crosses an LE page decodes
  the correct bytes. The strict baseline is recorded.

### Slice 3 — scope

Add a provenance value to every record. Triage the unknown records.
Separate game functions, library functions, compiler helpers, non-functions,
and data. Report byte coverage and counts by provenance. This slice fixes the
denominator before more C work.

- **Status:** Implemented
- **Depends on:** Slice 1
- **Requirements:** FR-010, FR-011, FR-012, FR-018
- **Manual prerequisites:** The Ghidra dev shell, Docker, DOSBox-X, and the game directory.
- **Plan:** docs/explanation/rebuild-quality-plan-scope.md
- **Done when:** Every record has a provenance value. The analysis count falls
  to the `unknown` records with provenance `game`. The report shows byte
  coverage and provenance counts.

### Slice 4 — library

Index the 10.0a libraries. Match an original function to its library object.
Verify the object byte for byte. Mark the record as `library`. Link the original
objects in the whole-image build. Do not rewrite a runtime function that the
library already holds.

- **Status:** Unplanned
- **Depends on:** Slice 1, Slice 3
- **Requirements:** FR-013, FR-014, FR-018
- **Manual prerequisites:** The period Watcom 10.0a toolchain.
- **Plan:** —
- **Done when:** `malloc`, `fopen`, and `__fprtf` are exact. The seven library
  shape matches are byte-identical. The library index is reproducible from the
  toolchain.

### Slice 5 — toolchain

Add toolchain selection to the runner. Search the compiler and flag
combinations for each module. Record the best result. Use the period compiler
where it wins and V2 where it wins. Keep the two harnesses side by side.

- **Status:** Unplanned
- **Depends on:** Slice 1
- **Requirements:** FR-015, FR-018
- **Manual prerequisites:** DOSBox-X and the period Watcom 10.0a toolchain.
- **Plan:** —
- **Done when:** Each module records its best compiler and flags. The strict
  delta from the matrix is measured. The period toolchain runs in the store
  path.

### Slice 6 — source form

Build a source-form idiom catalogue. Add a first-difference report. Close the
six rebuilt modules above 80% instruction match. Keep C first. Use assembly
only after the exact toolchain, flags, and source forms fail, and record the
reason.

- **Status:** Unplanned
- **Depends on:** Slice 2, Slice 5
- **Requirements:** FR-016, FR-018
- **Manual prerequisites:** Human review of the source idioms.
- **Plan:** —
- **Done when:** The six near-miss modules improve. The median instruction
  match rises. The idiom catalogue is in the docs.

### Slice 7 — whole image

Link the rebuilt objects in the original object order with the original
libraries and data. Produce a rebuilt `STARTH.EXE`. Make
`image_compare.py` a mandatory stage. Track the byte score for each region.

- **Status:** Unplanned
- **Depends on:** Slice 3, Slice 4, Slice 5
- **Requirements:** FR-017, FR-018
- **Manual prerequisites:** The data pipeline and the game directory.
- **Plan:** —
- **Done when:** A rebuilt `STARTH.EXE` exists. The whole-image stage reports a
  byte score and is not a skip.

## Definition Of Done

All slices are Implemented. The design success measures SM-1 to SM-5 are met.
The unit suite and the full check pass. The documentation and the check counts
match the code. No Critical or Important review finding stays open.
