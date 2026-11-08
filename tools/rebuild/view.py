"""Render the self-contained status page of the rebuild store.

The page holds one row per function record and one row per module record that
holds no function record.  The board groups the records by the section names of the
program map, then by module, then by the records without a section.  The style and
the script are inline, so the page needs no server and no external reference.

Usage:
    python3 tools/rebuild/view.py html [--out PATH]

Exit code 0 on success and 2 on a missing record or a broken store file.
"""
from __future__ import annotations

import argparse
import html
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import callgraph  # noqa: E402
import docs  # noqa: E402
import store  # noqa: E402

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent.parent
DEFAULT_OUT = REPOSITORY_ROOT / "work/rebuild/status.html"
UNDEFINED = "—"
STALE_MARK = "⚠"
PASS_MARK = "✓"
MODULE_GROUP = "Modules"
UNMAPPED_GROUP = "Unmapped"
MISSING_NAME = "(unnamed)"
MAP_MIN_WIDTH = 0.25
MAP_TICK_COUNT = 4
TREEMAP_WIDTH = 1000
TREEMAP_HEIGHT = 620
TREEMAP_GAP = 2
TREEMAP_HEADER = 18
GRAPH_NODE_WIDTH = 156
GRAPH_NODE_HEIGHT = 38
GRAPH_COLUMN_GAP = 56
GRAPH_ROW_GAP = 10
GRAPH_MARGIN = 16
GRAPH_NAME_LIMIT = 20

