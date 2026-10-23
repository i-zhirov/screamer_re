# File formats

This page records the observed formats.  The formats are not decoded yet.
Every claim below comes from file bytes and from `file` output.

### The TABS table loaders

- `FUN_0004cf94` loads `TABS\persp01.tab` into the `PERSPTAB` resource and
  stores the pointer at `DAT_001214ec` (module `0x1214EC`, linear `0x2E34EC`).
  The loader holds 43 instructions, at `obj.1 + 0x3cf94`, page 60 (file page
  61), file offset `0x57794`.
- `FUN_0004d030` loads `TABS\VIS.TAB` into the buffer `PTR_DAT_0008c1c4`
  (module `0x8C1C4`, linear `0x24E1C4`).  The loader holds 31 instructions, at
  `obj.1 + 0x3d030`, page 61 (file page 62), file offset `0x57830`.
- The two loaders sit in adjacent pages of `obj.1`, and the `PERSPTAB` loader
  comes first in the image.
- The read order is not observed at run time: the demo reaches neither loader,
  and the race-start walk leaves both watches armed with the `PERSPTAB` loader
  byte changed from `00` to `55`, so the page is touched by a write and the
  loader does not execute in that window.
  Evidence: `tools/scripts/probe_race_start.dbg` and the full breakpoint list.
- `PERSP01.TAB` holds 262656 bytes of 32-bit values and `VIS.TAB` holds 161024
  bytes of 16-bit values, so the `PERSPTAB` table holds 65664 values and the
  `VIS` table holds 80512 values.
  Evidence: the file sizes.

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

The map data files are LZW streams with no container.  The reader is the one
the graphics members use.  The expanded bytes are text.

| File | Raw | Expanded | Content |
| --- | --- | --- | --- |
| `DATA/MAP1.DAT`, `MAP2.DAT`, `MAP4.DAT`, `MAP6.DAT` | about 17 to 26 KB | 459 bytes | the `TRACK_DATA` list and the `OPPONENTS` list |
| `DATA/MAP3.DAT` | 21640 bytes | 179 bytes | the same two lists with one opponent |
| `DATA/GEN.DAT` | 4101 bytes | 6169 bytes | the `CARS_DATA` list and 15 more lists |
| `DATA/LOCAL.DAT` | 2490 bytes | 969 bytes | the credits text, `#` is the separator |

The lists hold one record per line.  The first byte of a field selects the type:
`d` is a 32-bit integer, `f` is a 32-bit float, and the other bytes are
separators.  A line that starts with `*` is a comment.  The parser reads until
the `END_LIST` line.

The `TRACK_DATA` record holds the fields `color`, `camera_hi`, `bonus_time`, and
`carnage`, all of the type `d`.  The record size is 0x10 and the record limit is
0x10.  The `MAP1.DAT` record is `d24 d300 d35 d10`.

The `OPPONENTS` record holds the fields `max_spd`, `accel`, `brakes`, `skill`,
and `sband`, all of the type `f`.  The record size is 0x14 and the record limit
is 0x590.  The `MAP1.DAT` list holds nine records.

The `MAP*I.DAT` files are not LZW.  They hold signed 16-bit values.  The first
dword is a count: 2 for `MAP1I.DAT` and `MAP2I.DAT`, 17 for `MAP3I.DAT`, 18
for `MAP4I.DAT`, 20 for `MAP6I.DAT`, and 27 for `MAP5I.DAT`.

The file holds records of the shape `1, 0, X, f1, Y, f2, Z, 0`, where the flags
`f1` and `f2` are `-1` or `-2` and the third flag is `0`.  `MAP1I.DAT` starts
`1, 0, 9301, -1, 5154, -1, 2300, 0`.  The file ends with one 32-bit value:
512 in `MAP1I.DAT` and 544 in `MAP5I.DAT`.  `MAP5I.DAT` holds 2 header words
and 2902 records of 7 words.  The values look like coordinates and the flags look
like a mode, so the field meanings stay open.

