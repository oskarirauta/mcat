# mcat

Markup-aware `cat`.

`mcat` prints files like `cat`, but with **syntax colours**, **binary detection**
and a **built-in browser** (pager) for long files — so colours survive paging,
which `more`/`less` don't handle well out of the box.

Following the "support enough to be useful, not everything" philosophy, it
recognises a handful of common formats:

| type      | detected from                                             |
|-----------|-----------------------------------------------------------|
| json      | `.json`, or content starting with `{` / `[`               |
| markdown  | `.md`, `.markdown`                                         |
| config    | `.ini .conf .cfg .toml .yml .yaml .env .service` …, `Makefile`, `Dockerfile`, dotfiles |
| c / c++   | `.c .h .cpp .cc .cxx .hpp .hxx .inc` …                     |
| shell     | `.sh .bash .zsh .ksh`, `.bashrc`/`.profile` …, or a `sh`/`bash` shebang |
| diff      | `.diff .patch`, or `diff --git` / `--- ` / `@@` content    |
| binary    | NUL bytes or a high ratio of control bytes                |

## Features

- **Colours** for json, markdown, config, c/c++, shell and diff/patch (keys,
  strings, numbers, keywords, comments, headings, added/removed lines, …).
- **json is pretty-printed by default** (via the bundled json library) and then
  coloured — a colourful `jq .`, if you like. Invalid/partial json is left as-is
  and still coloured. Note that reformatting sorts object keys alphabetically;
  use `-r`/`--raw` to keep the file exactly as written.
- **Indentation** preserved; tabs expanded consistently (`-T` to set width).
- **Binary detection**: instead of dumping garbage, prints `binary file`,
  the size and a guessed format (ELF, PNG, JPEG, ZIP, gzip, PDF, …) or a hex
  magic peek.
- **Browser mode** (pager): scroll with arrow keys / PgUp / PgDn, `Space`
  pages forward, `b` pages back, `←`/`→` scroll horizontally (`0` resets),
  `/` searches (`n`/`N` for next/previous), `g`/`G` jump to top/bottom,
  `q` or `Esc` quits. A status line shows `line N/total` and the position
  (`TOP`/`END`/`%`).

Paging kicks in automatically when the output is a terminal and the file is
longer than the screen. When output is piped, `mcat` behaves like plain `cat`
(no colours, no paging) unless told otherwise.

## Options

```
-n, --number        show line numbers
-b, --browser       always use the browser (pager)
-p, --no-pager      disable the browser: print everything and exit
-C, --no-color      disable colours
-r, --raw           json: don't reformat, colour the file as-is
-t, --type <type>   force type: json|md|config|cpp|sh|diff|text
-T, --tab <n>       tab width (default 8)
-h, --help          usage help
-v, --version       show version
```

## Build

```
git submodule update --init --recursive
make
```

Depends on [usage_cpp](https://github.com/oskarirauta/usage_cpp) (argument
parsing) and [json_cpp](https://github.com/oskarirauta/json_cpp), both included
as submodules. C++17, no other dependencies (the pager uses raw termios + ANSI,
no ncurses).
