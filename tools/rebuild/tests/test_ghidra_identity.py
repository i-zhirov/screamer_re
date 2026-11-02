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
IDENTITY = (
    Path(__file__).resolve().parent.parent.parent
    / "ghidra"
    / "identity.sh"
)
GHIDRA_VERSION = "12.0.4"
IDENTITY_KEYS = (
    "entry=0x300f8\n"
    "page0_module=0x10000\n"
    "page0_file=0x1a800\n"
    "main=0x10020\n"
    "caller=0xd4324\n"
    "call_site=0xd4364\n"
    "call_target=0x10020\n"
)


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


class IdentityTestCase(unittest.TestCase):
    """The shared setup for the identity wrapper tests."""

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.install = self.root / "ghidra"
        self.user = self.root / "user"
        self.bin = self.root / "bin"
        self.bin.mkdir()

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

    def fake_analyze_headless(self, output: str = "") -> None:
        script = self.bin / "analyzeHeadless"
        script.write_text(
            "#!/bin/sh\n"
            "cat <<'ANALYZE'\n"
            f"{output}\n"
            "ANALYZE\n"
        )
        script.chmod(0o755)

    def base_path(self) -> str:
        return os.pathsep.join([str(self.bin), "/usr/bin", "/bin"])

    def run_identity(
        self,
        *arguments: str,
        cwd: Path | None = None,
        path: str | None = None,
    ) -> subprocess.CompletedProcess:
        environment = {
            "GHIDRA_INSTALL_DIR": str(self.install),
            "GHIDRA_USER_DIR": str(self.user),
            "PATH": path if path is not None else self.base_path(),
        }
        return subprocess.run(
            ["/bin/sh", str(IDENTITY), *arguments],
            cwd=str(cwd) if cwd is not None else None,
            env=environment,
            capture_output=True,
            text=True,
            timeout=60,
        )

    def write_executable(self) -> Path:
        executable = self.root / "STARTH.EXE"
        executable.write_bytes(b"MZ")
        return executable


class IdentityCheckTestCase(IdentityTestCase):
    """The check mode verifies the Ghidra setup without a download."""

    def test_identity_check_accepts_a_working_setup(self) -> None:
        self.write_ghidra()
        self.write_extension()
        self.fake_analyze_headless()
        result = self.run_identity("--check")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("ok", result.stdout)
        self.assertIn(GHIDRA_VERSION, result.stdout)

    def test_identity_check_rejects_a_missing_analyzeheadless(self) -> None:
        self.write_ghidra()
        self.write_extension()
        result = self.run_identity("--check", path="/usr/bin:/bin")
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn("cross-check", result.stderr)

    def test_identity_check_rejects_a_missing_loader(self) -> None:
        self.write_ghidra()
        self.fake_analyze_headless()
        result = self.run_identity("--check")
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn("install_loader.sh", result.stderr)


class IdentityRunTestCase(IdentityTestCase):
    """The run mode requires the marker and every key."""

    def prefixed(self, body: str) -> str:
        return "".join(
            f"INFO  LeIdentity.java> {line}\n"
            for line in body.splitlines()
        )

    def suffixed(self, body: str) -> str:
        return "".join(
            f"INFO  LeIdentity.java> {line} (GhidraScript)  \n"
            for line in body.splitlines()
        )

    def run_with(self, body: str) -> subprocess.CompletedProcess:
        self.fake_analyze_headless(self.prefixed(body))
        return self.run_identity(
            "--exe",
            str(self.write_executable()),
            "--project-dir",
            str(self.root / "proj"),
            "--main",
            "0x10020",
            "--call-site",
            "0xd4364",
        )

    def run_suffixed(self, body: str) -> subprocess.CompletedProcess:
        self.fake_analyze_headless(self.suffixed(body))
        return self.run_identity(
            "--exe",
            str(self.write_executable()),
            "--project-dir",
            str(self.root / "proj"),
            "--main",
            "0x10020",
            "--call-site",
            "0xd4364",
        )

    def test_identity_run_accepts_the_seven_keys(self) -> None:
        result = self.run_with("=== identity\n" + IDENTITY_KEYS)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("=== identity", result.stdout)
        for key in IDENTITY_KEYS.splitlines():
            self.assertIn(key, result.stdout)

    def test_identity_run_strips_the_ghidra_script_suffix(self) -> None:
        result = self.run_suffixed("=== identity\n" + IDENTITY_KEYS)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("=== identity\n", result.stdout)
        for key in IDENTITY_KEYS.splitlines():
            self.assertIn(key + "\n", result.stdout)
        self.assertNotIn("GhidraScript", result.stdout)

    def test_identity_run_accepts_an_unknown_entry(self) -> None:
        keys = IDENTITY_KEYS.replace("entry=0x300f8", "entry=unknown")
        result = self.run_with("=== identity\n" + keys)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("entry=unknown", result.stdout)

    def test_identity_run_rejects_a_missing_marker(self) -> None:
        result = self.run_with(IDENTITY_KEYS)
        self.assertEqual(result.returncode, 2, result.stderr)

    def test_identity_run_rejects_a_missing_key(self) -> None:
        keys = IDENTITY_KEYS.replace("call_target=0x10020\n", "")
        result = self.run_with("=== identity\n" + keys)
        self.assertEqual(result.returncode, 2, result.stderr)

    def test_identity_run_rejects_a_java_error_line(self) -> None:
        body = "=== identity\n" + IDENTITY_KEYS + "error: bad address\n"
        result = self.run_with(body)
        self.assertEqual(result.returncode, 2, result.stderr)

    def test_identity_run_rejects_a_missing_exe(self) -> None:
        result = self.run_identity(
            "--exe",
            str(self.root / "missing.exe"),
            "--project-dir",
            str(self.root / "proj"),
            "--main",
            "0x10020",
            "--call-site",
            "0xd4364",
        )
        self.assertEqual(result.returncode, 2, result.stderr)


if __name__ == "__main__":
    unittest.main()