STYLE = """\
:root {
  color-scheme: light;
  --page-bg: linear-gradient(160deg, #eef0ff 0%, #f7f6ff 42%, #eaf7ff 100%);
  --bg-surface: #ffffff;
  --bg-elevated: #f1f2ff;
  --border: #e2e3f6;
  --border-strong: #cbcef0;
  --text-primary: #1b1b2f;
  --text-secondary: #666680;
  --text-tertiary: #9a9ab4;
  --accent: #5b5bd6;
  --accent-soft: #ededff;
  --success: #16a34a;
  --warning: #d97706;
  --error: #dc2626;
  --selected-bg: #f0f0ff;
  --selected-border: #5b5bd6;
  --shadow-card: 0 1px 2px rgba(27, 27, 47, 0.05);
}
@media (prefers-color-scheme: dark) {
  :root {
    color-scheme: dark;
    --page-bg: linear-gradient(160deg, #101226 0%, #171936 45%, #0f1c2e 100%);
    --bg-surface: #1e1f38;
    --bg-elevated: #2a2b4a;
    --border: #353760;
    --border-strong: #454780;
    --text-primary: #f1f1fb;
    --text-secondary: #a5a5c6;
    --text-tertiary: #71729a;
    --accent: #8b8bfa;
    --accent-soft: rgba(139, 139, 250, 0.16);
    --success: #4ade80;
    --warning: #fbbf24;
    --error: #f87171;
    --selected-bg: rgba(139, 139, 250, 0.14);
    --selected-border: #8b8bfa;
    --shadow-card: 0 1px 2px rgba(0, 0, 0, 0.25);
  }
}
* { box-sizing: border-box; margin: 0; padding: 0; }
[hidden] { display: none !important; }
body {
  font-family: system-ui, -apple-system, BlinkMacSystemFont, sans-serif;
  background: var(--page-bg);
  color: var(--text-primary);
  line-height: 1.5;
  min-height: 100vh;
  padding: 1.5rem;
}
code { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 0.9em; }
h3 { font-size: 0.8rem; font-weight: 600; margin-bottom: 0.25rem; }
.page-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 0.75rem;
  flex-wrap: wrap;
  background: var(--bg-surface);
  border: 1.5px solid var(--border);
  border-radius: 14px;
  padding: 0.5rem 1rem;
  margin-bottom: 0.75rem;
  box-shadow: var(--shadow-card);
  font-size: 0.9rem;
}
.header-meta { color: var(--text-secondary); }
.header-stale { color: var(--warning); }
.filter-row {
  display: flex;
  gap: 0.4rem;
  align-items: center;
  flex-wrap: wrap;
  margin-bottom: 0.75rem;
}
.filter {
  font-size: 0.8rem;
  color: var(--text-secondary);
  display: inline-flex;
  gap: 0.25rem;
  align-items: center;
}
.filter select, .filter input[type="search"] {
  border: 1px solid var(--border-strong);
  background: var(--bg-surface);
  color: var(--text-primary);
  border-radius: 4px;
  padding: 2px 8px;
  font: inherit;
}
.view-switch {
  border: 1px solid var(--border-strong);
  background: var(--bg-surface);
  color: var(--text-secondary);
  border-radius: 4px;
  padding: 2px 8px;
  font: inherit;
  cursor: pointer;
}
.view-switch.selected { border-color: var(--accent); color: var(--accent); }
.spacer { flex: 1; }
.progress-row {
  display: flex;
  gap: 0.5rem;
  align-items: center;
  margin-bottom: 0.75rem;
}
.progress-track {
  flex: 1;
  height: 6px;
  background: var(--bg-elevated);
  border-radius: 3px;
  overflow: hidden;
}
.progress-fill { display: block; height: 100%; background: var(--success); }
.counters { color: var(--text-secondary); font-size: 0.8rem; white-space: nowrap; }
.content { display: flex; gap: 0.75rem; align-items: flex-start; }
.board-column { flex: 2.1; min-width: 0; display: flex; flex-direction: column; gap: 0.5rem; }
.board-group {
  background: var(--bg-surface);
  border: 1px solid var(--border);
  border-radius: 6px;
  padding: 0.5rem;
}
.board-group summary {
  display: flex;
  gap: 0.5rem;
  justify-content: space-between;
  color: var(--text-secondary);
  cursor: pointer;
  font-size: 0.85rem;
}
.group-name { font-weight: 600; }
.group-statuses { color: var(--text-tertiary); }
.chips { display: flex; gap: 5px; flex-wrap: wrap; margin-top: 0.4rem; }
.chip {
  border: 1px solid var(--border-strong);
  background: var(--bg-surface);
  color: var(--text-secondary);
  border-radius: 10px;
  padding: 1px 8px;
  font: inherit;
  font-size: 0.75rem;
  cursor: pointer;
}
.chip.status-verified, .chip.status-matching { border-color: var(--success); color: var(--success); }
.chip.status-rebuilt { border-color: var(--error); color: var(--error); }
.chip.status-unknown, .chip.status-not_a_function { color: var(--text-tertiary); }
.chip.stale { border-color: var(--warning); color: var(--warning); }
.chip.selected, .record-row.selected {
  background: var(--selected-bg);
  border-color: var(--selected-border);
}
.chip-address { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; }
.chip-status { opacity: 0.75; }
.legend { color: var(--text-tertiary); font-size: 0.75rem; padding-left: 4px; }
.table-view {
  background: var(--bg-surface);
  border: 1px solid var(--border);
  border-radius: 6px;
  overflow: hidden;
}
.status-table { width: 100%; border-collapse: collapse; font-size: 0.8rem; }
.status-table th, .status-table td {
  text-align: left;
  padding: 4px 8px;
  border-bottom: 1px solid var(--border);
}
.status-table th {
  color: var(--text-secondary);
  font-weight: 600;
  background: var(--bg-elevated);
}
.status-table tbody tr:last-child td { border-bottom: none; }
.record-row { cursor: pointer; }
.cell-address { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; }
.cell-lang { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; color: var(--text-secondary); }
.chip-lang { color: var(--text-secondary); font-size: 0.72rem; letter-spacing: 0.04em; }
.detail-pane {
  flex: 1;
  min-width: 0;
  background: var(--bg-surface);
  border: 1px solid var(--border);
  border-radius: 6px;
  padding: 0.6rem;
  font-size: 0.8rem;
  position: sticky;
  top: 0.75rem;
}
.detail-empty { color: var(--text-tertiary); }
.detail-head {
  display: flex;
  justify-content: space-between;
  align-items: baseline;
  gap: 0.5rem;
}
.detail-name { font-size: 1rem; font-weight: 600; }
.detail-address { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; color: var(--text-secondary); }
.detail-role, .detail-line, .detail-percent, .detail-meta, .detail-command { margin-top: 0.4rem; }
.detail-meta { color: var(--text-secondary); }
.detail-evidence ul, .detail-notes ul { list-style: none; }
.detail-evidence li, .detail-notes li { color: var(--text-secondary); }
.detail-command { color: var(--text-tertiary); }
.status-word { font-weight: 600; }
.status-word.status-verified, .status-word.status-matching { color: var(--success); }
.status-word.status-rebuilt { color: var(--error); }
.status-word.status-named { color: var(--text-secondary); }
.status-word.status-unknown, .status-word.status-not_a_function { color: var(--text-tertiary); }
.stale-mark { color: var(--warning); }
.evidence-kind { color: var(--text-tertiary); }
.map-view {
  background: var(--bg-surface);
  border: 1px solid var(--border);
  border-radius: 6px;
  padding: 0.5rem;
}
.map-axis { position: relative; height: 1.1rem; margin: 0 0 0.35rem; }
.map-tick {
  position: absolute;
  top: 0;
  transform: translateX(-50%);
  color: var(--text-tertiary);
  font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-size: 0.65rem;
  white-space: nowrap;
}
.map-tick:first-child { transform: none; }
.map-tick:last-child { transform: translateX(-100%); }
.map-group { padding: 0.4rem 0.5rem; }
.map-track {
  position: relative;
  height: 1.15rem;
  margin-top: 0.35rem;
  background: var(--bg-elevated);
  border-radius: 3px;
  overflow: hidden;
}
.map-block {
  position: absolute;
  top: 0;
  height: 100%;
  min-width: 2px;
  border-radius: 2px;
  border: 1px solid var(--border-strong);
  background: var(--bg-surface);
  color: var(--text-secondary);
  font-size: 0.6rem;
  line-height: 1.1rem;
  padding: 0;
  text-align: left;
  overflow: hidden;
  white-space: nowrap;
  cursor: pointer;
}
.map-block-label {
  display: block;
  padding: 0 3px;
  overflow: hidden;
  text-overflow: ellipsis;
}
.map-block.status-verified, .map-block.status-matching {
  background: rgba(22, 163, 74, 0.22);
  border-color: var(--success);
}
.map-block.status-rebuilt {
  background: rgba(220, 38, 38, 0.18);
  border-color: var(--error);
}
.map-block.status-unknown, .map-block.status-not_a_function {
  background: var(--bg-elevated);
  border-color: var(--border);
  color: var(--text-tertiary);
}
.map-block.stale {
  border-color: var(--warning);
  outline: 1px solid var(--warning);
}
.map-block.selected {
  background: var(--selected-bg);
  border-color: var(--selected-border);
}
.map-note { color: var(--text-tertiary); font-size: 0.75rem; margin-top: 0.5rem; padding-left: 4px; }
.treemap-view {
  background: var(--bg-surface);
  border: 1px solid var(--border);
  border-radius: 6px;
  padding: 0.5rem;
}
.treemap-canvas {
  position: relative;
  width: 100%;
  aspect-ratio: 1000 / 620;
  background: var(--bg-elevated);
  border-radius: 4px;
  overflow: hidden;
}
.treemap-group {
  position: absolute;
  border: 1px solid var(--border-strong);
  border-radius: 3px;
  background: var(--bg-surface);
  overflow: hidden;
}
.treemap-group-label {
  position: absolute;
  top: 1px;
  left: 4px;
  right: 4px;
  color: var(--text-secondary);
  font-size: 0.6rem;
  font-weight: 600;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  z-index: 2;
  pointer-events: none;
}
.treemap-block {
  position: absolute;
  border-radius: 2px;
  border: 1px solid var(--border);
  background: var(--bg-elevated);
  color: var(--text-tertiary);
  padding: 0;
  overflow: hidden;
  white-space: nowrap;
  text-align: left;
  font-size: 0.55rem;
  line-height: 1.1;
  cursor: pointer;
}
.treemap-label {
  display: block;
  padding: 1px 2px;
  overflow: hidden;
  text-overflow: ellipsis;
}
.treemap-block.status-verified, .treemap-block.status-matching {
  background: rgba(22, 163, 74, 0.22);
  border-color: var(--success);
  color: var(--text-primary);
}
.treemap-block.status-rebuilt {
  background: rgba(220, 38, 38, 0.18);
  border-color: var(--error);
  color: var(--text-primary);
}
.treemap-block.stale {
  border-color: var(--warning);
  outline: 1px solid var(--warning);
}
.treemap-block.selected {
  background: var(--selected-bg);
  border-color: var(--selected-border);
}
.graph-view {
  background: var(--bg-surface);
  border: 1px solid var(--border);
  border-radius: 6px;
  padding: 0.5rem;
  overflow-x: auto;
}
.graph-canvas { min-width: 720px; }
.graph-canvas svg { display: block; height: auto; }
.graph-edge {
  fill: none;
  stroke: var(--text-tertiary);
  stroke-width: 1;
  opacity: 0.55;
}
.graph-arrow { fill: var(--text-tertiary); }
.graph-node { cursor: pointer; }
.graph-node rect {
  fill: var(--bg-surface);
  stroke: var(--border-strong);
  stroke-width: 1;
}
.graph-node.status-verified rect, .graph-node.status-matching rect {
  fill: rgba(22, 163, 74, 0.18);
  stroke: var(--success);
}
.graph-node.status-rebuilt rect {
  fill: rgba(220, 38, 38, 0.16);
  stroke: var(--error);
}
.graph-node.status-unknown rect, .graph-node.status-not_a_function rect {
  fill: var(--bg-elevated);
  stroke: var(--border);
}
.graph-node.stale rect { stroke: var(--warning); stroke-width: 2; }
.graph-node.selected rect { stroke: var(--selected-border); stroke-width: 2.5; }
.graph-label { fill: var(--text-primary); font-size: 10px; font-weight: 600; }
.graph-address {
  fill: var(--text-tertiary);
  font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-size: 8px;
}
@media (max-width: 760px) {
  .content { flex-direction: column; align-items: stretch; }
  .board-column, .detail-pane { width: 100%; }
  .detail-pane { position: static; }
  .progress-row { flex-wrap: wrap; }
  .counters { white-space: normal; }
}
"""

