"""Tests for the three-tool identity cross-check.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests -p 'test_cross_check.py'
"""
from __future__ import annotations

import contextlib
import copy
import io
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

import cross_check  # noqa: E402

ENTRY = 0x300F8
PAGE0_FILE = 0x1A800
PAGE0_MODULE = 0x10000
MAIN = 0x10020
CALLER = 0xD4324
CALL_SITE = 0xD4364
CALL_TARGET = 0x10020

GOOD_VIEW = {
    "entry": {"entry": ENTRY},
    "page0": {"page0_module": PAGE0_MODULE, "page0_file": PAGE0_FILE},
    "main": {"main": MAIN},
    "caller": {
        "caller": CALLER,
        "call_site": CALL_SITE,
        "call_target": CALL_TARGET,
    },
}

R2_TEXT = (
    "paddr      vaddr      phaddr     vhaddr     type\n"
    "------------------------------------------------\n"
    "0x00000000 0x000300f8 ---------- ---------- program\n"
    "nth paddr         size vaddr         vsize perm flags type name\n"
    "---------------------------------------------------------------\n"
    "0   0x0001a800  0x1000 0x00010000   0x1000 -r-x 0x0   ---- obj.1.page.0\n"
    "1   0x0001b800  0x1000 0x00011000   0x1000 -r-x 0x0   ---- obj.1.page.1\n"
    "            ; CALL XREF from fcn.000d4324 @ 0xd4364(x)\n"
    "            ; CALL XREF from fcn.000d5000 @ 0xd4400(x)\n"
    "/ 194: fcn.00010020 ();\n"
    "|           0x00010020      53             push ebx\n"
)

GHIDRA_TEXT = (
    "=== identity\n"
    "INFO  LeIdentity.java> entry=0x300f8\n"
    "INFO  LeIdentity.java> page0_module=0x10000\n"
    "INFO  LeIdentity.java> page0_file=0x1a800\n"
    "INFO  LeIdentity.java> main=0x10020\n"
    "INFO  LeIdentity.java> caller=0xd4324\n"
    "INFO  LeIdentity.java> call_site=0xd4364\n"
    "INFO  LeIdentity.java> call_target=0x10020\n"
)


class FakeImage:
    """The minimal image the fact reader and the command read."""

    def __init__(
        self, mapped: tuple[int, ...] = (PAGE0_MODULE, MAIN, CALL_SITE)
    ) -> None:
        self.entry_linear = ENTRY
        self.mapped = set(mapped)

    def page_at(self, address: int) -> object | None:
        return object() if address in self.mapped else None

    def to_file_offset(self, address: int) -> int | None:
        return PAGE0_FILE if address == PAGE0_MODULE else None


class FakeInstruction:
    def __init__(self, mnemonic: str, operands: str) -> None:
        self.mnemonic = mnemonic
        self.operands = operands


def fact_line(name: str, status: str, values: dict[str, str]) -> str:
    return (
        f"{name}: {status} le={values['le']} r2={values['r2']} "
        f"ghidra={values['ghidra']}"
    )


class StripAnsiTestCase(unittest.TestCase):
    def test_strip_ansi_removes_escape_sequences(self) -> None:
        self.assertEqual(
            cross_check.strip_ansi("\x1b[38;2;1;2;3mred\x1b[0m text"),
            "red text",
        )


