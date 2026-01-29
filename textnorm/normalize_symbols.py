from __future__ import annotations

import argparse
from pathlib import Path

from common import WalkOptions, Reporter, iter_files, is_probably_text_bytes, safe_write_bytes

MAP = {
    "￥": "\\",
    "¥": "\\",
    "¯": "~",
    "‾": "~",
}

def main() -> int:
    ap = argparse.ArgumentParser(description="Recursively normalize JIS-variant symbols.")
    ap.add_argument("dir", type=Path)
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--verbose", action="store_true", help="Also print NOOP (no changes) files.")
    ap.add_argument("--ext", default="c,h,cpp,cc,hpp,inc,mcr,x86,x84,py,md,txt,json,html,css,js,yml,yaml,toml,xml,ini", help="Comma-separated extension allowlist (recommended).")
    ap.add_argument("--follow-symlinks", action="store_true")
    ap.add_argument("--include-hidden", action="store_true")
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
        try:
            raw = f.read_bytes()
        except PermissionError:
            rep.skip(f, "permission")
            continue
        except Exception:
            rep.skip(f, "read error")
            continue

        if not is_probably_text_bytes(raw):
            continue

        try:
            text = raw.decode("utf-8-sig", errors="strict")
        except Exception:
            continue

        new = text
        for k, v in MAP.items():
            new = new.replace(k, v)

        if new == text:
            rep.noop(f, "no changes")
            continue

        out = new.encode("utf-8")
        if raw.startswith(b"\xef\xbb\xbf"):
            out = b"\xef\xbb\xbf" + out

        if args.dry_run:
            rep.ok(f, "would normalize symbols")
            continue

        try:
            safe_write_bytes(f, out)
            rep.ok(f, "normalized symbols")
        except PermissionError:
            rep.skip(f, "permission")
        except Exception:
            rep.skip(f, "write error")

    if not args.dry_run:
        rep.done()
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
