# File formats

This page records the observed formats.  The formats are not decoded yet.
Every claim below comes from file bytes and from `file` output.

## CD image pair

| File | Format |
| --- | --- |
| `SCREAM.DAT` | CUE sheet, ASCII with CRLF |
| `SCREAM.gog` | raw CD image, 2352-byte sectors |

`SCREAM.DAT` holds:

```text
FILE "SCREAM.gog" BINARY
  TRACK 01 MODE1/2352
    INDEX 01 00:00:00
  TRACK 02 AUDIO
    INDEX 01 00:02:24
  ...
  TRACK 08 AUDIO
    INDEX 01 31:09:07
```

Track 1 is the data track.  Tracks 2 to 8 are CD audio tracks.  The game
manual names seven CD audio tracks, and the CUE sheet lists seven audio
tracks.

The data track holds an ISO 9660 volume with the label `SCREAMER`.  The
volume holds three files:

| File | Bytes | Content |
| --- | ---: | --- |
| `DISKID` | 3 | ASCII `1` and CRLF |
| `FILES.A` | 444 | fixed-width file list |
| `FILES.N` | 403 | fixed-width file list |

`FILES.A` and `FILES.N` are installer file lists.  The entries name
`CONFIG`, `DATA`, `GFX`, `TABS`, `REPLAYS`, `*.386`, `dos4gw.exe`,
`setup.exe`, `start?.exe`, `screamer.pif`, and `setup.pif`.  The lists
serve the original CD installer.  The GOG package does not use them.

## Executables and drivers

`STARTH.EXE`, `STARTL.EXE`, and `SETUP.EXE` are LE images.  See
[Executables](executables.md) for the header metadata.

`HMIDRV.386`, `HMIDET.386`, and `HMIMDRV.386` are HMI (Human Machine
Interfaces, Inc.) drivers.  They carry driver banners and embedded
programs, for example `gus8m.com` and `fmmidi.com`.  The format is not
decoded yet.

## Configuration files

| File | Bytes | Content |
| --- | ---: | --- |
| `SCREAMER.CFG` | 10 | ASCII `C:\tD:\tC:\tc` |
| `CONFIG/SETUP.CFG` | 144 | 32-bit little-endian fields |
| `CONFIG/CONTROLS.CFG` | 1328 | 32-bit little-endian fields |
| `CONFIG/DEFCTRL.CFG` | 1328 | default copy of `CONTROLS.CFG` |

`SCREAMER.CFG` holds the drive letters for the game and for the CD.

## Music banks

`DATA/MUSIC.DAT` and `CONFIG/SETUP.DAT` carry the ASCII signature
`ADLIB-`.  Both files hold instrument names, for example `<Blank`,
`$clap`, `$scratch`, `$rimshot`, `=woodblok`, `glock`, `Kick2`, `#Kick`,
`$RimShot`, `<Snare`, and `;Clap`.

The signature and the instrument names point to the HMI Sound Operating
System music format.  The format is not decoded yet.

## Sound data

`DATA/SOUND.DAT` holds 8-bit unsigned samples.  The first bytes are
`0x80`, which is silence in unsigned 8-bit PCM.  The file holds no ASCII
strings.  The exact sample rate and the split into samples are not known.

## Map data

| File | Content |
| --- | --- |
| `DATA/MAP1.DAT` to `DATA/MAP6.DAT` | compressed map data, high entropy |
| `DATA/MAP1I.DAT` to `DATA/MAP6I.DAT` | 32-bit little-endian fields |
| `DATA/GEN.DAT` | compressed data, high entropy |
| `DATA/LOCAL.DAT` | compressed data, high entropy |

The `*I.DAT` files hold small 32-bit little-endian values at the start.
The values look like offsets or counts.  The format is not decoded yet.

## Graphics data

| File | Content |
| --- | --- |
| `GFX/GRAF.DAT` | 32-bit little-endian field list |
| `GFX/MENU.DAT` | 16-bit field list |
| `GFX/GAME.DAT` | compressed data |
| `GFX/MAP1H.DAT` to `GFX/MAP6H.DAT` | high-resolution map graphics |
| `GFX/MAP1L.DAT` to `GFX/MAP6L.DAT` | low-resolution map graphics |

