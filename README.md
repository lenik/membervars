# membervars

`membervars` rewrites C/C++ identifier naming styles for member variables in source files.

- `-u`: convert `m_<name>` to `<name>_`
- `-m`: convert `<name>_` to `m_<name>`

The converter is token-aware for basic C/C++ lexical constructs and skips replacements
inside string literals, character literals, and comments.

## Usage

```bash
membervars [OPTIONS] files...
```

Options:

- `-u` Convert member vars from `m_<name>` to `<name>_`
- `-m` Convert member vars from `<name>_` to `m_<name>`
- `-c`, `--stdout` Write converted output to stdout (do not modify files)
- `-v`, `--verbose` Increase log verbosity
- `-q`, `--quiet` Decrease log verbosity
- `-h`, `--help` Show help
- `--version` Show version

Only member variables are renamed; identifiers in parameter lists (after `(` or `,`) are left unchanged.

## How It Works

`membervars` does not do a global regex replacement. It scans source code character by character using a lightweight lexical state machine.

- In `normal` state, it recognizes identifier tokens and checks conversion patterns:
  - `-u`: `m_<name>` -> `<name>_`
  - `-m`: `<name>_` -> `m_<name>`
- In string, character literal, line-comment, and block-comment states, it copies content as-is and performs no renaming.
- When a function parameter list is detected, parameter names are recorded and protected within the corresponding function-body scope to avoid renaming parameter variables.
- Member access forms such as `obj.m_x` and `ptr->m_x` are still converted, so member variables can be renamed while same-name parameters remain unchanged.
- By default, files are rewritten in place; `-c/--stdout` writes converted output to stdout only.

The goal is a "safe enough" lexical conversion: it does not require a full C++ parser, but it handles common real-world code patterns while avoiding the most frequent false replacements.

## 原理说明

`membervars` 不是用正则做全局替换，而是按字符扫描源码并维护一个轻量词法状态机：

- 在 `normal` 状态下识别标识符 token，并判断是否符合可转换模式：
  - `-u`：`m_<name>` → `<name>_`
  - `-m`：`<name>_` → `m_<name>`
- 进入字符串、字符字面量、行注释、块注释状态后，只做原样拷贝，不做重命名。
- 检测到函数参数列表时，会记录参数名并在对应函数体作用域内保护这些名字，避免把参数变量误改成成员变量风格。
- 对 `obj.m_x` / `ptr->m_x` 这样的成员访问仍会按规则转换，因此成员变量可改、同名参数可保留。
- 默认就地改写文件；`-c/--stdout` 只输出转换结果，不落盘。

这个实现目标是“足够安全的词法级转换”：不依赖完整 C++ 语法解析，但能覆盖常见工程代码场景并避免最常见误改。

## Build

```bash
meson setup build
meson compile -C build
```

`--version` output is taken from `VERSION` in generated `config.h`, which is
derived from the Meson project version.

## Tests

Run the test suite with:

```bash
meson test -C build --print-errorlogs
```

The conversion tests use input/expected pairs in `tests/` (e.g. `tests/m_to_u_basic.in.cpp` and `tests/m_to_u_basic.out.cpp`).

## Release iteration

Add a new stanza to `debian/changelog` (version and release notes) by hand, then run:

```bash
./scripts/iteration.sh .
# or: ninja -C build iteration
```

The script copies that version into `meson.build` and `man/membervars.1`, runs build and tests, and on success commits the three files.

## Examples

- `tests/m_to_u_basic.in.cpp` → `tests/m_to_u_basic.out.cpp` (and other pairs in `tests/`)
- Example fixtures are also in `examples/` for reference.

## Bash completion

A completion script is provided at `completions/membervars` and installed with Meson.

## Author

Lenik ([membervars@bodz.net](mailto:membervars@bodz.net))

## License

AGPL-3.0-or-later with an additional anti-AI use statement. See `LICENSE`.