`MAP5.DAT` is not LZW.  It holds 41224 bytes: the count 23 and the size `0x10cc`
in the header, and then 23 records of `0x700` bytes, because `23 * 0x700 + 8` is
41224, the file size.  The record holds a mix of 32-bit floats and 32-bit
integers: the first record starts `0.0, 1.0, 0.0, 2, -44376, 0.0, 104296, 4300,
0.0, 0.9, 0.94, 0.0, 67985, 0.0`.  The exact record layout stays open.

The loaders are `FUN_00015460` (the named list parser), `FUN_000131a8` (the race
data load), `FUN_00027770` (the level start), and `FUN_00029d08` (the menu data
load).  See the findings page.

## Graphics data

The graphics files are containers with named members.

| File | Content |
| --- | --- |
| `GFX/GRAF.DAT` | the PCX images `menu1h.pcx`, `menu2h.pcx`, `menu3h.pcx`, `menu4h.pcx`, `champ5h.pcx`, `load%d%c.pcx`, and `net%c.pcx` |
| `GFX/MENU.DAT` | the sprites `menu%c.spr` and the fonts `menu%c.fnt` |
| `GFX/GAME.DAT` | compressed data |
| `GFX/MAP1H.DAT` to `GFX/MAP6H.DAT` | high-resolution map graphics |
| `GFX/MAP1L.DAT` to `GFX/MAP6L.DAT` | low-resolution map graphics |

`GFX/MAP5H.DAT` and `GFX/MAP5L.DAT` hold the same bytes.

### The PCX decoder

`pcx_load` at `0x1b5a4` loads a PCX member.  It builds the member name with
the format `%s\%s` and opens it in the mode `rb`.

| Step | Action |
| --- | --- |
| 1 | seeks 0x300 bytes before the end and reads the 256-colour palette |
| 2 | shifts every palette byte right by 2, so the palette becomes 6-bit VGA values |
| 3 | rewinds and reads the 128-byte PCX header |
| 4 | computes the pixel count as `(xmax - xmin + 1) * (ymax - ymin + 1)` |
| 5 | decodes the RLE runs: a byte with the mask `0xc0` equal to `0xc0` repeats the next byte `byte & 0x3f` times |

Evidence: the decompiled `FUN_0001b5a4` and the strings at `0x1009f5` and
`0x1007f80`.

### The sprite and font structures

`menu_space_init` at `0x29d08` loads the sprite and font data through two
loaders.

**The sprite structure.**  The sprite loader at `0x257b0` reads the head of the
file and fills two `SPRITES` resources.

| Offset | Size | Content |
| --- | ---: | --- |
| `+0x00` | 4 | the sprite count `DAT_00025cd4` |
| `+0x04` | 4 | the pixel data size `DAT_00025cdc` |
| `+0x08` | `count * 0x14` | the sprite records, one `SPRITES` resource |
| after | the size | the pixel data, the second `SPRITES` resource |

A sprite record is 20 bytes.  When the file is missing, the loader closes the
stream and calls `exit(0)`.
Evidence: the disassembly of `0x257b0` at `0x257cd` to `0x258b4`.

**The font structure.**  The font loader at `0x25658` reads a count into
`DAT_00025440`, then per font a 12-byte descriptor at `DAT_0011ca98`, then per
font a glyph table at `DAT_00119898` in a 0x500-byte slot, and finally the
`FONTS` resource with the packed glyphs.  The loader increments the font count
`DAT_00025440`.
Evidence: the disassembly of `0x25658`.

**Verification.**  `GFX/GAME.DAT` does not match the plain sprite structure at
its head.  The first dword is `0xfbf05b00`, which is not a plausible sprite
count for a file of 637267 bytes, and the head bytes do not hold text.
`GFX/MENU.DAT` does not match it either: the first dword is `0x03000000` and the
first word is 0.  Both files are compressed or encoded, so the record structures
above are the layouts of the loaded resources, not of the file bytes.  The
container
layout of both files stays open.  Evidence: the byte scans of both files.

`menu_space_init` also loads the named lists `menu`, `cardat`, `trackdat` (with
the separator `-----`), and `champ` through the list loader at `0x1b394`, and it
stores the `OPT_PCX` resource in `DAT_0011fad0`, the back buffer.
Evidence: the decompiled `FUN_00029d08`, `FUN_0001b394`, and `FUN_0002eed0`.

