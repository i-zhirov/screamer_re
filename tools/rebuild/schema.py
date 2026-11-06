"""Validate a JSON value against the schema subset used by the contracts.

The contracts under `tools/rebuild/contracts/` use a small part of JSON Schema:
types, enumerations, required fields, properties, additional properties, array
items, length and number bounds, patterns, and references.  The project forbids a
new dependency, so this module implements that subset with the standard library.

Usage:
    from schema import validate

    violations = validate(record, schema, base_dir=contracts_dir)

The validator returns one message per violation.  It prints nothing and raises
no error for a schema violation.
"""
from __future__ import annotations

import json
import re
from pathlib import Path

_JSON_TYPES: dict[str, type | tuple[type, ...]] = {
    "object": dict,
    "array": list,
    "string": str,
    "integer": int,
    "number": (int, float),
    "boolean": bool,
    "null": type(None),
}


def _type_name(value: object) -> str:
    if value is None:
        return "null"
    if isinstance(value, bool):
        return "boolean"
    if isinstance(value, int):
        return "integer"
    if isinstance(value, float):
        return "number"
    if isinstance(value, str):
        return "string"
    if isinstance(value, list):
        return "array"
    if isinstance(value, dict):
        return "object"
    return type(value).__name__


def _matches_type(value: object, expected: str) -> bool:
    if expected == "integer" and isinstance(value, bool):
        return False
    if expected == "number" and isinstance(value, bool):
        return False
    expected_type = _JSON_TYPES.get(expected)
    if expected_type is None:
        return True
    return isinstance(value, expected_type)


def _resolve_pointer(document: object, pointer: str) -> object:
    if pointer in ("", "/"):
        return document
    current = document
    for part in pointer.lstrip("/").split("/"):
        part = part.replace("~1", "/").replace("~0", "~")
        if isinstance(current, list):
            current = current[int(part)]
        elif isinstance(current, dict):
            current = current[part]
        else:
            raise KeyError(part)
    return current


def _resolve_ref(
    reference: str,
    base_dir: Path,
    document: object,
) -> tuple[object, object]:
    """Return the referenced schema and the document that holds it."""
    if reference.startswith("#"):
        return _resolve_pointer(document, reference[1:]), document
    file_part, _, pointer = reference.partition("#")
    referenced = json.loads((base_dir / file_part).read_text())
    return _resolve_pointer(referenced, pointer), referenced


def validate(
    value: object,
    schema: object,
    *,
    base_dir: Path | str | None = None,
    path: str = "$",
) -> list[str]:
    """Return one message per schema violation of `value`."""
    root = Path(base_dir) if base_dir is not None else Path.cwd()
    return _validate(value, schema, root, schema, path)


def _validate(
    value: object,
    schema: object,
    base_dir: Path,
    document: object,
    path: str,
) -> list[str]:
    if not isinstance(schema, dict):
        return []
    violations: list[str] = []
    reference = schema.get("$ref")
    if isinstance(reference, str):
        try:
            target, target_document = _resolve_ref(reference, base_dir, document)
        except (OSError, KeyError, ValueError, IndexError) as error:
            return [f"{path}: the reference {reference!r} does not resolve: {error}"]
        violations.extend(
            _validate(value, target, base_dir, target_document, path)
        )

    expected = schema.get("type")
    if expected is not None:
        types = expected if isinstance(expected, list) else [expected]
        if not any(_matches_type(value, item) for item in types):
            rendered = " or ".join(str(item) for item in types)
            violations.append(
                f"{path}: the type {_type_name(value)} is not {rendered}"
            )

    if "enum" in schema:
        allowed = schema["enum"]
        if value not in allowed:
            violations.append(f"{path}: the value {value!r} is not in {allowed!r}")

    if isinstance(value, str):
        minimum = schema.get("minLength")
        if isinstance(minimum, int) and len(value) < minimum:
            violations.append(
                f"{path}: the string holds {len(value)} characters, less than {minimum}"
            )
        pattern = schema.get("pattern")
        if isinstance(pattern, str) and re.search(pattern, value) is None:
            violations.append(f"{path}: the string does not match {pattern!r}")

    if isinstance(value, int) and not isinstance(value, bool):
        minimum = schema.get("minimum")
        if isinstance(minimum, (int, float)) and value < minimum:
            violations.append(f"{path}: the number {value} is less than {minimum}")

    if isinstance(value, list):
        minimum = schema.get("minItems")
        if isinstance(minimum, int) and len(value) < minimum:
            violations.append(
                f"{path}: the array holds {len(value)} items, less than {minimum}"
            )
        if schema.get("uniqueItems"):
            rendered = [json.dumps(item, sort_keys=True) for item in value]
            if len(set(rendered)) != len(rendered):
                violations.append(f"{path}: the array items are not unique")
        items = schema.get("items")
        if isinstance(items, dict):
            for index, item in enumerate(value):
                violations.extend(
                    _validate(item, items, base_dir, document, f"{path}[{index}]")
                )

    if isinstance(value, dict):
        minimum = schema.get("minProperties")
        if isinstance(minimum, int) and len(value) < minimum:
            violations.append(
                f"{path}: the object holds {len(value)} properties, less than {minimum}"
            )
        for name in schema.get("required", []):
            if name not in value:
                violations.append(f"{path}: the required field {name!r} is missing")
        properties = schema.get("properties", {})
        for name, item in value.items():
            if name in properties:
                violations.extend(
                    _validate(
                        item,
                        properties[name],
                        base_dir,
                        document,
                        f"{path}.{name}",
                    )
                )
                continue
            additional = schema.get("additionalProperties", True)
            if additional is False:
                violations.append(f"{path}: the field {name!r} is not allowed")
            elif isinstance(additional, dict):
                violations.extend(
                    _validate(item, additional, base_dir, document, f"{path}.{name}")
                )
    return violations


def validate_file(path: Path, schema_path: Path) -> list[str]:
    """Validate the JSON file at `path` against `schema_path`."""
    try:
        value = json.loads(path.read_text())
    except FileNotFoundError:
        return [f"{path}: the file is missing"]
    except json.JSONDecodeError as error:
        return [f"{path}: broken JSON: {error}"]
    except OSError as error:
        return [f"{path}: {error}"]
    schema = json.loads(schema_path.read_text())
    return validate(value, schema, base_dir=schema_path.parent)
