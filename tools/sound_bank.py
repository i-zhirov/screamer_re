#!/usr/bin/env python3
"""Decode the Screamer sound formats: the ADLIB- banks, the sample bank, and the songs.

The tool decodes the three sound formats of the shipped artifacts:

- The `ADLIB-` instrument banks `DATA/MUSIC.DAT` and `CONFIG/SETUP.DAT` hold a
  header of `0x1E` bytes, an instrument table of 127 records of 12 bytes and a
  10-byte remainder, and the instrument data of 128 records of 30 bytes.  A
  table record holds the key, the name, and the index.  A data record holds the
  OPL register values in the driver write order, which the rebuilt driver
  confirms: the register `0x20` at `+0x06`, `0x40` at `+0x0A`, `0x60` at
  `+0x0D`, `0x80` at `+0x11`, `0xE0` at `+0x13`, `0x23` at `+0x17`, `0x43` at
  `+0x1A`, and `0xC0` at `+0x1B`.

- The sample bank `DATA/SOUND.DAT` is a member container of 92 members that are
  named `<name>.RAW`.  The `SOUND_FX` lists of `DATA/GEN.DAT` and the
  `DATA/MAP*I.DAT` containers name the members.  A list record holds the name,
  the volume, the loop, the loop start, the loop length, the pitch, and the pan.
  The sample loader at `0x1f2e0` derives the runtime flags from the record: the
  flag `0x100` from the bit 4 of the volume high byte, the flag `0x40` from a
  non-zero loop length, the flag `0x4000` from a non-zero loop, the flag `0x400`
  from the bit 4 of the pitch high byte, and the flag `0x200` from the bit 4 of
  the pan high byte.  A member holds 8-bit unsigned samples at the rate 11025,
  which the HMI digital driver init at `0x1f1fb` passes.  The lists name 87 of the
  92 members; the five members `CRY`, `TOUR0`, `TOUR1`, `TOUR2`, and `TRAIN`
  are in no list, so the sample loader never opens them.

- The songs `MUSIC1.HMP` to `MUSIC7.HMP` and `MIDITEST.HMP` hold a header and
  the chunks.  The header holds the signature `HMIMIDIP`, the version `013195`,
  the file length at `+0x20`, the track count at `+0x30`, the value `0x180` at
  `+0x34`, the division `0x78` at `+0x38`, the duration at `+0x3C`, and the
  instrument table of 175 records at `+0x40`.  The chunks start at `+0x388` and
  hold the chunk number, the chunk length, the track number, and the MIDI data.
  The delta runs 7 bits per byte, the low bits first, and the byte with the bit 7
  set ends the number.  The events are standard MIDI events, and the track ends
  with the meta event `FF 2F 00`.

Usage:
    python3 tools/sound_bank.py bank <file>
    python3 tools/sound_bank.py samples <sound.dat> --lists <container>... [--out DIR]
    python3 tools/sound_bank.py song <file> [--out DIR]
"""

from __future__ import annotations

import argparse
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import gfx_container  # noqa: E402

BANK_HEADER_SIZE = 0x1E
BANK_INDEX_OFFSET = 10
BANK_RECORD_SIZE = 12
BANK_INSTRUMENT_COUNT = 28
BANK_DATA_RECORDS = 128
BANK_DATA_RECORD_SIZE = 30

# The OPL register of each instrument record byte.  The driver HMIMDRV.386 at
# 0x14157 writes the register then the value in this order, and the rebuilt
# driver reproduces the eight pairs of the first DRUM.BNK record.
BANK_REGISTERS = (
    (0x20, 0x06),
    (0x40, 0x0A),
    (0x60, 0x0D),
    (0x80, 0x11),
    (0xE0, 0x13),
    (0x23, 0x17),
    (0x43, 0x1A),
    (0xC0, 0x1B),
)

