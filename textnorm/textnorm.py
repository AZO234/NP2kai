from __future__ import annotations

import argparse
from pathlib import Path
import sys

import to_utf8
import to_lf
import normalize_symbols

def _run_module(mod, argv) -> int:
    """
    Call a module's main() by temporarily replacing sys.argv.
    This avoids spawning subprocesses and keeps behavior consistent.
    """
    old_argv = sys.argv
    try:
        sys.argv = argv
        return int(mod.main() or 0)
    finally:
        sys.argv = old_argv

def main() -> int:
    ap = argparse.ArgumentParser(
        prog="textnorm.py",
        description="Text normalization runner: utf8 -> lf -> symbols (or individual steps).",
    )
    sub = ap.add_subparsers(dest="cmd", required=True)

    def add_common(subp):
        subp.add_argument("dir", type=Path)
        subp.add_argument("--dry-run", action="store_true")
        subp.add_argument("--verbose", action="store_true")
        subp.add_argument("--ext", default=None, help="Comma-separated extension allowlist. If omitted, uses each tool's default.")
        subp.add_argument("--follow-symlinks", action="store_true")
        subp.add_argument("--include-hidden", action="store_true")

    p_utf8 = sub.add_parser("utf8", help="Convert to UTF-8 (default: no BOM).")
    add_common(p_utf8)
    p_utf8.add_argument("--bom", action="store_true", help="Add UTF-8 BOM. Default: no BOM.")

    p_lf = sub.add_parser("lf", help="Normalize line endings to LF.")
    add_common(p_lf)

    p_sym = sub.add_parser("symbols", help="Normalize JIS-variant symbols (￥/¥ and ￣/¯/‾).")
    add_common(p_sym)

    p_all = sub.add_parser("all", help="Run utf8 -> lf -> symbols in that order.")
    add_common(p_all)
    p_all.add_argument("--bom", action="store_true", help="Add UTF-8 BOM in the utf8 step. Default: no BOM.")

    args = ap.parse_args()

    def build_argv(tool_name: str, extra: list[str] | None = None) -> list[str]:
        argv = [tool_name]
        if args.dry_run:
            argv.append("--dry-run")
        if args.verbose:
            argv.append("--verbose")
        if args.ext is not None:
            argv += ["--ext", args.ext]
        if args.follow_symlinks:
            argv.append("--follow-symlinks")
        if args.include_hidden:
            argv.append("--include-hidden")
        if extra:
            argv += extra
        argv.append(str(args.dir))
        return argv

    if args.cmd == "utf8":
        extra = ["--bom"] if getattr(args, "bom", False) else []
        return _run_module(to_utf8, build_argv("to_utf8.py", extra))

    if args.cmd == "lf":
        return _run_module(to_lf, build_argv("to_lf.py"))

    if args.cmd == "symbols":
        return _run_module(normalize_symbols, build_argv("normalize_symbols.py"))

    if args.cmd == "all":
        extra = ["--bom"] if getattr(args, "bom", False) else []
        rc = _run_module(to_utf8, build_argv("to_utf8.py", extra))
        if rc != 0:
            return rc
        rc = _run_module(to_lf, build_argv("to_lf.py"))
        if rc != 0:
            return rc
        return _run_module(normalize_symbols, build_argv("normalize_symbols.py"))

    return 2

if __name__ == "__main__":
    raise SystemExit(main())
