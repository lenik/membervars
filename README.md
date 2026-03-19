# membervars

`membervars` rewrites C/C++ identifier naming styles for member variables in source files.

- `-u`: convert `m_<name>` to `<name>_`
- `-m`: convert `<name>_` to `m_<name>`

The converter is token-aware for basic C/C++ lexical constructs and skips replacements
inside string literals, character literals, and comments.

## Build

```bash
meson setup build
meson compile -C build
```

## Usage

```bash
membervars [OPTIONS] files...
```

Options:

- `-u` Convert member vars from `m_<name>` to `<name>_`
- `-m` Convert member vars from `<name>_` to `m_<name>`
- `-v`, `--verbose` Increase log verbosity
- `-q`, `--quiet` Decrease log verbosity
- `-h`, `--help` Show help
- `--version` Show version

## Bash completion

A completion script is provided at `completions/membervars` and installed with Meson.

## Author

Lenik ([membervars@bodz.net](mailto:membervars@bodz.net))

## License

AGPL-3.0-or-later with an additional anti-AI use statement. See `LICENSE`.