SAMPLE_FLAG_LOOP_LENG = 0x40
SAMPLE_FLAG_VOLUME = 0x100
SAMPLE_FLAG_PAN = 0x200
SAMPLE_FLAG_PITCH = 0x400
SAMPLE_FLAG_LOOP = 0x4000

SAMPLE_RATE = 11025

SONG_SIGNATURE = b"HMIMIDIP"
SONG_VERSION = b"013195"
SONG_DATA_OFFSET = 0x388
SONG_CHUNK_HEADER = 12
SONG_META = 0xFF
SONG_META_END = 0x2F

EVENT_DATA = {0x80: 2, 0x90: 2, 0xA0: 2, 0xB0: 2, 0xC0: 1, 0xD0: 1, 0xE0: 2}
EVENT_NAMES = {
    0x80: "note-off",
    0x90: "note-on",
    0xA0: "aftertouch",
    0xB0: "control-change",
    0xC0: "program-change",
    0xD0: "channel-aftertouch",
    0xE0: "pitch-bend",
}


@dataclass
class Bank:
    path: Path
    data: bytes
    version: int
    count: int
    data_offset: int
    names: list[tuple[int, str, int]]
    table_remainder: bytes
    records: list[bytes]


@dataclass
class Sample:
    name: str
    member: str
    offset: int
    size: int
    flags: int
    fields: dict[str, int]
    path: Path


@dataclass
class Event:
    delta: int
    status: int
    data: tuple[int, ...]


@dataclass
class Chunk:
    number: int
    length: int
    track: int
    data: bytes
    events: list[Event] = field(default_factory=list)


@dataclass
class Song:
    path: Path
    data: bytes
    signature: bytes
    version: bytes
    length: int
    track_count: int
    value_180: int
    division: int
    duration: int
    instruments: bytes
    chunks: list[Chunk]


def parse_bank(path: Path) -> Bank:
    """Parse an `ADLIB-` instrument bank.

    The header is `0x1E` bytes: the version at `+0x00`, the signature
    `ADLIB-` at `+0x02`, the values `0x007F` at `+0x08` and `+0x0A`, the
    instrument count 28 at `+0x0C`, and the instrument data offset `0x61C` at
    `+0x10`.  The instrument table holds 127 records of 12 bytes and a
    10-byte remainder to the data offset, and the instrument data holds 128
    records of 30 bytes that reach the file end.
    """
    data = path.read_bytes()
    if len(data) < BANK_HEADER_SIZE:
        raise ValueError(f"{path}: the file is shorter than the bank header")
    if data[2:8] != b"ADLIB-":
        raise ValueError(f"{path}: the signature at +0x02 is not ADLIB-")
    version = struct.unpack_from("<H", data, 0)[0]
    count = struct.unpack_from("<I", data, 0x0C)[0]
    data_offset = struct.unpack_from("<I", data, 0x10)[0]
    if count != BANK_INSTRUMENT_COUNT:
        raise ValueError(f"{path}: the instrument count {count} is not {BANK_INSTRUMENT_COUNT}")
    if data_offset + BANK_DATA_RECORDS * BANK_DATA_RECORD_SIZE != len(data):
        raise ValueError(
            f"{path}: the instrument data at {data_offset:#x} plus "
            f"{BANK_DATA_RECORDS * BANK_DATA_RECORD_SIZE} bytes does not reach "
            f"the file size {len(data)}"
        )
    names: list[tuple[int, str, int]] = []
    offset = BANK_HEADER_SIZE
    while offset + BANK_RECORD_SIZE <= data_offset:
        record = data[offset : offset + BANK_RECORD_SIZE]
        # The record is the key (1 byte), the name (9 bytes), and the index
        # (2 bytes).  The drum records use the key as the MIDI note, and the
        # melodic records use the key 0x01.
        name = record.split(b"\0")[0]
        if name[:1] and name[:1][0] < 0x20:
            name = name[1:]
        names.append((record[0], name.decode("ascii", "replace"), struct.unpack_from("<H", record, BANK_INDEX_OFFSET)[0]))
        offset += BANK_RECORD_SIZE
    table_remainder = data[offset:data_offset]
    records = [
        data[data_offset + index * BANK_DATA_RECORD_SIZE : data_offset + (index + 1) * BANK_DATA_RECORD_SIZE]
        for index in range(BANK_DATA_RECORDS)
    ]
    return Bank(path, data, version, count, data_offset, names, table_remainder, records)


