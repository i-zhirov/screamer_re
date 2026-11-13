# Library Object Verification Implementation Plan

> Execute this plan with the `executing-plans` skill. Each task is a
> checkbox. Mark it done when its changes are implemented and its Verify
> command passes.

- **Date**: 2026-11-13
- **Status**: Implemented
- **Design**: docs/explanation/rebuild-quality-design.md
- **Roadmap**: docs/explanation/rebuild-quality-roadmap.md — slice: library
- **Depends on**: Slice 1, Slice 3
- **Manual prerequisites**: The period Watcom 10.0a toolchain. Run
  `python3 tools/rebuild/watcom10_setup.py`; it fetches the image
  `WATCOM_C10A.ISO` (SHA-256
  `f51ed9358e7eaf6a0d292268bbbc15832530489dd8239a98f6938c244f1736fc`) and
  extracts `work/watcom10/WATCOM`. The libraries are under
  `work/watcom10/WATCOM/LIB386/DOS/`. Docker, DOSBox-X, and the Ghidra loader
  stay the prerequisites of the full check.
- **Goal**: Index the 10.0a libraries with a direct OMF parser, match an
  original function to its library object, verify the object under the strict
  gate, write the result into the record, and produce the link manifest that
  the whole-image build consumes.

## Constraints

- C-1: Python 3.14.7. Use the standard library and the `ctypes` capstone binding only.
- C-2: Tests use `unittest` under `tools/rebuild/tests/`.
- C-3: A tool takes its configuration from the command line. It reads no environment variable.
- C-4: A tool writes only under `work/` or to an explicit output path. The record writes of `triage.apply` and `library.apply` go through `store.save_record`, the only record writer. C-4 still applies to the shipped artifacts.
- C-5: Prefer C. Use assembly only when C cannot reproduce the original bytes, and record the reason.
- C-6: A harness output name stays within the 8.3 limit.
- C-7: The 10.0a toolchain runs alongside OpenWatcom V2. Do not replace V2.
- C-8: The status ladder stays `unknown`, `not_a_function`, `named`, `rebuilt`, `matching`, `verified`.
- C-9: The identity verdict strings stay unchanged.
- C-10: At the plan date, the full check reports 304 checks, `--skip-cross` reports 300, `--skip-runtime` reports 289, and both flags report 285. This plan adds one stage-12 check. The target is 305, 301, 290, and 286. Update these counts when a change adds or removes a check.
- C-11: Do not commit. The user commits.
- The index libraries are `CLIB3R.LIB`, `PC9833R.LIB`, and `EMU387.LIB`, because the driver link names `clib3r,pc9833r,emu387`.
- The library directory is `work/watcom10/WATCOM/LIB386/DOS/`. The index is `work/rebuild/library-index.json`. The committed link manifest is `tools/rebuild/library.json`.
- The library bytes stay out of git and out of the index. The index holds module, public-symbol, offset, fixup, page/dictionary, and SHA-256 metadata; the manifest holds the five-key entries and the library digests. No library bytes enter the index or the manifest.
- The library parser walks the OMF records directly. It needs no DOSBox-X and no `wlib`.
- The library identity gate is strict. It accepts only `exact-bytes` and `exact-relocations`. A `same-shape`, `same-mnemonics`, or `different` result fails. `--allow-shape` does not relax the library gate.
- The library gate masks the import target identity. Both sides rewrite every `import-ordinal`, `import-name`, and `entry` slot to the kind `import` before the oracle call, so the slot offset and the width compare and the module, ordinal, name offset, and target datum do not.
- The relocation-normalized comparison is the meaning of "byte-identical" in this slice. The linked original and the relocatable object differ at every link-time slot by construction.
- `object_body` decodes the object byte slice linearly. A function whose original body follows a compiler jump table is outside the plan-date acceptance set, because the object table entries are relocatable and the linear decode would compare table data. Such a body reports `different`; it does not pass falsely.
- The plan-date library identity matches are 4 `same-shape` (`printf` 0x3009b, `fprintf` 0x4bf64, `sprintf` 0x4d32c, `runtime_free` 0xd2008), 5 `exact-relocations`, 5 `exact-bytes`, 7 `different`, and 450 without an identity result. The design's "seven library shape matches" is the design-date count. The other three design-date shape matches are not library records: 0xd4324 (`compiler_helper`), 0x2f8ca (`game`), and 0xd4410 (`compiler_helper`). They stay outside the library measure. No tool hardcodes seven.
- The record `library` object holds exactly `file`, `sha256`, `module`, `symbol`, and `object_sha256`. The `sha256` values are `sha256:` plus 64 lowercase hexadecimal characters.
- The `object_sha256` is the SHA-256 of the public extent bytes with every fixup slot zeroed. The extent is `[symbol_offset, symbol_offset + size)` in the public symbol segment. The zeroed slot is the `width` bytes at `fixup.offset - symbol_offset` for each fixup with `symbol_offset <= fixup.offset < symbol_offset + size`.
- The `source_digest` is the SHA-256 of the canonical five-key `library` object with `json.dumps(..., sort_keys=True, separators=(",", ":"))`. Both `apply_record` and `run._write_result` hash the five-key `library` field, so the apply result is not stale on the first runner pass.
- A passing library identity writes `checks` as `["identity"]` and the status `verified`. A library-verified record declares only the identity check; the pass replaces the record-level build and behavior declarations and their result. The pass names an unnamed record from the library public symbol. The pass does not overwrite the top-level `symbol` of a record that keeps its module recipe.
- A failing library identity writes nothing. The record keeps its stored result and status.
- A library record's mode is `library.library_mode()`, which returns `store.run_mode(True, False)`. Both `apply_record` and `run.py` use it, so the CLI `--allow-shape` and `--skip-runtime` flags do not change the library fingerprint. The library object is pinned by the `library` digest and the fingerprint.
- The candidate order is the exact symbol entries, then the `name + "_"` entries, then the `name` entries, then the prefix matches; within each group the index order stays. The best candidate is the first strict pass in that order, else the first candidate.
- The fingerprint covers the library entry. `rebuild/library.py` joins `store.TOOL_FILES`. `rebuild/library.py` goes at the end of `TOOL_FILES`. `le.py`, `store.py`, `run.py`, and `oracle.py` are already inputs. Task 1 changes `le.py`, and Task 3 changes `store.py` and `TOOL_FILES`, so every stored fingerprint is stale until Task 6 re-baselines.
- The link manifest is the FR-014 input for Slice 7. This slice does not build `STARTH.EXE`.

## Spec Coverage

| Requirement | Task |
| --- | --- |
| FR-013 | Task 1 (index), Task 2 (match and verify), Task 3 (record field and fingerprint), Task 4 (write and runner), Task 6 (evidence pass) |
| FR-014 | Task 5 (link manifest and stage-12 check), Task 6 (manifest data) |
| FR-018 | Task 5 (check count), Task 7 (documentation and counts) |
| SM-4 | Task 6 (the plan-date library scope), Task 7 (the design-date non-library shape matches) |

FR-014 has a boundary. The whole-image build is Slice 7. This plan delivers a
reproducible link manifest and a check that the manifest agrees with the
records. The Slice 7 link adds the three libraries and uses the manifest to
assert that every verified library record resolves to its named object.

