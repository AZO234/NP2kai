# TextNorm (Python)

A small toolkit to normalize text files in a directory tree.

## What changed in this version

- `common.py` now includes a **Reporter** class to unify output + dry-run behavior.
- Scripts accept `--verbose` to show NOOP files (already normalized).

## Scripts

- `to_utf8.py [--bom] [--ext "..."] [--dry-run] [--verbose] <dir>`
  - Converts likely-text files to UTF-8.
  - Default: **no BOM**. Add `--bom` if you really need it.
  - Skips binary files and skipped dirs (like `.git/`).

- `to_lf.py [--ext "..."] [--dry-run] [--verbose] <dir>`
  - Normalizes line endings to LF.

- `normalize_symbols.py [--ext "..."] [--dry-run] [--verbose] <dir>`
  - `￥`/`¥` → `\`
  - `￣`/`¯`/`‾` → `~`

## Recommended usage

Always restrict by extensions for safety:

```sh
python3 to_utf8.py --ext "c,h,cpp,cc,java,py,php,js,ts,md,txt,json,yml,yaml,toml,xml,ini" ./NP2kai
python3 to_lf.py --ext "c,h,cpp,cc,java,py,php,js,ts,md,txt,json,yml,yaml,toml,xml,ini" ./NP2kai
python3 normalize_symbols.py --ext "c,h,cpp,cc,java,py,php,js,ts,md,txt,json,yml,yaml,toml,xml,ini" ./NP2kai
```

Dry-run:

```sh
python3 to_utf8.py --dry-run --ext "c,h,cpp,cc,md,txt" ./NP2kai
```

Show NOOP files too:

```sh
python3 to_lf.py --verbose --ext "c,h,cpp,cc,md,txt" ./NP2kai
```


Default extensions:
c,h,cpp,cc,hpp,inc,mcr,x86,x84,py,md,txt,json,html,css,js,yml,yaml,toml,xml,ini

## Note

This version uses an SJIS/CP932-friendly binary heuristic so Shift_JIS files won't be misclassified as binary.

## Runner: textnorm.py

`common.py` is a library. If you want a single entrypoint to run steps in order, use `textnorm.py`.

Examples:

```sh
# run all steps: utf8 -> lf -> symbols
python3 textnorm.py all ./np21w-src-rev99

# dry-run + verbose
python3 textnorm.py all --dry-run --verbose ./np21w-src-rev99

# only utf8 (no BOM by default)
python3 textnorm.py utf8 ./np21w-src-rev99

# utf8 with BOM
python3 textnorm.py utf8 --bom ./np21w-src-rev99

# override extensions (recommended when targeting repos)
python3 textnorm.py all --ext "c,h,cpp,cc,hpp,inc,py,md,txt,json,html,css,js,yml,yaml,toml,xml,ini" ./np21w-src-rev99
```