def bank_pairs(record: bytes) -> list[tuple[int, int]]:
    """Return the OPL register and value pairs of an instrument record.

    The driver HMIMDRV.386 at `0x14157` writes the register then the value,
    and the rebuilt driver confirms the pairs of the first `DRUM.BNK` record.
    """
    return [(register, record[offset]) for register, offset in BANK_REGISTERS]


def print_bank(bank: Bank) -> int:
    """Print the header, the instrument table, and the instrument records."""
    print(f"file:        {bank.path}")
    print(f"bytes:       {len(bank.data)}")
    print(f"version:     {bank.version >> 8}.{bank.version & 0xFF}")
    print(f"signature:   {bank.data[2:8].decode()}")
    print(f"count:       {bank.count}")
    print(f"data offset: {bank.data_offset:#x}")
    print(f"header:      {bank.data[:BANK_HEADER_SIZE].hex(' ')}")
    print(f"instrument table: {len(bank.names)} records, remainder {bank.table_remainder.hex(' ')}")
    for index, (key, name, ordinal) in enumerate(bank.names):
        print(f"  {index:3d}  key {key:#04x}  {name:<10} index {ordinal}")
    print("instrument records:")
    for index, record in enumerate(bank.records):
        pairs = "  ".join(f"{register:02X}:{value:02X}" for register, value in bank_pairs(record))
        print(f"  {index:3d}  {record.hex(' ')}  {pairs}")
    return 0


def collect_lists(paths: list[Path]) -> tuple[dict[str, dict[str, int]], int]:
    """Collect the `SOUND_FX` records of the list containers by the member name.

    A level can name the sample of another level, so a name can repeat.  The last
    record wins and the repeat count is reported.
    """
    records: dict[str, dict[str, int]] = {}
    repeats = 0
    for path in paths:
        container = gfx_container.parse(path)
        for member in container.members:
            if not member.flag & gfx_container.COMPRESSED_BIT:
                continue
            text = gfx_container.member_read(container, member).decode("ascii", "replace")
            if not any(line.strip() == "SOUND_FX" for line in text.splitlines()):
                continue
            fields, rows = gfx_container.parse_list(text, "SOUND_FX")
            for row in rows:
                record = dict(zip(fields, (value for _, value in row)))
                name = str(record["name"]).lower()
                if name in records:
                    repeats += 1
                records[name] = {key: int(value) for key, value in record.items() if key != "name"}
    return records, repeats


def sample_flags(record: dict[str, int]) -> int:
    """Derive the runtime flags of a sample record.

    The sample loader at `0x1f2e0` reads the record fields and sets the flags:
    the flag `0x100` from the bit 4 of the volume high byte, the flag `0x40`
    from a non-zero loop length, the flag `0x4000` from a non-zero loop, the
    flag `0x400` from the bit 4 of the pitch high byte, and the flag `0x200`
    from the bit 4 of the pan high byte.
    """
    flags = 0
    if (record["volume"] >> 24) & 0x10:
        flags |= SAMPLE_FLAG_VOLUME
    if record["loop_leng"] != 0:
        flags |= SAMPLE_FLAG_LOOP_LENG
    if record["loop"] != 0:
        flags |= SAMPLE_FLAG_LOOP
    if (record["pitch"] >> 24) & 0x10:
        flags |= SAMPLE_FLAG_PITCH
    if (record["pan"] >> 24) & 0x10:
        flags |= SAMPLE_FLAG_PAN
    return flags