SCRIPT = """\
const state = { view: "board", selected: null };
const TARGETS = ".chip, .record-row, .map-block, .treemap-block, .graph-node";

function showDetail(key) {
  state.selected = key;
  for (const detail of document.querySelectorAll(".record-detail")) {
    detail.hidden = detail.dataset.key !== key;
  }
  document.getElementById("detail-empty").hidden = key !== null;
  for (const target of document.querySelectorAll(TARGETS)) {
    const selected = target.dataset.key === key;
    target.classList.toggle("selected", selected);
    if (selected) {
      target.setAttribute("aria-current", "true");
    } else {
      target.removeAttribute("aria-current");
    }
  }
}

function applyFilters() {
  const status = document.getElementById("status-filter").value;
  const module = document.getElementById("module-filter").value;
  const search = document.getElementById("search-filter").value.trim().toLowerCase();
  const staleOnly = document.getElementById("stale-filter").checked;
  const counts = {};
  const seen = new Map();
  let visible = 0;
  let verified = 0;
  for (const target of document.querySelectorAll(TARGETS)) {
    const name = target.dataset.name.toLowerCase();
    const match = (status === "" || target.dataset.status === status)
      && (module === "" || target.dataset.module === module)
      && (search === "" || name.includes(search)
        || target.dataset.address.toLowerCase().includes(search))
      && (!staleOnly || target.dataset.stale === "true");
    target.hidden = !match;
    if (match && !seen.has(target.dataset.key)) {
      seen.set(target.dataset.key, target.dataset);
    }
  }
  for (const data of seen.values()) {
    if (data.status !== "not_a_function") {
      visible += 1;
      if (data.status === "verified") {
        verified += 1;
      }
    }
    counts[data.status] = (counts[data.status] || 0) + 1;
  }
  for (const counter of document.querySelectorAll(".counter")) {
    const count = counts[counter.dataset.status] || 0;
    counter.textContent = count + " " + counter.dataset.status;
  }
  document.getElementById("progress-fill").style.width =
    visible === 0 ? "0%" : Math.round((100 * verified) / visible) + "%";
  updateGraphEdges();
}

function updateGraphEdges() {
  const visible = new Set();
  for (const node of document.querySelectorAll(".graph-node")) {
    if (!node.hidden) {
      visible.add(node.dataset.key);
    }
  }
  for (const edge of document.querySelectorAll(".graph-edge")) {
    edge.hidden =
      !(visible.has(edge.dataset.from) && visible.has(edge.dataset.to));
  }
}

function setView(view) {
  state.view = view;
  for (const name of ["board", "table", "map", "treemap", "calls"]) {
    document.getElementById(name).hidden = view !== name;
    const button = document.getElementById("view-" + name);
    button.classList.toggle("selected", view === name);
    button.setAttribute("aria-pressed", String(view === name));
  }
  applyFilters();
  showDetail(state.selected);
}

document.addEventListener("click", (event) => {
  const target = event.target.closest(TARGETS);
  if (target) {
    showDetail(target.dataset.key);
  }
});

document.addEventListener("keydown", (event) => {
  if (event.key !== "Enter" && event.key !== " ") {
    return;
  }
  const target = event.target.closest(".graph-node");
  if (target) {
    event.preventDefault();
    showDetail(target.dataset.key);
  }
});

document.getElementById("view-board").addEventListener("click", () => setView("board"));
document.getElementById("view-table").addEventListener("click", () => setView("table"));
document.getElementById("view-map").addEventListener("click", () => setView("map"));
document.getElementById("view-treemap").addEventListener("click", () => setView("treemap"));
document.getElementById("view-calls").addEventListener("click", () => setView("calls"));
document.getElementById("status-filter").addEventListener("input", applyFilters);
document.getElementById("module-filter").addEventListener("input", applyFilters);
document.getElementById("search-filter").addEventListener("input", applyFilters);
document.getElementById("stale-filter").addEventListener("input", applyFilters);
applyFilters();
"""


def _escaped(value: object) -> str:
    return html.escape(str(value), quote=True)


def _identity(record: dict) -> dict | None:
    result = record.get("result")
    if not result:
        return None
    return result.get("checks", {}).get("identity")


def _percent(record: dict, equal_key: str, total_key: str) -> str:
    identity = _identity(record)
    if identity is None:
        return UNDEFINED
    total = identity.get(total_key, 0)
    if not total:
        return UNDEFINED
    return f"{round(100 * identity.get(equal_key, 0) / total)}%"


def instruction_percent(record: dict) -> str:
    """The instruction match of the last identity result, or a dash."""
    return _percent(record, "instructions_equal", "instructions_total")


def byte_percent(record: dict) -> str:
    """The byte match of the last identity result, or a dash."""
    return _percent(record, "bytes_equal", "bytes_total")


def module_rows(records: list[dict], modules: list[dict]) -> list[dict]:
    """The module records that hold no function record of the store."""
    used = {record.get("module") for record in records}
    return [module for module in modules if module["name"] not in used]


def _row_module(row: dict) -> str | None:
    if "address" in row:
        return row.get("module")
    return row.get("name")


def _is_stale(row: dict, digests: dict[str, str]) -> bool:
    # The page marks a record whose module sources changed, because it must
    # render without the game image.  The runner uses the full input fingerprint.
    return store.source_is_stale(row, digests.get(_row_module(row)))


def _sorted_records(records: list[dict]) -> list[dict]:
    return sorted(records, key=lambda record: int(record["address"], 16))


def _status_counts(rows: list[dict]) -> dict[str, int]:
    counts = {status: 0 for status in store.STATUS_LADDER}
    for row in rows:
        status = row.get("status", "unknown")
        if status in counts:
            counts[status] += 1
    return counts


def _status_summary(rows: list[dict]) -> str:
    counts = _status_counts(rows)
    return " · ".join(
        f"{counts[status]} {status}"
        for status in reversed(store.STATUS_LADDER)
        if counts[status]
    )


def _counters(rows: list[dict]) -> str:
    counts = _status_counts(rows)
    return " · ".join(
        f'<span class="counter counter-{status}" data-status="{status}">'
        f"{counts[status]} {status}</span>"
        for status in reversed(store.STATUS_LADDER)
    )


def _counted_rows(rows: list[dict]) -> list[dict]:
    """The rows that count toward the progress total."""
    return [row for row in rows if row.get("status") != "not_a_function"]


def _progress_width(rows: list[dict]) -> str:
    counted = _counted_rows(rows)
    if not counted:
        return "0%"
    verified = sum(1 for row in counted if row.get("status") == "verified")
    return f"{round(100 * verified / len(counted))}%"


def _last_run(rows: list[dict]) -> str:
    dates = [
        row["result"].get("date", "")
        for row in rows
        if row.get("result")
    ]
    dates = [value for value in dates if value]
    return max(dates) if dates else UNDEFINED


