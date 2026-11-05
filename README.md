# Screamer Reverse Engineering

This project reconstructs the DOS game Screamer (1995) into rebuildable
source code.  The reference artifact is the GOG.com distribution for macOS.
The first target is the original DOS platform.  Later targets are modern
platforms, starting with the current host (macOS on arm64).

## Status

Initial reconnaissance is complete.  The project has:

- a documented inventory of the distribution and the installed game,
- a parser and disassembler for the DOS/4GW linear executables (`tools/le.py`),
- a CD image extractor (`tools/extract_cd_image.py`),
- an inventory generator (`tools/inventory.py`),
- a rebuild store with one record per function and its status
  (`tools/rebuild/store.py`),
- a check runner that builds and tests the declared checks
  (`tools/rebuild/run.py`),
- a self-contained status page of the store (`tools/rebuild/view.py`),
- a generated program map with a drift check (`tools/rebuild/docs.py`),
- decoders for the graphics assets and the sound formats,
- an investigation log with evidence and open questions.

No reconstructed source code exists yet.

## Repository layout

```text
AGENTS.md   conventions for AI agents
docs/       project documentation
  explanation/  findings and roadmap
  reference/    distribution, inventory, formats, executables, tooling
tools/      analysis tools
work/       extracted artifacts and generated output (not in git)
task.md     the original task definition
```

## Quick start

The game runs on the host through the installed GOG bundle:

```sh
open ~/Documents/Screamer.app
```

The bundle carries a universal DOSBox binary.  On Apple silicon it runs
through Rosetta 2.

Inspect a DOS/4GW linear executable:

```sh
python3 tools/le.py info ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE
python3 tools/le.py map  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE 0x300f8
python3 tools/le.py disasm ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE 0x300f8
python3 tools/le.py calls ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE 0x300f8
python3 tools/le.py tree  ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE 0x300f8
```

Find the program entry at run time.  The tool needs DOSBox-X:

```sh
python3 tools/entry_probe.py \
  --game-dir ~/Documents/Screamer.app/Contents/Resources/game
```

Extract the data track of the CD image:

```sh
python3 tools/extract_cd_image.py \
  ~/Documents/Screamer.app/Contents/Resources/game/SCREAM.DAT \
  work/screamer_data_track.iso
```

Build a file inventory with SHA-256 digests:

```sh
python3 tools/inventory.py \
  ~/Documents/Screamer.app/Contents/Resources/game
```

Run the full pipeline end to end.  The check needs Docker, DOSBox-X, a built
driver artifact, and the installed Ghidra loader, and takes about 8 minutes
plus the Ghidra import:

```sh
python3 tools/rebuild/harness.py build --module driver
nix develop .#cross-check --command python3 tools/pipeline_check.py \
  --game-dir ~/Documents/Screamer.app/Contents/Resources/game
```

Run the three-tool cross-check alone.  It prints four fact lines and writes the
raw tool output under `work/cross-check/`:

```sh
nix develop .#cross-check --command python3 tools/cross_check.py \
  --original ~/Documents/Screamer.app/Contents/Resources/game/STARTH.EXE \
  --main 0x10020 --call-site 0xd4364 --work-root work
```

## Development environment

The project tools run inside a Nix shell:

```sh
nix develop
r2 -v
```

`flake.nix` and `flake.lock` pin Python, capstone, and radare2.  DOSBox-X comes
from Homebrew.  The `ghidra` shell carries Ghidra with the LX/LE loader:

```sh
nix develop .#ghidra --command sh tools/ghidra/install_loader.sh
nix develop .#ghidra --command sh tools/ghidra/decompile.sh 0x10020 0x2ecf8
```

The `cross-check` shell carries Python, capstone, radare2, Ghidra, `curl`,
and `unzip`.  The full check runs there.  Install the loader once per user:

```sh
nix develop .#cross-check --command sh tools/ghidra/install_loader.sh
```

## Checks

The full check runs every stage in the `cross-check` shell and reports
205 checks.  `--skip-cross` skips stage 8 and the tool preflight, so the run
reports 201 checks.  `--skip-runtime` skips stages 5, 6, 7, and 9 and still
runs stage 8, so the run reports 190 checks.  Both flags report 186 checks.
The record run skips a record whose input fingerprint still matches its
recorded result, so after one full run the record run takes only the hashing
time for the unchanged modules.  CI runs the syntax gate and the unit suite on
every push and pull request, and the full check on a self-hosted runner.
The pre-commit hook regenerates the status page; enable the hooks once per clone:

```sh
git config core.hooksPath .githooks
```

The full check needs the game directory, DOSBox-X, and the installed Ghidra
loader.  Set `SCREAMER_GAME_DIR` to use another game directory.

GitHub Actions runs the syntax gate on every push and pull request.  The full
check runs on a self-hosted runner that holds the game, DOSBox-X, Nix, and the
loader.  Register the runner with the labels `macOS` and `screamer-artifacts`,
and set the repository variable `SCREAMER_SELF_HOSTED` to `true` to enable that
job.

## Documentation

- [Findings](docs/explanation/findings.md) — evidence-backed observations
  and open questions.
- [Roadmap](docs/explanation/roadmap.md) — milestones and the
  reconstruction plan.
- [Distribution](docs/reference/distribution.md) — GOG package and
  installed bundle layout.
- [Inventory](docs/reference/inventory.md) — game files with sizes and
  SHA-256 digests.
- [Executables](docs/reference/executables.md) — linear executable
  metadata.
- [Program map](docs/reference/program-map.md) — the recovered startup
  path and the program functions.
- [File formats](docs/reference/file-formats.md) — observed data formats.
- [Tooling](docs/reference/tooling.md) — tools, commands, and limits.
