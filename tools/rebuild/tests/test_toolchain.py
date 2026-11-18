"""Tests for the toolchain selection tool, with a temporary module store.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import argparse
import contextlib
import hashlib
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import store  # noqa: E402
import toolchain  # noqa: E402


def temporary_store(case: unittest.TestCase) -> None:
    """Rebind the store directories of a test to a temporary tree."""
    case.temporary = tempfile.TemporaryDirectory()
    case.root = Path(case.temporary.name)
    case.saved_modules = store.MODULES_DIR
    case.saved_work = store.WORK_DIR
    case.saved_functions = store.FUNCTIONS_DIR
    store.MODULES_DIR = case.root / "modules"
    store.WORK_DIR = case.root / "work"
    store.FUNCTIONS_DIR = case.root / "functions"
    store.WORK_DIR.mkdir(parents=True, exist_ok=True)


def restore_store(case: unittest.TestCase) -> None:
    store.MODULES_DIR = case.saved_modules
    store.WORK_DIR = case.saved_work
    store.FUNCTIONS_DIR = case.saved_functions
    case.temporary.cleanup()


class StoreTestCase(unittest.TestCase):
    """A test case with a temporary module store and no external tool."""

    def setUp(self) -> None:
        temporary_store(self)

    def tearDown(self) -> None:
        restore_store(self)

    def module(self, **overrides: object) -> dict:
        base: dict = {
            "name": "free",
            "exe": "free.exe",
            "map": "free.map",
            "sources": [{"file": "free.c", "compiler": "wcc386"}],
            "run": {
                "command": "free.exe",
                "output": "free.txt",
                "expect": ["R"],
            },
            "toolchain": "v2",
        }
        base.update(overrides)
        return base

    def write_module(self, module: dict) -> Path:
        path = store.module_path(module["name"])
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(module, indent=2, sort_keys=True) + "\n")
        return path

    def record(self, address: str = "0x100", name: str = "free") -> dict:
        return {
            "address": address,
            "module": name,
            "symbol": f"{name}_",
            "checks": ["identity"],
        }

    def descriptor(self, **overrides: object) -> dict:
        base: dict = {
            "order": 0,
            "toolchain": "v2",
            "label": "existing",
            "sources": [{"file": "free.c", "compiler": "wcc386", "flags": []}],
        }
        base.update(overrides)
        return base

    def row(
        self,
        descriptor: dict,
        match: str | None = "same-shape",
        build: str = "pass",
        order: int | None = None,
        bytes_equal: int | None = 10,
        bytes_total: int | None = 10,
        instructions_equal: int | None = 5,
        instructions_total: int | None = 5,
    ) -> dict:
        if build != "pass":
            match = None
        return {
            "order": descriptor["order"] if order is None else order,
            "toolchain": descriptor["toolchain"],
            "label": descriptor["label"],
            "sources": descriptor["sources"],
            "build": build,
            "verdict": "pass" if build == "pass" else build,
            "match": match,
            "strict": match in ("exact-bytes", "exact-relocations"),
            "detail": "",
            "bytes_equal": bytes_equal,
            "bytes_total": bytes_total,
            "instructions_equal": instructions_equal,
            "instructions_total": instructions_total,
        }

    def evaluator(self, match_for, calls: list[dict]):
        """A fake evaluate that records the descriptor and returns one row."""

        def evaluate(
            module: dict,
            record: dict,
            descriptor: dict,
            original: Path,
            timeout: int | None = None,
        ) -> dict:
            calls.append(descriptor)
            match = match_for(descriptor) if callable(match_for) else match_for
            return self.row(descriptor, match=match)

        return evaluate


class MigrateTestCase(StoreTestCase):
    """The migration runs on a temporary module store, with no external tool."""

    def test_migrate_writes_the_default_toolchain(self) -> None:
        module = self.module()
        module.pop("toolchain")
        self.write_module(module)
        with contextlib.redirect_stdout(io.StringIO()):
            code = toolchain.command_migrate(argparse.Namespace(check=False))
        self.assertEqual(code, 0)
        self.assertEqual(store.load_module("free")["toolchain"], "v2")

    def test_migrate_check_reports_an_unmigrated_module(self) -> None:
        module = self.module()
        module.pop("toolchain")
        path = self.write_module(module)
        before = path.read_text()
        modules = [store.load_module("free")]
        self.assertEqual(toolchain.migrate_modules(modules, check=True), 1)
        self.assertEqual(path.read_text(), before)

    def test_migrate_is_idempotent(self) -> None:
        module = self.module()
        module.pop("toolchain")
        self.write_module(module)
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(
                toolchain.command_migrate(argparse.Namespace(check=False)), 0
            )
        modules = [store.load_module("free")]
        self.assertEqual(toolchain.migrate_modules(modules), 0)


class CandidateTestCase(StoreTestCase):
    """The candidate sets, the candidate sources, and the recipe writer."""

    def test_candidate_sets_keep_the_pinned_order(self) -> None:
        module = self.module(
            sources=[
                {"file": "free.c", "compiler": "wcc386", "flags": ["-ox"]}
            ]
        )
        sets = toolchain.candidate_sets(module)
        self.assertEqual(
            [label for label, _ in sets],
            ["existing", "-of+", "-od", "-of+ -oi", "-of+ -ot", "empty"],
        )
        self.assertEqual(
            [flags for _, flags in sets],
            [None, ("-of+",), ("-od",), ("-of+", "-oi"), ("-of+", "-ot"), ()],
        )

    def test_candidate_sets_label_the_existing_set(self) -> None:
        sets = toolchain.candidate_sets(self.module())
        self.assertEqual(sets[0][0], "existing")

    def test_candidate_sets_drop_a_duplicate_of_the_existing_flags(self) -> None:
        module = self.module(
            sources=[
                {"file": "free.c", "compiler": "wcc386", "flags": ["-of+"]}
            ]
        )
        labels = [label for label, _ in toolchain.candidate_sets(module)]
        self.assertNotIn("-of+", labels[1:])
        self.assertEqual(len(labels), 5)

    def test_candidate_sources_keep_the_source_compilers(self) -> None:
        module = self.module(
            sources=[
                {"file": "free.c", "compiler": "wcc386"},
                {"file": "free.asm", "compiler": "wasm"},
            ]
        )
        sources = toolchain.candidate_sources(module, ("-of+",))
        self.assertEqual(
            [source["compiler"] for source in sources], ["wcc386", "wasm"]
        )

    def test_candidate_sources_keep_the_wasm_flags(self) -> None:
        module = self.module(
            sources=[
                {"file": "free.c", "compiler": "wcc386"},
                {"file": "free.asm", "compiler": "wasm", "flags": ["-d2"]},
            ]
        )
        for flags in (None, ("-of+",), ("-od",)):
            sources = toolchain.candidate_sources(module, flags)
            self.assertEqual(sources[1]["flags"], ["-d2"])

    def test_candidate_sources_apply_one_set_to_every_wcc386_source(self) -> None:
        module = self.module(
            sources=[
                {"file": "free.c", "compiler": "wcc386"},
                {"file": "free_extra.c", "compiler": "wcc386"},
            ]
        )
        sources = toolchain.candidate_sources(module, ("-od",))
        self.assertEqual(
            [source["flags"] for source in sources], [["-od"], ["-od"]]
        )

    def test_write_recipe_keeps_the_source_compilers(self) -> None:
        module = self.module(
            sources=[
                {"file": "free.c", "compiler": "wcc386"},
                {"file": "free.asm", "compiler": "wasm"},
            ]
        )
        descriptor = self.descriptor(
            sources=[
                {"file": "free.c", "compiler": "wcc386", "flags": ["-od"]},
                {"file": "free.asm", "compiler": "wasm", "flags": []},
            ]
        )
        path = self.root / "recipe.json"
        toolchain.write_recipe(module, descriptor, path)
        recipe = json.loads(path.read_text())
        self.assertEqual(
            [source["compiler"] for source in recipe["sources"]],
            ["wcc386", "wasm"],
        )

    def test_write_recipe_replaces_the_wcc386_flags(self) -> None:
        module = self.module(
            sources=[
                {"file": "free.c", "compiler": "wcc386", "flags": ["-of+"]}
            ]
        )
        descriptor = self.descriptor(
            sources=[
                {"file": "free.c", "compiler": "wcc386", "flags": ["-od"]}
            ]
        )
        path = self.root / "recipe.json"
        toolchain.write_recipe(module, descriptor, path)
        recipe = json.loads(path.read_text())
        self.assertEqual(recipe["sources"][0]["flags"], ["-od"])

    def test_write_recipe_keeps_the_wasm_flags(self) -> None:
        module = self.module(
            sources=[
                {"file": "free.c", "compiler": "wcc386"},
                {"file": "free.asm", "compiler": "wasm", "flags": ["-d2"]},
            ]
        )
        descriptor = self.descriptor(
            sources=[
                {"file": "free.c", "compiler": "wcc386", "flags": ["-od"]},
                {"file": "free.asm", "compiler": "wasm", "flags": ["-d2"]},
            ]
        )
        path = self.root / "recipe.json"
        toolchain.write_recipe(module, descriptor, path)
        recipe = json.loads(path.read_text())
        self.assertEqual(recipe["sources"][1]["flags"], ["-d2"])

    def test_write_recipe_keeps_the_run_block(self) -> None:
        module = self.module()
        descriptor = self.descriptor()
        path = self.root / "recipe.json"
        toolchain.write_recipe(module, descriptor, path)
        recipe = json.loads(path.read_text())
        self.assertEqual(recipe["run"], module["run"])
        self.assertEqual(recipe["name"], module["name"])
        self.assertEqual(recipe["exe"], module["exe"])
        self.assertEqual(recipe["map"], module["map"])


class IdentityRecordTestCase(StoreTestCase):
    """The identity record selection."""

    def test_identity_record_chooses_the_lowest_address(self) -> None:
        records = [
            self.record("0x200"),
            self.record("0x100"),
        ]
        chosen = toolchain.identity_record(records, "free")
        self.assertEqual(chosen["address"], "0x100")

    def test_identity_record_is_none_without_an_identity_check(self) -> None:
        record = self.record()
        record["checks"] = ["behavior"]
        self.assertIsNone(toolchain.identity_record([record], "free"))


class RankTestCase(StoreTestCase):
    """The row rank and the winner selection."""

    def test_rank_orders_the_match_values(self) -> None:
        matches = [
            "exact-bytes",
            "exact-relocations",
            "same-shape",
            "same-mnemonics",
            "different",
        ]
        ranks = [
            toolchain.rank(self.row(self.descriptor(order=index), match=match))
            for index, match in enumerate(matches)
        ]
        self.assertEqual([value[0] for value in ranks], [0, 1, 2, 3, 4])

    def test_rank_breaks_a_tie_by_fewer_differing_bytes(self) -> None:
        better = self.row(
            self.descriptor(order=1), bytes_equal=10, bytes_total=10
        )
        worse = self.row(
            self.descriptor(order=0), bytes_equal=8, bytes_total=10
        )
        self.assertLess(toolchain.rank(better), toolchain.rank(worse))

    def test_rank_breaks_a_tie_by_fewer_differing_instructions(self) -> None:
        better = self.row(
            self.descriptor(order=1),
            instructions_equal=5,
            instructions_total=5,
        )
        worse = self.row(
            self.descriptor(order=0),
            instructions_equal=3,
            instructions_total=5,
        )
        self.assertLess(toolchain.rank(better), toolchain.rank(worse))

    def test_rank_breaks_a_tie_by_the_candidate_order(self) -> None:
        first = self.row(self.descriptor(order=0))
        second = self.row(self.descriptor(order=1))
        self.assertLess(toolchain.rank(first), toolchain.rank(second))

    def test_rank_treats_a_missing_count_as_zero(self) -> None:
        missing = self.row(
            self.descriptor(order=1), bytes_equal=None, bytes_total=None
        )
        differing = self.row(
            self.descriptor(order=0), bytes_equal=8, bytes_total=10
        )
        self.assertLess(toolchain.rank(missing), toolchain.rank(differing))

    def test_rank_places_a_failed_build_after_every_identity(self) -> None:
        failed = self.row(self.descriptor(order=0), build="fail")
        different = self.row(self.descriptor(order=1), match="different")
        self.assertEqual(toolchain.rank(failed)[0], toolchain.BUILD_FAIL_RANK)
        self.assertLess(toolchain.rank(different), toolchain.rank(failed))

    def test_winner_prefers_exact_bytes_over_exact_relocations(self) -> None:
        rows = [
            self.row(self.descriptor(order=0), match="exact-relocations"),
            self.row(self.descriptor(order=1), match="exact-bytes"),
        ]
        self.assertEqual(toolchain.winner(rows)["match"], "exact-bytes")

    def test_winner_prefers_a_strict_match_over_a_shape_match(self) -> None:
        rows = [
            self.row(self.descriptor(order=0), match="same-shape"),
            self.row(self.descriptor(order=1), match="exact-relocations"),
        ]
        self.assertEqual(
            toolchain.winner(rows)["match"], "exact-relocations"
        )

    def test_winner_returns_a_shape_row_when_it_is_the_best_measured_row(
        self,
    ) -> None:
        rows = [self.row(self.descriptor(order=0), match="same-shape")]
        self.assertEqual(toolchain.winner(rows)["match"], "same-shape")

    def test_winner_returns_a_different_row_when_it_is_the_only_measured_row(
        self,
    ) -> None:
        rows = [self.row(self.descriptor(order=0), match="different")]
        self.assertEqual(toolchain.winner(rows)["match"], "different")

    def test_winner_is_none_without_a_measured_row(self) -> None:
        rows = [
            self.row(self.descriptor(order=0), build="fail"),
            self.row(self.descriptor(order=1), build="skip"),
        ]
        self.assertIsNone(toolchain.winner(rows))

    def test_row_marks_a_strict_match(self) -> None:
        for match in ("exact-bytes", "exact-relocations"):
            row = toolchain._row(
                self.descriptor(), "pass", verdict="pass", match=match
            )
            self.assertTrue(row["strict"])

    def test_row_marks_a_shape_match_not_strict(self) -> None:
        for match in ("same-shape", "same-mnemonics"):
            row = toolchain._row(
                self.descriptor(), "pass", verdict="pass", match=match
            )
            self.assertFalse(row["strict"])


class StrictTestCase(StoreTestCase):
    """The strict gate of a report row."""

    def test_is_strict_accepts_the_strict_matches(self) -> None:
        for match in ("exact-bytes", "exact-relocations"):
            row = self.row(self.descriptor(), match=match)
            self.assertTrue(toolchain.is_strict(row))
        for match in ("same-shape", "same-mnemonics", "different", None):
            row = self.row(self.descriptor(), match=match)
            self.assertFalse(toolchain.is_strict(row))


class ApplyWinnerTestCase(StoreTestCase):
    """The winner recipe write."""

    def test_apply_winner_removes_period_flags(self) -> None:
        module = self.module(
            sources=[
                {
                    "file": "free.c",
                    "compiler": "wcc386",
                    "flags": ["-of+"],
                    "period_flags": ["-od"],
                }
            ]
        )
        row = {
            "toolchain": "watcom10",
            "sources": [
                {"file": "free.c", "compiler": "wcc386", "flags": ["-od"]}
            ],
        }
        result = toolchain.apply_winner(module, winner=row)
        self.assertNotIn("period_flags", result["sources"][0])
        self.assertEqual(result["sources"][0]["flags"], ["-od"])
        self.assertEqual(result["toolchain"], "watcom10")

    def test_apply_winner_removes_an_empty_flags_field(self) -> None:
        module = self.module(
            sources=[
                {"file": "free.c", "compiler": "wcc386", "flags": ["-of+"]}
            ]
        )
        row = {
            "toolchain": "v2",
            "sources": [
                {"file": "free.c", "compiler": "wcc386", "flags": []}
            ],
        }
        result = toolchain.apply_winner(module, winner=row)
        self.assertNotIn("flags", result["sources"][0])


class DescriptorTestCase(StoreTestCase):
    """The candidate descriptors."""

    def test_candidate_descriptors_hold_the_toolchain_and_sources(self) -> None:
        module = self.module()
        descriptors = toolchain.candidate_descriptors(module, "watcom10")
        self.assertEqual(
            len(descriptors), len(toolchain.candidate_sets(module))
        )
        for index, descriptor in enumerate(descriptors):
            self.assertEqual(descriptor["order"], index)
            self.assertEqual(descriptor["toolchain"], "watcom10")
            self.assertIn("label", descriptor)
            self.assertEqual(
                [source["file"] for source in descriptor["sources"]],
                ["free.c"],
            )

    def test_candidate_descriptors_keep_the_global_order(self) -> None:
        calls: list[dict] = []
        item = toolchain.matrix_module(
            self.module(),
            self.record(),
            Path("original.exe"),
            toolchains=("watcom10", "v2"),
            evaluate=self.evaluator("same-shape", calls),
        )
        self.assertEqual(
            [row["order"] for row in item["rows"]],
            list(range(len(item["rows"]))),
        )
        toolchains = [row["toolchain"] for row in item["rows"]]
        self.assertEqual(toolchains[0], "v2")
        self.assertEqual(toolchains[-1], "watcom10")


class MatrixModuleTestCase(StoreTestCase):
    """The measurement of one module, with a fake evaluate callable."""

    def test_matrix_module_measures_each_selected_candidate(self) -> None:
        calls: list[dict] = []
        item = toolchain.matrix_module(
            self.module(),
            self.record(),
            Path("original.exe"),
            toolchains=("v2",),
            evaluate=self.evaluator("same-shape", calls),
        )
        self.assertEqual(len(item["rows"]), len(calls))
        self.assertEqual(len(calls), len(toolchain.candidate_sets(self.module())))

    def test_matrix_module_tries_v2_first(self) -> None:
        calls: list[dict] = []
        item = toolchain.matrix_module(
            self.module(),
            self.record(),
            Path("original.exe"),
            toolchains=("watcom10", "v2"),
            evaluate=self.evaluator("same-shape", calls),
        )
        self.assertEqual(calls[0]["toolchain"], "v2")
        self.assertEqual(item["rows"][0]["toolchain"], "v2")
        self.assertIn("watcom10", [row["toolchain"] for row in item["rows"]])

    def test_matrix_module_stops_at_the_first_exact_bytes(self) -> None:
        calls: list[dict] = []
        item = toolchain.matrix_module(
            self.module(),
            self.record(),
            Path("original.exe"),
            evaluate=self.evaluator("exact-bytes", calls),
        )
        self.assertEqual(len(calls), 1)
        self.assertEqual(item["rows"][0]["match"], "exact-bytes")

    def test_matrix_module_skips_the_period_with_a_strict_v2_result(self) -> None:
        calls: list[dict] = []

        def match_for(descriptor: dict) -> str:
            return (
                "exact-relocations"
                if descriptor["toolchain"] == "v2"
                else "same-shape"
            )

        item = toolchain.matrix_module(
            self.module(),
            self.record(),
            Path("original.exe"),
            evaluate=self.evaluator(match_for, calls),
        )
        self.assertFalse(any(call["toolchain"] == "watcom10" for call in calls))
        self.assertFalse(
            any(row["toolchain"] == "watcom10" for row in item["rows"])
        )

    def test_matrix_module_tries_the_period_without_a_strict_v2_result(
        self,
    ) -> None:
        calls: list[dict] = []
        item = toolchain.matrix_module(
            self.module(),
            self.record(),
            Path("original.exe"),
            evaluate=self.evaluator("same-shape", calls),
        )
        self.assertTrue(any(call["toolchain"] == "watcom10" for call in calls))
        self.assertTrue(
            any(row["toolchain"] == "watcom10" for row in item["rows"])
        )

    def test_matrix_module_reports_a_module_without_an_identity_record(
        self,
    ) -> None:
        calls: list[dict] = []
        item = toolchain.matrix_module(
            self.module(),
            None,
            Path("original.exe"),
            evaluate=self.evaluator("same-shape", calls),
        )
        self.assertFalse(item["measured"])
        self.assertEqual(item["reason"], "no identity record")
        self.assertEqual(calls, [])

    def test_matrix_module_reports_a_module_without_a_measured_result(
        self,
    ) -> None:
        calls: list[dict] = []

        def evaluate(
            module: dict,
            record: dict,
            descriptor: dict,
            original: Path,
            timeout: int | None = None,
        ) -> dict:
            calls.append(descriptor)
            return self.row(descriptor, build="fail")

        item = toolchain.matrix_module(
            self.module(),
            self.record(),
            Path("original.exe"),
            evaluate=evaluate,
        )
        self.assertFalse(item["measured"])
        self.assertEqual(item["reason"], "no measured result")

    def manifest_row(
        self,
        module: dict,
        match: str = "exact-bytes",
        measured: bool = True,
        toolchain_name: str = "v2",
    ) -> dict:
        return {
            "name": module["name"],
            "order": 0,
            "toolchain": toolchain_name,
            "label": "existing",
            "sources": [
                {"file": source["file"], "compiler": source["compiler"]}
                for source in module["sources"]
            ],
            "measured": measured,
            "build": "pass" if measured else "skip",
            "verdict": "pass" if measured else "skip",
            "match": match if measured else None,
            "strict": measured and match in ("exact-bytes", "exact-relocations"),
            "detail": "",
            "bytes_equal": 10 if measured else None,
            "bytes_total": 10 if measured else None,
            "instructions_equal": 5 if measured else None,
            "instructions_total": 5 if measured else None,
        }

    def test_matrix_period_filter_skips_a_strict_v2_module(self) -> None:
        module = self.module()
        calls: list[dict] = []
        manifest = {"modules": [self.manifest_row(module)]}
        item = toolchain.matrix_module(
            module,
            self.record(),
            Path("original.exe"),
            toolchains=("watcom10",),
            base_manifest=manifest,
            evaluate=self.evaluator("same-shape", calls),
        )
        self.assertEqual(calls, [])
        self.assertTrue(item["measured"])
        self.assertEqual(item["winner"]["toolchain"], "v2")

    def test_matrix_mixed_pass_skips_a_strict_base_module(self) -> None:
        module = self.module()
        calls: list[dict] = []
        manifest = {"modules": [self.manifest_row(module)]}
        item = toolchain.matrix_module(
            module,
            self.record(),
            Path("original.exe"),
            toolchains=("watcom10", "v2"),
            base_manifest=manifest,
            evaluate=self.evaluator("same-shape", calls),
        )
        self.assertEqual(calls, [])
        self.assertTrue(item["measured"])
        self.assertEqual(item["winner"]["toolchain"], "v2")

    def test_matrix_period_pass_measures_a_non_strict_v2_module(self) -> None:
        module = self.module()
        calls: list[dict] = []
        manifest = {
            "modules": [self.manifest_row(module, match="same-shape")]
        }
        item = toolchain.matrix_module(
            module,
            self.record(),
            Path("original.exe"),
            toolchains=("watcom10",),
            base_manifest=manifest,
            evaluate=self.evaluator("same-shape", calls),
        )
        self.assertTrue(calls)
        self.assertTrue(
            any(row["toolchain"] == "watcom10" for row in item["rows"])
        )

    def test_matrix_period_pass_keeps_a_better_v2_winner(self) -> None:
        module = self.module()
        calls: list[dict] = []
        manifest = {
            "modules": [self.manifest_row(module, match="same-shape")]
        }
        item = toolchain.matrix_module(
            module,
            self.record(),
            Path("original.exe"),
            toolchains=("watcom10",),
            base_manifest=manifest,
            evaluate=self.evaluator("different", calls),
        )
        self.assertEqual(item["winner"]["toolchain"], "v2")

    def test_matrix_period_pass_takes_a_better_period_winner(self) -> None:
        module = self.module()
        calls: list[dict] = []
        manifest = {
            "modules": [self.manifest_row(module, match="same-shape")]
        }
        item = toolchain.matrix_module(
            module,
            self.record(),
            Path("original.exe"),
            toolchains=("watcom10",),
            base_manifest=manifest,
            evaluate=self.evaluator("exact-relocations", calls),
        )
        self.assertEqual(item["winner"]["toolchain"], "watcom10")

    def test_matrix_only_missing_skips_a_measured_module(self) -> None:
        module = self.module()
        calls: list[dict] = []
        manifest = {
            "modules": [self.manifest_row(module, match="same-shape")]
        }
        item = toolchain.matrix_module(
            module,
            self.record(),
            Path("original.exe"),
            only_missing=True,
            resume_manifest=manifest,
            evaluate=self.evaluator("exact-bytes", calls),
        )
        self.assertEqual(calls, [])
        self.assertTrue(item["measured"])
        self.assertTrue(item["skipped"])
        self.assertEqual(item["reason"], "already measured")

    def test_matrix_only_missing_measures_an_unmeasured_module(self) -> None:
        module = self.module()
        calls: list[dict] = []
        manifest = {
            "modules": [
                self.manifest_row(module, measured=False, match="")
            ]
        }
        item = toolchain.matrix_module(
            module,
            self.record(),
            Path("original.exe"),
            only_missing=True,
            resume_manifest=manifest,
            evaluate=self.evaluator("same-shape", calls),
        )
        self.assertTrue(calls)
        self.assertTrue(item["measured"])

    def test_matrix_only_missing_without_a_resume_manifest_measures_every_module(
        self,
    ) -> None:
        module = self.module()
        calls: list[dict] = []
        item = toolchain.matrix_module(
            module,
            self.record(),
            Path("original.exe"),
            only_missing=True,
            resume_manifest=None,
            evaluate=self.evaluator("same-shape", calls),
        )
        self.assertTrue(calls)
        self.assertTrue(item["measured"])

    def test_matrix_returns_the_report_without_a_module_write(self) -> None:
        module = self.module()
        original = self.root / "original.exe"
        original.write_bytes(b"MZ")
        with mock.patch.object(store, "save_module") as save:
            report = toolchain.matrix(
                [module],
                [self.record()],
                original,
                evaluate=self.evaluator("same-shape", []),
            )
        save.assert_not_called()
        self.assertEqual(len(report["modules"]), 1)
        self.assertEqual(report["toolchains"], list(store.TOOLCHAINS))
        self.assertIn("flag_sets", report)

    def test_matrix_clears_before_each_candidate(self) -> None:
        calls: list[dict] = []
        with mock.patch.object(toolchain, "clear_artifacts") as clear:
            toolchain.matrix_module(
                self.module(),
                self.record(),
                Path("original.exe"),
                toolchains=("v2",),
                evaluate=self.evaluator("same-shape", calls),
            )
        self.assertEqual(clear.call_count, len(calls))


class ClearArtifactsTestCase(StoreTestCase):
    """The artifact cleanup of the matrix."""

    def test_clear_artifacts_removes_the_old_outputs(self) -> None:
        module = self.module()
        names = ["free.exe", "free.map", "M.MAP", "dos_build.log"]
        for name in names:
            (store.WORK_DIR / name).write_bytes(b"old")
        toolchain.clear_artifacts(module)
        for name in names:
            self.assertFalse((store.WORK_DIR / name).exists())


class CommandMatrixTestCase(StoreTestCase):
    """The command wrapper with a fake matrix and a temporary store."""

    def setUp(self) -> None:
        super().setUp()
        self.ok_module = self.module()
        self.write_module(self.ok_module)
        self.original = self.root / "STARTH.EXE"
        self.original.write_bytes(b"MZ")
        self.out = self.root / "matrix.json"
        self.saved_candidate = toolchain.CANDIDATE_DIR
        toolchain.CANDIDATE_DIR = self.root / "candidates"

    def tearDown(self) -> None:
        toolchain.CANDIDATE_DIR = self.saved_candidate
        super().tearDown()

    def arguments(self, **overrides: object) -> argparse.Namespace:
        values: dict = {
            "module": None,
            "all": True,
            "toolchain": None,
            "manifest": None,
            "only_missing": False,
            "resume": None,
            "original": self.original,
            "timeout": None,
            "dry_run": False,
            "out": self.out,
        }
        values.update(overrides)
        return argparse.Namespace(**values)

    def item(self, name: str = "free", winner: dict | None = None) -> dict:
        return {
            "name": name,
            "measured": winner is not None,
            "record": "0x100",
            "skipped": False,
            "current": {"toolchain": "v2", "sources": []},
            "winner": winner,
            "rows": [] if winner is None else [winner],
        }

    def winner_row(
        self, toolchain_name: str = "watcom10", flags: list[str] | None = None
    ) -> dict:
        return {
            "order": 0,
            "toolchain": toolchain_name,
            "label": "existing",
            "sources": [
                {
                    "file": "free.c",
                    "compiler": "wcc386",
                    "flags": flags if flags is not None else ["-of+"],
                }
            ],
            "build": "pass",
            "verdict": "pass",
            "match": "exact-bytes",
            "strict": True,
            "detail": "",
            "bytes_equal": 10,
            "bytes_total": 10,
            "instructions_equal": 5,
            "instructions_total": 5,
        }

    def test_command_matrix_writes_the_report(self) -> None:
        item = self.item(winner=self.winner_row())

        def fake_matrix(modules, records, original, on_module=None, **kwargs):
            on_module(item)
            return {"modules": [item]}

        with mock.patch.object(toolchain, "matrix", side_effect=fake_matrix), (
            mock.patch.object(toolchain.le, "load", return_value=object())
        ), contextlib.redirect_stdout(io.StringIO()):
            code = toolchain.command_matrix(self.arguments())
        self.assertEqual(code, 0)
        report = json.loads(self.out.read_text())
        self.assertEqual(len(report["modules"]), 1)

    def test_command_matrix_writes_the_report_after_each_module(self) -> None:
        first = self.item("free", self.winner_row())
        second = self.item("exit", self.winner_row())
        lengths: list[int] = []

        def fake_matrix(modules, records, original, on_module=None, **kwargs):
            on_module(first)
            lengths.append(len(json.loads(self.out.read_text())["modules"]))
            on_module(second)
            lengths.append(len(json.loads(self.out.read_text())["modules"]))
            return {"modules": [first, second]}

        with mock.patch.object(toolchain, "matrix", side_effect=fake_matrix), (
            mock.patch.object(toolchain.le, "load", return_value=object())
        ), contextlib.redirect_stdout(io.StringIO()):
            code = toolchain.command_matrix(self.arguments())
        self.assertEqual(code, 0)
        self.assertEqual(lengths, [1, 2])

    def test_command_matrix_rejects_a_broken_original(self) -> None:
        broken = self.root / "broken.exe"
        broken.write_bytes(b"MZ")
        argv = [
            "toolchain.py",
            "matrix",
            "--all",
            "--original",
            str(broken),
        ]
        with mock.patch.object(sys, "argv", argv), mock.patch.object(
            toolchain, "matrix"
        ) as matrix, contextlib.redirect_stderr(io.StringIO()):
            code = toolchain.main()
        self.assertEqual(code, 2)
        matrix.assert_not_called()

    def test_command_matrix_rejects_a_missing_original(self) -> None:
        missing = self.root / "missing.exe"
        argv = [
            "toolchain.py",
            "matrix",
            "--all",
            "--original",
            str(missing),
        ]
        with mock.patch.object(sys, "argv", argv), mock.patch.object(
            toolchain, "matrix"
        ) as matrix, contextlib.redirect_stderr(io.StringIO()):
            code = toolchain.main()
        self.assertEqual(code, 2)
        matrix.assert_not_called()

    def test_command_matrix_rejects_a_module_without_a_source(self) -> None:
        bad = {"name": "bad", "exe": "bad.exe"}
        self.write_module(bad)
        argv = [
            "toolchain.py",
            "matrix",
            "--module",
            "bad",
            "--original",
            str(self.original),
        ]
        with mock.patch.object(sys, "argv", argv), mock.patch.object(
            toolchain, "matrix"
        ) as matrix, mock.patch.object(
            toolchain.le, "load", return_value=object()
        ), contextlib.redirect_stderr(io.StringIO()):
            code = toolchain.main()
        self.assertEqual(code, 2)
        matrix.assert_not_called()

    def test_command_matrix_writes_the_winner(self) -> None:
        item = self.item(winner=self.winner_row())

        def fake_matrix(modules, records, original, on_module=None, **kwargs):
            on_module(item)
            return {"modules": [item]}

        with mock.patch.object(toolchain, "matrix", side_effect=fake_matrix), (
            mock.patch.object(toolchain.le, "load", return_value=object())
        ), contextlib.redirect_stdout(io.StringIO()):
            code = toolchain.command_matrix(self.arguments())
        self.assertEqual(code, 0)
        saved = store.load_module("free")
        self.assertEqual(saved["toolchain"], "watcom10")
        self.assertEqual(saved["sources"][0]["flags"], ["-of+"])

    def test_command_matrix_applies_the_winner_after_each_module(self) -> None:
        self.write_module(self.module(name="exit", exe="exit.exe", map="exit.map"))
        first = self.item("free", self.winner_row())
        second = self.item("exit", self.winner_row())
        counts: list[int] = []

        def fake_matrix(modules, records, original, on_module=None, **kwargs):
            with mock.patch.object(store, "save_module") as save:
                on_module(first)
                counts.append(save.call_count)
                on_module(second)
                counts.append(save.call_count)
            return {"modules": [first, second]}

        with mock.patch.object(toolchain, "matrix", side_effect=fake_matrix), (
            mock.patch.object(toolchain.le, "load", return_value=object())
        ), contextlib.redirect_stdout(io.StringIO()):
            code = toolchain.command_matrix(self.arguments())
        self.assertEqual(code, 0)
        self.assertEqual(counts, [1, 2])

    def test_command_matrix_exits_three_when_every_candidate_skips(self) -> None:
        item = self.item("free", None)
        item["rows"] = [{"build": "skip"}]

        def fake_matrix(modules, records, original, on_module=None, **kwargs):
            on_module(item)
            return {"modules": [item]}

        with mock.patch.object(toolchain, "matrix", side_effect=fake_matrix), (
            mock.patch.object(toolchain.le, "load", return_value=object())
        ), contextlib.redirect_stdout(io.StringIO()):
            code = toolchain.command_matrix(self.arguments())
        self.assertEqual(code, 3)

    def test_command_matrix_dry_run_writes_no_module(self) -> None:
        before = store.module_path("free").read_text()
        item = self.item(winner=self.winner_row())

        def fake_matrix(modules, records, original, on_module=None, **kwargs):
            on_module(item)
            return {"modules": [item]}

        with mock.patch.object(toolchain, "matrix", side_effect=fake_matrix), (
            mock.patch.object(toolchain.le, "load", return_value=object())
        ), contextlib.redirect_stdout(io.StringIO()):
            code = toolchain.command_matrix(self.arguments(dry_run=True))
        self.assertEqual(code, 0)
        self.assertTrue(self.out.exists())
        self.assertEqual(store.module_path("free").read_text(), before)


class ReportTestCase(StoreTestCase):
    """The strict-delta report over a temporary module store."""

    def result_record(
        self,
        name: str = "free",
        toolchain_name: str = "v2",
        match: str | None = "exact-bytes",
        address: str = "0x100",
    ) -> dict:
        record = self.record(address=address, name=name)
        record["result"] = {
            "mode": {"toolchain": toolchain_name},
            "checks": {"identity": {"match": match}},
        }
        return record

    def toolchain_row(self, document: dict, name: str) -> dict:
        for item in document["toolchains"]:
            if item["toolchain"] == name:
                return item
        raise AssertionError(f"no {name} row")

    def test_report_counts_modules_per_toolchain(self) -> None:
        modules = [
            self.module(),
            self.module(name="exit", exe="exit.exe", map="exit.map"),
        ]
        records = [
            self.result_record("free", "v2", "same-shape", "0x100"),
            self.result_record("exit", "watcom10", "same-shape", "0x200"),
        ]
        document = toolchain.report_document(modules, records)
        self.assertEqual(self.toolchain_row(document, "v2")["modules"], 1)
        self.assertEqual(self.toolchain_row(document, "watcom10")["modules"], 1)

    def test_report_counts_the_strict_matches(self) -> None:
        modules = [
            self.module(),
            self.module(name="exit", exe="exit.exe", map="exit.map"),
        ]
        records = [
            self.result_record("free", "v2", "exact-bytes", "0x100"),
            self.result_record("exit", "v2", "exact-relocations", "0x200"),
        ]
        row = self.toolchain_row(
            toolchain.report_document(modules, records), "v2"
        )
        self.assertEqual(row["exact-bytes"], 1)
        self.assertEqual(row["exact-relocations"], 1)
        self.assertEqual(row["strict"], 2)

    def test_report_counts_each_match_value(self) -> None:
        modules = [
            self.module(),
            self.module(name="exit", exe="exit.exe", map="exit.map"),
            self.module(name="bad", exe="bad.exe", map="bad.map"),
        ]
        records = [
            self.result_record("free", "v2", "same-shape", "0x100"),
            self.result_record("exit", "v2", "same-mnemonics", "0x200"),
            self.result_record("bad", "v2", "different", "0x300"),
        ]
        row = self.toolchain_row(
            toolchain.report_document(modules, records), "v2"
        )
        self.assertEqual(row["shape"], 2)
        self.assertEqual(row["different"], 1)
        self.assertEqual(row["no-result"], 0)

    def test_report_counts_a_module_without_an_identity_record(self) -> None:
        module = self.module(toolchain="watcom10")
        document = toolchain.report_document([module], [])
        row = self.toolchain_row(document, "watcom10")
        self.assertEqual(row["modules"], 1)
        self.assertEqual(row["no-result"], 1)

    def test_report_groups_by_the_identity_record(self) -> None:
        module = self.module(toolchain="watcom10")
        records = [self.result_record("free", "v2", "exact-bytes")]
        document = toolchain.report_document([module], records)
        self.assertEqual(self.toolchain_row(document, "v2")["modules"], 1)
        self.assertEqual(self.toolchain_row(document, "watcom10")["modules"], 0)

    def test_format_report_prints_the_total_line(self) -> None:
        document = {
            "toolchains": [
                {
                    "toolchain": "v2",
                    "modules": 2,
                    "strict": 1,
                    "exact-bytes": 1,
                    "exact-relocations": 0,
                    "shape": 1,
                    "different": 0,
                    "no-result": 0,
                }
            ],
            "total": {
                "modules": 2,
                "strict": 1,
                "exact-bytes": 1,
                "exact-relocations": 0,
                "shape": 1,
                "different": 0,
                "no-result": 0,
            },
        }
        lines = toolchain.format_report(document).splitlines()
        self.assertEqual(
            lines[0],
            "toolchain modules strict exact-bytes exact-relocations shape "
            "different no-result",
        )
        self.assertEqual(len(lines), 3)
        self.assertEqual(lines[1], "v2 2 1 1 0 1 0 0")
        self.assertEqual(lines[2], "total 2 1 1 0 1 0 0")

    def test_report_is_stable(self) -> None:
        modules = [
            self.module(),
            self.module(name="exit", exe="exit.exe", map="exit.map"),
        ]
        records = [
            self.result_record("free", "v2", "exact-bytes", "0x100"),
            self.result_record("exit", "watcom10", "different", "0x200"),
        ]
        first = toolchain.report_document(modules, records)
        second = toolchain.report_document(modules, records)
        self.assertEqual(first, second)


class ManifestTestCase(StoreTestCase):
    """The committed manifest of the measured modules."""

    def report(
        self, modules: list[dict], records: list[dict]
    ) -> dict:
        return toolchain.matrix(
            modules,
            records,
            Path("original.exe"),
            toolchains=("v2",),
            evaluate=self.evaluator("exact-bytes", []),
        )

    def mixed_report(self, modules: list[dict], records: list[dict]) -> dict:
        def evaluate(
            module: dict,
            record: dict,
            descriptor: dict,
            original: Path,
            timeout: int | None = None,
        ) -> dict:
            if module["name"] == "free":
                return self.row(descriptor, match="exact-bytes")
            return self.row(descriptor, build="fail")

        return toolchain.matrix(
            modules,
            records,
            Path("original.exe"),
            toolchains=("v2",),
            evaluate=evaluate,
        )

    def rows(self, document: dict) -> dict[str, dict]:
        return {row["name"]: row for row in document["modules"]}

    def test_manifest_lists_every_measured_module(self) -> None:
        modules = [
            self.module(),
            self.module(name="exit", exe="exit.exe", map="exit.map"),
        ]
        records = [
            self.record("0x100", "free"),
            self.record("0x200", "exit"),
        ]
        report = self.report(modules, records)
        document = toolchain.manifest_document(
            report, self.root / "matrix.json", "0" * 64
        )
        self.assertEqual(len(document["modules"]), 2)
        self.assertEqual(set(self.rows(document)), {"free", "exit"})

    def test_manifest_lists_an_unmeasured_module_with_an_identity_record(
        self,
    ) -> None:
        modules = [
            self.module(),
            self.module(name="exit", exe="exit.exe", map="exit.map"),
        ]
        records = [self.record("0x100", "free"), self.record("0x200", "exit")]
        report = self.mixed_report(modules, records)
        document = toolchain.manifest_document(
            report, self.root / "matrix.json", "0" * 64
        )
        row = self.rows(document)["exit"]
        self.assertFalse(row["measured"])
        self.assertEqual(row["detail"], "no measured result")

    def test_manifest_row_shape(self) -> None:
        modules = [
            self.module(),
            self.module(name="exit", exe="exit.exe", map="exit.map"),
        ]
        records = [self.record("0x100", "free"), self.record("0x200", "exit")]
        report = self.mixed_report(modules, records)
        document = toolchain.manifest_document(
            report, self.root / "matrix.json", "0" * 64
        )
        rows = self.rows(document)
        keys = {
            "name",
            "order",
            "toolchain",
            "label",
            "sources",
            "measured",
            "build",
            "verdict",
            "match",
            "strict",
            "detail",
            "bytes_equal",
            "bytes_total",
            "instructions_equal",
            "instructions_total",
        }
        measured = rows["free"]
        self.assertEqual(set(measured), keys)
        self.assertNotIn("winner", measured)
        self.assertTrue(measured["measured"])
        self.assertEqual(measured["toolchain"], "v2")
        self.assertEqual(measured["sources"], report["modules"][0]["winner"]["sources"])
        unmeasured = rows["exit"]
        self.assertEqual(set(unmeasured), keys)
        self.assertNotIn("winner", unmeasured)
        self.assertEqual(unmeasured["order"], -1)
        self.assertEqual(unmeasured["label"], "")
        self.assertFalse(unmeasured["measured"])
        self.assertEqual(unmeasured["build"], "skip")
        self.assertEqual(unmeasured["verdict"], "skip")
        self.assertIsNone(unmeasured["match"])
        self.assertFalse(unmeasured["strict"])
        self.assertEqual(unmeasured["detail"], "no measured result")
        for key in (
            "bytes_equal",
            "bytes_total",
            "instructions_equal",
            "instructions_total",
        ):
            self.assertIsNone(unmeasured[key])

    def test_manifest_measured_row_can_be_ranked(self) -> None:
        module = self.module()
        report = self.report([module], [self.record()])
        document = toolchain.manifest_document(
            report, self.root / "matrix.json", "0" * 64
        )
        row = document["modules"][0]
        self.assertEqual(
            toolchain.rank(row), toolchain.rank(report["modules"][0]["winner"])
        )

    def test_manifest_omits_a_module_without_an_identity_record(self) -> None:
        modules = [
            self.module(),
            self.module(name="driver", exe="driver.exe", map="driver.map"),
        ]
        report = self.report(modules, [self.record("0x100", "free")])
        document = toolchain.manifest_document(
            report, self.root / "matrix.json", "0" * 64
        )
        self.assertNotIn("driver", self.rows(document))

    def test_manifest_records_the_report_digest(self) -> None:
        report = self.report([self.module()], [self.record()])
        report_path = self.root / "matrix.json"
        report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
        out = self.root / "toolchain.json"
        arguments = argparse.Namespace(report=report_path, out=out)
        with contextlib.redirect_stdout(io.StringIO()):
            code = toolchain.command_manifest(arguments)
        self.assertEqual(code, 0)
        document = json.loads(out.read_text())
        digest = hashlib.sha256(report_path.read_bytes()).hexdigest()
        self.assertEqual(document["report"]["sha256"], digest)
        self.assertEqual(document["report"]["path"], str(report_path))


class CheckMatrixTestCase(StoreTestCase):
    """The committed manifest check against the module recipes."""

    def manifest_row(self, module: dict, **overrides: object) -> dict:
        row = {
            "name": module["name"],
            "order": 0,
            "toolchain": store.module_toolchain(module),
            "label": "existing",
            "sources": [
                {
                    "file": source["file"],
                    "compiler": source["compiler"],
                    "flags": list(source.get("flags", [])),
                }
                for source in module["sources"]
            ],
            "measured": True,
            "build": "pass",
            "verdict": "pass",
            "match": "exact-bytes",
            "strict": True,
            "detail": "",
            "bytes_equal": 10,
            "bytes_total": 10,
            "instructions_equal": 5,
            "instructions_total": 5,
        }
        row.update(overrides)
        return row

    def test_check_matrix_accepts_a_matching_manifest(self) -> None:
        module = self.module()
        manifest = {"modules": [self.manifest_row(module)]}
        messages = toolchain.check_matrix(
            [module], [self.record()], manifest, Path("toolchain.json")
        )
        self.assertEqual(messages, [])

    def test_command_check_matrix_reports_a_missing_manifest(self) -> None:
        missing = self.root / "missing.json"
        out = io.StringIO()
        error = io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(error):
            code = toolchain.command_check_matrix(
                argparse.Namespace(manifest=missing)
            )
        self.assertEqual(code, 1)
        self.assertIn(str(missing), out.getvalue() + error.getvalue())

    def test_check_matrix_reports_a_changed_toolchain(self) -> None:
        module = self.module()
        manifest = {"modules": [self.manifest_row(module, toolchain="watcom10")]}
        messages = toolchain.check_matrix(
            [module], [self.record()], manifest, Path("toolchain.json")
        )
        self.assertEqual(len(messages), 1)
        self.assertIn("free", messages[0])

    def test_check_matrix_reports_changed_flags(self) -> None:
        module = self.module(
            sources=[{"file": "free.c", "compiler": "wcc386"}]
        )
        row = self.manifest_row(module)
        row["sources"][0]["flags"] = ["-of+"]
        messages = toolchain.check_matrix(
            [module], [self.record()], {"modules": [row]}, Path("toolchain.json")
        )
        self.assertEqual(len(messages), 1)
        self.assertIn("free", messages[0])
        self.assertIn("free.c", messages[0])

    def test_check_matrix_reports_a_changed_compiler(self) -> None:
        module = self.module(
            sources=[{"file": "free.c", "compiler": "wcc386"}]
        )
        row = self.manifest_row(module)
        row["sources"][0]["compiler"] = "wasm"
        messages = toolchain.check_matrix(
            [module], [self.record()], {"modules": [row]}, Path("toolchain.json")
        )
        self.assertEqual(len(messages), 1)
        self.assertIn("free", messages[0])
        self.assertIn("free.c", messages[0])

    def test_check_matrix_reports_a_row_without_a_module(self) -> None:
        row = self.manifest_row(
            self.module(name="ghost", exe="ghost.exe", map="ghost.map")
        )
        messages = toolchain.check_matrix(
            [self.module()], [], {"modules": [row]}, Path("toolchain.json")
        )
        self.assertEqual(len(messages), 1)
        self.assertIn("ghost", messages[0])

    def test_check_matrix_reports_a_module_with_an_identity_record_without_a_row(
        self,
    ) -> None:
        messages = toolchain.check_matrix(
            [self.module()], [self.record()], {"modules": []},
            Path("toolchain.json"),
        )
        self.assertEqual(len(messages), 1)
        self.assertIn("free", messages[0])

    def test_check_matrix_reports_a_row_for_a_module_without_an_identity_record(
        self,
    ) -> None:
        module = self.module()
        manifest = {"modules": [self.manifest_row(module)]}
        messages = toolchain.check_matrix([module], [], manifest, Path("toolchain.json"))
        self.assertEqual(len(messages), 1)
        self.assertIn("free", messages[0])

    def test_check_matrix_treats_an_absent_flags_field_as_empty(self) -> None:
        module = self.module(
            sources=[{"file": "free.c", "compiler": "wcc386"}]
        )
        row = self.manifest_row(module)
        row["sources"][0].pop("flags", None)
        messages = toolchain.check_matrix(
            [module], [self.record()], {"modules": [row]}, Path("toolchain.json")
        )
        self.assertEqual(messages, [])


class ParseArgumentsTestCase(unittest.TestCase):
    """The matrix command line."""

    def parse(self, argv: list[str]) -> argparse.Namespace:
        with mock.patch.object(
            sys, "argv", ["toolchain.py", *argv]
        ):
            return toolchain.parse_arguments()

    def test_parse_arguments_accepts_the_matrix_flags(self) -> None:
        arguments = self.parse(["matrix", "--module", "free", "--dry-run"])
        self.assertEqual(arguments.module, "free")
        self.assertTrue(arguments.dry_run)

        arguments = self.parse(["matrix", "--all"])
        self.assertTrue(arguments.all)

        arguments = self.parse(
            ["matrix", "--all", "--toolchain", "watcom10"]
        )
        self.assertEqual(arguments.toolchain, ["watcom10"])

        arguments = self.parse(
            [
                "matrix",
                "--all",
                "--only-missing",
                "--resume",
                "/tmp/resume.json",
            ]
        )
        self.assertTrue(arguments.only_missing)
        self.assertEqual(arguments.resume, Path("/tmp/resume.json"))

        arguments = self.parse(
            ["matrix", "--all", "--manifest", "/tmp/base.json"]
        )
        self.assertEqual(arguments.manifest, Path("/tmp/base.json"))

        arguments = self.parse(["matrix", "--all", "--out", "/tmp/out.json"])
        self.assertEqual(arguments.out, Path("/tmp/out.json"))


if __name__ == "__main__":
    unittest.main()