def _status_word(status: str) -> str:
    return (
        f'<span class="status-word status-{_escaped(status)}">'
        f"{_escaped(status)}</span>"
    )


def _stale_mark() -> str:
    return f'<span class="stale-mark">{STALE_MARK} stale</span>'


def _mark(status: str, stale: bool) -> str:
    if stale:
        return STALE_MARK
    if status in ("verified", "matching"):
        return PASS_MARK
    return ""


def _chip(
    key: str,
    label: str,
    name: str,
    address: str,
    module: str,
    status: str,
    stale: bool,
    lang: str = "",
) -> str:
    classes = f"chip status-{status}" + (" stale" if stale else "")
    mark = _mark(status, stale)
    mark_html = f' <span class="chip-mark">{mark}</span>' if mark else ""
    lang_html = f' <span class="chip-lang">{_escaped(lang)}</span>' if lang else ""
    return (
        f'<button class="{classes}" type="button"'
        f' data-key="{_escaped(key)}" data-address="{_escaped(address)}"'
        f' data-name="{_escaped(name)}" data-module="{_escaped(module)}"'
        f' data-status="{_escaped(status)}"'
        f' data-stale="{str(stale).lower()}">'
        f'<span class="chip-label">{_escaped(label)}</span>'
        f' <span class="chip-status">{_escaped(status)}</span>{mark_html}'
        f"{lang_html}</button>"
    )


def _function_chip(record: dict, stale: bool) -> str:
    address = record["address"]
    name = record.get("name")
    label = f"{address} {name}" if name else f"{address} {MISSING_NAME}"
    return _chip(
        key=address,
        label=label,
        name=name or "",
        address=address,
        module=record.get("module", ""),
        status=record.get("status", "unknown"),
        stale=stale,
        lang=_language_label(record),
    )


def _module_chip(module: dict, stale: bool) -> str:
    name = module["name"]
    return _chip(
        key=name,
        label=name,
        name=name,
        address="",
        module=name,
        status=module.get("status", "unknown"),
        stale=stale,
    )


def _group(name: str, records: list[dict], digests: dict[str, str]) -> str:
    chips = "".join(
        _function_chip(record, _is_stale(record, digests)) for record in records
    )
    count_text = "1 function" if len(records) == 1 else f"{len(records)} functions"
    return (
        f'<details class="board-group" data-group="{_escaped(name)}" open>'
        f'<summary><span class="group-name">{_escaped(name)}</span>'
        f'<span class="group-count">{count_text}</span>'
        f'<span class="group-statuses">{_status_summary(records)}</span></summary>'
        f'<div class="chips">{chips}</div></details>'
    )


def _module_group(rows: list[dict], digests: dict[str, str]) -> str:
    chips = "".join(
        _module_chip(module, _is_stale(module, digests)) for module in rows
    )
    count_text = "1 module" if len(rows) == 1 else f"{len(rows)} modules"
    return (
        f'<details class="board-group" data-group="{MODULE_GROUP}" open>'
        f'<summary><span class="group-name">{MODULE_GROUP}</span>'
        f'<span class="group-count">{count_text}</span>'
        f'<span class="group-statuses">{_status_summary(rows)}</span></summary>'
        f'<div class="chips">{chips}</div></details>'
    )


def _board(records: list[dict], rows: list[dict], digests: dict[str, str]) -> str:
    groups = [
        _group(
            section,
            [record for record in records if record.get("section") == section],
            digests,
        )
        for section in docs.TABLE_SECTIONS
    ]
    groups.append(_module_group(rows, digests))
    unmapped = [
        record
        for record in records
        if record.get("section") not in docs.TABLE_SECTIONS
    ]
    groups.append(_group(UNMAPPED_GROUP, unmapped, digests))
    groups.append(
        '<p class="legend">legend: '
        f"{PASS_MARK} verified or matching · {STALE_MARK} stale · "
        "red = rebuilt with a difference · grey = unknown</p>"
    )
    return "\n".join(groups)


def _language_label(record: dict) -> str:
    """The short language label of a record, or an empty string."""
    implementation = record.get("implementation")
    if implementation == "assembly":
        return "asm"
    if implementation == "c":
        return "C"
    return ""


def _table_row(record: dict, stale: bool) -> str:
    address = record["address"]
    name = record.get("name")
    module = record.get("module", "")
    status = record.get("status", "unknown")
    lang = _language_label(record)
    classes = f"record-row status-{status}" + (" stale" if stale else "")
    mark = f" {_stale_mark()}" if stale else ""
    return (
        f'<tr class="{classes}" data-key="{_escaped(address)}"'
        f' data-address="{_escaped(address)}" data-name="{_escaped(name or "")}"'
        f' data-module="{_escaped(module)}" data-status="{_escaped(status)}"'
        f' data-stale="{str(stale).lower()}">'
        f'<td class="cell-address">{_escaped(address)}</td>'
        f'<td class="cell-name">{_escaped(name) if name else MISSING_NAME}</td>'
        f'<td class="cell-lang">{_escaped(lang) if lang else UNDEFINED}</td>'
        f'<td class="cell-module">{_escaped(module)}</td>'
        f'<td class="cell-status">{_status_word(status)}{mark}</td>'
        f'<td class="cell-instructions">{instruction_percent(record)}</td>'
        f'<td class="cell-bytes">{byte_percent(record)}</td></tr>'
    )


def _module_table_row(module: dict, stale: bool) -> str:
    name = module["name"]
    status = module.get("status", "unknown")
    classes = f"record-row status-{status}" + (" stale" if stale else "")
    mark = f" {_stale_mark()}" if stale else ""
    return (
        f'<tr class="{classes}" data-key="{_escaped(name)}" data-address=""'
        f' data-name="{_escaped(name)}" data-module="{_escaped(name)}"'
        f' data-status="{_escaped(status)}" data-stale="{str(stale).lower()}">'
        f'<td class="cell-address">{UNDEFINED}</td>'
        f'<td class="cell-name">{_escaped(name)}</td>'
        f'<td class="cell-lang">{UNDEFINED}</td>'
        f'<td class="cell-module">{_escaped(name)}</td>'
        f'<td class="cell-status">{_status_word(status)}{mark}</td>'
        f'<td class="cell-instructions">{UNDEFINED}</td>'
        f'<td class="cell-bytes">{UNDEFINED}</td></tr>'
    )


def _table(records: list[dict], rows: list[dict], digests: dict[str, str]) -> str:
    body = "".join(
        _table_row(record, _is_stale(record, digests)) for record in records
    )
    body += "".join(
        _module_table_row(module, _is_stale(module, digests)) for module in rows
    )
    head = (
        "<tr><th>address</th><th>name</th><th>lang</th><th>module</th>"
        "<th>status</th><th>instructions</th><th>bytes</th></tr>"
    )
    return (
        f'<table class="status-table"><thead>{head}</thead>'
        f"<tbody>{body}</tbody></table>"
    )


