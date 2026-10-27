"""Tests for the status view percentages, the page markup, and the inline script.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import re
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import view  # noqa: E402


class ViewTestCase(unittest.TestCase):
    """Build every record in the test, so no store file is needed."""

    def record(self, address: str, **overrides: object) -> dict:
        base: dict = {
            "address": address,
            "status": "unknown",
            "checks": [],
            "evidence": [],
            "notes": [],
        }
        base.update(overrides)
        return base

    def result(self, **counts: object) -> dict:
        identity = {
            "verdict": "pass",
            "detail": "the instruction sequences match",
            "instructions_equal": 3,
            "instructions_total": 3,
            "bytes_equal": 6,
            "bytes_total": 9,
        }
        identity.update(counts)
        return {
            "date": "2026-10-27",
            "command": "python3 tools/rebuild/run.py run --name free",
            "tool": "run.py",
            "source_digest": "sha256:" + "1" * 64,
            "checks": {"identity": identity},
        }

    def module(self, name: str = "driver", **overrides: object) -> dict:
        base: dict = {
            "name": name,
            "exe": f"{name}.exe",
            "sources": [{"file": f"{name}.c", "compiler": "wcc386"}],
            "status": "verified",
            "checks": ["behavior"],
            "result": self.result(),
        }
        base.update(overrides)
        return base

    def page(self) -> str:
        records = [
            self.record(
                "0x300ec",
                name="free",
                status="verified",
                section="Program functions",
                module="free",
                role="forwards a non-null pointer",
                checks=["build", "identity"],
                evidence=[
                    {
                        "kind": "command",
                        "text": "python3 tools/le.py disasm STARTH.EXE 0x300ec",
                    },
                    {"kind": "finding", "text": "The rebuilt free module"},
                ],
                notes=[
                    {
                        "date": "2026-10-24",
                        "text": "the tail jump displacement is a link-time value",
                    }
                ],
                result=self.result(),
            ),
            self.record(
                "0x300f0",
                name="exit_flush",
                status="rebuilt",
                section="Program functions",
                module="free",
                result=self.result(instructions_equal=2, bytes_equal=3),
            ),
        ]
        return view.render_page(records, [], {"free": "sha256:" + "1" * 64})

    def rows(self, page: str) -> list[str]:
        return re.findall(
            r'<tr class="record-row[^"]*"[^>]*>.*?</tr>', page, re.DOTALL
        )

    def chips(self, page: str) -> list[str]:
        return re.findall(
            r'<button class="chip[^"]*"[^>]*>.*?</button>', page, re.DOTALL
        )

    def detail(self, page: str, key: str) -> str:
        match = re.search(
            rf'<article class="record-detail" data-key="{re.escape(key)}"[^>]*>'
            r".*?</article>",
            page,
            re.DOTALL,
        )
        self.assertIsNotNone(match, f"no detail article for {key}")
        return match.group(0)

    def script(self, page: str) -> str:
        match = re.search(r"<script>(.*)</script>", page, re.DOTALL)
        self.assertIsNotNone(match, "the page holds no inline script block")
        return match.group(1)

    def test_instruction_percent(self) -> None:
        self.assertEqual(
            view.instruction_percent(self.record("0x300ec", result=self.result())),
            "100%",
        )
        self.assertEqual(
            view.instruction_percent(
                self.record("0x300f0", result=self.result(instructions_equal=2))
            ),
            "67%",
        )
        self.assertEqual(view.instruction_percent(self.record("0x300f4")), "—")

    def test_byte_percent_zero_total(self) -> None:
        self.assertEqual(
            view.byte_percent(
                self.record(
                    "0x300ec", result=self.result(bytes_equal=0, bytes_total=0)
                )
            ),
            "—",
        )
        self.assertEqual(view.byte_percent(self.record("0x300f0")), "—")

    def test_page_holds_one_row_per_record(self) -> None:
        page = self.page()
        rows = self.rows(page)
        self.assertEqual(len(rows), 2)
        self.assertIn("free", rows[0])
        self.assertIn("100%", rows[0])
        self.assertIn("67%", rows[0])
        self.assertIn("exit_flush", rows[1])
        self.assertIn("67%", rows[1])
        self.assertIn("33%", rows[1])

    def test_page_excludes_not_a_function_from_the_total(self) -> None:
        records = [
            self.record(
                "0x300ec",
                name="free",
                status="verified",
                section="Program functions",
                module="free",
                result=self.result(),
            ),
            self.record(
                "0x300f0",
                status="not_a_function",
                section="Program functions",
                module="free",
            ),
        ]
        page = view.render_page(records, [], {"free": "sha256:" + "1" * 64})
        self.assertIn(
            'id="progress-fill" class="progress-fill" style="width: 100%"',
            page,
        )
        self.assertIn("<b>Screamer rebuild</b> · 1 function</span>", page)
        rows = self.rows(page)
        self.assertEqual(len(rows), 2)
        self.assertIn('data-status="verified"', rows[0])
        self.assertIn(
            '<span class="status-word status-verified">verified</span>', rows[0]
        )
        self.assertIn('data-status="not_a_function"', rows[1])
        self.assertIn(
            '<span class="status-word status-not_a_function">'
            "not_a_function</span>",
            rows[1],
        )
        chips = self.chips(page)
        self.assertEqual(len(chips), 2)
        self.assertIn('data-status="verified"', chips[0])
        self.assertIn('data-status="not_a_function"', chips[1])
        self.assertIn(
            'data-status="not_a_function">1 not_a_function</span>', page
        )

    def test_page_marks_stale(self) -> None:
        changed = self.result()
        changed["source_digest"] = "sha256:" + "2" * 64
        records = [
            self.record(
                "0x300ec",
                name="free",
                status="verified",
                section="Program functions",
                module="free",
                result=self.result(),
            ),
            self.record(
                "0x300f0",
                name="exit_flush",
                status="verified",
                section="Program functions",
                module="free",
                result=changed,
            ),
        ]
        page = view.render_page(records, [], {"free": "sha256:" + "1" * 64})
        rows = self.rows(page)
        self.assertIn('data-stale="false"', rows[0])
        self.assertNotIn("⚠", rows[0])
        self.assertIn('data-stale="true"', rows[1])
        self.assertIn("⚠", rows[1])
        chips = self.chips(page)
        self.assertNotIn("⚠", chips[0])
        self.assertIn("⚠", chips[1])
        self.assertIn("1 stale", page)

    def test_page_has_no_external_reference(self) -> None:
        page = self.page()
        self.assertNotIn("http", page)
        self.assertNotIn("<script src", page)
        self.assertNotIn("<link", page)

    def test_page_holds_the_driver_module_row(self) -> None:
        page = view.render_page(
            [], [self.module("driver")], {"driver": "sha256:" + "1" * 64}
        )
        group = re.search(
            r'<details class="board-group" data-group="Modules".*?</details>',
            page,
            re.DOTALL,
        )
        self.assertIsNotNone(group, "the page holds no Modules group")
        self.assertIn("driver", group.group(0))
        self.assertIn("verified", group.group(0))
        self.assertIn('data-status="verified"', group.group(0))
        detail = self.detail(page, "driver")
        self.assertIn("verified", detail)
        self.assertIn("run.py run --module driver", detail)

    def test_page_filters_and_switch(self) -> None:
        page = self.page()
        script = self.script(page)
        self.assertEqual(page.count("<script>"), 1)
        for control in (
            "status-filter",
            "module-filter",
            "search-filter",
            "stale-filter",
        ):
            self.assertIn(f'id="{control}"', page)
        self.assertIn('id="view-board"', page)
        self.assertIn('id="view-table"', page)
        self.assertIn("function applyFilters", script)
        self.assertIn('document.querySelectorAll(".chip, .record-row")', script)
        self.assertIn('document.querySelectorAll(".counter")', script)
        self.assertIn('getElementById("progress-fill")', script)
        self.assertIn("function setView", script)
        self.assertIn("applyFilters();", script)
        self.assertIn("showDetail(state.selected);", script)

    def test_page_detail_pane_click(self) -> None:
        page = self.page()
        script = self.script(page)
        self.assertIn('id="detail"', page)
        self.assertIn("function showDetail", script)
        self.assertIn('event.target.closest(".chip, .record-row")', script)
        self.assertIn("showDetail(target.dataset.key)", script)
        detail = self.detail(page, "0x300ec")
        self.assertIn("free", detail)
        self.assertIn("forwards a non-null pointer", detail)
        self.assertIn("python3 tools/le.py disasm", detail)
        self.assertIn("The rebuilt free module", detail)
        self.assertIn("the tail jump displacement is a link-time value", detail)
        self.assertIn("run.py run --name free", detail)
        self.assertNotIn("<form", page)
        self.assertNotIn("window.location", script)


if __name__ == "__main__":
    unittest.main()
