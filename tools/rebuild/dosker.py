"""Call the Dosker CLI from the rebuild tools.

The client is the only place that starts the `dosker` binary.  It checks the
pinned version, creates the toolchain volume, starts a container, runs a guest
command, and copies files in and out.  It reads its paths from the caller.  It
passes `DOSKER_ROOT` to the child.

Usage:
    from dosker import DoskerClient
    client = DoskerClient("dosker", Path("work/dosker/root"), Path("work/rebuild"))
    client.check_version()
    client.start("freedos:1.3", "screamer-free", volumes=[("tools", "D:")])
    result = client.exec("screamer-free", "C:\\\\WORK\\\\BUILD.BAT")
    client.remove("screamer-free")

The CLI exit codes are 0 on success, 124 on a guest timeout, and 125 when the
container is not running.  A missing binary raises `DoskerMissing`.  A failed
command raises `DoskerError`.
"""
from __future__ import annotations

import os
import subprocess
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path

PINNED_VERSION = "0.1.2"
DEBUG_IMAGE = "freedos:1.3"
RUN_IMAGE = "screamer-run:1"
TOOLCHAIN_VOLUME = "screamer-watcom-v2-tools"
TOOLCHAIN_SIZE = "256M"


class DoskerMissing(RuntimeError):
    """The Dosker binary is not on the host."""


class DoskerError(RuntimeError):
    """A Dosker command failed."""


@dataclass
class ExecResult:
    """The result of one guest command."""

    exit_code: int
    output: str
    timed_out: bool


@dataclass(frozen=True)
class Volume:
    """One named volume and its drive letter."""

    name: str
    drive: str


class DoskerClient:
    """The only boundary to the Dosker CLI."""

    def __init__(self, binary: str, data_root: Path, work_dir: Path) -> None:
        self.binary = binary
        self.data_root = Path(data_root).resolve()
        self.work_dir = Path(work_dir)

    def _run(
        self,
        arguments: Sequence[str],
        timeout: int | None = 120,
        check: bool = True,
    ) -> subprocess.CompletedProcess[str]:
        env = os.environ.copy()
        env["DOSKER_ROOT"] = str(self.data_root)
        try:
            completed = subprocess.run(
                [self.binary, *arguments],
                capture_output=True,
                text=True,
                env=env,
                timeout=timeout,
            )
        except FileNotFoundError as error:
            raise DoskerMissing(
                f"the dosker command {self.binary!r} is missing"
            ) from error
        except subprocess.TimeoutExpired as error:
            raise DoskerError(
                f"the dosker command {arguments[0]!r} did not return in time"
            ) from error
        if check and completed.returncode != 0:
            detail = (completed.stderr or completed.stdout).strip().splitlines()
            message = detail[-1] if detail else "no message"
            raise DoskerError(
                f"the dosker command {arguments[0]!r} failed: {message}"
            )
        return completed

    def version(self) -> str:
        """The reported Dosker version, for example `0.1.2`."""
        completed = self._run(["--version"])
        parts = completed.stdout.strip().split()
        return parts[-1] if parts else ""

    def check_version(self) -> str:
        """Return the pinned version or raise on a mismatch."""
        reported = self.version()
        if reported != PINNED_VERSION:
            raise DoskerError(
                f"the dosker version is {reported or 'unknown'}, "
                f"the tools pin {PINNED_VERSION}"
            )
        return reported

    def images(self) -> list[str]:
        """The image references in the store."""
        completed = self._run(["images"])
        lines = completed.stdout.splitlines()[1:]
        return [line.split()[0] for line in lines if line.strip()]

    def image_exists(self, reference: str) -> bool:
        """True when the store holds the reference."""
        return reference in self.images()

    def image_digest(self, reference: str) -> str:
        """The digest of one image reference, or an empty string."""
        completed = self._run(["images"])
        for line in completed.stdout.splitlines()[1:]:
            parts = line.split()
            if len(parts) >= 2 and parts[0] == reference:
                return parts[1]
        return ""

    def volumes(self) -> list[str]:
        """The volume names in the store."""
        completed = self._run(["volume", "ls"])
        lines = completed.stdout.splitlines()[1:]
        return [line.split()[0] for line in lines if line.strip()]

    def volume_exists(self, name: str) -> bool:
        """True when the store holds the volume."""
        return name in self.volumes()

    def volume_create(self, name: str, size: str = TOOLCHAIN_SIZE) -> None:
        """Create one named volume when it is absent."""
        if not self.volume_exists(name):
            self._run(["volume", "create", "--size", size, name])

    def volume_image(self, name: str) -> Path:
        """The raw image of one named volume."""
        return self.data_root / "volumes" / name / "volume.img"

    def build_image(self, reference: str, context: Path) -> None:
        """Build one image from a context directory."""
        self._run(
            ["build", "--update", "-t", reference, str(context)],
            timeout=None,
        )

    def start(
        self,
        reference: str,
        name: str,
        volumes: Sequence[Volume] = (),
        engine: str | None = None,
        boot_timeout: int = 120,
    ) -> None:
        """Start one detached container."""
        arguments = [
            "run",
            "-d",
            "--name",
            name,
            "--boot-timeout",
            str(boot_timeout),
        ]
        if engine is not None:
            arguments += ["--engine", engine]
        for volume in volumes:
            arguments += ["-v", f"{volume.name}:{volume.drive}"]
        arguments.append(reference)
        self._run(arguments, timeout=boot_timeout + 120)

    def exec(
        self,
        name: str,
        command: str,
        workdir: str = "C:\\",
        timeout: int = 30,
    ) -> ExecResult:
        """Run one guest command and return its exit code and output."""
        completed = self._run(
            [
                "exec",
                "-w",
                workdir,
                "--timeout",
                str(timeout),
                name,
                command,
            ],
            timeout=timeout + 120,
            check=False,
        )
        code = completed.returncode
        return ExecResult(
            exit_code=code,
            output=completed.stdout + completed.stderr,
            timed_out=code == 124,
        )

    def copy_in(self, name: str, host: Path, guest: str) -> None:
        """Copy one host file into the guest."""
        self._run(["cp", str(host), f"{name}:{guest}"])

    def copy_out(self, name: str, guest: str, host: Path) -> None:
        """Copy one guest file to the host."""
        self._run(["cp", f"{name}:{guest}", str(host)])

    def logs(self, name: str, kind: str = "container") -> str:
        """The container log, the engine log, or the serial log."""
        arguments = ["logs", name]
        if kind != "container":
            arguments.append(f"--{kind}")
        completed = self._run(arguments, check=False)
        return completed.stdout

    def stop(self, name: str) -> None:
        """Stop one container and ignore an absent one."""
        self._run(["stop", name], check=False)

    def remove(self, name: str) -> None:
        """Stop and remove one container, ignoring the absent case."""
        self.stop(name)
        self._run(["rm", name], check=False)