Verification: `GFX/MENU.DAT` holds no member names.  A scan of the 2230684
bytes finds no `menu`, `.spr`, `.fnt`, or `.pcx` text, so the file is not a
name-indexed container.  The names `menu%c.spr` and `menu%c.fnt` belong to the
resource layer, which resolves a name inside a file the same way `pcx_load`
resolves `gfx\graf.dat` and `menu1h.pcx`.  The member layout of the file is
open; the font structure above is the layout of the loaded `FONTS` resource, not
of
the file bytes.
Evidence: the byte scan of `GFX/MENU.DAT` and the decompiled `FUN_00025658`.

### The file directory

The last 4 bytes hold the size of the directory.  The directory holds
`size / 32` records of 32 bytes each.  The decoder at `0x4c128` reads the
records from the end and compares each block with the wanted member name through
the case-insensitive compare `0x4d9ba`.  The open wrapper `0x4c08d` calls the
decoder, so the file layer decodes a file when it opens it.

| Offset | Size | Content |
| --- | ---: | --- |
| `+0x00` | 16 | the member name, exclusive-ORed with `0x3a`, NUL-terminated |
| `+0x10` | 4 | the member offset in bytes |
| `+0x14` | 4 | the member size in bytes |
| `+0x18` | 4 | the end offset of the member |
| `+0x1c` | 4 | a flag |

The offsets form one chain: `offset + size` of a member is the `offset` of the
next member, and the last member ends at the start of the directory.
`GFX/GAME.DAT` holds 39 members, because `1248 / 32` is 39.
`GFX/MENU.DAT` holds 18 members, because `576 / 32` is 18, with two empty
records at the end.  Evidence: the decode of both directories.

### The members

| Member | Offset | Size |
| --- | ---: | ---: |
| `GAMEL.SPR` | 559712 | 10010 |
| `GAMEH.SPR` | 569722 | 28736 |
| `GAMEL.FNT` | 598458 | 11565 |
| `GAMEH.FNT` | 610023 | 25992 |

The names of `GFX/GAME.DAT` form low and high resolution pairs, as in
`GAMEL.SPR` and `GAMEH.SPR`, and the sprite and font members are the ones
that `sprite_load` and `font_load` read.  `GFX/MENU.DAT` holds the palette
`MENU.COL` of 768 bytes at the offset 0, which is 256 colours of 3 bytes, the
`MENU.MLB` member, and the 3D objects `ALETTER.3DX`, `AMAP1.3DX` to
`AMAP6.3DX`, `AAUTO00.3DX` to `AAUTO06.3DX`, and `ASHADOW.3DX`.
Evidence: the exclusive-OR decode of both directories.

### The font member layout

The font loader at `0x25658` reads the font members of `GFX/GRAF.DAT`:

| Offset | Content |
| --- | --- |
| `+0x00` | the count, one byte |
| `+0x01` | `count + 1` descriptors of 12 bytes each |
| then | one glyph table per descriptor of `num_chars` entries of 8 bytes |
| then | the tail size, four bytes |
| then | the tail with the packed glyphs |

The descriptor holds the glyph height, the first character code, and the
character count.  `MENUH.FNT` holds `(22, 32, 76)`, `(18, 32, 76)`,
`(38, 32, 76)`, and `(60, 32, 76)`, and the high fonts are exactly double the
low
fonts.  The heights confirm the two resolutions: `MENUL.FNT` holds 11, 9, 19,
and
30, `MENUH.FNT` holds 22, 18, 38, and 60, and `ATTRL.FNT` holds 19 against
`ATTRH.FNT` with 38.

The glyph table entry holds the glyph width in pixels and the offset of the
glyph
in the tail.  The glyph holds `height` rows of `width` pixels packed four bits
per
pixel, so the glyph size is `ceil(width / 2) * height`, and the next offset
follows
exactly.  For `ATTRH.FNT` the first entries are `(19, 0)`, `(13, 380)`, `(13,
646)`,
`(27, 912)`, and `(31, 1444)`: the first glyph is the space of 19 by 38 pixels,
which is 380 bytes, and the second glyph starts at 380.