def load_samples(sound_path: Path, list_paths: list[Path]) -> tuple[gfx_container.Container, dict[str, dict[str, int]], list[Sample], int]:
    """Load the sample bank and join the members with the `SOUND_FX` records."""
    container = gfx_container.parse(sound_path)
    lists, repeats = collect_lists(list_paths)
    samples: list[Sample] = []
    for member in container.members:
        name = member.name[:-4].lower() if member.name.upper().endswith(".RAW") else member.name.lower()
        if name not in lists:
            continue
        record = lists[name]
        samples.append(
            Sample(
                name=name,
                member=member.name,
                offset=member.offset,
                size=member.size,
                flags=sample_flags(record),
                fields=record,
                path=sound_path,
            )
        )
    return container, lists, samples, repeats


def print_samples(container: gfx_container.Container, lists: dict[str, dict[str, int]], samples: list[Sample], repeats: int) -> int:
    """Print the sample bank and the joined sample records."""
    print(f"file:    {container.path}")
    print(f"bytes:   {len(container.data)}")
    print(f"members: {len(container.members)}")
    print(f"records: {len(lists)}")
    print(f"joined:  {len(samples)} (the list names repeat {repeats} times)")
    unlisted = [member.name for member in container.members if not any(
        member.name == sample.member for sample in samples
    )]
    print(f"members with no record: {len(unlisted)} {', '.join(unlisted)}")
    print(f"{'name':<12} {'offset':>8} {'size':>8} {'volume':>10} {'loop':>5} "
          f"{'start':>6} {'leng':>6} {'pitch':>10} {'pan':>10} flags")
    for sample in samples:
        fields = sample.fields
        print(
            f"{sample.name:<12} {sample.offset:>8} {sample.size:>8} "
            f"{fields['volume']:>10} {fields['loop']:>5} {fields['loop_start']:>6} "
            f"{fields['loop_leng']:>6} {fields['pitch']:>10} {fields['pan']:>10} "
            f"{sample.flags:#06x}"
        )
    return 0


def wav_header(data: bytes, rate: int) -> bytes:
    """Build a WAV header for 8-bit unsigned mono samples."""
    return b"".join(
        (
            b"RIFF",
            struct.pack("<I", 36 + len(data)),
            b"WAVE",
            b"fmt ",
            struct.pack("<IHHIIHH", 16, 1, 1, rate, rate, 1, 8),
            b"data",
            struct.pack("<I", len(data)),
        )
    )


def write_samples(container: gfx_container.Container, samples: list[Sample], out_dir: Path, rate: int) -> int:
    """Write one WAV file per joined sample member."""
    out_dir.mkdir(parents=True, exist_ok=True)
    for sample in samples:
        raw = container.data[sample.offset : sample.offset + sample.size]
        out = out_dir / f"{sample.name.upper()}.WAV"
        out.write_bytes(wav_header(raw, rate) + raw)
        print(f"wrote {out} ({len(raw)} bytes, rate {rate}, 8-bit unsigned mono)")
    return 0


def read_delta(data: bytes, position: int, end: int) -> tuple[int, int]:
    """Read one HMP delta: 7 bits per byte, the low bits first, bit 7 ends.

    The songs of the game confirm the order: every chunk of `MUSIC1.HMP` to
    `MUSIC7.HMP` and `MIDITEST.HMP` reaches its end with this reader, and the
    first event of the chunk 1 of `MUSIC1.HMP` is the control change `B2 20 00`
    with the delta 0.
    """
    value = 0
    shift = 0
    while True:
        if position >= end:
            raise ValueError(f"the delta runs past the chunk end at {position:#x}")
        byte = data[position]
        position += 1
        value |= (byte & 0x7F) << shift
        shift += 7
        if byte & 0x80:
            return value, position
        if shift > 35:
            raise ValueError(f"the delta is too long at {position:#x}")


