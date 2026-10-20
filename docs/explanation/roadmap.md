# Roadmap

## Goal

The goal is rebuildable source code for Screamer.  The first target is the
original DOS platform.  Later targets are modern platforms, starting with the
current host.

## Constraints

- The original build uses Watcom C/C++ 32-bit with the DOS/4GW extender.
- The project can rebuild the program with OpenWatcom V2.  OpenWatcom V2
  builds on macOS and targets 32-bit DOS.
- The project can run and test the original and the rebuild in DOSBox.
- The shipped artifacts are the reference.  Every format claim needs evidence
  from those artifacts.

## Milestones

### M0: Reconnaissance (done)

- Inventory the distribution and the installed game.
- Parse the LE headers and the object tables.
- Confirm the toolchain, the runtime, and the extender.
- Record the open questions.

### M1: Program map

- Resolve the entry alignment with a DOSBox debugger session (done).
- Find `main` and the subsystem entry points.
- Map the code object into functions with names and roles.
- Map the data references: video, sound, input, file I/O, network.
- Record the call graph for the main loop and the menu system.

### M2: Format decoders

- Decode the graphics format (`GFX/GRAF.DAT`, `GFX/MENU.DAT`, `GFX/GAME.DAT`).
- Decode the map and index formats (`DATA/MAP*`, `DATA/GEN.DAT`,
  `DATA/LOCAL.DAT`, `TABS/*`).
- Decode the HMI `ADLIB-` music bank.
- Decode the sample bank (`DATA/SOUND.DAT`).
- Verify each decoder against the original data.

### M3: Rebuild

- Rebuild the program in C or C++ for 32-bit DOS with OpenWatcom V2.
- Rebuild the data pipeline that produces the shipped data files.
- Compare the rebuild with the original: same files, same behavior.

### M4: Modern platform

- Port the rebuild to a modern platform.  Start with macOS on arm64.
- Replace the DOS/4GW and DOS services with host libraries.
- Replace the CD audio path with a modern audio path.

### M5: Verification

- Run differential tests between the original and the rebuild.
- Record every test with its evidence and its limits.

## Next steps

1. Trace the start path from the entry point to `main`.  The entry jumps
   to `0x03a970`, which sets up the stack, checks the DOS version, and
   calls the Watcom run-time startup.
2. Map the Watcom startup code and find the call to the program `main`.
3. Start the graphics decoder with `GFX/MENU.DAT`, which is the smallest
   graphics file.
4. Set up the OpenWatcom V2 build harness under `work/`.
