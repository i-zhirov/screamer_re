# Executables

## Summary

| File | Format | Bytes | Entry (object offset) | Entry object | Module name |
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

## Entry point

The LE header entry value is an offset inside the start object.  The file
offset of the entry is:

```text
file_offset = data_pages_offset
            + (page_table_index - 1 + entry_offset // page_size) * page_size
            + entry_offset % page_size
```

The entry code is a short jump over the Watcom run-time banner string:

```text
0x03a8f8: eb76      jmp short +0x76
0x03a8fa: 57 41 54 43 4f 4d ...   "WATCOM C/C++32 Run-Time system. ..."
0x03a970: fb        sti
0x03a971: 83e4fc    and esp, 0xfffffffc
0x03a974: 89e3      mov ebx, esp
0x03a976: 891d94420000  mov [0x4294], ebx
0x03a97c: 891d80420000  mov [0x4280], ebx
0x03a982: 66b82400  mov ax, 0x24
0x03a986: 66a38c420000  mov [0x428c], ax
0x03a98c: bb52414850  mov ebx, 0x50484152
0x03a991: 29c0      sub eax, eax
0x03a993: b430      mov ah, 0x30
0x03a995: cd21      int 0x21
```

All three executables use the same entry pattern.  The entry file offsets
are:

| File | Header entry | Entry file offset | First instruction |
| --- | ---: | ---: | --- |
| `STARTH.EXE` | `0x200f8` | `0x3a8f8` | `jmp short +0x76` |
| `STARTL.EXE` | `0x1feb8` | `0x3a6b8` | `jmp short +0x76` |
| `SETUP.EXE` | `0x12c28` | `0x20628` | `jmp short +0x76` |

The run-time log confirms the rule.  The game code starts at selector
`0x0150` and EIP `0x1f20f8` with all registers zero.  The module load base
is `0x1c2000`.  The module-relative entry is `0x300f8`, which equals the
object base `0x10000` plus the header entry `0x200f8`.