def read_varlen(data: bytes, position: int, end: int) -> tuple[int, int]:
    """Read a standard MIDI variable-length number, the high bits first."""
    value = 0
    while True:
        if position >= end:
            raise ValueError(f"the length runs past the chunk end at {position:#x}")
        byte = data[position]
        position += 1
        value = (value << 7) | (byte & 0x7F)
        if not byte & 0x80:
            return value, position


def parse_events(data: bytes, start: int, end: int) -> list[Event]:
    """Parse the MIDI events of one chunk.

    The events are standard MIDI events with a running status.  The track ends
    with the meta event `FF 2F 00`.
    """
    events: list[Event] = []
    position = start
    status = None
    while position < end:
        delta, position = read_delta(data, position, end)
        if position >= end:
            raise ValueError(f"the status runs past the chunk end at {position:#x}")
        byte = data[position]
        if byte & 0x80:
            status = byte
            position += 1
        elif status is None:
            raise ValueError(f"the first event has no status at {position:#x}")
        if status == SONG_META:
            if position >= end:
                raise ValueError(f"the meta event runs past the chunk end at {position:#x}")
            payload = position
            meta_type = data[position]
            position += 1
            length, position = read_varlen(data, position, end)
            if position + length > end:
                raise ValueError(f"the meta data runs past the chunk end at {position:#x}")
            events.append(Event(delta, status, tuple(data[payload : position + length])))
            position += length
            if meta_type == SONG_META_END:
                break
            continue
        command = status & 0xF0
        if command not in EVENT_DATA:
            raise ValueError(f"the status {status:#04x} is not an event at {position:#x}")
        size = EVENT_DATA[command]
        if position + size > end:
            raise ValueError(f"the event data runs past the chunk end at {position:#x}")
        events.append(Event(delta, status, tuple(data[position : position + size])))
        position += size
    return events


def parse_song(path: Path) -> Song:
    """Parse an HMP song: the header, the instrument table, and the chunks."""
    data = path.read_bytes()
    if len(data) < SONG_DATA_OFFSET:
        raise ValueError(f"{path}: the file is shorter than the song header")
    signature = data[:8]
    version = data[8:14]
    if signature != SONG_SIGNATURE:
        raise ValueError(f"{path}: the signature {signature!r} is not HMIMIDIP")
    if version != SONG_VERSION:
        raise ValueError(f"{path}: the version {version!r} is not 013195")
    length = struct.unpack_from("<I", data, 0x20)[0]
    track_count = struct.unpack_from("<I", data, 0x30)[0]
    value_180 = struct.unpack_from("<I", data, 0x34)[0]
    division = struct.unpack_from("<I", data, 0x38)[0]
    duration = struct.unpack_from("<I", data, 0x3C)[0]
    if length + track_count != len(data):
        raise ValueError(
            f"{path}: the length {length} plus the track count {track_count} "
            f"is not the file size {len(data)}"
        )
    chunks: list[Chunk] = []
    position = SONG_DATA_OFFSET
    while position + SONG_CHUNK_HEADER <= length:
        number, chunk_length, track = struct.unpack_from("<III", data, position)
        if chunk_length < SONG_CHUNK_HEADER:
            raise ValueError(f"{path}: the chunk {number} is shorter than its header")
        end = position + chunk_length
        if end > length:
            raise ValueError(f"{path}: the chunk {number} runs past the data end")
        chunk = Chunk(number, chunk_length, track, data[position + SONG_CHUNK_HEADER : end])
        chunk.events = parse_events(data, position + SONG_CHUNK_HEADER, end)
        chunks.append(chunk)
        position = end
    if position != length:
        raise ValueError(f"{path}: the chunks end at {position:#x}, not at {length:#x}")
    if len(chunks) != track_count:
        raise ValueError(f"{path}: the chunks {len(chunks)} do not match the track count {track_count}")
    return Song(
        path,
        data,
        signature,
        version,
        length,
        track_count,
        value_180,
        division,
        duration,
        data[0x40:0x40 + 175 * 4],
        chunks,
    )