SM-4 has a boundary. It covers the plan-date library records: `malloc`,
`fopen` at 0xc5fe5, `__fprtf`, and the four plan-date library shape matches. The record at 0x4c08d is the game container member open, so its provenance becomes `game` and it leaves the library measure. The design-date
seven includes three shape matches that are not library records: 0xd4324
(`compiler_helper`), 0x2f8ca (`game`), and 0xd4410 (`compiler_helper`). Task 7
rewrites SM-4 to this scope and names the three records as outside the measure.

## Shared Interfaces

- `library.DEFAULT_LIBRARY_DIR: Path` is `REPOSITORY_ROOT / "work/watcom10/WATCOM/LIB386/DOS"`.
- `library.LIBRARIES: tuple[str, ...]` is `("CLIB3R.LIB", "PC9833R.LIB", "EMU387.LIB")`.
- `library.INDEX_DOCUMENT: Path` is `store.WORK_DIR / "library-index.json"`.
- `library.LINK_DOCUMENT: Path` is `store.REBUILD_DIR / "library.json"`.
- `library.OMF_HEADER = 3`; `library.LIBRARY_HEADER_RECORD = 0xF0`; `library.THEADR = 0x80`; `library.MODEND = (0x8A, 0x8B)`; `library.LNAMES = (0x96, 0xCA)`; `library.SEGDEF = {0x98: 2, 0x99: 4}`; `library.PUBDEF = {0x90: 2, 0x91: 4}`; `library.LEDATA = {0xA0: 2, 0xA1: 4}`; `library.FIXUPP = (0x9C, 0x9D)`.
- `library.FIXUP_WIDTHS = {0: 1, 1: 2, 2: 2, 3: 4, 4: 1, 5: 4, 6: 6, 9: 4, 11: 6, 13: 4}`. The location `5` is the PharLap 32-bit offset. An unknown location raises `ValueError` with the library, the module, and the slot offset; it does not skip the module silently.
- `library.PREFIX_BYTES = 16`.
- `library.OmfRecord` holds `command`, `offset`, `length`, and `payload`.
- `library.OmfModule` holds `name`, `offset`, `length`, `segments`, `publics`, and `fixups`.
- `library.OmfPublic` holds `name`, `segment`, `offset`, and `size`.
- `library.OmfFixup` holds `segment`, `offset`, `width`, `location`, and `target`. The `target` is `internal`, `import`, or `entry`.
- `library.read_library(path: Path) -> dict` returns one library document: `file`, `sha256`, `size`, `page_length`, `dictionary`, and `modules`. `dictionary` holds `offset`, `size`, and `sha256`. `modules` is a list of module documents with `name`, `offset`, `length`, `segments`, `publics`, and `fixups`. A segment holds `index`, `name`, `class`, `size`, and `data`.
- `library.build_index(paths: list[Path]) -> dict` returns `directory`, `libraries`, and `entries`. A library item holds `file`, `sha256`, `size`, `page_length`, `dictionary`, `module_count`, and `symbol_count`. It drops the segment `data`; the index holds the entry digests. An entry holds `file`, `sha256` (the library file digest), `module`, `module_offset`, `symbol`, `segment`, `offset`, `size`, `object_sha256`, `prefix_sha256`, and `fixups` (each with `offset`, `width`, `location`, and `target`).
- `library.load_index(path: Path) -> dict` validates the three top-level keys, the list types, and the entry keys. It raises `ValueError` with the path on a bad document.
- `library.format_index(index: dict) -> str` prints one line per library item with the seven library keys.
- `library.entry_for(index: dict, file: str, module: str, symbol: str) -> dict | None`.
- `library.candidates(index: dict, record: dict, original: le.LinearExecutable) -> list[dict]` returns the candidates in the pinned order: the exact symbol entries, the `name + "_"` entries, the `name` entries, then the prefix matches. Each group keeps the index order. The result is de-duplicated by `(file, module, symbol)`. The original supplies the LE relocation slots for the prefix query.
- `library.object_body(entry: dict, library_document: dict, base: int, count: int) -> list[le.Insn]` decodes the object code at `base` linearly for `count` instructions. The entry is a resolved entry.
- `library.object_fixups(entry: dict, base: int) -> list[le.Relocation]` builds the object slots in the original address space with the kinds `internal`, `import`, and `entry`.
- `library.normalize_imports(relocations: list[le.Relocation]) -> list[le.Relocation]` rewrites `import-ordinal`, `import-name`, and `entry` slots to `kind="import"` and drops the target fields. It keeps `offset` and `width`.
- `library.masked_bytes(instructions: list[le.Insn], relocations: list[le.Relocation]) -> int` returns the sum of the per-instruction mask sizes. The mask holds the structural branch bytes and the slot bytes.
- `library.format_verify(record: dict, original: le.LinearExecutable, library_document: dict, library_field: dict) -> str` resolves the entry, decodes both bodies, builds the object slots, computes the masked counts, and returns the line with the address, file, module, symbol, verdict, instruction count, byte count, and the masked counts.
- `library.library_field_from_entry(entry: dict) -> dict` returns the five `library` keys of an index entry: `file`, `sha256`, `module`, `symbol`, and `object_sha256`.
- `library.resolve_entry(library_document: dict, library_field: dict) -> dict` finds the module and the public symbol in the parsed document and rebuilds `file`, `module`, `module_offset`, `symbol`, `segment`, `offset`, `size`, `object_sha256`, `prefix_sha256`, and `fixups` from the library document. It raises `ValueError` when the module, the symbol, or the digest does not resolve.
- `library.verify_entry(record: dict, original: le.LinearExecutable, library_document: dict, library_field: dict) -> dict` returns the identity check result: `verdict`, `detail`, `match`, `bytes_equal`, `bytes_total`, `instructions_equal`, and `instructions_total`. It resolves the entry with `resolve_entry`, compares the field `sha256` with the document `sha256`, compares the field `object_sha256` with the resolved entry, normalizes the imports, and calls `oracle.compare`.
- `library.verify_candidate(record: dict, original: le.LinearExecutable, index: dict, library_dir: Path) -> tuple[dict, dict] | None` passes the original image to `candidates` for the prefix query, and returns the chosen index entry and the check of the first strict pass in candidate order, else the first candidate, or `None` when no candidate exists.
- `library.verify(record: dict, original: le.LinearExecutable, index: dict, library_dir: Path) -> tuple[str, dict] | None` returns the verdict and the check from `verify_candidate`.
- `library.source_digest(entry: dict) -> str` returns `sha256:` plus the SHA-256 of the canonical five-key object with `sort_keys=True` and `separators=(",", ":")`.
- `library.library_mode() -> dict` returns `store.run_mode(True, False)`.
- `library.apply_record(record: dict, original: le.LinearExecutable, index: dict, library_dir: Path) -> str` returns `written`, `skipped`, or `different`.
- `library.apply(records: list[dict], original: le.LinearExecutable, index: dict, library_dir: Path, addresses: list[int] | None) -> dict` returns the counts per outcome.
- `library.manifest_document(records: list[dict], index: dict) -> dict` returns `libraries` and `entries`. `libraries` maps each index library file to its digest. `entries` maps each record address with a `library` field to the five keys.
- `library.check_manifest(records: list[dict], document: dict, path: Path) -> list[str]` returns one message per violation.
- `le.decode_bytes(data: bytes, base: int, max_instructions: int = 64, end: int | None = None) -> list[Insn]` decodes a byte slice at a base address and stops before `end`.
- `store._library_fields(library: dict | None) -> bytes` returns `b"<none>"` for `None`, else the canonical five fields.
- `store.compute_fingerprint(record: dict, module: dict | None, *, library: dict | None = None, rebuild_dir=None, work_dir=None, tools_dir=None, original=None, mode=None) -> str` feeds the recipe, the module sources, and the shared sources only when `module` is not `None`, then the record fields, the library fields, the mode, the tools, the original, and the built artifacts when `module` is not `None`.
- `run.run_library(record: dict, original: Path, library_dir: Path) -> tuple[str, dict]` loads the image with `le.load`, loads the library document with `library.read_library`, calls `library.verify_entry(record, image, document, record["library"])`, and returns the verdict and the check. The runner uses `library.library_mode()` separately for the fingerprint. The runner reads the record entry, not the index.

