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
- [File formats](docs/reference/file-formats.md) — observed data formats.
- [Tooling](docs/reference/tooling.md) — tools, commands, and limits.
