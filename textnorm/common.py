from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import os
import tempfile
from typing import Iterator, Optional, Tuple

DEFAULT_SKIP_DIR_NAMES = {
    ".git", ".svn", ".hg", ".bzr",
    "node_modules", ".venv", "venv", "__pycache__",
    ".tox", ".mypy_cache", ".pytest_cache",
    ".idea", ".vscode",
}


@dataclass(frozen=True)
class WalkOptions:
    follow_symlinks: bool = False
    include_hidden: bool = False
    skip_dir_names: Tuple[str, ...] = tuple(sorted(DEFAULT_SKIP_DIR_NAMES))
    ext_allowlist: Optional[Tuple[str, ...]] = None  # lower-case without dot, e.g. ("txt","md")
    dry_run: bool = False


@dataclass
class Reporter:
    """
    Small helper to unify output + dry-run behavior across scripts.

    Usage:
        rep = Reporter(dry_run=args.dry_run)
        rep.ok(path)               # changed / would change
        rep.noop(path)             # already normalized (printed only in verbose)
        rep.skip(path, "reason")   # skipped with reason
        rep.done()                 # prints summary (optional)
    """
    dry_run: bool = False
    verbose: bool = False
    changed: int = 0
    skipped: int = 0
    noops: int = 0

    def ok(self, path: Path, note: str = "") -> None:
        self.changed += 1
        if self.dry_run:
            msg = "[DRY]"
        else:
            msg = "[OK ]"
        if note:
            print(f"{msg} {path} ({note})")
        else:
            print(f"{msg} {path}")

    def noop(self, path: Path, note: str = "") -> None:
        self.noops += 1
        if not self.verbose:
            return
        if note:
            print(f"[NOOP] {path} ({note})")
        else:
            print(f"[NOOP] {path}")

    def skip(self, path: Path, reason: str = "") -> None:
        self.skipped += 1
        if reason:
            print(f"[SKIP] {path} ({reason})")
        else:
            print(f"[SKIP] {path}")

    def done(self) -> None:
        print(f"Done. Changed: {self.changed}, Skipped: {self.skipped}, Noop: {self.noops}")


def _is_hidden_path(p: Path) -> bool:
    # Any segment starting with '.' is considered hidden
    return any(part.startswith(".") for part in p.parts)


def iter_files(root: Path, opt: WalkOptions) -> Iterator[Path]:
    root = root.resolve()
    if not root.exists():
        return iter(())

    # Walk using os.walk for directory pruning
    for dirpath, dirnames, filenames in os.walk(root, followlinks=opt.follow_symlinks):
        dp = Path(dirpath)

        # Prune directories
        pruned = []
        for d in dirnames:
            if d in opt.skip_dir_names:
                continue
            if not opt.include_hidden and d.startswith("."):
                continue
            pruned.append(d)
        dirnames[:] = pruned

        for fn in filenames:
            if not opt.include_hidden and fn.startswith("."):
                continue
            p = dp / fn
            if not opt.include_hidden:
                try:
                    rel = p.relative_to(root)
                except Exception:
                    rel = p
                if _is_hidden_path(rel):
                    continue

            if opt.ext_allowlist is not None:
                ext = p.suffix.lower().lstrip(".")
                if not ext or ext not in opt.ext_allowlist:
                    continue

            if p.is_file():
                yield p


def is_probably_text_bytes(data: bytes) -> bool:
    """
    Heuristic to avoid touching obvious binaries while staying friendly to legacy encodings
    like Shift_JIS/CP932.

    Rules:
    - Reject NUL-containing data.
    - Penalize *C0* control bytes (<0x20) except common whitespace controls.
    - Do NOT penalize bytes in 0x80-0xFF (they are common in non-UTF8 encodings).
    """
    if not data:
        return True
    if b"\x00" in data:
        return False

    sample = data[:65536]
    bad = 0
    for b in sample:
        # C0 controls other than \t \n \r \f \b are suspicious in text
        if b < 32 and b not in (9, 10, 13, 12, 8):
            bad += 1

    # If too many suspicious controls, it's probably binary
    return (bad / max(1, len(sample))) < 0.01


def safe_write_bytes(target: Path, data: bytes) -> None:
    """
    Atomic-ish overwrite: write temp in the same directory, then replace.
    Ensures temp file is cleaned up on failure.
    """
    target = Path(target)
    tmp_path: Optional[Path] = None
    try:
        d = target.parent
        fd, tmp_name = tempfile.mkstemp(prefix=f".{target.name}.tmp.", dir=d)
        tmp_path = Path(tmp_name)
        with os.fdopen(fd, "wb") as f:
            f.write(data)
            f.flush()
            os.fsync(f.fileno())
        tmp_path.replace(target)
    finally:
        if tmp_path is not None and tmp_path.exists():
            try:
                tmp_path.unlink()
            except Exception:
                pass