## Tasks

- [x] **Task 1: The OMF library index and the byte decoder**
    - **Implements:** FR-013
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_le_helpers.py`**
        - Add `ByteDecodeTestCase`. Each method carries `@unittest.skipUnless(le.capstone_available(), "capstone is not available")`, so the bare unit suite runs without capstone and the cross-check shell runs the methods.
        - `test_decode_bytes_decodes_a_slice` — `55 89 e5 c3` at base `0x10000` gives three instructions and the base address.
        - `test_decode_bytes_uses_the_base_address` — the same bytes at base `0x20000` give the same mnemonics at the new addresses.
        - `test_decode_bytes_stops_at_the_end` — an end in the middle of the slice decodes no instruction past it.
        - `test_decode_bytes_returns_empty_for_empty_data` — the result is an empty list.
    - **Modify `tools/le.py`**
        - Add `decode_bytes(data, base, max_instructions=64, end=None)`. It mirrors `decode` on a byte slice. It truncates the slice at `end - base` when `end` is not `None`. An empty slice gives an empty list.
        - Update the `le.py` module docstring with the byte decoder.
    - **Create `tools/rebuild/tests/test_library.py`**
        - Add an OMF builder: `omf(command, payload)` as the raw-payload escape hatch, `header_page(page_length, dictionary_offset, dictionary_size)`, `theader(name)`, `lnames(names)`, `segdef(index, name_index, size, class_index=1)`, `pubdef(segment, entries, record_type=0x91)`, `ledata(segment, offset, data, record_type=0xA1)`, `fixupp(location, location_offset, target="internal", p_bit=0, thread=None)`, `modend()`, and `library(modules, page_length, dictionary)`. A test that needs a THREAD subrecord or a raw FIXUPP payload uses `omf` directly. The expected width of `fixupp` comes from `library.FIXUP_WIDTHS`.
        - Add a temporary-directory test case.
        - `test_index_reads_the_library_header` — the header record is `0xF0`; the page length, the dictionary offset, and the dictionary size are read.
        - `test_index_reads_a_module_name` — the THEADR name is the module name.
        - `test_index_reads_a_segment` — the SEGDEF name, the class, and the size are read from LNAMES.
        - `test_index_reads_a_public_symbol` — the PUBDEF name, the segment, and the 32-bit offset are read.
        - `test_index_reads_the_16_bit_public_offset` — the `0x90` record reads a 2-byte offset.
        - `test_index_concatenates_the_ledata_records` — two LEDATA records at different offsets give one segment and the object bytes.
        - `test_index_reads_a_fixup_slot` — the LOCAT location type and the 10-bit data-record offset give the slot offset and the width.
        - `test_index_reads_a_thread_subrecord` — a THREAD subrecord before a FIXUP subrecord does not change the location.
        - `test_index_reads_an_internal_fixup` — a target method 0 gives the target `internal`.
        - `test_index_reads_an_import_fixup` — a target method 2 or a target thread with method 2 gives the target `import`.
        - `test_index_rejects_an_unknown_fixup_location` — the error names the library, the module, and the slot offset.
        - `test_index_skips_the_dictionary` — the pages at or above `dict_start` are not modules.
        - `test_index_sizes_a_public_at_the_next_symbol` — the extent reaches the next public symbol in the same segment.
        - `test_index_sizes_the_last_public_at_the_segment_end` — the extent reaches the segment size.
        - `test_index_rejects_a_bad_library_header` — a first byte that is not `0xF0` raises `ValueError`.
        - `test_index_rejects_a_truncated_record` — a record length past the file raises `ValueError`.
        - `test_index_rejects_an_iterated_data_record` — an LIDATA record raises `ValueError`.
        - `test_index_is_stable` — the same input gives the same JSON document and the same entries.
        - `test_load_index_reads_the_document` — the loader returns the top-level keys.
        - `test_load_index_rejects_a_missing_file` — the error names the path.
        - `test_load_index_rejects_a_missing_entry_key` — an entry without `object_sha256` raises `ValueError`.
    - **Create `tools/rebuild/library.py`**
        - Add the module docstring and the usage block for the `index` subcommand. Each later task adds its subcommand to the usage block.
        - Add the constants, the four dataclasses, and the parser:
            - The library header sits at offset 0: the command `0xF0`, the 16-bit payload length at offset 1, the 32-bit dictionary offset at offset 3, and the 16-bit dictionary size at offset 7. `page_length = length + 3`.
            - The module pages start at `page_length`. A page whose first byte is `0x80` starts a module. Walk the OMF records from that byte until a `MODEND` record. The next page is `round_up(record_end, page_length)`.
            - A page whose first byte is not `0x80` and is below `dict_start` is a continuation page; skip it.
            - The walk ends at `dict_start`, or at the file end when `dict_start` is 0.
            - An OMF record is the command byte, the 16-bit length, and the payload.
            - LNAMES and LLNAMES hold length-prefixed names in order. The first name index is 1.
            - SEGDEF and SEGDEF32 hold the ACBP byte, the length field (`2` or `4` bytes by the record type), the name index, the class index, and the overlay index. The length field width is pinned by the record type.
            - PUBDEF and PUBDEF32 hold the base group, the base segment, the base frame, then repeated name, offset (`2` or `4` bytes by the record type), and type index. The entry offset is relative to the base segment.
            - LEDATA and LEDATA32 hold the segment index, the offset (`2` or `4` bytes by the record type), and the data. The data is copied into the segment at the offset.
            - FIXUPP and FIXUPP32 hold THREAD and FIXUP subrecords. The LOCAT field is a big-endian word: bit 15 is 1, bit 14 is M, bits 13-10 are the location type, and bits 9-0 are the data-record offset. The FixDat byte follows: bit 7 is F, bits 6-4 are FRAME, bit 3 is T, bit 2 is P, and bits 1-0 are TARGT. An explicit frame method 0, 1, or 2 has a frame datum index. The parser keeps four frame threads and four target threads across the record. A target thread has no target datum and takes its method from the thread table. An explicit target method 0, 1, or 2 has a target datum index; target method 2 is `import`, 0 or 1 is `internal`, and 3 is `entry`. A P bit of 0 has a 2-byte target displacement on `0x9C` and a 4-byte displacement on `0x9D`. The fixup stores the target class.
            - An unknown location type raises `ValueError` with the library name, the module name, and the slot offset. The index does not skip the module silently.
            - A segment data map holds the concatenated LEDATA bytes.
            - The public extent reaches the next public offset in the same segment, else the segment size.
        - Add `read_library`, `build_index`, `load_index`, `format_index`, `command_index`, `parse_arguments`, and `main`. `main` dispatches `index` and returns 0 on success and 2 on a bad input.
        - `index` resolves the three names case-insensitively in `--library-dir`. A missing file raises `ValueError` with the path.
        - `index` writes the document to `--out` (default `INDEX_DOCUMENT`). It prints the `format_index` summary.
        - `build_index` sorts the libraries by file, the modules by offset, the publics by `(segment, offset)`, and the entries by `(file, module_offset, segment, offset)`.
    - **Modify `tools/pipeline_check.py`**
        - Add `rebuild/library.py` to `TOOLS`.
    - **Verify:**
        - `python3 tools/rebuild/watcom10_setup.py` — the period toolchain prerequisite; the command is idempotent.
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_library.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_le_helpers.py"`
        - `python3 tools/rebuild/library.py index --out work/rebuild/library-index.json` — the summary names `CLIB3R.LIB`, its SHA-256, the module count, and the public symbol count.
        - `python3 -m py_compile tools/rebuild/library.py`

