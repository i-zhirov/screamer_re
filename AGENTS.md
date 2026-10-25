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
- **Storage**: files on disk.  `work/` holds extracted artifacts and
  generated output.  The project uses no database.
- **Testing**: no test framework.  Each tool is verified against the
  shipped artifacts with the commands in Build And Test Commands.
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
│   └── pre-push               # run the checks before every push
├── .github/workflows/
│   └── pipeline.yml          # CI: the syntax gate and the full check
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
│   ├── dosboxx_control.py     # drive the DOSBox-X debugger control socket
│   ├── ghidra/                # Ghidra loader install and decompile scripts
│   ├── entry_probe.py         # find the program entry in a CPU log
│   ├── extract_cd_image.py    # extract the CD data track to a plain ISO
│   ├── inventory.py           # build a file inventory with SHA-256 digests
│   ├── le.py                  # parse an LE image and disassemble x86 code
│   ├── pipeline_check.py      # run the pipeline end to end
│   ├── rebuild/               # the OpenWatcom V2 build harness
│   └── scripts/               # debugger session scripts (data, not code)
└── work/                       # extracted artifacts and output (not in git)
```

## Build And Test Commands

- **Build**: N/A.  The tools are plain Python scripts.  Install no build
  step.
- **Test**: run the syntax gate and the tool checks:

```sh
python3 -m py_compile tools/le.py tools/inventory.py tools/extract_cd_image.py tools/dosboxx_control.py tools/entry_probe.py tools/gfx_container.py tools/export_assets.py tools/pipeline_check.py
python3 tools/le.py info ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE
python3 tools/inventory.py ~/Documents/Screamer.app/Contents/Resources/game
```

The full check runs every stage and needs DOSBox-X.  It takes about 15
seconds and reports 28 checks:

```sh
python3 tools/pipeline_check.py --game-dir ~/Documents/Screamer.app/Contents/Resources/game
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
      tools/gfx_container.py tools/pipeline_check.py` to check the syntax
    - `python3 tools/le.py info
      ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE` to check the
      LE parser
    - `python3 tools/inventory.py
      ~/Documents/Screamer.app/Contents/Resources/game` to check the inventory
      tool
    - `python3 tools/pipeline_check.py --game-dir
      ~/Documents/Screamer.app/Contents/Resources/game` to check every stage

- You MUST run the changed tool against the shipped artifacts and
  confirm the output.
- You MUST enable the pre-push hook in a new clone with
  `git config core.hooksPath .githooks`.  The hook runs the syntax gate and
  the full check before every push.  It reports a skip when the game
  directory or DOSBox-X is missing.
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

Layers:

| Layer | Responsibility | Example |
| --- | --- | --- |
| Tools | parse, extract, and report | `tools/le.py` |
| Documentation | record findings and reference facts | `docs/reference/` |
| Work | hold extracted artifacts and generated output | `work/` |

```text
shipped artifacts -> tools -> stdout / work/ -> docs/
```

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

- The project holds no test framework.  Verify a tool by running it
  against the shipped artifacts.
- Record the exact tool command with every new finding in
  `docs/explanation/findings.md`.
- Compare a regenerated table with the table in
  `docs/reference/inventory.md` before a commit.
- Confirm every documented command runs before a commit.

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