def _map_sizes(records: list[dict]) -> tuple[dict[str, int], int, int]:
    """The byte span of each record, the low address, and the total span.

    A record reaches the next record, because the stored `size` is that
    distance.  The last record reaches its own `size`, or one byte.
    """
    ordered = _sorted_records(records)
    sizes: dict[str, int] = {}
    for index, record in enumerate(ordered):
        address = int(record["address"], 16)
        if index + 1 < len(ordered):
            end = int(ordered[index + 1]["address"], 16)
        else:
            end = address + max(record.get("size") or 0, 1)
        sizes[record["address"]] = max(end - address, 1)
    low = int(ordered[0]["address"], 16)
    high = max(
        int(record["address"], 16) + sizes[record["address"]]
        for record in ordered
    )
    return sizes, low, max(high - low, 1)


def _map_block(
    record: dict, size: int, stale: bool, low: int, span: int
) -> str:
    address = int(record["address"], 16)
    name = record.get("name")
    status = record.get("status", "unknown")
    left = 100 * (address - low) / span
    width = max(100 * size / span, MAP_MIN_WIDTH)
    left = min(left, 100 - width)
    classes = f"map-block status-{status}" + (" stale" if stale else "")
    label = (
        f"{record['address']} {name}"
        if name
        else f"{record['address']} {MISSING_NAME}"
    )
    title = f"{label} · {status} · {size} bytes"
    if stale:
        title += f" · {STALE_MARK} stale"
    return (
        f'<button class="{classes}" type="button"'
        f' data-key="{_escaped(record["address"])}"'
        f' data-address="{_escaped(record["address"])}"'
        f' data-name="{_escaped(name or "")}"'
        f' data-module="{_escaped(record.get("module", ""))}"'
        f' data-status="{_escaped(status)}"'
        f' data-stale="{str(stale).lower()}"'
        f' style="left: {left:.3f}%; width: {width:.3f}%"'
        f' title="{_escaped(title)}">'
        f'<span class="map-block-label">{_escaped(label)}</span></button>'
    )


def _map_group(
    name: str,
    records: list[dict],
    sizes: dict[str, int],
    low: int,
    span: int,
    digests: dict[str, str],
) -> str:
    blocks = "".join(
        _map_block(
            record,
            sizes[record["address"]],
            _is_stale(record, digests),
            low,
            span,
        )
        for record in records
    )
    count_text = "1 function" if len(records) == 1 else f"{len(records)} functions"
    return (
        f'<details class="board-group map-group" data-group="{_escaped(name)}" open>'
        f'<summary><span class="group-name">{_escaped(name)}</span>'
        f'<span class="group-count">{count_text}</span>'
        f'<span class="group-statuses">{_status_summary(records)}</span></summary>'
        f'<div class="map-track">{blocks}</div></details>'
    )


def _map_axis(low: int, span: int) -> str:
    ticks = []
    for index in range(MAP_TICK_COUNT + 1):
        fraction = index / MAP_TICK_COUNT
        value = low + round(span * fraction)
        ticks.append(
            f'<span class="map-tick" style="left: {100 * fraction:.1f}%">'
            f"0x{value:x}</span>"
        )
    return f'<div class="map-axis">{"".join(ticks)}</div>'


def _map(records: list[dict], digests: dict[str, str]) -> str:
    """The address-space map: one lane per section, blocks by module address."""
    if not records:
        return '<p class="map-note">no function records</p>'
    sizes, low, span = _map_sizes(records)
    ordered = _sorted_records(records)
    lanes = []
    for section in docs.TABLE_SECTIONS:
        selected = [record for record in ordered if record.get("section") == section]
        if selected:
            lanes.append(
                _map_group(section, selected, sizes, low, span, digests)
            )
    unmapped = [
        record
        for record in ordered
        if record.get("section") not in docs.TABLE_SECTIONS
    ]
    if unmapped:
        lanes.append(
            _map_group(UNMAPPED_GROUP, unmapped, sizes, low, span, digests)
        )
    note = (
        '<p class="map-note">The map places each function by its module '
        "address. A module record has no module address, so it stays in the "
        "board and the table.</p>"
    )
    legend = (
        '<p class="legend">legend: green = verified or matching · '
        "red = rebuilt with a difference · amber outline = stale · "
        "grey = unknown</p>"
    )
    return "\n".join([_map_axis(low, span)] + lanes + [note, legend])


def _treemap_rects(
    items: list[tuple[str, int]],
    x: float,
    y: float,
    width: float,
    height: float,
) -> dict[str, tuple[float, float, float, float]]:
    """Split a rectangle into one rectangle per weighted item.

    The split follows the longer side and divides the item list near half of
    the total weight.  Each item reaches the rectangle that its weight gives.
    """
    if not items or width <= 0 or height <= 0:
        return {}
    if len(items) == 1:
        key, _ = items[0]
        return {key: (x, y, width, height)}
    total = sum(value for _, value in items)
    if total <= 0:
        return {}
    running = 0
    split = 1
    for index, (_, value) in enumerate(items[:-1]):
        running += value
        if running >= total / 2:
            split = index + 1
            break
    left = items[:split]
    right = items[split:]
    left_total = sum(value for _, value in left)
    fraction = left_total / total
    rects: dict[str, tuple[float, float, float, float]] = {}
    if width >= height:
        left_width = width * fraction
        rects.update(_treemap_rects(left, x, y, left_width, height))
        rects.update(
            _treemap_rects(right, x + left_width, y, width - left_width, height)
        )
    else:
        left_height = height * fraction
        rects.update(_treemap_rects(left, x, y, width, left_height))
        rects.update(
            _treemap_rects(right, x, y + left_height, width, height - left_height)
        )
    return rects


def _style_rect(rect: tuple[float, float, float, float]) -> str:
    x, y, width, height = rect
    return (
        f"left: {100 * x / TREEMAP_WIDTH:.3f}%; "
        f"top: {100 * y / TREEMAP_HEIGHT:.3f}%; "
        f"width: {100 * width / TREEMAP_WIDTH:.3f}%; "
        f"height: {100 * height / TREEMAP_HEIGHT:.3f}%"
    )


def _treemap_block(
    record: dict,
    rect: tuple[float, float, float, float],
    stale: bool,
    group_width: float,
    group_height: float,
) -> str:
    x, y, width, height = rect
    address = record["address"]
    name = record.get("name")
    status = record.get("status", "unknown")
    label = f"{address} {name}" if name else f"{address} {MISSING_NAME}"
    title = f"{label} · {status}"
    if stale:
        title += f" · {STALE_MARK} stale"
    classes = f"treemap-block status-{status}" + (" stale" if stale else "")
    return (
        f'<button class="{classes}" type="button"'
        f' data-key="{_escaped(address)}" data-address="{_escaped(address)}"'
        f' data-name="{_escaped(name or "")}"'
        f' data-module="{_escaped(record.get("module", ""))}"'
        f' data-status="{_escaped(status)}"'
        f' data-stale="{str(stale).lower()}"'
        f' style="left: {100 * x / group_width:.3f}%;'
        f" top: {100 * y / group_height:.3f}%;"
        f" width: {100 * width / group_width:.3f}%;"
        f' height: {100 * height / group_height:.3f}%"'
        f' title="{_escaped(title)}">'
        f'<span class="treemap-label">{_escaped(label)}</span></button>'
    )