- [x] **Task 2: The candidate match and the object verification**
    - **Implements:** FR-013
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_library.py`**
        - Add a synthetic original body and a synthetic object code slice. Mock `le.function_body` and `library.object_body` (or `le.decode_bytes`), and give the fake original a `relocations()` method (or patch `le.LinearExecutable.relocations`). Build the object index from the OMF builder. The library tests need no capstone; only the `ByteDecodeTestCase` methods do, and they skip when it is absent.
        - `test_candidates_prefer_the_record_symbol` — the entries of the exact symbol come first.
        - `test_candidates_add_the_watcom_underscore` — a record name `malloc` matches the symbol `malloc_`.
        - `test_candidates_fall_back_to_the_prefix_digest` — a record with no symbol or name finds the object by the prefix digest, and the call takes the original image.
        - `test_candidates_are_unique_and_ordered` — a symbol that occurs in two modules gives two candidates in index order and no duplicate.
        - `test_candidates_keep_the_group_order` — the exact symbol entries come before the `name + "_"` entries, the `name` entries, and the prefix matches; each group stays in index order.
        - `test_library_field_from_entry_keeps_the_five_keys` — an index entry with the full keys gives exactly the five `library` keys, including the library file digest.
        - `test_resolve_entry_rebuilds_the_public_extent` — the module and the public symbol give the segment, the offset, and the next-symbol size.
        - `test_resolve_entry_rebuilds_the_fixups` — the fixups in the extent come back with the width and the target class.
        - `test_resolve_entry_rejects_a_missing_symbol` — the error names the module and the symbol.
        - `test_object_sha256_zeroes_the_fixup_slots` — the digest equals the hash of the extent with the slot bytes zeroed.
        - `test_prefix_sha256_zeroes_the_prefix_fixups` — the digest equals the hash of the first 16 bytes with the prefix slots zeroed.
        - `test_source_digest_is_the_canonical_five_key_object` — the value equals the hash of the five keys with `sort_keys=True` and `separators=(",", ":")`.
        - `test_verify_entry_accepts_the_record_field_shape` — the five-key record field resolves against the library document.
        - `test_verify_entry_accepts_the_index_field_shape` — the five keys from an index entry give the same check.
        - `test_verify_entry_rejects_a_file_digest_mismatch` — the field `sha256` differs from the document and the call raises `ValueError`.
        - `test_verify_entry_rejects_an_object_digest_mismatch` — the field `object_sha256` differs from the resolved entry and the call raises `ValueError`.
        - `test_format_verify_prints_the_masked_counts` — the line holds the address, the verdict, and the original and object masked counts.
        - `test_format_verify_does_not_change_the_check` — the check from `verify_entry` holds exactly the seven keys.
        - `test_verify_reports_exact_relocations` — the bodies differ only at the LE and OMF fixup slots. The result is `exact-relocations` and the verdict is `pass`.
        - `test_verify_reports_exact_bytes` — a body with no fixup slot is `exact-bytes`.
        - `test_verify_rejects_a_non_relocation_difference` — one unmasked byte differs. The result is not a strict match and the verdict is `fail`.
        - `test_verify_rejects_a_shape_match` — the mnemonics are equal and the shapes differ. The verdict is `fail`.
        - `test_verify_masks_the_object_fixup_slots` — the object slot bytes differ from the original and the result stays exact.
        - `test_verify_accepts_an_import_slot_with_a_different_target` — the original import ordinal and the object import name differ. The result stays exact.
        - `test_normalize_imports_masks_an_ordinal` — an `import-ordinal` slot becomes `kind="import"` with no module or ordinal.
        - `test_normalize_imports_masks_a_name` — an `import-name` slot becomes `kind="import"` with no module or name offset.
        - `test_normalize_imports_keeps_an_internal_slot` — an internal slot keeps its kind, its offset, and its width.
        - `test_verify_counts_the_masked_bytes` — the total is the original count plus the object count. Each side counts its own slots and branch bytes.
        - `test_verify_decodes_the_original_instruction_count` — the object body holds exactly the original count.
        - `test_object_body_is_linear_for_a_jump_table` — a synthetic object body with a jump table decodes the table data linearly, so the limitation is pinned.
        - `test_verify_uses_the_entry_size_as_the_end` — the original and object bodies cover the full entry extent.
        - `test_verify_returns_none_without_a_candidate` — the result is `None`.
        - `test_verify_prefers_the_first_strict_pass` — two candidates pass and the first strict pass in candidate order gives the check.
        - `test_verify_returns_the_first_candidate_when_none_passes` — two candidates fail and the first candidate in candidate order gives the check.
    - **Modify `tools/rebuild/library.py`**
        - Add `PREFIX_BYTES` and `STRICT_MATCHES` (from `oracle`).
        - Add `entry_for`, `candidates`, `library_field_from_entry`, `resolve_entry`, `normalize_imports`, `object_body`, `object_fixups`, `masked_bytes`, `verify_entry`, `verify_candidate`, `verify`, `source_digest`, and `format_verify`.
        - The candidate order is the exact symbol entries, the `name + "_"` entries, the `name` entries, then the prefix matches. Each group keeps the index order. The union is de-duplicated by `(file, module, symbol)`. `candidates` takes the original image for the LE relocation slots of the prefix query. The best candidate is the first strict pass in that order, else the first candidate.
        - The index `prefix_sha256` zeroes every object FIXUP byte in the first `min(PREFIX_BYTES, size)` bytes. The query zeroes every LE relocation byte and every object FIXUP position in the same range and compares the digest. A candidate whose fixups cover every compared prefix byte is skipped. The digest is the fast filter; the full comparison confirms the candidate.
        - `resolve_entry` takes the parsed library document and the five-key field. It finds the module by name and the public symbol by name, rebuilds the extent to the next public symbol in the segment, keeps the fixups in the extent, and computes `object_sha256` and `prefix_sha256`. `object_sha256` is the SHA-256 of the extent bytes with every fixup slot zeroed; the slot is `width` bytes at `fixup.offset - symbol_offset`.
        - `verify_entry` takes the record, the original image, the parsed library document, and the five-key field. It checks the field `sha256` against the document `sha256`, resolves the entry, checks the field `object_sha256` against the resolved entry, decodes the object at the original address, builds the object slots, normalizes the imports on both sides, and calls `oracle.compare` with the original relocations and the object slots. It passes no symbol resolvers, because the object has no link-time symbol table. The verify command builds the field with `library_field_from_entry(index_entry)`; `run.run_library` passes `record["library"]`.
        - `source_digest` hashes the five-key object with `sort_keys=True` and `separators=(",", ":")`.
        - The check `detail` is `oracle.VERDICTS[match]`. The check `verdict` is `pass` only for `exact-bytes` and `exact-relocations`.
        - `verify_entry` uses the entry `size` as the end of the original and the object body, because the extent is the library object. A zero entry size falls back to the record extent.
        - `format_verify(record, original, library_document, library_field)` resolves the entry, decodes both bodies, builds the object slots, computes `masked_bytes` on both sides, and returns the address, file, module, symbol, verdict, instruction count, byte count, and the two masked counts. `command_verify` calls `verify_entry`, calls `format_verify`, and prints the line; the masked counts do not enter the check result. `run_library` returns only the verdict and the check.
        - Extend `parse_arguments` and `main` with `verify` and update the usage block. `command_verify` accepts `--address` one or more times, `--original`, `--index`, and `--library-dir`. It loads the record; a record with a `library` field uses that field, else the matcher builds the field from the index entry. It loads the library document, calls `verify_entry`, and prints the line. A target without a strict pass exits 1.
        - Update the module docstring with the match order, the field shapes, the mask, and the strict library gate.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_library.py"`
        - `python3 tools/rebuild/library.py verify --address 0xd1f25 --address 0xc5fe5 --address 0xd429f --address 0x3009b --address 0x4bf64 --address 0x4d32c --address 0xd2008 --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE` — each line shows a strict match, and the command exits 0.