class R2ParserTestCase(unittest.TestCase):
    def test_parse_r2_reads_the_entry_and_page_zero(self) -> None:
        facts = cross_check.parse_r2(R2_TEXT, CALL_SITE)
        self.assertEqual(facts["entry"]["entry"], ENTRY)
        self.assertEqual(facts["page0"]["page0_file"], PAGE0_FILE)
        self.assertEqual(facts["page0"]["page0_module"], PAGE0_MODULE)

    def test_parse_r2_reads_the_main_function(self) -> None:
        facts = cross_check.parse_r2(R2_TEXT, CALL_SITE)
        self.assertEqual(facts["main"]["main"], MAIN)

    def test_parse_r2_selects_the_call_xref_at_the_call_site(self) -> None:
        facts = cross_check.parse_r2(R2_TEXT, CALL_SITE)
        self.assertEqual(facts["caller"]["caller"], CALLER)
        self.assertEqual(facts["caller"]["call_site"], CALL_SITE)
        self.assertEqual(facts["caller"]["call_target"], CALL_TARGET)

    def test_parse_r2_marks_the_caller_unknown_for_zero_matches(self) -> None:
        facts = cross_check.parse_r2(R2_TEXT, 0xD9999)
        self.assertIsNone(facts["caller"]["caller"])
        self.assertIsNone(facts["caller"]["call_site"])
        self.assertIsNone(facts["caller"]["call_target"])

    def test_parse_r2_marks_the_caller_unknown_for_several_matches(self) -> None:
        text = R2_TEXT.replace(
            "fcn.000d5000 @ 0xd4400(x)",
            "fcn.000d5000 @ 0xd4364(x)",
        )
        facts = cross_check.parse_r2(text, CALL_SITE)
        self.assertIsNone(facts["caller"]["caller"])
        self.assertIsNone(facts["caller"]["call_site"])
        self.assertIsNone(facts["caller"]["call_target"])

    def test_parse_r2_marks_a_missing_entry_unknown(self) -> None:
        text = R2_TEXT.replace(
            "0x00000000 0x000300f8 ---------- ---------- program\n", ""
        )
        facts = cross_check.parse_r2(text, CALL_SITE)
        self.assertIsNone(facts["entry"]["entry"])


class GhidraParserTestCase(unittest.TestCase):
    def test_parse_ghidra_reads_the_seven_keys(self) -> None:
        facts = cross_check.parse_ghidra(GHIDRA_TEXT)
        self.assertEqual(facts["entry"], ENTRY)
        self.assertEqual(facts["page0_module"], PAGE0_MODULE)
        self.assertEqual(facts["page0_file"], PAGE0_FILE)
        self.assertEqual(facts["main"], MAIN)
        self.assertEqual(facts["caller"], CALLER)
        self.assertEqual(facts["call_site"], CALL_SITE)
        self.assertEqual(facts["call_target"], CALL_TARGET)

    def test_parse_ghidra_maps_unknown_to_none(self) -> None:
        facts = cross_check.parse_ghidra(
            GHIDRA_TEXT.replace("entry=0x300f8", "entry=unknown")
        )
        self.assertIsNone(facts["entry"])

    def test_parse_ghidra_maps_a_missing_key_to_none(self) -> None:
        text = GHIDRA_TEXT.replace(
            "INFO  LeIdentity.java> main=0x10020\n", ""
        )
        facts = cross_check.parse_ghidra(text)
        self.assertIsNone(facts["main"])


class LeFactsTestCase(unittest.TestCase):
    def facts(self, starts: list[int], caller: int | None, target: str) -> dict:
        image = FakeImage()
        with mock.patch.object(
            cross_check.le, "function_starts", return_value=starts
        ), mock.patch.object(
            cross_check.le, "nearest_function_start", return_value=caller
        ), mock.patch.object(
            cross_check.le,
            "decode",
            return_value=[FakeInstruction("call", target)],
        ):
            return cross_check.le_facts(image, MAIN, CALL_SITE)

    def test_le_facts_reads_the_entry_and_page_zero(self) -> None:
        facts = self.facts([MAIN], CALLER, "0x10020")
        self.assertEqual(facts["entry"]["entry"], ENTRY)
        self.assertEqual(facts["page0"]["page0_file"], PAGE0_FILE)
        self.assertEqual(facts["page0"]["page0_module"], PAGE0_MODULE)

    def test_le_facts_takes_the_greatest_start_at_or_below_main(self) -> None:
        facts = self.facts([0x10000, MAIN, 0x10030], CALLER, "0x10020")
        self.assertEqual(facts["main"]["main"], MAIN)

    def test_le_facts_reads_the_caller_and_call_target(self) -> None:
        facts = self.facts([MAIN], CALLER, "0x10020")
        self.assertEqual(facts["caller"]["caller"], CALLER)
        self.assertEqual(facts["caller"]["call_site"], CALL_SITE)
        self.assertEqual(facts["caller"]["call_target"], CALL_TARGET)

    def test_le_facts_marks_an_absent_prologue_unknown(self) -> None:
        facts = self.facts([], None, "0x10020")
        self.assertIsNone(facts["main"]["main"])
        self.assertIsNone(facts["caller"]["caller"])