def _treemap_group(
    name: str,
    records: list[dict],
    sizes: dict[str, int],
    rect: tuple[float, float, float, float],
    digests: dict[str, str],
) -> str:
    _x, _y, width, height = rect
    header = TREEMAP_HEADER if height > TREEMAP_HEADER + 8 else 0
    if width <= 2 * TREEMAP_GAP or height <= header + 2 * TREEMAP_GAP:
        inner_x, inner_y = 0, header
        inner_width = width
        inner_height = max(height - header, 0)
    else:
        inner_x, inner_y = TREEMAP_GAP, header + TREEMAP_GAP
        inner_width = width - 2 * TREEMAP_GAP
        inner_height = height - header - 2 * TREEMAP_GAP
    ordered = sorted(
        ((record["address"], sizes[record["address"]]) for record in records),
        key=lambda item: item[1],
        reverse=True,
    )
    rects = _treemap_rects(
        ordered,
        inner_x,
        inner_y,
        inner_width,
        inner_height,
    )
    blocks = "".join(
        _treemap_block(
            record,
            rects[record["address"]],
            _is_stale(record, digests),
            width,
            height,
        )
        for record in records
        if record["address"] in rects
    )
    count_text = "1 function" if len(records) == 1 else f"{len(records)} functions"
    label = f"{name} · {count_text}"
    return (
        f'<div class="treemap-group" style="{_style_rect(rect)}">'
        f'<span class="treemap-group-label">{_escaped(label)}</span>'
        f"{blocks}</div>"
    )


def _treemap(records: list[dict], digests: dict[str, str]) -> str:
    """The section treemap: the byte weight of each section and function."""
    if not records:
        return '<p class="map-note">no function records</p>'
    sizes, _low, _span = _map_sizes(records)
    ordered = _sorted_records(records)
    sections: list[tuple[str, list[dict]]] = []
    for section in docs.TABLE_SECTIONS:
        selected = [record for record in ordered if record.get("section") == section]
        if selected:
            sections.append((section, selected))
    unmapped = [
        record
        for record in ordered
        if record.get("section") not in docs.TABLE_SECTIONS
    ]
    if unmapped:
        sections.append((UNMAPPED_GROUP, unmapped))
    section_items = sorted(
        (
            (section, sum(sizes[record["address"]] for record in selected))
            for section, selected in sections
        ),
        key=lambda item: item[1],
        reverse=True,
    )
    rects = _treemap_rects(
        section_items, 0, 0, TREEMAP_WIDTH, TREEMAP_HEIGHT
    )
    groups = "".join(
        _treemap_group(section, selected, sizes, rects[section], digests)
        for section, selected in sections
        if section in rects
    )
    note = (
        '<p class="map-note">The treemap sizes each section and function by '
        "the byte span to the next record. The block color is the status.</p>"
    )
    legend = (
        '<p class="legend">legend: green = verified or matching · '
        "red = rebuilt with a difference · amber outline = stale · "
        "grey = unknown</p>"
    )
    return (
        f'<div class="treemap-canvas">{groups}</div>' + note + legend
    )


def _graph_name(record: dict) -> str:
    name = record.get("name") or MISSING_NAME
    if len(name) <= GRAPH_NAME_LIMIT:
        return name
    return name[: GRAPH_NAME_LIMIT - 1] + "…"


def _barycenter(
    neighbors: list[str], positions: dict[str, int]
) -> float:
    values = [positions[neighbor] for neighbor in neighbors if neighbor in positions]
    if not values:
        return float("inf")
    return sum(values) / len(values)


def _graph_ranks(
    addresses: list[str], edges: list[tuple[str, str]]
) -> dict[int, list[str]]:
    """One column per call depth, with the nodes ordered to reduce crossings."""
    adjacency: dict[str, list[str]] = {address: [] for address in addresses}
    predecessors: dict[str, list[str]] = {address: [] for address in addresses}
    indegree = {address: 0 for address in addresses}
    for source, target in edges:
        adjacency[source].append(target)
        predecessors[target].append(source)
        indegree[target] += 1
    roots = [address for address in addresses if indegree[address] == 0]
    depth: dict[str, int] = {address: 0 for address in roots}
    queue = list(roots)
    index = 0
    while index < len(queue):
        current = queue[index]
        index += 1
        for target in adjacency[current]:
            if target not in depth:
                depth[target] = depth[current] + 1
                queue.append(target)
    for address in addresses:
        depth.setdefault(address, 0)
    ranks: dict[int, list[str]] = {}
    for address in addresses:
        ranks.setdefault(depth[address], []).append(address)
    max_depth = max(ranks, default=0)
    for _ in range(2):
        for level in range(1, max_depth + 1):
            parent_positions = {
                address: index
                for index, address in enumerate(ranks.get(level - 1, []))
            }
            ranks.setdefault(level, []).sort(
                key=lambda address: (
                    _barycenter(predecessors[address], parent_positions),
                    int(address, 16),
                )
            )
        for level in range(max_depth - 1, -1, -1):
            child_positions = {
                address: index
                for index, address in enumerate(ranks.get(level + 1, []))
            }
            ranks.setdefault(level, []).sort(
                key=lambda address: (
                    _barycenter(adjacency[address], child_positions),
                    int(address, 16),
                )
            )
    return ranks


def _graph_positions(
    ranks: dict[int, list[str]],
) -> tuple[dict[str, tuple[float, float]], float, float]:
    max_depth = max(ranks, default=0)
    max_rows = max((len(addresses) for addresses in ranks.values()), default=0)
    width = (
        2 * GRAPH_MARGIN
        + (max_depth + 1) * GRAPH_NODE_WIDTH
        + max_depth * GRAPH_COLUMN_GAP
    )
    height = (
        2 * GRAPH_MARGIN
        + max_rows * GRAPH_NODE_HEIGHT
        + max(max_rows - 1, 0) * GRAPH_ROW_GAP
    )
    positions: dict[str, tuple[float, float]] = {}
    for level in sorted(ranks):
        addresses = ranks[level]
        column_height = (
            len(addresses) * GRAPH_NODE_HEIGHT
            + max(len(addresses) - 1, 0) * GRAPH_ROW_GAP
        )
        top = (height - column_height) / 2
        x = GRAPH_MARGIN + level * (GRAPH_NODE_WIDTH + GRAPH_COLUMN_GAP)
        for index, address in enumerate(addresses):
            y = top + index * (GRAPH_NODE_HEIGHT + GRAPH_ROW_GAP)
            positions[address] = (x, y)
    return positions, width, height


