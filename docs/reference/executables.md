# Executables

## Summary

| File | Format | Bytes | Entry (linear) | Entry object | Module name |
| --- | --- | ---: | ---: | --- | --- |
| `STARTH.EXE` | LE | 1034923 | `0x200f8` | 1 | `start` |
| `STARTL.EXE` | LE | 1034919 | `0x1feb8` | 1 | `start` |
| `SETUP.EXE` | LE | 251079 | `0x12c28` | 1 | `setup` |
| `DOS4GW.EXE` | MZ + extender | 254532 | n/a | n/a | `dos4gw` |

`STARTH.EXE` is the game executable for the single-player path.  The
`dosboxScreamer_single.conf` file runs it.

## Common structure

All three game executables are linear executables (LE) with a 16-bit MZ
stub.  The LE header starts at file offset `0x2a50`.  The header reports:

- CPU type `0x0002` (80386),
- OS type `0x0001` (OS/2),
- module type `EXE`,
- page size 4096 bytes,
- a zero import module count.

Each executable holds three objects:

| Object | Permissions | Role |
| --- | --- | --- |
| `obj.1` | `r-x` | code and read-only data |
| `obj.2` | `rw-` | small data |
| `obj.3` | `rw-` | stack |

The file data of the objects is contiguous.  Page numbers in the page table
run in order, and the last page ends at the end of the file.  The
`to_file_offset` function in `tools/le.py` uses this mapping.

## STARTH.EXE

| Field | Value |
| --- | --- |
| Header offset | `0x2a50` |
| Pages | 227 × 4096 bytes, last page 683 bytes |
| Data pages offset | `0x1a800` |
| Entry | linear `0x200f8` (object 1) |
| Stack | object 3, ESP `0x472c0` |
| Module name | `start` |
| Name table | `start` (ordinal 0) |

| Object | Base | Virtual size | Flags | Pages |
| --- | ---: | ---: | --- | ---: |
| `obj.1` | `0x10000` | `0xdba93` | `0x2045` `r-x` | 220 |
| `obj.2` | `0xf0000` | `0x82` | `0x2043` `rw-` | 0 |
| `obj.3` | `0x100000` | `0x472c0` | `0x2043` `rw-` | 7 |

The code object starts with a trap stub (`int3`, `jmp $`) and NOP padding.
The first function prologue is at file offset `0x1a818`.

## STARTL.EXE

`STARTL.EXE` matches `STARTH.EXE` in structure.  It differs in size by 4
bytes and in 405284 bytes of content.  The object 1 virtual size is
`0xdb7a3`, and the entry is linear `0x1feb8`.

## SETUP.EXE

| Field | Value |
| --- | --- |
| Header offset | `0x2a50` |
| Pages | 48 × 4096 bytes, last page 2759 bytes |
| Data pages offset | `0xda00` |
| Entry | linear `0x12c28` (object 1) |
| Stack | object 3, ESP `0x29460` |
| Module name | `setup` |

`SETUP.EXE` is the setup and configuration program.  It reads and writes
`CONFIG/SETUP.DAT` and `CONFIG/*.CFG`.

## Runtime

The executables carry the string:

```text
WATCOM C/C++32 Run-Time system. (c) Copyright by WATCOM International Corp. 1988-1994. All rights reserved.
```

The runtime is the Watcom C/C++ 32-bit run-time.  The programs use the
DOS/4GW protected-mode extender.  The string `dos4gw.exe` appears in the
runtime code.

## Sound drivers

| File | Bytes | Driver |
| --- | ---: | --- |
| `HMIDRV.386` | 317317 | HMI DMA driver for Gravis Ultrasound |
| `HMIDET.386` | 83774 | HMI detect driver for Gravis Ultrasound |
| `HMIMDRV.386` | 117852 | HMI MIDI driver for OPL2 |

The three `.386` files carry HMI (Human Machine Interfaces, Inc.) driver
banners.  They hold Gravis Ultrasound and MPU-401/OPL2 support code.

## Open question: entry alignment

The header entry value of `STARTH.EXE` maps to file offset `0x2a8f8`.
The bytes at that offset are `00 a1 4c 63 00 00`.  A disassembly at the
offset is mid-instruction.  A disassembly at offset `+1` gives clean code:

```text
0x0200f9: a14c630000      mov eax, dword ptr [0x634c]
0x0200fe: 39f8            cmp eax, edi
0x020100: 7d12            jge 0x20114
0x020102: 69c090050000    imul eax, eax, 0x590
0x020108: 05b80a0100      add eax, 0x10ab8
0x02010d: e8fec6ffff      call 0x1c810
```

The header entry value of `STARTL.EXE` maps to file offset `0x2a6b8`, and a
disassembly at that offset is clean.  The header entry value of
`SETUP.EXE` maps to file offset `0x10628`, and a disassembly at that offset
starts with a zero byte.

The page mapping is confirmed: the last page ends at the end of the file.  The
entry alignment of `STARTH.EXE` and `SETUP.EXE` stays an open question.
