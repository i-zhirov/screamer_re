# AGENTS.md

## Table of Contents

- [Project Overview](#project-overview)
- [Technical Context](#technical-context)
- [Project Structure](#project-structure)
- [Build And Test Commands](#build-and-test-commands)
- [Contribution Instructions](#contribution-instructions)
- [Code Guidelines](#code-guidelines)
    - [System Design](#system-design)
    - [Architecture](#architecture)
    - [Code Quality](#code-quality)
    - [Testing](#testing)
    - [Dependency Management](#dependency-management)
    - [Configuration & Documentation](#configuration--documentation)
    - [Other](#other)

## Project Overview

This project reconstructs the DOS game Screamer (1995) into rebuildable
source code.  The reference artifact is the GOG.com distribution and the
installed game in `~/Documents/Screamer.app`.  Reverse engineers and
developers use the tools and the documentation to recover the program
structure, the data formats, and the behavior.  The project targets the
original DOS platform first and modern platforms later.

## Technical Context

- **Language/Version**: Python 3.14.7 for the tools.  The reconstructed
  game source does not exist yet.  The planned target is C or C++ for
  32-bit DOS with OpenWatcom V2.
- **Primary Dependencies**: the Python standard library and capstone 5.0.9
  (loaded through `ctypes`).  The run-time probe uses DOSBox-X 2026.10.01
  from Homebrew.  The static cross-check uses radare2 6.2.4 from the Nix
  dev shell.  The decompilation uses Ghidra 12.0.4 from the `ghidra` dev
  shell with the `yetmorecode/ghidra-lx-loader` extension.
- **Cross-checks**: the full pipeline compares four identity facts from
  `tools/le.py`, radare2 6.2.4, and Ghidra 12.0.4 with
  `tools/cross_check.py`.  The facts are the entry, the page-0 mapping, the
  `main` function, and the `main` caller.  The check runs in the
  `cross-check` dev shell.
- **Toolchains**: the module rebuild uses the pinned OpenWatcom V2 container
  harness.  The period Watcom C/C++ 10.0a toolchain (the original build
  toolchain, abandonware) runs under DOSBox-X and stays alongside the V2
  harness.  The two harnesses do not replace each other.
- **Storage**: files on disk.  `work/` holds extracted artifacts and
  generated output.  The project uses no database.
- **Testing**: the standard-library `unittest` suite covers the pure logic of
  the rebuild framework.  Each tool is verified against the shipped artifacts
  with the commands in Build And Test Commands.
- **Target Platform**: the tools run on macOS 15.5 on arm64.  The game
  target is 32-bit DOS.
- **Project Type**: CLI tools plus a documentation set.
- **Performance Goals**: N/A.
- **Constraints**: the game uses Watcom C/C++ 32-bit and the DOS/4GW
  extender.  REA does not open the DOS LE executables.  The bundled
  DOSBox 0.74-2 has no debugger, so the run-time probe needs DOSBox-X.
  Large artifacts stay out of git.
- **Scale/Scope**: one game, 56 game files, 4 executables, 3 sound
  drivers, and 1 CD image.

## Project Structure

```text
screamer_re/
├── AGENTS.md                  # this file
├── flake.nix                  # pinned project tools for the Nix dev shell
├── flake.lock                 # the pinned nixpkgs revision
├── README.md                  # landing page: status, layout, quick start
├── task.md                    # the original task definition
├── .githooks/
│   └── pre-commit             # update the status page before every commit
├── .github/workflows/
│   └── pipeline.yml           # CI: the syntax gate, the status page, the Pages deploy, and the full check
├── docs/
│   ├── explanation/
│   │   ├── findings.md        # evidence-backed observations and open questions
│   │   └── roadmap.md         # milestones and next steps
│   └── reference/
│       ├── distribution.md    # GOG package and installed bundle layout
│       ├── executables.md     # LE header metadata and runtime facts
│       ├── file-formats.md    # observed data formats
│       ├── inventory.md       # game files with sizes and SHA-256 digests
│       ├── program-map.md      # the recovered startup path and functions
│       └── tooling.md         # tools, commands, host tools, and limits
├── tools/
│   ├── cross_check.py         # compare four identity facts across three tools
│   ├── dosboxx_control.py     # drive the DOSBox-X debugger control socket
│   ├── ghidra/                # Ghidra loader install, decompile, and identity scripts
│   │   ├── identity.sh        # run the identity script or check the Ghidra setup
│   │   ├── LeIdentity.java    # the Ghidra headless identity script
│   │   └── ...                # the loader install and decompile scripts
│   ├── entry_probe.py         # find the program entry in a CPU log
│   ├── export_assets.py       # export the graphics assets as PNG files
│   ├── extract_cd_image.py    # extract the CD data track to a plain ISO
│   ├── gfx_container.py       # list and extract the members of a graphics container
│   ├── inventory.py           # build a file inventory with SHA-256 digests
│   ├── le.py                  # parse an LE image and disassemble x86 code
│   ├── member_probe.py        # hook the container member open at run time
│   ├── pipeline_check.py      # run the pipeline end to end
│   ├── sound_bank.py          # decode the ADLIB- banks, the samples, and the songs
│   ├── rebuild/               # the rebuild framework and the two toolchain harnesses
│   │   ├── store.py           # the function and module record store
│   │   ├── harness.py         # the pinned container and DOSBox-X harness
│   │   ├── harness_dos.py     # the period Watcom 10.0a build under DOSBox-X
│   │   ├── watcom10_setup.py  # fetch and extract the period Watcom 10.0a toolchain
│   │   ├── run.py             # run the declared checks and print progress
│   │   ├── oracle.py          # compare function bodies with relocations normalized
│   │   ├── diff_module.py     # diff one rebuilt module against the original
│   │   ├── image_compare.py   # compare a rebuilt whole image with the original
│   │   ├── schema.py          # validate a record against the contract subset
│   │   ├── view.py            # render the five status-page views
│   │   ├── docs.py            # render the program-map tables
│   │   ├── callgraph.py       # build and check the named call graph
│   │   ├── callgraph.json     # the direct call edges between named records
│   │   ├── contracts/         # the JSON Schema contracts of the records
│   │   ├── functions/         # one record per function, 0x<address>.json
│   │   ├── modules/           # the three hundred and forty-eight module recipes
│   │   └── tests/             # the unittest suite of the pure logic
│   └── scripts/               # debugger session scripts (data, not code)
└── work/                       # extracted artifacts and output (not in git)
```

## Build And Test Commands

- **Build**: N/A for the tools.  The tools are plain Python scripts.  The
  module rebuild uses the OpenWatcom V2 container harness and needs Docker:

```sh
python3 tools/rebuild/harness.py build --module free
python3 tools/rebuild/harness.py run --module free
python3 tools/rebuild/run.py run --all
python3 tools/rebuild/run.py run --all --skip-unchanged
python3 tools/rebuild/run.py run --all --strict
python3 tools/rebuild/run.py run --recheck-record 0x300ec
python3 tools/rebuild/store.py check
python3 tools/rebuild/image_compare.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --rebuilt work/rebuild/STARTH.EXE
python3 tools/rebuild/docs.py render
python3 tools/rebuild/callgraph.py build
python3 tools/rebuild/callgraph.py check
python3 tools/rebuild/view.py html
```

The period build uses the Watcom 10.0a toolchain under DOSBox-X (needs
DOSBox-X, not Docker):

```sh
python3 tools/rebuild/watcom10_setup.py
python3 tools/rebuild/harness_dos.py build --module free
```

- **Test**: run the syntax gate and the tool checks:

```sh
python3 -m py_compile tools/le.py tools/inventory.py tools/extract_cd_image.py tools/dosboxx_control.py tools/entry_probe.py tools/gfx_container.py tools/export_assets.py tools/sound_bank.py tools/member_probe.py tools/cross_check.py tools/rebuild/diff_module.py tools/rebuild/oracle.py tools/rebuild/store.py tools/rebuild/schema.py tools/rebuild/image_compare.py tools/rebuild/harness.py tools/rebuild/harness_dos.py tools/rebuild/watcom10_setup.py tools/rebuild/run.py tools/rebuild/view.py tools/rebuild/docs.py tools/rebuild/callgraph.py tools/pipeline_check.py
python3 tools/le.py info ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE
python3 tools/inventory.py ~/Documents/Screamer.app/Contents/Resources/game
python3 tools/rebuild/diff_module.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address 0x300ec --rebuilt work/rebuild/free.exe --rebuilt-map work/rebuild/free.map --rebuilt-symbol free_
python3 tools/rebuild/diff_module.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address 0x300be --rebuilt work/rebuild/exitmod.exe --rebuilt-map work/rebuild/exit.map --rebuilt-symbol exit_
python3 tools/rebuild/diff_module.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address 0x300d6 --count 6 --rebuilt work/rebuild/exitmod.exe --rebuilt-map work/rebuild/exit.map --rebuilt-symbol exit_flush_
python3 tools/rebuild/diff_module.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address 0x3009b --rebuilt work/rebuild/prtfmod.exe --rebuilt-map work/rebuild/printf.map --rebuilt-symbol printf_
python3 tools/rebuild/diff_module.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --original-address 0x8c150 --rebuilt work/rebuild/memmod.exe --rebuilt-map work/rebuild/memcpy.map --rebuilt-symbol memcpy_
```

The full check runs every stage in the `cross-check` shell.  It needs Docker,
DOSBox-X, a built driver artifact, and the installed Ghidra loader.  It takes
about 8 minutes plus the Ghidra import and reports 375 checks.  `--skip-cross`
skips stage 8 and the tool preflight, so the run reports 371 checks.
`--skip-runtime` skips stages 5, 6, 7, and 9 and still runs stage 8, so the
run reports 360 checks.  Both flags report 356 checks and take about 16
seconds.  Stage 11 also compares the rebuilt whole image when
`--rebuilt-image` exists.  The image check is a skip until a whole image is
built, so it does not change these counts:

```sh
nix develop .#cross-check --command sh tools/ghidra/install_loader.sh
python3 tools/rebuild/harness.py build --module driver
nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game
nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-cross
nix develop .#cross-check --command python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime
python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game --skip-runtime --skip-cross
nix develop .#cross-check --command python3 tools/cross_check.py --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --main 0x10020 --call-site 0xd4364 --work-root work
```

Check the Ghidra setup without a run:

```sh
nix develop .#cross-check --command sh tools/ghidra/install_loader.sh --check
nix develop .#cross-check --command sh tools/ghidra/identity.sh --check
```

- **Decompile**: run the Ghidra decompiler on the functions of interest:

```sh
nix develop .#ghidra --command sh tools/ghidra/install_loader.sh
nix develop .#ghidra --command sh tools/ghidra/decompile.sh 0x10020 0x2ecf8
```

- **Lint**: N/A.  The project configures no linter and no formatter.
  Use `py_compile` as the syntax gate.
- **Run**: run a tool with its documented arguments.  Run the game with:

```sh
open ~/Documents/Screamer.app
```

## Contribution Instructions

- You MUST verify every change with the syntax gate and the tool checks in
  Build And Test Commands.

  Use the following commands:
    - `python3 -m py_compile tools/le.py tools/inventory.py
      tools/extract_cd_image.py tools/dosboxx_control.py tools/entry_probe.py
      tools/gfx_container.py tools/export_assets.py tools/sound_bank.py
      tools/member_probe.py tools/cross_check.py
      tools/rebuild/diff_module.py
      tools/rebuild/oracle.py
      tools/rebuild/store.py tools/rebuild/schema.py
      tools/rebuild/image_compare.py tools/rebuild/harness.py
      tools/rebuild/harness_dos.py tools/rebuild/watcom10_setup.py
      tools/rebuild/run.py tools/rebuild/view.py tools/rebuild/docs.py
      tools/rebuild/callgraph.py tools/pipeline_check.py` to check the syntax
    - `python3 tools/le.py info
      ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE` to check the
      LE parser
    - `python3 tools/inventory.py
      ~/Documents/Screamer.app/Contents/Resources/game` to check the inventory
      tool
    - `python3 tools/rebuild/diff_module.py --original
      ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE
      --original-address 0x300ec --rebuilt work/rebuild/free.exe --rebuilt-map
      work/rebuild/free.map --rebuilt-symbol free_` to check the module diffs; the
      Build And Test Commands section holds the exit module commands
    - `python3 tools/rebuild/image_compare.py --original
      ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE --rebuilt
      ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE` to check the
      whole-image oracle against itself
    - `python3 tools/pipeline_check.py --game-dir
      ~/Documents/Screamer.app/Contents/Resources/game` to check every stage

- You MUST run the changed tool against the shipped artifacts and
  confirm the output.
- You MUST enable the pre-commit hook in a new clone with
  `git config core.hooksPath .githooks`.  The hook regenerates
  `work/rebuild/status.html` before every commit.  The syntax gate, the unit
  suite, and the full check run in CI.
- You MUST record a new finding with its evidence in
  `docs/explanation/findings.md`.
- You MUST update the affected pages under `docs/` when a change affects
  what they describe.
- You MUST update the Project Structure section of `AGENTS.md` when the
  project structure changes.
- You MUST keep large artifacts out of git.  `work/` and `*.pkg` stay
  ignored.  Record a SHA-256 digest instead.
- If a prompt asks for a refactoring or an improvement, capture the
  resulting rule in the relevant Code Guidelines section of `AGENTS.md`.
- After completing a task, you MUST verify the code against the Code
  Guidelines in `AGENTS.md`.

## Code Guidelines

### System Design

- A tool runs and exits.  It holds no long-lived state and starts no
  background process.
- A tool prints its result to stdout and its errors to stderr.
- A tool exits with code 0 on success and a non-zero code on failure.
- A tool validates its inputs early and reports the first error with a clear
  message.
- A tool takes its paths and its addresses from the command line.  It reads
  no hidden configuration.
- A tool does not modify the shipped artifacts.  It writes only under
  `work/` or to an explicit output path.

### Architecture

- Separation of Concerns: each tool handles one aspect.  `le.py` parses and
  disassembles, `inventory.py` hashes, `extract_cd_image.py` converts.
- Single Responsibility Principle: one function has one task, for example
  `LinearExecutable.parse` parses the header and the tables.
- Dependency Direction: the tools depend on the shipped artifacts and on the
  Python standard library.  No module depends on `work/`.
- Explicit Boundaries: the tools expose functions and a command-line
  interface.  A caller does not reach into file parsing internals.
- Data Flow Clarity: data moves from the shipped artifact to the parser to
  the report.  `tools/le.py` follows this path.
- Minimize Coupling, Maximize Cohesion: `tools/le.py` holds all LE
  knowledge.  The other tools do not parse LE headers.
- Make Invalid States Impossible: `tools/le.py` rejects a non-MZ file, a
  non-LE image, and an unmapped address with a clear error.
- Observability Built-in: a tool reports its input path and its result
  counts, for example the page count and the object count.
- Keep It Boring: the tools use the standard library and one
  `ctypes` binding to capstone.
- Shared View Logic: every record element of the status page carries
  `data-key`, `data-address`, `data-name`, `data-module`, `data-status`, and
  `data-stale`. The filter and the selection use these attributes, so a new
  view adds no separate filter or selection code.

Layers:

| Layer | Responsibility | Example |
| --- | --- | --- |
| Tools | parse, extract, and report | `tools/le.py` |
| Store | hold the identity, the status, and the evidence per function | `tools/rebuild/functions/` |
| Documentation | record findings and reference facts | `docs/reference/` |
| Work | hold extracted artifacts and generated output | `work/` |

```text
shipped artifacts -> tools -> stdout / work/ -> docs/
```

A function record lives at `tools/rebuild/functions/0x<address>.json`, and
`store.py` is the only writer.  Every other tool reads and writes a record
through the store.

### Code Quality

- Every tool file starts with a module docstring that holds the purpose
  and the usage.
- Every function carries type hints.  Every file uses
  `from __future__ import annotations`.
- Names: files and functions use `lower_case_with_underscores`, classes
  use `CamelCase`, constants use `UPPER_CASE`.
- An error raises `ValueError` with a clear message, for example
  `raise ValueError("not an MZ image")`.
- A parser returns parsed fields or raises.  It does not print and return
  `None` at the same time.
- The tools import only the standard library and `ctypes`.  Add no import
  without a recorded reason.

### Testing

- The rebuild framework uses the standard-library `unittest` suite under
  `tools/rebuild/tests/`.  Run it with
  `python3 -m unittest discover -s tools/rebuild/tests`.  Every test uses a
  temporary directory, so the suite needs no Docker, no DOSBox-X, and no game
  directory.
- The full check reports 375 checks in the `cross-check` shell.  `--skip-cross`
  gives 371 checks, `--skip-runtime` gives 360 checks, and both flags give 356
  checks.  `--skip-runtime` still runs stage 8.  `--skip-cross` skips stage 8
  and the tool preflight.
- Stage 12 checks the program-map tables and the call graph for drift.  The
  call graph check needs capstone and the original executable.
- The identity gate has two modes.  The default gate accepts the three
  historical verdicts.  `--strict` accepts only the byte-identical verdict and
  the verdict whose differences are the link-time addresses.  A reordered
  instruction sequence and a shape match are diagnostic, not exact.
- `oracle.py` masks every byte that an LE fixup slot controls and every byte
  that encodes a direct branch target.  It compares the remaining bytes in
  instruction order.  An internal target compares by kind and width, because
  the object number and the object offset are link-time values.  An imported
  target compares by module and ordinal or name offset.  A relocation-only
  difference passes `--strict`, and a reordered, inserted, or non-relocation
  difference does not.
- `store.py check` validates every record against `contracts/record.schema.json`
  and every module against `contracts/module.schema.json`, in addition to the
  semantic checks.  The validator uses the standard library only.
- The whole-image oracle `image_compare.py` is the final gate.  It compares the
  stub, the header, the resident tables, the fixup section, every object, the
  non-resident name table, and the whole file.  Stage 11 runs it when
  `--rebuilt-image` exists and reports a skip when no whole image is built.
- `tools/le.py` is a fingerprint input (`store.TOOL_FILES`).  The helper change
  updates every computed fingerprint, so the next verification run re-verifies
  every record once.  The record files stay unchanged.
- The pre-commit hook regenerates `work/rebuild/status.html`, and the CI
  `status` job regenerates the page into `_site/index.html` and publishes it as
  an artifact.  On a push to `master`, the `deploy` job publishes the packaged
  site to GitHub Pages after the syntax gate.  The page is generated output, so
  it stays out of git.
- Verify a tool by running it against the shipped artifacts.
- Record the exact tool command with every new finding in
  `docs/explanation/findings.md`.
- A harness output file name stays within the 8.3 limit (`name.ext`), because
  DOS truncates a longer name and the run check misses the file.  The store's
  module validation rejects a longer name and a redirect target that differs
  from the output.
- Compare a regenerated table with the table in
  `docs/reference/inventory.md` before a commit.
- Confirm every documented command runs before a commit.
- The verification run (`run.py run --all` and stage 11 of the pipeline) must
  skip a record whose inputs are unchanged since the previous run.  The input
  fingerprint of a record covers the module recipe, the module sources, the
  shared harness sources (`chk.asm`, `stack.asm`, `cstart.c`), the record
  fields that steer the checks, the participating tools (`run.py`,
  `harness.py`, `diff_module.py`, `store.py`, `le.py`), the original image, and
  the built artifacts.  The fingerprint lives in the `result.fingerprint` of
  the record, `store.compute_fingerprint` computes it, and `run.py
  --skip-unchanged` uses it.  `pipeline_check.py` verifies the stored
  fingerprint of an unchanged record itself.  A documentation-only change to a
  record (`evidence`, `notes`, `name`, `role`, `confidence`, `section`,
  `implementation`) must not invalidate a result.  The optional `implementation`
  field is `c` or `assembly`; the status page shows `asm` for `assembly` and
  `C` for `c`.
- A mutation that changes the record addresses must call `sync_extents()`.
  `save_record` marks a new address and `remove_record` marks a deletion, and
  the seed, the command dispatch, and the `atexit` hook call `sync_extents()`,
  so a later migration cannot leave a stale extent.
- The pipeline checks the debugger control port and the inputs before any
  stage.  When another process holds the port (`--port`, default 9876), when
  the game directory or the reference executable is missing, when the reference
  executable does not parse as an LE image, or when the work root cannot be
  created, it prints a clear error and exits with code 2, so a bad input cannot
  surface as a stage failure or a stage 9 bind error.

### Dependency Management

- Pin every dependency version explicitly.  Record the version in the docs.
  `flake.nix` with `flake.lock` pins the Python, capstone, and radare2
  versions.  Run the tools inside `nix develop`.  The `ghidra` dev shell
  pins Ghidra 12.0.4 through a second nixpkgs input, because the LX/LE
  loader extension is released for the 12.0.x series.
- Prefer the Python standard library.  Add a dependency only for
  significant value.
- Use reputable and maintained sources only.  capstone comes from
  Homebrew.
- Minimize the dependency count.  capstone is the only third-party
  dependency.
- Use the latest stable version.  Check the registry before an addition.

**Rationale**: fewer and well-vetted dependencies reduce security,
supply-chain, and maintenance costs.

### Configuration & Documentation

- The tools take their configuration from the command line.  They read no
  environment variable.
- Update `docs/reference/tooling.md` when a tool or a command changes.
- Update `docs/reference/file-formats.md` when a format fact changes.
- Update `docs/explanation/findings.md` when a finding or an open
  question changes.
- Record the SHA-256 digest of every large artifact.  Do not commit the
  artifact.
- Commit no secret.  The distribution holds none.

### Other

- Write all prose in ASD-STE100 Simplified Technical English.
- Separate observations, inferences, and unknowns.  Cite the evidence for
  every claim.
- Record a file format as unknown until a decoder confirms it.
- Keep `work/` disposable.  Regenerate it from the shipped artifacts.
- The period Watcom 10.0a toolchain stays alongside the pinned OpenWatcom V2
  harness.  The two harnesses serve two goals: V2 runs the declared checks, and
  10.0a targets the byte-for-byte match.  Do not replace one with the other.
- Prefer a C implementation.  Add an assembly implementation only when the C
  build cannot reproduce the original bytes, and record the reason in the
  finding and in the record.  Do not convert a module to assembly only to raise
  the verified count; leave it `rebuilt` when C is insufficient.  The FPU and
  math stubs are harness support, not module implementations.