def _graph_edge(
    source: str,
    target: str,
    positions: dict[str, tuple[float, float]],
) -> str:
    source_x, source_y = positions[source]
    target_x, target_y = positions[target]
    start_x = source_x + GRAPH_NODE_WIDTH
    start_y = source_y + GRAPH_NODE_HEIGHT / 2
    end_x = target_x
    end_y = target_y + GRAPH_NODE_HEIGHT / 2
    if end_x >= start_x:
        middle = (start_x + end_x) / 2
        path = (
            f"M {start_x:.1f} {start_y:.1f} "
            f"C {middle:.1f} {start_y:.1f}, {middle:.1f} {end_y:.1f}, "
            f"{end_x:.1f} {end_y:.1f}"
        )
    else:
        loop = min(start_y, end_y) - 2 * GRAPH_ROW_GAP
        path = (
            f"M {start_x:.1f} {start_y:.1f} "
            f"C {start_x + 30:.1f} {loop:.1f}, "
            f"{end_x - 30:.1f} {loop:.1f}, {end_x:.1f} {end_y:.1f}"
        )
    return (
        f'<path class="graph-edge" data-from="{_escaped(source)}"'
        f' data-to="{_escaped(target)}" d="{path}"'
        ' marker-end="url(#graph-arrow)"/>'
    )


def _graph_node(
    record: dict,
    position: tuple[float, float],
    stale: bool,
) -> str:
    x, y = position
    address = record["address"]
    status = record.get("status", "unknown")
    name = record.get("name") or MISSING_NAME
    classes = f"graph-node status-{status}" + (" stale" if stale else "")
    title = f"{address} {name} · {status}"
    if stale:
        title += f" · {STALE_MARK} stale"
    return (
        f'<g class="{classes}" data-key="{_escaped(address)}"'
        f' data-address="{_escaped(address)}" data-name="{_escaped(name)}"'
        f' data-module="{_escaped(record.get("module", ""))}"'
        f' data-status="{_escaped(status)}"'
        f' data-stale="{str(stale).lower()}"'
        ' tabindex="0" role="button">'
        f"<title>{_escaped(title)}</title>"
        f'<rect x="{x:.1f}" y="{y:.1f}" width="{GRAPH_NODE_WIDTH}"'
        f' height="{GRAPH_NODE_HEIGHT}" rx="4"/>'
        f'<text class="graph-label" x="{x + 8:.1f}" y="{y + 15:.1f}">'
        f"{_escaped(_graph_name(record))}</text>"
        f'<text class="graph-address" x="{x + 8:.1f}" y="{y + 29:.1f}">'
        f"{_escaped(address)}</text></g>"
    )


def _call_graph(
    records: list[dict], edges: list[dict], digests: dict[str, str]
) -> str:
    """The layered call graph of the named function records."""
    named = [record for record in _sorted_records(records) if record.get("name")]
    if not named:
        return '<p class="map-note">no named function records</p>'
    by_address = {record["address"]: record for record in named}
    pairs = sorted(
        {
            (edge["from"], edge["to"])
            for edge in edges
            if edge["from"] in by_address
            and edge["to"] in by_address
            and edge["from"] != edge["to"]
        }
    )
    addresses = [record["address"] for record in named]
    ranks = _graph_ranks(addresses, pairs)
    positions, width, height = _graph_positions(ranks)
    paths = "".join(
        _graph_edge(source, target, positions) for source, target in pairs
    )
    nodes = "".join(
        _graph_node(record, positions[record["address"]], _is_stale(record, digests))
        for record in named
        if record["address"] in positions
    )
    arrow = (
        '<defs><marker id="graph-arrow" viewBox="0 0 10 10" refX="9" refY="5"'
        ' markerWidth="6" markerHeight="6" orient="auto-start-reverse">'
        '<path class="graph-arrow" d="M0,0 L10,5 L0,10 z"/></marker></defs>'
    )
    note = (
        f'<p class="map-note">The call graph holds {len(named)} named '
        f"functions and {len(pairs)} direct edges. An edge to an unnamed or "
        "missing function is not shown.</p>"
    )
    legend = (
        '<p class="legend">legend: green = verified or matching · '
        "red = rebuilt with a difference · amber outline = stale · "
        "grey = unknown</p>"
    )
    return (
        '<div class="graph-canvas">'
        f'<svg viewBox="0 0 {width:.0f} {height:.0f}"'
        f' width="{width:.0f}" height="{height:.0f}"'
        ' role="group" aria-label="Call graph of the named functions">'
        f"{arrow}{paths}{nodes}</svg></div>" + note + legend
    )


def _count_line(record: dict) -> str:
    identity = _identity(record)
    if identity is None:
        return f"instructions {UNDEFINED} · bytes {UNDEFINED}"
    return (
        f"instructions {identity.get('instructions_equal', 0)}/"
        f"{identity.get('instructions_total', 0)} ({instruction_percent(record)})"
        f" · bytes {identity.get('bytes_equal', 0)}/"
        f"{identity.get('bytes_total', 0)} ({byte_percent(record)})"
    )


def _meta_line(record: dict) -> str:
    checks = record.get("checks", [])
    checks_text = _escaped(", ".join(checks)) if checks else UNDEFINED
    result = record.get("result")
    run_text = _escaped(result.get("date", "")) if result else "no run"
    return f"checks {checks_text} · run {run_text}"


def _evidence_block(record: dict) -> str:
    entries = record.get("evidence", [])
    if not entries:
        return ""
    items = []
    for entry in entries:
        kind = str(entry.get("kind", ""))
        text = _escaped(entry.get("text", ""))
        if kind == "command":
            items.append(f"<li><code>{text}</code></li>")
        else:
            items.append(
                f'<li><span class="evidence-kind">{_escaped(kind)}:</span>'
                f" {text}</li>"
            )
    return (
        f'<div class="detail-evidence"><h3>evidence ({len(entries)})</h3>'
        f"<ul>{''.join(items)}</ul></div>"
    )


def _notes_block(record: dict) -> str:
    notes = record.get("notes", [])
    if not notes:
        return ""
    items = "".join(
        f'<li>{_escaped(note.get("date", ""))}:'
        f' {_escaped(note.get("text", ""))}</li>'
        for note in notes
    )
    return (
        f'<div class="detail-notes"><h3>notes ({len(notes)})</h3>'
        f"<ul>{items}</ul></div>"
    )


def _command_line(command: str) -> str:
    return (
        '<p class="detail-command">test this record: '
        f"<code>{_escaped(command)}</code></p>"
    )


def _function_command(record: dict) -> str:
    name = record.get("name")
    if name:
        return f"python3 tools/rebuild/run.py run --name {name}"
    return f"python3 tools/rebuild/run.py run --function {record['address']}"


def _function_detail(record: dict, stale: bool) -> str:
    address = record["address"]
    name = record.get("name")
    status = record.get("status", "unknown")
    lines = [
        f'<div class="detail-head"><span class="detail-name">'
        f"{_escaped(name) if name else MISSING_NAME}</span>"
        f'<span class="detail-address">{_escaped(address)}</span></div>'
    ]
    role = record.get("role")
    if role:
        lines.append(f'<p class="detail-role">{_escaped(role)}</p>')
    line = _status_word(status)
    module = record.get("module")
    if module:
        line += f' · module <code>{_escaped(module)}</code>'
    if stale:
        line += f" · {_stale_mark()}"
    lines.append(f'<p class="detail-line">{line}</p>')
    language = _language_label(record)
    if language:
        lines.append(
            f'<p class="detail-line">language: <code>{_escaped(language)}</code></p>'
        )
    lines.append(f'<p class="detail-percent">{_count_line(record)}</p>')
    lines.append(f'<p class="detail-meta">{_meta_line(record)}</p>')
    evidence = _evidence_block(record)
    if evidence:
        lines.append(evidence)
    notes = _notes_block(record)
    if notes:
        lines.append(notes)
    lines.append(_command_line(_function_command(record)))
    return "\n".join(lines)


