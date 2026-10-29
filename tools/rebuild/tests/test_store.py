"""Tests for the rebuild store records, their validation, and their digests.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import json
import re
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import store  # noqa: E402


class StoreTestCase(unittest.TestCase):
    """A test case with a temporary store and a temporary evidence root."""

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.saved_functions = store.FUNCTIONS_DIR
        self.saved_modules = store.MODULES_DIR
        store.FUNCTIONS_DIR = self.root / "functions"
        store.MODULES_DIR = self.root / "modules"

    def tearDown(self) -> None:
        store.FUNCTIONS_DIR = self.saved_functions
        store.MODULES_DIR = self.saved_modules
        self.temporary.cleanup()

    def write_record(self, address: int, record: dict) -> Path:
        path = store.record_path(address)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n")
        return path

    def write_module(self, name: str, record: dict) -> Path:
        path = store.module_path(name)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n")
        return path

    def record(self, **overrides: object) -> dict:
        base = {
            "address": "0x300ec",
            "status": "unknown",
            "checks": [],
            "evidence": [],
            "notes": [],
        }
        base.update(overrides)
        return base

    def result(self, checks: dict[str, tuple[str, str]]) -> dict:
        return {
            "date": "2026-10-26",
            "command": "python3 tools/rebuild/run.py run --function 0x300ec",
            "tool": "run.py",
            "source_digest": "sha256:" + "0" * 64,
            "fingerprint": "sha256:" + "0" * 64,
            "checks": {
                name: {"verdict": verdict, "detail": detail}
                for name, (verdict, detail) in checks.items()
            },
        }

    def full_record(self) -> dict:
        """A function record that holds every field of the schema."""
        record = self.record(
            object_offset="0x200ec",
            size=12,
            name="free",
            role="Release a block.",
            confidence="observed",
            section="Program functions",
            module="free",
            symbol="free_",
            count=3,
        )
        record["result"] = self.result({"build": ("pass", "the module builds")})
        return record

    def module_record(self, **overrides: object) -> dict:
        base: dict = {
            "name": "free",
            "exe": "free.exe",
            "sources": [{"file": "free.c", "compiler": "wcc386"}],
        }
        base.update(overrides)
        return base

    def full_module(self) -> dict:
        """A module record that holds every field of the schema."""
        module = self.module_record(
            map="free.map",
            run={
                "command": "free.exe > free_out.txt",
                "output": "free_out.txt",
                "expect": ["DONE"],
            },
            status="named",
            checks=["build"],
        )
        module["result"] = self.result({"build": ("pass", "the module builds")})
        return module

    def schema(self, name: str) -> dict:
        return json.loads((store.CONTRACTS_DIR / name).read_text())

    def fingerprint_env(
        self,
    ) -> tuple[Path, Path, Path, Path, dict, dict]:
        """A rebuild tree, a record, and a module for the fingerprint tests."""
        rebuild = self.root / "rebuild"
        work = self.root / "work"
        tools = self.root / "tools"
        original = self.root / "STARTH.EXE"
        rebuild.mkdir(parents=True, exist_ok=True)
        work.mkdir(parents=True, exist_ok=True)
        (tools / "rebuild").mkdir(parents=True, exist_ok=True)
        for name in ("chk.asm", "stack.asm", "cstart.c"):
            (rebuild / name).write_bytes(f"shared {name}".encode())
        (rebuild / "free.c").write_bytes(b"int free_(void *p) { return 0; }")
        for name in store.TOOL_FILES:
            (tools / name).write_bytes(f"tool {name}".encode())
        (work / "free.exe").write_bytes(b"EXE")
        (work / "free.map").write_bytes(b"MAP")
        original.write_bytes(b"ORIGINAL")
        module = {
            "name": "free",
            "exe": "free.exe",
            "map": "free.map",
            "sources": [{"file": "free.c", "compiler": "wcc386"}],
        }
        record = self.record(
            name="free",
            module="free",
            symbol="free_",
            count=3,
            checks=["build", "identity"],
        )
        return rebuild, work, tools, original, record, module

    def fingerprint_of(
        self, rebuild: Path, work: Path, tools: Path, original: Path, record: dict, module: dict
    ) -> str:
        return store.compute_fingerprint(
            record,
            module,
            rebuild_dir=rebuild,
            work_dir=work,
            tools_dir=tools,
            original=original,
        )

    def test_advance_status_build_passes(self) -> None:
        self.assertEqual(
            store.advance_status(["build"], {"build": "pass"}), "rebuilt"
        )

    def test_advance_status_identity_fails(self) -> None:
        verdicts = {"build": "pass", "identity": "fail"}
        self.assertEqual(
            store.advance_status(["build", "identity"], verdicts), "rebuilt"
        )

    def test_advance_status_behavior_fails(self) -> None:
        verdicts = {"build": "pass", "identity": "pass", "behavior": "fail"}
        checks = ["build", "identity", "behavior"]
        self.assertEqual(store.advance_status(checks, verdicts), "matching")

    def test_advance_status_identity_only(self) -> None:
        self.assertEqual(
            store.advance_status(["identity"], {"identity": "pass"}), "verified"
        )

    def test_advance_status_all_pass(self) -> None:
        checks = ["build", "identity", "behavior"]
        verdicts = {name: "pass" for name in checks}
        self.assertEqual(store.advance_status(checks, verdicts), "verified")

    def test_advance_status_behavior_only(self) -> None:
        self.assertEqual(
            store.advance_status(["behavior"], {"behavior": "pass"}), "verified"
        )

    def test_advance_status_skip(self) -> None:
        verdicts = {"build": "pass", "identity": "skip"}
        self.assertIsNone(store.advance_status(["build", "identity"], verdicts))

    def test_advance_status_build_fails(self) -> None:
        verdicts = {"build": "fail", "identity": "pass"}
        self.assertIsNone(store.advance_status(["build", "identity"], verdicts))

    def test_supported_status_from_result(self) -> None:
        record = self.record(
            name="free",
            status="rebuilt",
            checks=["build", "identity", "behavior"],
            result=self.result(
                {
                    "build": ("pass", "the module builds"),
                    "identity": ("pass", "the modules are byte-identical"),
                    "behavior": ("fail", "the output differs"),
                }
            ),
        )
        self.assertEqual(store.supported_status(record), "matching")

    def test_is_stale_no_result(self) -> None:
        self.assertFalse(store.is_stale(self.record(), "sha256:" + "0" * 64))

    def test_is_stale_fingerprint_change(self) -> None:
        record = self.record(result={"fingerprint": "sha256:" + "1" * 64})
        self.assertFalse(store.is_stale(record, "sha256:" + "1" * 64))
        self.assertTrue(store.is_stale(record, "sha256:" + "2" * 64))

    def test_is_stale_without_stored_fingerprint(self) -> None:
        record = self.record(result={"source_digest": "sha256:" + "0" * 64})
        self.assertTrue(store.is_stale(record, "sha256:" + "1" * 64))

    def test_source_is_stale_no_result(self) -> None:
        self.assertFalse(store.source_is_stale(self.record(), "sha256:" + "0" * 64))

    def test_source_is_stale_digest_change(self) -> None:
        record = self.record(result={"source_digest": "sha256:" + "1" * 64})
        self.assertFalse(store.source_is_stale(record, "sha256:" + "1" * 64))
        self.assertTrue(store.source_is_stale(record, "sha256:" + "2" * 64))

    def test_source_digest_changes_with_content(self) -> None:
        (self.root / "one.c").write_bytes(b"one")
        (self.root / "two.c").write_bytes(b"two")
        first = store.source_digest(
            {"sources": [{"file": "one.c", "compiler": "wcc386"}]}, self.root
        )
        second = store.source_digest(
            {"sources": [{"file": "two.c", "compiler": "wcc386"}]}, self.root
        )
        self.assertNotEqual(first, second)
        self.assertEqual(
            first,
            store.source_digest(
                {"sources": [{"file": "one.c", "compiler": "wcc386"}]},
                self.root,
            ),
        )
        pattern = re.compile(r"^sha256:[0-9a-f]{64}$")
        self.assertRegex(first, pattern)
        self.assertRegex(second, pattern)

    def test_fingerprint_is_stable(self) -> None:
        rebuild, work, tools, original, record, module = self.fingerprint_env()
        self.assertEqual(
            self.fingerprint_of(rebuild, work, tools, original, record, module),
            self.fingerprint_of(rebuild, work, tools, original, record, module),
        )
        self.assertRegex(
            self.fingerprint_of(rebuild, work, tools, original, record, module),
            re.compile(r"^sha256:[0-9a-f]{64}$"),
        )

    def test_fingerprint_changes_with_source_content(self) -> None:
        rebuild, work, tools, original, record, module = self.fingerprint_env()
        before = self.fingerprint_of(rebuild, work, tools, original, record, module)
        (rebuild / "free.c").write_bytes(b"changed")
        after = self.fingerprint_of(rebuild, work, tools, original, record, module)
        self.assertNotEqual(before, after)

    def test_fingerprint_changes_with_shared_source(self) -> None:
        rebuild, work, tools, original, record, module = self.fingerprint_env()
        before = self.fingerprint_of(rebuild, work, tools, original, record, module)
        (rebuild / "chk.asm").write_bytes(b"changed")
        after = self.fingerprint_of(rebuild, work, tools, original, record, module)
        self.assertNotEqual(before, after)

    def test_fingerprint_changes_with_recipe(self) -> None:
        rebuild, work, tools, original, record, module = self.fingerprint_env()
        before = self.fingerprint_of(rebuild, work, tools, original, record, module)
        changed = {
            **module,
            "sources": [
                {**module["sources"][0], "flags": ["-of+"]}
            ],
        }
        after = self.fingerprint_of(rebuild, work, tools, original, record, changed)
        self.assertNotEqual(before, after)

    def test_fingerprint_changes_with_record_fields(self) -> None:
        rebuild, work, tools, original, record, module = self.fingerprint_env()
        before = self.fingerprint_of(rebuild, work, tools, original, record, module)
        self.assertNotEqual(
            before,
            self.fingerprint_of(
                rebuild, work, tools, original, {**record, "symbol": "other_"}, module
            ),
        )
        self.assertNotEqual(
            before,
            self.fingerprint_of(
                rebuild, work, tools, original, {**record, "count": 4}, module
            ),
        )
        self.assertNotEqual(
            before,
            self.fingerprint_of(
                rebuild,
                work,
                tools,
                original,
                {**record, "checks": ["build"]},
                module,
            ),
        )

    def test_fingerprint_changes_with_tool_code(self) -> None:
        rebuild, work, tools, original, record, module = self.fingerprint_env()
        before = self.fingerprint_of(rebuild, work, tools, original, record, module)
        (tools / "rebuild/run.py").write_bytes(b"changed")
        after = self.fingerprint_of(rebuild, work, tools, original, record, module)
        self.assertNotEqual(before, after)

    def test_fingerprint_changes_with_original(self) -> None:
        rebuild, work, tools, original, record, module = self.fingerprint_env()
        before = self.fingerprint_of(rebuild, work, tools, original, record, module)
        original.write_bytes(b"changed")
        after = self.fingerprint_of(rebuild, work, tools, original, record, module)
        self.assertNotEqual(before, after)

    def test_fingerprint_changes_when_artifact_missing(self) -> None:
        rebuild, work, tools, original, record, module = self.fingerprint_env()
        before = self.fingerprint_of(rebuild, work, tools, original, record, module)
        (work / "free.exe").unlink()
        after = self.fingerprint_of(rebuild, work, tools, original, record, module)
        self.assertNotEqual(before, after)
        (work / "free.map").unlink()
        again = self.fingerprint_of(rebuild, work, tools, original, record, module)
        self.assertNotEqual(after, again)

    def test_fingerprint_ignores_status_and_result(self) -> None:
        rebuild, work, tools, original, record, module = self.fingerprint_env()
        before = self.fingerprint_of(rebuild, work, tools, original, record, module)
        annotated = {
            **module,
            "status": "verified",
            "result": {"date": "2026-10-26", "checks": {}},
        }
        self.assertEqual(
            before,
            self.fingerprint_of(rebuild, work, tools, original, record, annotated),
        )

    def test_fingerprint_ignores_documentation_fields(self) -> None:
        rebuild, work, tools, original, record, module = self.fingerprint_env()
        before = self.fingerprint_of(rebuild, work, tools, original, record, module)
        documented = {
            **record,
            "name": "another name",
            "role": "Release a block.",
            "confidence": "inferred",
            "section": "Program functions",
            "evidence": [{"kind": "note", "text": "a note"}],
            "notes": [{"date": "2026-10-26", "text": "a note"}],
        }
        self.assertEqual(
            before,
            self.fingerprint_of(
                rebuild, work, tools, original, documented, module
            ),
        )

    def test_validate_record_address_mismatch(self) -> None:
        path = self.write_record(0x300EC, self.record(address="0x300f0"))
        violations = store.validate_record(path)
        self.assertEqual(len(violations), 1)
        self.assertIn(str(path), violations[0])
        self.assertIn("address", violations[0])

    def test_validate_record_unknown_status(self) -> None:
        path = self.write_record(0x300EC, self.record(status="done"))
        violations = store.validate_record(path)
        self.assertEqual(len(violations), 1)
        self.assertIn("status", violations[0])

    def test_validate_record_named_without_name(self) -> None:
        path = self.write_record(0x300EC, self.record(status="named"))
        violations = store.validate_record(path)
        self.assertEqual(len(violations), 1)
        self.assertIn("requires a name", violations[0])

    def test_validate_record_verified_without_result(self) -> None:
        path = self.write_record(
            0x300EC, self.record(name="free", status="verified")
        )
        violations = store.validate_record(path)
        self.assertEqual(len(violations), 1)
        self.assertIn("requires a result", violations[0])

    def test_validate_record_verified_without_passing_identity(self) -> None:
        self.write_module(
            "free",
            {
                "name": "free",
                "exe": "free.exe",
                "map": "free.map",
                "sources": [{"file": "free.c", "compiler": "wcc386"}],
            },
        )
        record = self.record(
            name="free",
            status="verified",
            module="free",
            symbol="free_",
            checks=["build", "identity"],
            result=self.result(
                {
                    "build": ("pass", "the module builds"),
                    "identity": ("fail", "the instruction sequences differ"),
                }
            ),
        )
        path = self.write_record(0x300EC, record)
        violations = store.validate_record(path)
        self.assertTrue(
            any("passing identity" in message for message in violations)
        )

    def test_validate_record_check_without_result(self) -> None:
        self.write_module(
            "free",
            {
                "name": "free",
                "exe": "free.exe",
                "map": "free.map",
                "sources": [{"file": "free.c", "compiler": "wcc386"}],
            },
        )
        record = self.record(
            name="free",
            status="named",
            module="free",
            symbol="free_",
            checks=["build", "identity"],
            result=self.result({"build": ("pass", "the module builds")}),
        )
        path = self.write_record(0x300EC, record)
        violations = store.validate_record(path)
        self.assertEqual(len(violations), 1)
        self.assertIn("no check result", violations[0])

    def test_validate_record_unknown_check(self) -> None:
        path = self.write_record(
            0x300EC, self.record(checks=["build", "compare"])
        )
        violations = store.validate_record(path)
        self.assertEqual(len(violations), 1)
        self.assertIn("compare", violations[0])

    def test_validate_record_unknown_module(self) -> None:
        record = self.record(module="ghost", symbol="ghost_")
        path = self.write_record(0x300EC, record)
        violations = store.validate_record(path)
        self.assertEqual(len(violations), 1)
        self.assertIn("ghost", violations[0])

    def test_validate_record_module_without_symbol(self) -> None:
        self.write_module(
            "free",
            {
                "name": "free",
                "exe": "free.exe",
                "sources": [{"file": "free.c", "compiler": "wcc386"}],
            },
        )
        path = self.write_record(0x300EC, self.record(module="free"))
        violations = store.validate_record(path)
        self.assertEqual(len(violations), 1)
        self.assertIn("symbol", violations[0])

    def test_validate_record_identity_without_module(self) -> None:
        path = self.write_record(
            0x300EC, self.record(checks=["build", "identity"])
        )
        violations = store.validate_record(path)
        self.assertEqual(len(violations), 1)
        self.assertIn("requires a module", violations[0])

    def test_validate_record_identity_without_map(self) -> None:
        self.write_module(
            "driver",
            {
                "name": "driver",
                "exe": "driver.exe",
                "sources": [{"file": "driver.c", "compiler": "wcc386"}],
            },
        )
        record = self.record(
            module="driver", symbol="driver_", checks=["build", "identity"]
        )
        path = self.write_record(0x300EC, record)
        violations = store.validate_record(path)
        self.assertEqual(len(violations), 1)
        self.assertIn("map", violations[0])

    def test_validate_record_status_stronger_than_result(self) -> None:
        record = self.record(
            name="free",
            status="matching",
            checks=["build"],
            result=self.result({"build": ("pass", "the module builds")}),
        )
        path = self.write_record(0x300EC, record)
        violations = store.validate_record(path)
        self.assertEqual(len(violations), 1)
        self.assertIn("stronger", violations[0])

    def test_validate_record_finding_heading(self) -> None:
        findings = self.root / "docs/explanation/findings.md"
        findings.parent.mkdir(parents=True, exist_ok=True)
        findings.write_text("# Findings\n\n## A real heading\n")
        missing = self.record(
            evidence=[{"kind": "finding", "text": "A missing heading"}]
        )
        missing_path = self.write_record(0x300EC, missing)
        violations = store.validate_record(missing_path, root=self.root)
        self.assertEqual(len(violations), 1)
        self.assertIn("heading", violations[0])
        present = self.record(
            address="0x300f0",
            evidence=[{"kind": "finding", "text": "A real heading"}],
        )
        present_path = self.write_record(0x300F0, present)
        self.assertEqual(
            store.validate_record(present_path, root=self.root), []
        )

    def test_validate_record_missing_file(self) -> None:
        record = self.record(
            evidence=[{"kind": "file", "text": "tools/missing.c"}]
        )
        path = self.write_record(0x300EC, record)
        violations = store.validate_record(path, root=self.root)
        self.assertEqual(len(violations), 1)
        self.assertIn("missing.c", violations[0])

    def test_validate_module_status_above_named(self) -> None:
        record = {
            "name": "driver",
            "exe": "driver.exe",
            "sources": [{"file": "driver.c", "compiler": "wcc386"}],
            "status": "verified",
            "checks": ["behavior"],
        }
        path = self.write_module("driver", record)
        violations = store.validate_module(path)
        self.assertEqual(len(violations), 1)
        self.assertIn("result", violations[0])

    def test_record_round_trip(self) -> None:
        record = self.record(
            name="free",
            role="Release a block.",
            status="named",
            checks=["build"],
            evidence=[
                {"kind": "command", "text": "python3 tools/le.py info STARTH.EXE"}
            ],
            notes=[{"date": "2026-10-26", "text": "The name is recovered."}],
        )
        store.save_record(record)
        self.assertEqual(store.load_record(0x300EC), record)
        text = store.record_path(0x300EC).read_text()
        self.assertEqual(text, json.dumps(record, indent=2, sort_keys=True) + "\n")

    def test_schema_matches_record_properties(self) -> None:
        schema = self.schema("record.schema.json")
        self.assertEqual(set(self.full_record()), set(schema["properties"]))
        self.assertEqual(set(schema["required"]), set(self.record()))

    def test_schema_matches_module_properties(self) -> None:
        schema = self.schema("module.schema.json")
        self.assertEqual(set(self.full_module()), set(schema["properties"]))
        self.assertEqual(set(schema["required"]), set(self.module_record()))

    def test_seed_diff_added_and_lost(self) -> None:
        candidates = [0x300EC]
        records = {
            0x300F0: self.record(address="0x300f0", status="unknown"),
            0x300F8: self.record(
                address="0x300f8", status="named", name="free"
            ),
        }
        added, lost = store.seed_diff(candidates, records)
        self.assertEqual(added, [0x300EC])
        self.assertEqual(lost, [0x300F0])


if __name__ == "__main__":
    unittest.main()