- [x] **Task 3: The record field, the fingerprint, and the store rule**
    - **Implements:** FR-013
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_store.py`**
        - Add a `library` fixture with the five keys and valid digests.
        - Add the same `library` fixture to `full_record`, so `test_schema_matches_record_properties` still compares the record keys with the schema properties.
        - Add a `library` keyword-only parameter to the `fingerprint_of` helper with default `None`, after `mode`, so the existing positional `mode` calls stay valid. Use `library=...` in the new tests.
        - `test_schema_accepts_a_library_field` — a record with the fixture validates with no violation.
        - `test_schema_rejects_a_library_without_the_symbol` — the violation names `symbol`.
        - `test_schema_rejects_a_bad_library_digest` — a `sha256` without the prefix gives one violation.
        - `test_fingerprint_changes_with_the_library_entry` — two entries that differ in `symbol` give different fingerprints.
        - `test_fingerprint_without_a_module_is_stable` — a record with a library entry and no module gives the same fingerprint twice.
        - `test_fingerprint_without_a_module_changes_with_the_original` — two original files give different fingerprints.
        - `test_identity_with_a_library_field_needs_no_module` — a record with `checks` `["identity"]`, a library field, and no module gives no violation.
        - `test_identity_without_a_module_and_without_a_library_still_fails` — the existing rule stays.
        - `test_library_field_is_a_record_input` — a difference in the library entry changes the fingerprint of a record with a module.
        - `test_fingerprint_ignores_a_name_only_change` — a changed `name` gives the same fingerprint, because `_record_fields` excludes `name`.
        - `test_fingerprint_ignores_a_status_only_change` — a changed `status` gives the same fingerprint.
    - **Modify `tools/rebuild/contracts/record.schema.json`**
        - Add the optional `library` property after `result`. It holds `file`, `sha256`, `module`, `symbol`, and `object_sha256`, all required, with `additionalProperties: false`. `sha256` and `object_sha256` use `^sha256:[0-9a-f]{64}$`. The description names the original library object of a library-verified record.
    - **Modify `tools/rebuild/store.py`**
        - Add `_library_fields(library)`.
        - Add `"library"` to the `_record_fields` key list. The list stays `address`, `symbol`, `count`, `size`, `checks`, `module`, and `library`; it excludes `name` and `status`.
        - Change `compute_fingerprint` to `module: dict | None` and add the keyword-only `library: dict | None = None`. Feed the recipe, the module sources, and the shared sources only when `module` is not `None`. Feed the record fields, then the library fields, then the mode, the tools, the original, and the built artifacts when `module` is not `None`.
        - Add `rebuild/library.py` to `TOOL_FILES`.
        - In `validate_record`, apply the identity module rule only when `record.get("library") is None`.
        - Update the `compute_fingerprint`, `_record_fields`, and `TOOL_FILES` docstrings.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_store.py"`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE` — the shipped records hold no library field, so the new schema is satisfied and the store check stays clean.

- [x] **Task 4: The record write and the runner path**
    - **Implements:** FR-013
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_library.py`**
        - `test_apply_writes_the_identity_check` — the record holds one identity result and no build or behavior check.
        - `test_apply_fingerprint_covers_the_checks_and_the_library_field` — the fingerprint changes when `checks` or the `library` field changes, and a name-only change gives the same fingerprint.
        - `test_apply_uses_the_library_mode` — the result mode is `store.run_mode(True, False)`.
        - `test_apply_sets_the_verified_status` — the status is `verified`.
        - `test_apply_names_an_unnamed_record` — the name comes from the library symbol without the trailing underscore.
        - `test_apply_keeps_the_module_symbol` — the top-level `symbol` of a record with a module stays.
        - `test_apply_writes_the_library_field` — the five keys equal the matched entry.
        - `test_apply_writes_the_fingerprint` — the fingerprint equals `store.compute_fingerprint(record, None, library=field, mode=library.library_mode(), ...)`.
        - `test_apply_writes_the_source_digest` — the value is `sha256:` plus 64 hexadecimal characters.
        - `test_apply_appends_the_command_evidence` — the record holds one `command` item for the verify command and a second apply adds no duplicate.
        - `test_apply_is_idempotent` — a second apply leaves the record equal.
        - `test_apply_leaves_a_different_record_unchanged` — the record bytes stay.
        - `test_apply_reports_the_outcomes` — the counts hold `written`, `skipped`, and `different`.
    - **Modify `tools/rebuild/tests/test_run.py`**
        - Add a library record fixture with the five `library` keys.
        - `test_library_record_runs_the_object_check` — mock `library.verify_entry` and assert one identity check and no build or behavior check.
        - `test_library_record_writes_the_library_fingerprint` — the result fingerprint uses `library=record["library"]`, `module=None`, and `mode=library.library_mode()`.
        - `test_library_record_uses_the_record_field` — the runner calls `verify_entry` with the parsed document and the five-key record field, not an index entry.
        - `test_library_record_fingerprint_ignores_the_cli_mode` — the default run and a `--skip-runtime` run give the same library fingerprint.
        - `test_library_record_skip_unchanged` — a matching stored fingerprint prints the stored verdict and no check runs.
        - `test_library_record_failed_check_demotes` — a failed identity sets `rebuilt` and keeps the failed result.
        - `test_library_record_missing_library_raises` — a missing file raises `ValueError`.
        - `test_parse_arguments_accepts_the_library_dir` — the parsed value is a `Path`.
    - **Modify `tools/rebuild/tests/test_view.py`**
        - Add a record fixture with a `library` field and a matching `result.source_digest`.
        - `test_page_marks_a_library_record_stale_by_the_library_field` — a matching library source digest gives `data-stale="false"`; a changed library field gives `data-stale="true"` and the page counts one stale record.
        - `test_page_marks_a_library_record_by_the_library_field_not_the_module` — the module source digest differs and the library digest matches; the record is not stale.
    - **Modify `tools/rebuild/tests/test_pipeline_check.py`**
        - `test_unchanged_is_verified_uses_the_library_entry` — a record with a `library` field verifies with `module=None` and the entry.
        - `test_unchanged_is_verified_rejects_a_changed_library_entry` — a changed entry fails.
    - **Modify `tools/rebuild/library.py`**
        - Add `source_digest` (already used by `verify`), `apply_record`, `apply`, and `command_apply`. `apply_record` calls `verify_candidate`, builds the field with `library_field_from_entry(entry)`, and writes the first strict pass.
        - `apply_record` runs only after a strict pass. It sets the record name, the library field, `checks`, the status, the evidence, the result (including `result.mode`), the source digest, and the fingerprint. It saves through `store.save_record`. It writes nothing for `different`. It uses `library.library_mode()` and `source_digest(record["library"])`.
        - Extend `parse_arguments` and `main` with `apply` and update the usage block. `command_apply` accepts `--original`, `--index`, `--library-dir`, and `--address` one or more times. Without `--address` it applies every record with provenance `library`. With `--address`, a target without a strict pass exits 1. Without `--address`, the command prints the counts and exits 0.
        - `apply_record` computes the fingerprint after it sets the name, the `library` field, `checks`, and the status.
        - The result `command` is `python3 tools/rebuild/library.py verify --address 0x<address>`. The result `tool` is `library.py`. The evidence item is a `command` item with the same text.
    - **Modify `tools/rebuild/run.py`**
        - Add `import le` and `import library`.
        - Add `run_library(record, original, library_dir)`. It loads the image with `le.load`, loads the library document with `library.read_library(library_dir / record["library"]["file"])`, and calls `library.verify_entry(record, image, document, record["library"])`.
        - In `_run_record`, a record with a `library` field runs `run_library` and no module. The other rules stay.
        - In `_current_fingerprint` and `stale_fn`, a record with a `library` field passes `module=None`, `library=record["library"]`, and `mode=library.library_mode()`.
        - Change `_result` to `_result(checks, module, command, fingerprint, mode, source_digest: str | None = None)`. The existing `test_result_holds_the_mode` positional call `_result(checks, module, command, fingerprint, mode)` stays valid. `_write_result` loads no module for a record with a `library` field; it computes the fingerprint with `module=None`, the field, and `library.library_mode()`, and passes `library.source_digest(record["library"])` and the same mode.
        - Add `--library-dir` with `type=Path` and the default `library.DEFAULT_LIBRARY_DIR`.
        - Update the `run.py` module docstring and the usage block.
    - **Modify `tools/rebuild/view.py`**
        - Add `import library`.
        - In `_is_stale`, a row with a `library` field compares `result.source_digest` with `library.source_digest(row["library"])`. The module digest map stays for the other rows.
        - Update the `_is_stale` docstring and the module docstring.
    - **Modify `tools/pipeline_check.py`**
        - Add `import library` beside the `store` import.
        - In `unchanged_is_verified`, a record with a `library` field computes the fingerprint with `module=None`, `library=record["library"]`, and `mode=library.library_mode()`, then compares it with the stored value.
        - Update the `unchanged_is_verified` docstring.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_library.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_run.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_store.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_view.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_pipeline_check.py"`