def _module_detail(module: dict, stale: bool) -> str:
    name = module["name"]
    status = module.get("status", "unknown")
    line = _status_word(status)
    line += f" · module <code>{_escaped(name)}</code>"
    if stale:
        line += f" · {_stale_mark()}"
    lines = [
        f'<div class="detail-head"><span class="detail-name">{_escaped(name)}</span>'
        '<span class="detail-address">module</span></div>',
        f'<p class="detail-line">{line}</p>',
        f'<p class="detail-meta">{_meta_line(module)}</p>',
    ]
    evidence = _evidence_block(module)
    if evidence:
        lines.append(evidence)
    notes = _notes_block(module)
    if notes:
        lines.append(notes)
    lines.append(
        _command_line(f"python3 tools/rebuild/run.py run --module {name}")
    )
    return "\n".join(lines)


def _article(key: str, body: str) -> str:
    return (
        f'<article class="record-detail" data-key="{_escaped(key)}" hidden>'
        f"\n{body}\n</article>"
    )


def _details(records: list[dict], rows: list[dict], digests: dict[str, str]) -> str:
    articles = [
        _article(record["address"], _function_detail(record, _is_stale(record, digests)))
        for record in records
    ]
    articles += [
        _article(module["name"], _module_detail(module, _is_stale(module, digests)))
        for module in rows
    ]
    return "\n".join(articles)


def _header(records: list[dict], rows: list[dict], stale_count: int) -> str:
    counted = _counted_rows(records)
    count_text = "1 function" if len(counted) == 1 else f"{len(counted)} functions"
    return (
        '<header class="page-header">'
        f'<span class="header-title"><b>Screamer rebuild</b> · {count_text}</span>'
        f'<span class="header-meta">last run {_last_run(records + rows)} · '
        f'<span class="header-stale">{stale_count} stale</span></span>'
        "</header>"
    )


def _module_names(records: list[dict], rows: list[dict]) -> list[str]:
    names = {record.get("module") for record in records if record.get("module")}
    names.update(module["name"] for module in rows)
    return sorted(names)


def _filter_row(module_names: list[str]) -> str:
    statuses = "".join(
        f'<option value="{status}">{status}</option>'
        for status in reversed(store.STATUS_LADDER)
    )
    modules = "".join(
        f'<option value="{_escaped(name)}">{_escaped(name)}</option>'
        for name in module_names
    )
    return (
        '<div class="filter-row">'
        '<label class="filter">status '
        f'<select id="status-filter"><option value="">all</option>'
        f"{statuses}</select></label>"
        '<label class="filter">module '
        f'<select id="module-filter"><option value="">all</option>'
        f"{modules}</select></label>"
        '<input id="search-filter" class="filter" type="search"'
        ' placeholder="search…">'
        '<label class="filter"><input id="stale-filter" type="checkbox">'
        " stale only</label>"
        '<span class="spacer"></span>'
        '<button id="view-board" class="view-switch selected" type="button"'
        ' aria-pressed="true">board</button>'
        '<button id="view-table" class="view-switch" type="button"'
        ' aria-pressed="false">table</button>'
        '<button id="view-map" class="view-switch" type="button"'
        ' aria-pressed="false">map</button>'
        '<button id="view-treemap" class="view-switch" type="button"'
        ' aria-pressed="false">treemap</button>'
        '<button id="view-calls" class="view-switch" type="button"'
        ' aria-pressed="false">calls</button>'
        "</div>"
    )


def _progress_row(rows: list[dict]) -> str:
    return (
        '<div class="progress-row">'
        '<span class="progress-track">'
        f'<span id="progress-fill" class="progress-fill"'
        f' style="width: {_progress_width(rows)}"></span></span>'
        f'<span id="counters" class="counters">{_counters(rows)}</span>'
        "</div>"
    )


def render_page(
    records: list[dict],
    modules: list[dict],
    digests: dict[str, str],
    edges: list[dict] | None = None,
) -> str:
    """The self-contained HTML page of the store."""
    functions = _sorted_records(records)
    rows = module_rows(functions, modules)
    all_rows = functions + rows
    stale_count = sum(1 for row in all_rows if _is_stale(row, digests))
    edge_list = edges or []
    return "\n".join(
        [
            "<!DOCTYPE html>",
            '<html lang="en">',
            "<head>",
            '<meta charset="utf-8">',
            '<meta name="viewport" content="width=device-width, initial-scale=1">',
            "<title>Screamer rebuild status</title>",
            "<style>",
            STYLE.rstrip("\n"),
            "</style>",
            "</head>",
            "<body>",
            _header(functions, rows, stale_count),
            _filter_row(_module_names(functions, rows)),
            _progress_row(all_rows),
            '<div class="content">',
            '<div class="board-column">',
            f'<div id="board" class="board">{_board(functions, rows, digests)}</div>',
            f'<div id="table" class="table-view" hidden>'
            f"{_table(functions, rows, digests)}</div>",
            f'<div id="map" class="map-view" hidden>'
            f"{_map(functions, digests)}</div>",
            f'<div id="treemap" class="treemap-view" hidden>'
            f"{_treemap(functions, digests)}</div>",
            f'<div id="calls" class="graph-view" hidden>'
            f"{_call_graph(functions, edge_list, digests)}</div>",
            "</div>",
            '<aside id="detail" class="detail-pane">',
            '<p id="detail-empty" class="detail-empty">'
            "select a record to read its detail</p>",
            _details(functions, rows, digests),
            "</aside>",
            "</div>",
            f"<script>{SCRIPT.rstrip()}</script>",
            "</body>",
            "</html>",
            "",
        ]
    )


def command_html(out: Path) -> int:
    records = [store.load_record(address) for address in store.candidate_addresses()]
    module_names = sorted(path.stem for path in store.MODULES_DIR.glob("*.json"))
    modules = [store.load_module(name) for name in module_names]
    digests = {module["name"]: store.source_digest(module) for module in modules}
    try:
        edges = callgraph.load_edges()
    except ValueError as error:
        print(f"warning: {error}", file=sys.stderr)
        edges = []
    page = render_page(records, modules, digests, edges)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(page, encoding="utf-8")
    count = len(records) + len(module_rows(records, modules))
    print(f"{out}: {count} rows")
    return 0


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Render the self-contained status page of the store."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    html_parser = subparsers.add_parser("html", help="write the status page")
    html_parser.add_argument(
        "--out", type=Path, default=DEFAULT_OUT, help="the output path"
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        if arguments.command == "html":
            return command_html(arguments.out)
    except (ValueError, OSError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