class CompareTestCase(unittest.TestCase):
    def test_compare_agrees_when_all_three_tools_match(self) -> None:
        facts = cross_check.compare_facts(GOOD_VIEW, GOOD_VIEW, GOOD_VIEW)
        self.assertEqual([fact.status for fact in facts], ["agree"] * 4)

    def test_compare_reports_only_the_differing_fact(self) -> None:
        other = copy.deepcopy(GOOD_VIEW)
        other["main"]["main"] = 0x10030
        facts = cross_check.compare_facts(GOOD_VIEW, GOOD_VIEW, other)
        status = {fact.name: fact.status for fact in facts}
        self.assertEqual(status["main"], "differ")
        self.assertEqual(status["entry"], "agree")
        self.assertEqual(status["page0"], "agree")
        self.assertEqual(status["caller"], "agree")

    def test_compare_treats_an_unknown_field_as_differ(self) -> None:
        other = copy.deepcopy(GOOD_VIEW)
        other["caller"]["call_target"] = None
        facts = cross_check.compare_facts(GOOD_VIEW, other, GOOD_VIEW)
        status = {fact.name: fact.status for fact in facts}
        self.assertEqual(status["caller"], "differ")

    def test_compare_returns_the_four_facts_in_fixed_order(self) -> None:
        facts = cross_check.compare_facts(GOOD_VIEW, GOOD_VIEW, GOOD_VIEW)
        self.assertEqual(
            [fact.name for fact in facts], ["entry", "page0", "main", "caller"]
        )


class RenderTestCase(unittest.TestCase):
    def test_render_prints_the_four_agree_lines(self) -> None:
        facts = cross_check.compare_facts(GOOD_VIEW, GOOD_VIEW, GOOD_VIEW)
        self.assertEqual(
            [cross_check.render_fact(fact) for fact in facts],
            [
                fact_line(
                    "entry",
                    "agree",
                    {"le": "0x300f8", "r2": "0x300f8", "ghidra": "0x300f8"},
                ),
                fact_line(
                    "page0",
                    "agree",
                    {
                        "le": "0x1a800->0x10000",
                        "r2": "0x1a800->0x10000",
                        "ghidra": "0x1a800->0x10000",
                    },
                ),
                fact_line(
                    "main",
                    "agree",
                    {"le": "0x10020", "r2": "0x10020", "ghidra": "0x10020"},
                ),
                fact_line(
                    "caller",
                    "agree",
                    {
                        "le": "0xd4364@0xd4324->0x10020",
                        "r2": "0xd4364@0xd4324->0x10020",
                        "ghidra": "0xd4364@0xd4324->0x10020",
                    },
                ),
            ],
        )

    def test_render_prints_a_differ_line_with_each_tool_value(self) -> None:
        other = copy.deepcopy(GOOD_VIEW)
        other["main"]["main"] = 0x10030
        facts = cross_check.compare_facts(GOOD_VIEW, GOOD_VIEW, other)
        main = next(fact for fact in facts if fact.name == "main")
        line = cross_check.render_fact(main)
        self.assertIn("differ", line)
        self.assertIn("le=0x10020", line)
        self.assertIn("r2=0x10020", line)
        self.assertIn("ghidra=0x10030", line)

    def test_render_prints_unknown_for_an_unresolved_field(self) -> None:
        other = copy.deepcopy(GOOD_VIEW)
        other["caller"]["call_target"] = None
        facts = cross_check.compare_facts(GOOD_VIEW, other, GOOD_VIEW)
        caller = next(fact for fact in facts if fact.name == "caller")
        self.assertIn("unknown", cross_check.render_fact(caller))


class MainTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.work = self.root / "work"
        self.wrapper = self.root / "identity.sh"
        self.wrapper.write_text("#!/bin/sh\n")

    def run_main(self, *extra: str, image: FakeImage | None = None):
        argv = [
            "--original",
            str(self.root / "STARTH.EXE"),
            "--main",
            f"{MAIN:#x}",
            "--call-site",
            f"{CALL_SITE:#x}",
            "--work-root",
            str(self.work),
            "--r2",
            "r2",
            "--ghidra",
            str(self.wrapper),
            *extra,
        ]
        out = io.StringIO()
        err = io.StringIO()
        fake = image if image is not None else FakeImage()
        with mock.patch.object(
            cross_check.le, "load", return_value=fake
        ), mock.patch.object(
            cross_check.le, "capstone_available", return_value=True
        ), mock.patch.object(
            cross_check.shutil, "which", return_value="/usr/bin/r2"
        ):
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                code = cross_check.main(argv)
        return code, out.getvalue(), err.getvalue()

    def test_main_exits_zero_when_every_fact_agrees(self) -> None:
        with mock.patch.object(
            cross_check, "le_facts", return_value=copy.deepcopy(GOOD_VIEW)
        ), mock.patch.object(
            cross_check, "run_radare2", return_value=R2_TEXT
        ), mock.patch.object(
            cross_check, "run_ghidra", return_value=GHIDRA_TEXT
        ):
            code, out, _ = self.run_main()
        self.assertEqual(code, 0)
        self.assertEqual(len(out.strip().splitlines()), 4)

    def test_main_exits_one_on_a_difference(self) -> None:
        different = GHIDRA_TEXT.replace("main=0x10020", "main=0x10030")
        with mock.patch.object(
            cross_check, "le_facts", return_value=copy.deepcopy(GOOD_VIEW)
        ), mock.patch.object(
            cross_check, "run_radare2", return_value=R2_TEXT
        ), mock.patch.object(
            cross_check, "run_ghidra", return_value=different
        ):
            code, out, _ = self.run_main()
        self.assertEqual(code, 1)
        self.assertEqual(len(out.strip().splitlines()), 4)

    def test_main_exits_one_on_a_tool_failure(self) -> None:
        with mock.patch.object(
            cross_check, "le_facts", return_value=copy.deepcopy(GOOD_VIEW)
        ), mock.patch.object(
            cross_check,
            "run_radare2",
            side_effect=subprocess.SubprocessError("radare2 failed"),
        ), mock.patch.object(
            cross_check, "run_ghidra", return_value=GHIDRA_TEXT
        ):
            code, out, _ = self.run_main()
        self.assertEqual(code, 1)
        self.assertEqual(len(out.strip().splitlines()), 4)
        self.assertIn("r2=unknown", out)

    def test_main_prints_four_lines_on_exit_one(self) -> None:
        different = GHIDRA_TEXT.replace("entry=0x300f8", "entry=0x300f0")
        with mock.patch.object(
            cross_check, "le_facts", return_value=copy.deepcopy(GOOD_VIEW)
        ), mock.patch.object(
            cross_check, "run_radare2", return_value=R2_TEXT
        ), mock.patch.object(
            cross_check, "run_ghidra", return_value=different
        ):
            code, out, _ = self.run_main()
        self.assertEqual(code, 1)
        self.assertEqual(len(out.strip().splitlines()), 4)

    def test_main_writes_the_raw_r2_and_ghidra_outputs(self) -> None:
        with mock.patch.object(
            cross_check, "le_facts", return_value=copy.deepcopy(GOOD_VIEW)
        ), mock.patch.object(
            cross_check,
            "run_radare2",
            side_effect=subprocess.SubprocessError("radare2 failed"),
        ), mock.patch.object(
            cross_check, "run_ghidra", return_value=GHIDRA_TEXT
        ):
            self.run_main()
        self.assertTrue((self.work / "cross-check" / "r2.txt").is_file())
        self.assertTrue((self.work / "cross-check" / "ghidra.txt").is_file())

    def test_main_exits_two_on_an_invalid_main_address(self) -> None:
        image = FakeImage(mapped=(CALL_SITE,))
        code, out, err = self.run_main(image=image)
        self.assertEqual(code, 2)
        self.assertIn(f"{MAIN:#x}", err)
        self.assertEqual(out, "")

    def test_main_exits_two_on_an_invalid_call_site(self) -> None:
        image = FakeImage(mapped=(MAIN,))
        code, out, err = self.run_main(image=image)
        self.assertEqual(code, 2)
        self.assertIn(f"{CALL_SITE:#x}", err)
        self.assertEqual(out, "")

    def test_main_exits_two_when_capstone_is_missing(self) -> None:
        out = io.StringIO()
        err = io.StringIO()
        argv = [
            "--original",
            str(self.root / "STARTH.EXE"),
            "--main",
            f"{MAIN:#x}",
            "--call-site",
            f"{CALL_SITE:#x}",
            "--work-root",
            str(self.work),
            "--ghidra",
            str(self.wrapper),
        ]
        with mock.patch.object(
            cross_check.le, "load", return_value=FakeImage()
        ), mock.patch.object(
            cross_check.le, "capstone_available", return_value=False
        ):
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                code = cross_check.main(argv)
        self.assertEqual(code, 2)
        self.assertIn("cross-check", err.getvalue())
        self.assertEqual(out.getvalue(), "")

    def test_main_exits_two_when_r2_is_missing(self) -> None:
        out = io.StringIO()
        err = io.StringIO()
        argv = [
            "--original",
            str(self.root / "STARTH.EXE"),
            "--main",
            f"{MAIN:#x}",
            "--call-site",
            f"{CALL_SITE:#x}",
            "--work-root",
            str(self.work),
            "--ghidra",
            str(self.wrapper),
        ]
        with mock.patch.object(
            cross_check.le, "load", return_value=FakeImage()
        ), mock.patch.object(
            cross_check.le, "capstone_available", return_value=True
        ), mock.patch.object(
            cross_check.shutil, "which", return_value=None
        ):
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                code = cross_check.main(argv)
        self.assertEqual(code, 2)
        self.assertEqual(out.getvalue(), "")

    def test_main_exits_two_when_the_ghidra_wrapper_is_missing(self) -> None:
        out = io.StringIO()
        err = io.StringIO()
        argv = [
            "--original",
            str(self.root / "STARTH.EXE"),
            "--main",
            f"{MAIN:#x}",
            "--call-site",
            f"{CALL_SITE:#x}",
            "--work-root",
            str(self.work),
            "--ghidra",
            str(self.root / "missing.sh"),
        ]
        with mock.patch.object(
            cross_check.le, "load", return_value=FakeImage()
        ), mock.patch.object(
            cross_check.le, "capstone_available", return_value=True
        ), mock.patch.object(
            cross_check.shutil, "which", return_value="/usr/bin/r2"
        ):
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                code = cross_check.main(argv)
        self.assertEqual(code, 2)
        self.assertEqual(out.getvalue(), "")

    def test_main_has_no_caller_option(self) -> None:
        argv = [
            "--original",
            str(self.root / "STARTH.EXE"),
            "--main",
            f"{MAIN:#x}",
            "--call-site",
            f"{CALL_SITE:#x}",
            "--caller",
            f"{CALLER:#x}",
        ]
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit) as raised:
                cross_check.main(argv)
        self.assertNotEqual(raised.exception.code, 0)

    def test_main_derives_the_caller_from_the_call_site(self) -> None:
        with mock.patch.object(
            cross_check, "le_facts", return_value=copy.deepcopy(GOOD_VIEW)
        ) as facts, mock.patch.object(
            cross_check, "run_radare2", return_value=R2_TEXT
        ), mock.patch.object(
            cross_check, "run_ghidra", return_value=GHIDRA_TEXT
        ):
            self.run_main()
        facts.assert_called_once()
        arguments = facts.call_args.args
        self.assertEqual(len(arguments), 3)
        self.assertEqual(arguments[1], MAIN)
        self.assertEqual(arguments[2], CALL_SITE)


if __name__ == "__main__":
    unittest.main()