- [x] **Task 5: The link manifest and the stage-12 check**
    - **Implements:** FR-014, FR-018
    - Write the tests first.
    - **Modify `tools/rebuild/tests/test_library.py`**
        - `test_manifest_lists_every_library_record` — one entry per record with a `library` field, sorted by address.
        - `test_manifest_lists_the_libraries` — one digest per index library file, sorted by name.
        - `test_manifest_is_stable` — the same records give the same document.
        - `test_check_manifest_accepts_a_matching_document` — no message.
        - `test_check_manifest_reports_a_missing_entry` — a record with a library field and no entry gives one message that names the address.
        - `test_check_manifest_reports_a_stale_entry` — an entry that differs from the record field gives one message.
        - `test_check_manifest_reports_an_entry_without_a_record` — one message.
        - `test_check_manifest_reports_a_missing_library_digest` — one message.
        - `test_check_manifest_reports_a_wrong_library_digest` — a manifest `libraries[file]` digest that differs from both the record `library.sha256` and the entry `sha256` gives one combined message for the record.
        - `test_check_manifest_reports_a_duplicate_object` — two entries with the same `(file, module, symbol)` but different five keys give one message. Two byte-identical entries for the same object are valid reuse and give no message.
        - `test_check_manifest_reports_a_record_that_is_not_verified` — a record with a library field and the status `rebuilt` gives one message.
        - The check applies to the records whose identity is the library object. A record verified by its C or assembly rebuild holds no `library` field and needs no manifest entry, because its object is not the source of its identity.
    - **Modify `tools/rebuild/tests/test_pipeline_check.py`**
        - `test_stage_twelve_checks_the_library_manifest` — mock `pipeline_check.run` and assert the `library.py check-manifest` command and the check name `stage 12: the library link manifest matches the records`.
    - **Modify `tools/rebuild/library.py`**
        - Add `manifest_document`, `check_manifest`, `command_manifest`, and `command_check_manifest`.
        - `manifest_document` takes the records with a `library` field and the index. It returns `libraries` (file to digest) and `entries` (address to the five keys).
        - Extend `parse_arguments` and `main` with `manifest` and `check-manifest` and update the usage block. `check_manifest` reports a record with a `library` field and no entry, an entry without a record, an entry that differs from the field, a `library.file` without a digest, one combined message per record when the record `library.sha256`, the entry `sha256`, and the manifest `libraries[file]` do not agree, a duplicate `(file, module, symbol)`, and a record whose status is not `verified`.
        - `command_manifest` reads `--index` (default `INDEX_DOCUMENT`) and writes `--out` (default `LINK_DOCUMENT`). A missing index raises `ValueError`. `command_check_manifest` reads `--manifest` (default `LINK_DOCUMENT`) and exits 1 on a message.
    - **Modify `tools/pipeline_check.py`**
        - In `check_store`, run `library.py check-manifest` and add the check `stage 12: the library link manifest matches the records`.
        - Update the stage 12 docstring.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_library.py"`
        - `python3 -m unittest discover -s tools/rebuild/tests -p "test_pipeline_check.py"`
    - Do not run the pipeline before Task 6. The manifest is written in Task 6, so the stage-12 check fails until then.

