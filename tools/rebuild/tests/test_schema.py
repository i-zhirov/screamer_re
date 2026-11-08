"""Tests for the JSON Schema subset used by the rebuild contracts.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import schema  # noqa: E402


class SchemaTestCase(unittest.TestCase):
    def test_required_field(self) -> None:
        violations = schema.validate(
            {"a": 1},
            {"type": "object", "required": ["b"], "properties": {"a": {}}},
        )
        self.assertEqual(len(violations), 1)
        self.assertIn("'b' is missing", violations[0])

    def test_additional_properties_false(self) -> None:
        violations = schema.validate(
            {"a": 1, "b": 2},
            {"type": "object", "properties": {"a": {}},
             "additionalProperties": False},
        )
        self.assertEqual(len(violations), 1)
        self.assertIn("'b' is not allowed", violations[0])

    def test_enum(self) -> None:
        violations = schema.validate("bad", {"enum": ["good"]})
        self.assertEqual(len(violations), 1)
        self.assertIn("not in", violations[0])

    def test_pattern(self) -> None:
        violations = schema.validate(
            "nope", {"type": "string", "pattern": "^0x[0-9a-f]+$"}
        )
        self.assertEqual(len(violations), 1)
        self.assertIn("does not match", violations[0])

    def test_minimum_length_and_value(self) -> None:
        self.assertEqual(
            schema.validate("", {"type": "string", "minLength": 1}),
            ["$: the string holds 0 characters, less than 1"],
        )
        self.assertEqual(
            schema.validate(2, {"type": "integer", "minimum": 3}),
            ["$: the number 2 is less than 3"],
        )

    def test_array_items_and_uniqueness(self) -> None:
        violations = schema.validate(
            ["a", "a", 3],
            {
                "type": "array",
                "uniqueItems": True,
                "items": {"type": "string"},
            },
        )
        self.assertEqual(len(violations), 2)
        self.assertTrue(any("not unique" in item for item in violations))
        self.assertTrue(any("the type integer is not string" in item
                            for item in violations))

    def test_boolean_is_not_integer(self) -> None:
        violations = schema.validate(True, {"type": "integer"})
        self.assertEqual(len(violations), 1)

    def test_local_reference(self) -> None:
        document = {
            "type": "object",
            "properties": {"item": {"$ref": "#/$defs/item"}},
            "$defs": {"item": {"type": "string"}},
        }
        self.assertEqual(schema.validate({"item": "ok"}, document), [])
        violations = schema.validate({"item": 4}, document)
        self.assertEqual(len(violations), 1)
        self.assertIn("$.item", violations[0])

    def test_file_reference(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "record.schema.json").write_text(
                json.dumps({"$defs": {"result": {"type": "object"}}})
            )
            violations = schema.validate(
                4,
                {"$ref": "record.schema.json#/$defs/result"},
                base_dir=root,
            )
        self.assertEqual(len(violations), 1)
        self.assertIn("the type integer is not object", violations[0])

    def test_validate_file_reports_broken_json(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path = root / "record.json"
            path.write_text("{")
            schema_path = root / "schema.json"
            schema_path.write_text("{}")
            violations = schema.validate_file(path, schema_path)
        self.assertEqual(len(violations), 1)
        self.assertIn("broken JSON", violations[0])


if __name__ == "__main__":
    unittest.main()
