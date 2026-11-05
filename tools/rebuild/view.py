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
"""

SCRIPT = """\
const state = { view: "board", selected: null };

function showDetail(key) {
  state.selected = key;
  for (const detail of document.querySelectorAll(".record-detail")) {
    detail.hidden = detail.dataset.key !== key;
  }
  document.getElementById("detail-empty").hidden = key !== null;
  for (const target of document.querySelectorAll(".chip, .record-row")) {
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
  let visible = 0;
  let verified = 0;
  for (const target of document.querySelectorAll(".chip, .record-row")) {
    const name = target.dataset.name.toLowerCase();
    const match = (status === "" || target.dataset.status === status)
      && (module === "" || target.dataset.module === module)
      && (search === "" || name.includes(search)
        || target.dataset.address.toLowerCase().includes(search))
      && (!staleOnly || target.dataset.stale === "true");
    target.hidden = !match;
    if (!match || !target.classList.contains("record-row")) {
      continue;
    }
    if (target.dataset.status !== "not_a_function") {
      visible += 1;
      if (target.dataset.status === "verified") {
        verified += 1;
      }
    }
    counts[target.dataset.status] = (counts[target.dataset.status] || 0) + 1;
  }
  for (const counter of document.querySelectorAll(".counter")) {
    const count = counts[counter.dataset.status] || 0;
    counter.textContent = count + " " + counter.dataset.status;
  }
  document.getElementById("progress-fill").style.width =
    visible === 0 ? "0%" : Math.round((100 * verified) / visible) + "%";
}

function setView(view) {
  state.view = view;
  document.getElementById("board").hidden = view !== "board";
  document.getElementById("table").hidden = view !== "table";
  document.getElementById("view-board").classList.toggle("selected", view === "board");
  document.getElementById("view-table").classList.toggle("selected", view === "table");
  document.getElementById("view-board").setAttribute("aria-pressed", String(view === "board"));
  document.getElementById("view-table").setAttribute("aria-pressed", String(view === "table"));
  applyFilters();
  showDetail(state.selected);
}

document.addEventListener("click", (event) => {
  const target = event.target.closest(".chip, .record-row");
  if (target) {
    showDetail(target.dataset.key);
  }
});

document.getElementById("view-board").addEventListener("click", () => setView("board"));
document.getElementById("view-table").addEventListener("click", () => setView("table"));
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
) -> str:
    classes = f"chip status-{status}" + (" stale" if stale else "")
    mark = _mark(status, stale)
    mark_html = f' <span class="chip-mark">{mark}</span>' if mark else ""
    return (
        f'<button class="{classes}" type="button"'
        f' data-key="{_escaped(key)}" data-address="{_escaped(address)}"'
        f' data-name="{_escaped(name)}" data-module="{_escaped(module)}"'
        f' data-status="{_escaped(status)}"'
        f' data-stale="{str(stale).lower()}">'
        f'<span class="chip-label">{_escaped(label)}</span>'
        f' <span class="chip-status">{_escaped(status)}</span>{mark_html}</button>'
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


def _table_row(record: dict, stale: bool) -> str:
    address = record["address"]
    name = record.get("name")
    module = record.get("module", "")
    status = record.get("status", "unknown")
    classes = f"record-row status-{status}" + (" stale" if stale else "")
    mark = f" {_stale_mark()}" if stale else ""
    return (
        f'<tr class="{classes}" data-key="{_escaped(address)}"'
        f' data-address="{_escaped(address)}" data-name="{_escaped(name or "")}"'
        f' data-module="{_escaped(module)}" data-status="{_escaped(status)}"'
        f' data-stale="{str(stale).lower()}">'
        f'<td class="cell-address">{_escaped(address)}</td>'
        f'<td class="cell-name">{_escaped(name) if name else MISSING_NAME}</td>'
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
        "<tr><th>address</th><th>name</th><th>module</th>"
        "<th>status</th><th>instructions</th><th>bytes</th></tr>"
    )
    return (
        f'<table class="status-table"><thead>{head}</thead>'
        f"<tbody>{body}</tbody></table>"
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
    records: list[dict], modules: list[dict], digests: dict[str, str]
) -> str:
    """The self-contained HTML page of the store."""
    functions = _sorted_records(records)
    rows = module_rows(functions, modules)
    all_rows = functions + rows
    stale_count = sum(1 for row in all_rows if _is_stale(row, digests))
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
    page = render_page(records, modules, digests)
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