- [x] **Task 6: The library evidence pass and the re-baseline**
    - **Implements:** FR-013, FR-014, FR-018
    - No new unit test. This task writes data. The Verify commands prove the data.
    - **Steps:**
        - Confirm that Docker and DOSBox-X are on `PATH`. The re-baseline needs both.
        - Run `python3 tools/rebuild/watcom10_setup.py`. It verifies the ISO digest and extracts `work/watcom10/WATCOM`.
        - Run `python3 tools/rebuild/library.py index --out work/rebuild/library-index.json`. Record the three library SHA-256 values, the page length, the dictionary offset, and the module and public symbol counts.
        - Run `python3 tools/rebuild/library.py verify --address 0xd1f25 --address 0xc5fe5 --address 0xd429f --address 0x3009b --address 0x4bf64 --address 0x4d32c --address 0xd2008 --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`. Each line must show a strict match. Record the masked byte count of each line. If a line shows `same-shape`, `same-mnemonics`, or `different`, stop and report the line; do not write a passing result.
        - Add a record entry for `0x4c08d` to `tools/rebuild/provenance.json` with provenance `game` and the finding `## The fopen module at 0x4c08d` as evidence. Run `python3 tools/rebuild/triage.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE` and `python3 tools/rebuild/triage.py apply --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`. The record leaves the library measure.
        - Run `python3 tools/rebuild/library.py apply --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`. Record the `written`, `skipped`, and `different` counts. A record without a candidate is `skipped`; a candidate that does not pass is `different`.
        - Run the same `library.py apply` command a second time. Confirm that the `written` count is 0 and every applied record is `skipped`; the pass is idempotent.
        - Run `python3 tools/rebuild/library.py manifest --index work/rebuild/library-index.json --out tools/rebuild/library.json`.
        - Run `python3 tools/rebuild/library.py check-manifest --manifest tools/rebuild/library.json`.
        - Re-baseline the store: `python3 tools/rebuild/run.py run --all`. The command prints the known failures and exits 1. That is the expected result. Every result now holds the new library fingerprint, and every library record with a `library` field is re-verified through `run.py`.
        - Run `python3 tools/rebuild/run.py run --all --skip-unchanged` and confirm that every record with a stored result prints the unchanged detail with its stored verdict. A record with no stored result prints `SKIP`.
        - Run `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`.
        - Run `python3 tools/rebuild/callgraph.py build` and `python3 tools/rebuild/callgraph.py check`.
        - Run `python3 tools/rebuild/docs.py render` and `python3 tools/rebuild/docs.py check`.
        - Run `python3 tools/rebuild/view.py html --out work/rebuild/status.html`. Confirm that the page holds no `data-stale="true"` marker.
        - Spot check `tools/rebuild/functions/0xd1f25.json`: it holds the `library` object, `checks` `["identity"]`, the status `verified`, and `result.checks.identity.match` of `exact-bytes` or `exact-relocations`.
        - Spot check `tools/rebuild/library.json`: it holds the three library digests and one entry for each record with a `library` field.
    - **The acceptance bar:** `malloc` 0xd1f25, `fopen` 0xc5fe5, and `__fprtf` 0xd429f hold a strict match. The record at 0x4c08d is the game container member open, so its provenance becomes `game` and it leaves the library measure. Each of the four plan-date `same-shape` records holds a strict match. The plan-date acceptance set has no compiler jump table; a target whose original body follows one is reported, not written. The library index is reproducible from the toolchain.
    - **Verify:**
        - `python3 tools/rebuild/library.py index --out work/rebuild/library-index.json`
        - `python3 tools/rebuild/library.py verify --address 0xd1f25 --address 0xc5fe5 --address 0xd429f --address 0x3009b --address 0x4bf64 --address 0x4d32c --address 0xd2008 --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
        - `python3 tools/rebuild/library.py apply --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE` — the first pass writes the library results; a second identical run writes 0 records.
        - `python3 tools/rebuild/library.py manifest --index work/rebuild/library-index.json --out tools/rebuild/library.json`
        - `python3 tools/rebuild/library.py check-manifest --manifest tools/rebuild/library.json`
        - `python3 tools/rebuild/run.py run --all --skip-unchanged` — after the `run.py run --all` re-baseline step, every record with a stored result prints the unchanged detail and its stored verdict; a record without a result prints `SKIP`.
        - `python3 tools/rebuild/view.py html --out work/rebuild/status.html`
        - `! grep -q 'data-stale="true"' work/rebuild/status.html`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
        - `python3 tools/rebuild/docs.py check`
        - `python3 tools/rebuild/callgraph.py check`

- [x] **Task 7: The documentation and the check counts**
    - **Implements:** FR-018
    - **Modify `AGENTS.md`**
        - Project Structure: add `library.py` under `tools/rebuild/` and `library.json` beside the records.
        - Build And Test Commands: add `library.py` to the `py_compile` list in Build And Test Commands and to the second `py_compile` list in Contribution Instructions. Add the `library.py index`, `library.py verify`, `library.py apply`, `library.py manifest`, and `library.py check-manifest` commands. Update the four check counts to 305, 301, 290, and 286.
        - Testing: add a bullet for the library index and the direct OMF parser, a bullet for the library match order and the strict library gate, a bullet for the `library` record field and the link manifest, and a bullet for the library fingerprint. State that the byte-decode tests skip when capstone is absent, so the syntax job runs without it.
        - Code Guidelines → Architecture: state that a library record holds its original object in the `library` field and that the object bytes stay out of git.
        - Update the four check counts at both places in the file.
    - **Modify `README.md`**
        - Add a Status bullet: a library object index, a strict library identity check, and a link manifest for the whole-image build.
        - Update the four check counts.
    - **Modify `docs/reference/file-formats.md`**
        - Add a section for the OMF library: the `0xF0` library header with the 3-byte record header, the page length, the dictionary offset and size; the page-aligned module page from THEADR to MODEND; the LNAMES, SEGDEF, SEGDEF32, PUBDEF, PUBDEF32, LEDATA, LEDATA32, LIDATA, FIXUPP, and FIXUPP32 records; the LOCAT and FixDat layout; the THREAD table; the location types and widths; and the import target class.
        - Record the observed plan-date location types after Task 6, and mark the others as the specification set.
    - **Modify `docs/reference/tooling.md`**
        - Add `tools/rebuild/library.py` to the rebuild tool table.
        - Add a section for the library index: the three library files, the index path, the direct OMF parser, the page and dictionary layout, the match order, the strict library gate, and the fact that the library bytes stay out of git.
        - Add a section for the link manifest: the path, the document shape, the check, and the Slice 7 consumer.
        - Add the `library` record field and the fingerprint input to the `store.py` paragraph.
        - State that the status page marks a library record stale by the library source digest, not by the module source digest.
        - Update the four check-count lines from 304, 300, 289, and 285 to 305, 301, 290, and 286.
    - **Modify `docs/explanation/findings.md`**
        - Insert `## The library object index and the verified library functions` before `## Open questions`.
        - Record the three library SHA-256 values, the page length, the dictionary offset, and the module and public symbol counts. Command: `python3 tools/rebuild/library.py index --out work/rebuild/library-index.json`.
        - Record the strict results of `malloc`, `fopen`, `__fprtf`, and the four plan-date shape records, with the masked byte count of each. Command: `python3 tools/rebuild/library.py verify --address 0xd1f25 --address 0xc5fe5 --address 0xd429f --address 0x3009b --address 0x4bf64 --address 0x4d32c --address 0xd2008 --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`.
        - Record the `written`, `skipped`, and `different` counts of the apply pass. Command: `python3 tools/rebuild/library.py apply --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`.
        - Record the manifest entries and the check. Command: `python3 tools/rebuild/library.py check-manifest --manifest tools/rebuild/library.json`.
        - In the count bullet near `## The record scope and the provenance`, mark 304, 300, 289, and 285 as the plan-date counts and add 305, 301, 290, and 286. Record the four counts from the pipeline commands in the new section.
    - **Modify `docs/explanation/rebuild-quality-design.md`**
        - Mark the four C-10 counts 304, 300, 289, and 285 as the plan-date counts. Add the target 305, 301, 290, and 286.
        - In Observations, add the plan-date library counts: 471 library records, 4 `same-shape`, 5 `exact-relocations`, 5 `exact-bytes`, 7 `different`, and 450 without an identity result. State that the design-date seven shape matches split into four library records and three records outside the library scope: 0xd4324 (`compiler_helper`), 0x2f8ca (`game`), and 0xd4410 (`compiler_helper`).
        - Rewrite SM-4 to: "`malloc` 0xd1f25, `fopen` 0xc5fe5, and `__fprtf` 0xd429f are exact. The four plan-date library shape matches are byte-identical. The record at 0x4c08d is the game container member open, not a library object, so it leaves the library measure. The design-date shape matches 0xd4324 (`compiler_helper`), 0x2f8ca (`game`), and 0xd4410 (`compiler_helper`) are outside the library measure."
        - Extend the C-4 exception to name `triage.apply` and `library.apply`: both write function records through `store.save_record`, and C-4 still applies to the shipped artifacts.
        - Add a Design Decisions bullet: a library record holds its original object in the `library` field; the library identity uses the strict gate and masks the import target identity; a library-verified record declares only the identity check and replaces the build and behavior declarations and their result; the object bytes stay out of git; the library mode is `store.run_mode(True, False)`.
    - **Modify `.github/workflows/pipeline.yml`**
        - Add `tools/rebuild/library.py` to the syntax-job `py_compile` list, after `tools/rebuild/callgraph.py`.
        - The syntax job installs no capstone. The `ByteDecodeTestCase` methods skip there; the cross-check shell runs them. State this in the AGENTS Testing bullet.
    - **Modify `docs/explanation/rebuild-quality-roadmap.md`**
        - Change the Slice 4 `Done when` to: "`malloc`, `fopen` at 0xc5fe5, and `__fprtf` are exact. The four plan-date library shape matches are exact. The record at 0x4c08d is a game function and leaves the library measure. The three design-date non-library shape matches are outside the library measure. The library index is reproducible from the toolchain."
        - Set the Slice 4 `Status` to `Implemented` and the `Plan` to `docs/explanation/rebuild-quality-plan-library.md`. Do this after the final review approves the slice.
    - **Steps:**
        - Run `python3 -m py_compile tools/le.py tools/inventory.py tools/extract_cd_image.py tools/dosboxx_control.py tools/entry_probe.py tools/gfx_container.py tools/export_assets.py tools/sound_bank.py tools/member_probe.py tools/cross_check.py tools/rebuild/diff_module.py tools/rebuild/oracle.py tools/rebuild/store.py tools/rebuild/schema.py tools/rebuild/image_compare.py tools/rebuild/harness.py tools/rebuild/harness_dos.py tools/rebuild/watcom10_setup.py tools/rebuild/run.py tools/rebuild/view.py tools/rebuild/docs.py tools/rebuild/callgraph.py tools/rebuild/triage.py tools/rebuild/coverage.py tools/rebuild/library.py tools/pipeline_check.py`.
        - Run the four pipeline commands in Verify.
    - **Verify:**
        - `python3 -m unittest discover -s tools/rebuild/tests`
        - `python3 tools/rebuild/store.py check --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE`
        - `python3 tools/rebuild/library.py check-manifest --manifest tools/rebuild/library.json`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-cross`
        - `nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime`
        - `python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime --skip-cross`
        - The full check reports 305 checks, `--skip-cross` reports 301, `--skip-runtime` reports 290, and both flags report 286.

## Deviations

- The plan names `PC9833R.LIB` as an index library. That file does not
  exist in the pinned Watcom 10.0a image. The name in `findings.md` is a
  typo for the OpenWatcom V2 library `pc983r.lib`. The user approved the
  period math library `MATH387R.LIB` in its place. The index resolves a
  library in `--library-dir` and then in its parent, because
  `MATH387R.LIB` is in `LIB386`, not `LIB386/DOS`.
- The plan rejects an LIDATA record. The shipped `CLIB3R.LIB` and
  `EMU387.LIB` contain valid LIDATA and LIDATA32 records, so the parser
  decodes them. A malformed iterated block still raises `ValueError`.
- The plan pins each record field width by the record command. The
  `EMU387.LIB` module `386inite` is a PharLap class `AA` module whose
  `0x98`, `0x90`, and `0xA0` records use 4-byte fields, so the parser
  detects that class.
- The plan treats the header field at offset 7 as a byte size. The field
  is a count of 512-byte dictionary blocks. The index stores and hashes
  the byte size.
- A PharLap `0x9C` FIXUPP record with the P bit 0 skips a 4-byte target
  displacement, as the `0x9D` record does.
- The index drops an absolute public symbol and counts only the emitted
  entries, so `symbol_count` agrees with the entry list.
- The plan passes the original and the object slots to `oracle.compare`.
  A relative call resolved at link time has an object fixup but no LE
  relocation. The byte mask is therefore the union of both slot sets. A
  separate slot-geometry check fails an original slot that has no object
  slot at the same offset and width, and a width mismatch. An object-only
  slot is allowed.
- The plan uses the record `size` as the end of the original body. The
  full library object extent is the entry `size`. The comparison decodes
  the original and the object over that extent, so a body that stops at an
  early `ret` cannot under-verify the rest of the object.
- The plan target for the full check is 305, 301, 290, and 286. The
  observed counts are 431, 427, 416, and 412. The library pass and the
  seed add 126 result records, and the new stage-12 check adds one, so
  the count grows by 127. Stage 11 checks each result record. All four
  runs pass with 0 failures.
- The seed's target scan found 10 mapped call targets below the new
  verified library roots. The seed wrote the 10 records, so the function
  count is 981. The target scan skips a call target that no LE object
  maps, so one unmapped target no longer stops the seed.
- The plan-date identity counts are 4 `same-shape`, 5
  `exact-relocations`, and 5 `exact-bytes`. The full-extent comparison,
  the prefix fallback, and the 10 seed records give 138 strict library
  matches: 106 `exact-relocations` and 32 `exact-bytes`. The apply pass
  writes 138 records, skips 338, and reports 4 `different`. The 4 are
  `free` 0x300ec, `memcpy` 0x8c150, `memset` 0x8c160, and the unnamed
  0xeb1e7. The second apply writes 0 records.
- The plan reports a duplicate `(file, module, symbol)` as a manifest
  violation. The original links four library objects twice at different
  record addresses with different link-time data addresses. The user
  approved that two byte-identical entries for the same object are valid
  reuse. A duplicate violation needs different five keys.
- The plan names `fopen` at 0x4c08d as a strict library match.
  `findings.md` proves that 0x4c08d is the game container member open.
  The library `fopen_` object matches the unnamed library record at
  0xc5fe5 as `exact-relocations`. The user approved the correction: the
  acceptance set, the verify commands, and SM-4 name 0xc5fe5. The record
  at 0x4c08d becomes provenance `game` and leaves the library measure.