`GFX/MAP5H.DAT` and `GFX/MAP5L.DAT` hold the same bytes.

## Table data

| File | Bytes | Content |
| --- | ---: | --- |
| `TABS/VIS.TAB` | 161024 | 16-bit little-endian values, 80512 entries |
| `TABS/PERSP01.TAB` | 262656 | 32-bit little-endian values, 65664 entries |

The values in `VIS.TAB` fall in small steps.  The values in
`PERSP01.TAB` repeat `0xffffff81` (signed -127) in the first rows.  The
meaning of both tables is not known yet.

## 3D object files

The 3D object files have the suffix `.3DX`.  The engine builds the name from
the object record name.  A shipped `.3dx` file is not present loose; the loader
reads the data through the main file.  See the main file section below.

### File header

The loader keeps a file record for every open file.  The loader fills the record
through `object_init_from_file` at `0x4cc10`.

| Offset | Size | Content |
| --- | ---: | --- |
| `+0x14` | 4 | the chunk count |
| `+0x18` | 4 each | the chunk offset table, one file-relative offset per chunk |

### Chunk header

The loader computes the chunk descriptor as the chunk start plus 12.  The
descriptor holds one 4-byte word and then six words.

| Offset | Size | Content |
| --- | ---: | --- |
| `+0x10` | 4 | chunk word 0 |
| `+0x14` | 4 | chunk word 1 |
| `+0x18` | 4 | chunk word 2 |
| `+0x1c` | 4 | chunk word 3 |
| `+0x20` | 4 | chunk word 4 |
| `+0x24` | 4 | chunk word 5 |

The loader copies the six words into the chunk table of the object record. 
Every
chunk table entry holds eight words: the file record, the chunk count, and the
six
chunk words.  The table ends with the value `-1`.

### Chunk names

The loader looks the chunks up by name through the resource layer.

| Name | Content |
| --- | --- |
| `3D_HEADER` | the object count at `+0x14` |
| `3D_BLKOBJ` | the block objects, size `count * 0x20 + 0x60` |
| `GOBJ_LIST`, `OBJ_LIST` | the object lists |
| `OSTACLES` | the obstacles |
| `TRACK_DATA` | the track data |
| `OPPONENTS` | the opponents |
| `ANIMATIONS` | the animations |
| `CARS_DATA` | the car data |

`3D_BLKOBJ` holds a 0x60-byte header and `count` block objects of 0x20 bytes.
The loader clears the six list words at `+0x24`, `+0x28`, `+0x2c`, `+0x30`,
`+0x34`, and `+0x38` of the header and links the block objects.

### Main file

The main file is a container with named members.  The loader reports
`Cannot open main file: %s` and `File %s not present in main file.`

The resource layer allocates the buffers through the DPMI services of interrupt
31h: the service `0x0100` allocates a DOS memory block with the size in 16-byte
paragraphs, and the service `0x0501` allocates a memory block.  The layer stores
the buffer at the file record `+0x0c` and the size at `+0x14`.
Evidence: the decompiled `FUN_0002f0fc`, `FUN_0002f172`, and `FUN_0002f062`.

## Replays

`REPLAYS/DUMMY.DAT` holds the ASCII text `replays` and CRLF.  It is a
placeholder.

`replay_read` at `0x22e94` and `replay_write` at `0x22f54` use 20-byte records
at `DAT_00118e5c + index * 0x14`.

| Offset | Size | Content |
| --- | ---: | --- |
| `+0x00` | 4 | the frame delta |
| `+0x04` | 4 | a state word |
| `+0x08` | 2 each | six 16-bit state words |

`replay_read` returns 1 at the end markers `DAT_00118e68` and `DAT_00118e70`.
Evidence: the decompiled `FUN_00022e94` and `FUN_00022f54`.

## Documents

`README.TXT` is the original game readme, in English.  It describes sound
volume, CD audio, and IPX network play.  `manual.pdf` is the game manual,
21786498 bytes.