The tail is the `FONTS` resource: `resource_get(engine, "FONTS", size)` at
`0x25755`
with the string `FONTS` at `0x100ead`, read into the buffer at `0x1543c`.  The
tail
size follows the last table and the member ends with the tail.  The sizes
confirm the
layout: `GAMEH.FNT` ends its tables at `+0x65d` with the tail size 93246 and
`ATTRH.FNT` ends them at `+0x26d` with the tail size 32490, and in both cases
the
tail size plus the table end plus four equals the member size exactly.

The character sheets confirm the layout: every font renders the ASCII set from
the
space to `z` with the accented characters `É`, `Ñ`, `¿`, `¡`, `À`, `Â`, `È`,
`Ê`,
`Ë`, `Î`, `Ï`, `Ô`, `Õ`, `Ù`, `Û`, and `Ü`.  The first character code 32 and the
count 76 cover the codes 32 to 107, and the game fonts with the first code 48
and
the count 10 or 11 cover the digits and the colon.
Evidence: the disassembly of `0x25658` and the font, render, and export
commands.

### The font pixel format and row order

The draw routine at `0x25000` to `0x25500` reads the glyph bitmaps and writes
them to the screen, and its instructions fix the pixel format and the row order:

| Instruction | Meaning |
| --- | --- |
| `0x25316` `al = byte [esi]` | the bitmap byte holds two pixels |
| `0x25322` `and al, 0xf0` | the high nibble is the first pixel |
| `0x25326` `shr al, 4` | the high nibble moves to the low half |
| `0x2532f` `[edi] = al` | the first screen pixel is written |
| `0x2533f` `and bl, 0xf` | the low nibble is the second pixel |
| `0x2534a` `[edi+1] = bl` | the second screen pixel is written |
| `0x2534d` `add edi, 2` | the screen advances two pixels per byte |
| `0x2537a` `add edi, 0x280` | the next row is 640 bytes away |
| `0x25329` `or eax, [0x15460]` | the colour base is added to the pixel |
| `0x25324` `je 0x25331` | the zero nibble is skipped |

The pixel format is therefore four bits per pixel, two pixels per byte, with the
high nibble first, the zero nibble transparent, and the colour base at `0x15460`
added to the value.  The row order is top to bottom with a stride of `0x280`,
which
is 640, the screen width.  The glyph width is the descriptor field 32 and the
glyph
height is the descriptor height, and the cell holds 2560 pixels, which is 1280
bytes.
The draw routine also fixes the per-font table slot as `0xa0 * 8`, which is
`0x500`, and the glyph width and height come from the table entry and the
descriptor.
Evidence: the disassembly of `0x25300` to `0x25500`.

The character sheets render every glyph as a letter: `MENUH.FNT` shows the
complete
ASCII set and `GAMEH.FNT` shows the italic game font with the accented
characters.
Evidence: the render command and the character sheets.

### The member data compression

The member data is LZW-compressed.  The read path is:

| Module address | Name | Evidence |
| --- | --- | --- |
| `0x4c4e5` | `stream_read` | tests the stream flag at `+0x10`; a clear flag reads raw and a set flag calls the expander |
| `0x4d3fc` | `lzw_expand` | loops on the block decoder until it returns 0 |
| `0x4d4c4` | `lzw_block` | the LZW block decoder; the codes 0x100 clear and 0x101 end the stream |
| `0x4d844` | `lzw_bit_read` | returns the code at the bit position, masked to the code width |

The block decoder holds the dictionary at `+0x4074` with four byte entries and
the index at `+0x4064`, a code width at `+0x4068` that starts at 9 and grows to
12, and the output count at `+0x4070`.  The codes `0x100` and `0x101` clear the
dictionary and end the stream, and the first free entry is `0x102`, which is the
classic LZW coding.
Evidence: the decompiled `FUN_0004d4c4` and `FUN_0004d3fc`.

