"""Build and check the named call graph of the rebuild store.

The graph holds one edge for each direct call from a named record to another
named record.  The tool reads the store and the original executable, so the
status page can render the graph without the game image.

Usage:
    python3 tools/rebuild/callgraph.py build [--original PATH] [--out PATH]
    python3 tools/rebuild/callgraph.py check [--original PATH] [--out PATH]

Exit code 0 on success, 1 on drift, and 2 on a missing input or a broken file.
"""
from __future__ import annotations

import argparse
import bisect
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import le  # noqa: E402
import store  # noqa: E402

DEFAULT_OUT = Path(__file__).resolve().parent / "callgraph.json"
EDGE_KEYS = ("from", "site", "to")


def _address(value: str) -> int:
    return int(value, 16)


def _sorted_addresses(records: list[dict]) -> list[int]:
    return sorted(_address(record["address"]) for record in records)


def resolve_target(target: int, addresses: list[int]) -> str | None:
    """The record that holds the target, or None when no record precedes it."""
    index = bisect.bisect_right(addresses, target) - 1
    if index < 0:
        return None
    return f"0x{addresses[index]:x}"


def make_edges(calls: list[tuple[str, int, int]], records: list[dict]) -> list[dict]:
    """The direct edges between named records.

    `calls` holds `(from_address, site_address, target_address)` triples.  A
    target belongs to the record that starts at or before it.  A call to an
    unnamed record, a missing record, or the same record adds no edge.
    """
    addresses = _sorted_addresses(records)
    named = {record["address"] for record in records if record.get("name")}
    edges: dict[tuple[str, str], dict] = {}
    for from_address, site_address, target in calls:
        if from_address not in named:
            continue
        to_address = resolve_target(target, addresses)
        if to_address is None or to_address not in named:
            continue
        if to_address == from_address:
            continue
        key = (from_address, to_address)
        if key in edges:
            continue
        edges[key] = {
            "from": from_address,
            "site": f"0x{site_address:x}",
            "to": to_address,
        }
    return sorted(
        edges.values(),
        key=lambda edge: (
            _address(edge["from"]),
            _address(edge["to"]),
            _address(edge["site"]),
        ),
    )


def build_edges(original: Path, records: list[dict]) -> list[dict]:
    """Scan the named records of the original image and return the edges."""
    image = le.load(original)
    calls: list[tuple[str, int, int]] = []
    for record in records:
        if not record.get("name"):
            continue
        body = le.function_body(
            image,
            _address(record["address"]),
            record.get("count", 4000),
        )
        for site in le.call_sites(body):
            if site.target is not None:
                calls.append((record["address"], site.address, site.target))
    return make_edges(calls, records)


def write_edges(edges: list[dict], path: Path = DEFAULT_OUT) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps({"edges": edges}, indent=2) + "\n")
    print(f"{path}: {len(edges)} edges")


def load_edges(path: Path = DEFAULT_OUT) -> list[dict]:
    """The edges of a call graph file, validated."""
    try:
        document = json.loads(path.read_text())
    except FileNotFoundError:
        raise ValueError(f"{path}: the file is missing") from None
    except OSError as error:
        raise ValueError(f"{path}: {error}") from error
    except json.JSONDecodeError as error:
        raise ValueError(f"{path}: broken JSON: {error}") from error
    edges = document.get("edges")
    if not isinstance(edges, list):
        raise ValueError(f"{path}: the `edges` list is missing")
    for edge in edges:
        if not isinstance(edge, dict) or set(edge) != set(EDGE_KEYS):
            raise ValueError(
                f"{path}: an edge does not hold {', '.join(EDGE_KEYS)}"
            )
        for key in EDGE_KEYS:
            value = edge[key]
            if not isinstance(value, str) or not store.ADDRESS_PATTERN.match(value):
                raise ValueError(
                    f"{path}: the {key} of an edge is not a hex address"
                )
    return edges


def _load_records() -> list[dict]:
    return [store.load_record(address) for address in store.candidate_addresses()]


def command_build(original: Path, out: Path) -> int:
    write_edges(build_edges(original, _load_records()), out)
    return 0


def command_check(original: Path, out: Path) -> int:
    expected = build_edges(original, _load_records())
    actual = load_edges(out)
    if expected != actual:
        print(f"{out}: the call graph drifted from the store or the original")
        return 1
    print(f"no drift: {len(actual)} edges")
    return 0


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build and check the named call graph of the store."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    for command, help_text in (
        ("build", "write the call graph file"),
        ("check", "write nothing and exit 1 on drift"),
    ):
        command_parser = subparsers.add_parser(command, help=help_text)
        command_parser.add_argument(
            "--original",
            type=Path,
            default=store.DEFAULT_ORIGINAL,
            help="the original executable",
        )
        command_parser.add_argument(
            "--out",
            type=Path,
            default=DEFAULT_OUT,
            help="the call graph file",
        )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        if arguments.command == "build":
            return command_build(arguments.original, arguments.out)
        return command_check(arguments.original, arguments.out)
    except (ValueError, OSError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