def print_song(song: Song) -> int:
    """Print the header and the chunks of a song."""
    print(f"file:        {song.path}")
    print(f"bytes:       {len(song.data)}")
    print(f"signature:   {song.signature.decode()}")
    print(f"version:     {song.version.decode()}")
    print(f"length:      {song.length:#x}")
    print(f"tracks:      {song.track_count}")
    print(f"value 180:   {song.value_180:#x}")
    print(f"division:    {song.division}")
    print(f"duration:    {song.duration} s")
    print(f"instrument table: {len(song.instruments)} bytes at +0x40")
    print("chunks:")
    for chunk in song.chunks:
        ticks = sum(event.delta for event in chunk.events)
        counts: dict[str, int] = {}
        for event in chunk.events:
            name = EVENT_NAMES.get(event.status & 0xF0, hex(event.status))
            counts[name] = counts.get(name, 0) + 1
        summary = "  ".join(f"{name}:{count}" for name, count in sorted(counts.items()))
        print(
            f"  {chunk.number:2d}  track {chunk.track:2d}  {chunk.length:#06x} bytes  "
            f"{len(chunk.events):6d} events  {ticks:6d} ticks  {summary}"
        )
    return 0


def write_varlen(value: int) -> bytes:
    """Write a standard MIDI variable-length number, the high bits first."""
    out = bytearray([value & 0x7F])
    value >>= 7
    while value:
        out.append(0x80 | (value & 0x7F))
        value >>= 7
    out.reverse()
    return bytes(out)


def write_song(song: Song, chunk: Chunk, out: Path) -> int:
    """Write one chunk as a standard MIDI file with one track."""
    events = bytearray()
    for event in chunk.events:
        events += write_varlen(event.delta)
        events.append(event.status)
        events += bytes(event.data)
    track = b"".join((b"MTrk", struct.pack(">I", len(events)), bytes(events)))
    header = b"".join((b"MThd", struct.pack(">IHHH", 6, 0, 1, song.division)))
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(header + track)
    print(f"wrote {out} ({len(header) + len(track)} bytes, the track {chunk.track})")
    return 0


def run_bank(args: argparse.Namespace) -> int:
    return print_bank(parse_bank(args.file))


def run_samples(args: argparse.Namespace) -> int:
    container, lists, samples, repeats = load_samples(args.sound, args.lists)
    print_samples(container, lists, samples, repeats)
    if args.out is not None:
        write_samples(container, samples, args.out, args.rate)
    return 0


def run_song(args: argparse.Namespace) -> int:
    song = parse_song(args.file)
    print_song(song)
    if args.out is not None:
        for chunk in song.chunks:
            write_song(song, chunk, args.out / f"{args.file.stem}_{chunk.number:02d}_track{chunk.track:02d}.mid")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    subparsers = parser.add_subparsers(dest="command", required=True)

    bank_parser = subparsers.add_parser("bank", help="decode an ADLIB- instrument bank")
    bank_parser.add_argument("file", type=Path)

    samples_parser = subparsers.add_parser("samples", help="decode the sample bank with its list records")
    samples_parser.add_argument("sound", type=Path)
    samples_parser.add_argument("--lists", type=Path, nargs="+", required=True, help="the list containers")
    samples_parser.add_argument("--out", type=Path, help="write one WAV per sample")
    samples_parser.add_argument("--rate", type=int, default=SAMPLE_RATE)

    song_parser = subparsers.add_parser("song", help="decode an HMP song")
    song_parser.add_argument("file", type=Path)
    song_parser.add_argument("--out", type=Path, help="write one MIDI file per chunk")

    args = parser.parse_args()
    try:
        if args.command == "bank":
            return run_bank(args)
        if args.command == "samples":
            return run_samples(args)
        return run_song(args)
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
