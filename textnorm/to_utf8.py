from __future__ import annotations

import argparse
from pathlib import Path
from typing import Optional, Tuple

from common import WalkOptions, Reporter, iter_files, is_probably_text_bytes, safe_write_bytes

CANDIDATE_ENCODINGS: Tuple[str, ...] = (
    "utf-8-sig",
    "utf-8",
    "cp932",
    "euc-jp",
    "iso-2022-jp",
)

def decode_best_effort(raw: bytes) -> Optional[str]:
    if not is_probably_text_bytes(raw):
        return None
    for enc in CANDIDATE_ENCODINGS:
        try:
            return raw.decode(enc, errors="strict")
        except Exception:
            continue
    return None

def convert_one(path: Path, add_bom: bool) -> Tuple[str, str]:
    """
    Returns (status, note)
      status: OK / NOOP / SKIP / PERM / READERR / WRITEERR
      note: optional detail
    """
    try:
        raw = path.read_bytes()
    except PermissionError:
        return ("PERM", "permission")
    except Exception:
        return ("READERR", "read error")

    text = decode_best_effort(raw)
    if text is None:
        return ("SKIP", "binary/unknown encoding")

    out = text.encode("utf-8")
    if add_bom:
        out = b"\xef\xbb\xbf" + out

    if out == raw:
        return ("NOOP", "already normalized")

    try:
        safe_write_bytes(path, out)
        return ("OK", "utf-8+bom" if add_bom else "utf-8")
    except PermissionError:
        return ("PERM", "permission")
    except Exception:
        return ("WRITEERR", "write error")

def main() -> int:
    ap = argparse.ArgumentParser(description="Recursively convert text files to UTF-8 (default: no BOM).")
    ap.add_argument("dir", type=Path)
    ap.add_argument("--bom", action="store_true", help="Add UTF-8 BOM to output (UTF-8-SIG). Default: no BOM.")
    ap.add_argument("--dry-run", action="store_true", help="Show what would be changed without writing.")
    ap.add_argument("--verbose", action="store_true", help="Also print NOOP (already normalized) files.")
    ap.add_argument("--ext", default="c,h,cpp,cc,hpp,inc,mcr,x86,x84,py,md,txt,json,html,css,js,yml,yaml,toml,xml,ini", help="Comma-separated extension allowlist (e.g. txt,md,c,h).")
    ap.add_argument("--follow-symlinks", action="store_true")
    ap.add_argument("--include-hidden", action="store_true", help="Include dotfiles and dot-directories (not recommended).")
    args = ap.parse_args()

    ext_allow = None
    if args.ext.strip():
        ext_allow = tuple(e.strip().lower().lstrip(".") for e in args.ext.split(",") if e.strip()) or None

    opt = WalkOptions(
        follow_symlinks=args.follow_symlinks,
        include_hidden=args.include_hidden,
        ext_allowlist=ext_allow,
        dry_run=args.dry_run,
    )

    rep = Reporter(dry_run=args.dry_run, verbose=args.verbose)

    for f in iter_files(args.dir, opt):
        status, note = convert_one(f, args.bom)
        if status == "OK":
            rep.ok(f, note)
        elif status == "NOOP":
            rep.noop(f, note)
        elif status in ("PERM", "READERR", "WRITEERR"):
            rep.skip(f, note)
        # SKIP is quiet unless verbose (treat as noop-ish info)
        elif status == "SKIP" and rep.verbose:
            rep.skip(f, note)

    if not args.dry_run:
        rep.done()
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
