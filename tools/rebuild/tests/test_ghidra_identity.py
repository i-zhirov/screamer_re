"""Tests for the Ghidra setup checks of the LX/LE loader.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests -p 'test_ghidra_identity.py'
"""
from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from pathlib import Path

SCRIPT = (
    Path(__file__).resolve().parent.parent.parent
    / "ghidra"
    / "install_loader.sh"
)
GHIDRA_VERSION = "12.0.4"


class LoaderCheckTestCase(unittest.TestCase):
    """The loader check must read the installed extension without network."""

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.install = self.root / "ghidra"
        self.user = self.root / "user"

    def write_ghidra(self, version: str = GHIDRA_VERSION) -> None:
        properties = self.install / "Ghidra" / "application.properties"
        properties.parent.mkdir(parents=True, exist_ok=True)
        properties.write_text(f"application.version={version}\n")

    def write_extension(self, version: str = GHIDRA_VERSION) -> Path:
        properties = (
            self.user
            / "Extensions"
            / "ghidra-lx-loader"
            / "extension.properties"
        )
        properties.parent.mkdir(parents=True, exist_ok=True)
        properties.write_text(f"version={version}\n")
        return properties

    def fake_curl(self) -> tuple[Path, str]:
        bin_dir = self.root / "bin"
        bin_dir.mkdir(exist_ok=True)
        marker = bin_dir / "curl-called"
        curl = bin_dir / "curl"
        curl.write_text(
            "#!/bin/sh\ntouch \"$(dirname \"$0\")/curl-called\"\n"
        )
        curl.chmod(0o755)
        path = os.pathsep.join([str(bin_dir), os.environ.get("PATH", "")])
        return marker, path

    def run_script(
        self,
        *arguments: str,
        cwd: Path | None = None,
        path: str | None = None,
    ) -> subprocess.CompletedProcess:
        environment = {
            "GHIDRA_INSTALL_DIR": str(self.install),
            "GHIDRA_USER_DIR": str(self.user),
            "PATH": path if path is not None else os.environ.get("PATH", ""),
        }
        return subprocess.run(
            ["/bin/sh", str(SCRIPT), *arguments],
            cwd=str(cwd) if cwd is not None else None,
            env=environment,
            capture_output=True,
            text=True,
            timeout=60,
        )

    def test_loader_check_accepts_an_installed_extension(self) -> None:
        self.write_ghidra()
        properties = self.write_extension()
        result = self.run_script("--check")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(str(properties), result.stdout)

    def test_loader_check_rejects_a_missing_extension(self) -> None:
        self.write_ghidra()
        result = self.run_script("--check")
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn("install_loader.sh", result.stderr)

    def test_loader_check_rejects_a_version_mismatch(self) -> None:
        self.write_ghidra()
        self.write_extension("12.0.3")
        result = self.run_script("--check")
        self.assertEqual(result.returncode, 2, result.stderr)

    def test_loader_check_rejects_a_missing_ghidra_install(self) -> None:
        self.write_extension()
        properties = self.install / "Ghidra" / "application.properties"
        result = self.run_script("--check")
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn(str(properties), result.stderr)

    def test_loader_check_prints_the_install_command(self) -> None:
        self.write_ghidra()
        result = self.run_script("--check")
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn("nix develop .#cross-check", result.stderr)
        self.assertIn("install_loader.sh", result.stderr)

    def test_loader_check_does_not_download(self) -> None:
        self.write_ghidra()
        marker, path = self.fake_curl()
        cwd = self.root / "cwd"
        cwd.mkdir()
        result = self.run_script("--check", cwd=cwd, path=path)
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertFalse(marker.exists(), "the check called curl")
        self.assertFalse((cwd / "work" / "ghidra" / "lx-loader.zip").exists())

    def test_install_mode_rejects_a_missing_ghidra_install(self) -> None:
        self.write_extension()
        _, path = self.fake_curl()
        result = self.run_script(path=path)
        self.assertEqual(result.returncode, 2, result.stderr)


if __name__ == "__main__":
    unittest.main()
