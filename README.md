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

`--version` output is taken from `VERSION` in generated `config.h`, which is
derived from the Meson project version.

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

## Tests

Run the test suite with:

```bash
meson test -C build --print-errorlogs
```

The main conversion regression test uses fixture files in `examples/`.

## Examples

- `examples/m_to_u_input.cpp` -> `examples/m_to_u_expected.cpp`
- `examples/u_to_m_input.cpp` -> `examples/u_to_m_expected.cpp`

## Bash completion

A completion script is provided at `completions/membervars` and installed with Meson.

## Author

Lenik ([membervars@bodz.net](mailto:membervars@bodz.net))

## License

AGPL-3.0-or-later with an additional anti-AI use statement. See `LICENSE`.