The bit reader `0x4d844` holds a bit buffer of 0x400 bytes at `+0x8086`, the bit
position at `+0x406c`, the shift count at `+0x4068`, and a mask table at
`+0x8050`.
It returns the 16-bit value at the bit position, shifted right by the low 3 bits
of
the position and masked with the mask of the shift count.  The mask table holds
the
values `(1 << n) - 1` for the shift counts 9 to 12, so the reader returns 9 to
12
bits at a time.

The tool reads the plain bit stream instead of emulating the 0x400 byte buffer:
the
code starts at the next bit, the low bit first, and the width runs from 9 to 12
bits.
The plain reader is the correct one.  The first port emulated the game buffer
and
diverged at the first refill, so the logo sprites showed the first rows and
noise
after them and every font glyph was noise.  The plain reader renders `LOGOH.SPR`
sprite 0 as the Virgin Interactive Entertainment logo, the Graffiti logo, and
the
SCREAMER logo, and it renders every font as a readable character set.
Evidence: the expansion test and the render of the sprites and the fonts.

The expansion is confirmed: every sprite and font member of `GFX/GRAF.DAT` and
`GFX/GAME.DAT` expands to its member end field exactly, which for a sprite is
`8 + count * 0x14 + size`.  `LOGOH.SPR` expands to 208636 bytes, the sprite
count at `+0x00` is 3, the pixel data size at `+0x04` is 208568, and the records
are 0, 0, 200, 240, 0, then 1, 1, 466, 188, 48000, then 0, 0, 640, 114, 135608,
which matches the run-time records at `0150:8D800` exactly.  `LOGOL.SPR` expands
to 52538, `MENUL.SPR` to 75344, `MENUH.SPR` to 294846, `GAMEL.SPR` to 24483,
and `GAMEH.SPR` to 93675.  The stream ends with the end code, and the plain
reader stops there.
Evidence: the expansion test and the record dump.

**Re-verification of the sprite layout.**  The member offsets, the sizes, and
the names resolve.  The member data does not match the plain sprite structure,
because it is LZW-compressed; the expansion runs when the member is read, not
when
the directory is read.  The expansion is now confirmed, so the re-verification
of
the sprite layout holds.  Evidence: the expansion test and the record dump.

### The resolution modes

The game ships two executables, `STARTH.EXE` and `STARTL.EXE`.  They differ only
in the resolution letter of the hardcoded graphics names: the high executable
names
`menu1h.pcx`, `menu2h.pcx`, `menu3h.pcx`, `menu4h.pcx`, and `champ5h.pcx`, and
the low executable names the same files with the letter `l`.

The members of the graphics containers come in low and high pairs, and the
executable picks the pair that fits its mode: `GAMEL.SPR` and `GAMEH.SPR`,
`GAMEL.FNT` and `GAMEH.FNT`, `IMPLET_L.PKC` and `IMPLET_H.PKC`,
`IMPRES_L.PKC` and `IMPRES_H.PKC`, `CDFRONTL.PKC` and `CDFRONTH.PKC`, and
`COCK00AL.PKC` and `COCK00AH.PKC`.  The map files follow the same rule:
`GFX/MAP1L.DAT` to `GFX/MAP6L.DAT` and `GFX/MAP1H.DAT` to
`GFX/MAP6H.DAT`.

The other names take the letter at run time through the `%c` field, as in
`game%c.spr` and `menu%c.pcx`.  The GOG configuration launches `starth.exe`,
so the installed single-player start is the high-resolution one.

## Table data

| File | Bytes | Content |
| --- | ---: | --- |
| `TABS/VIS.TAB` | 161024 | 16-bit little-endian values, 80512 entries |
| `TABS/PERSP01.TAB` | 262656 | 32-bit little-endian values, 65664 entries |

The values in `VIS.TAB` fall in small steps.  The values in
`PERSP01.TAB` repeat `0xffffff81` (signed -127) in the first rows.  The
meaning of both tables is not known yet.

The loaders are known.  `FUN_0004d030` reads `TABS/VIS.TAB` into the buffer
`PTR_DAT_0008c1c4`.  `FUN_0004cf94` reads `TABS/PERSP01.TAB` into the
`PERSPTAB` resource and then points the resource pointer 0x40000 bytes past the
start, so the resource holds two tables of 0x40000 bytes each.

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
