#!/bin/sh
set -eu

MEMBERVARS_BIN="$1"
SRC_ROOT="$2"

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT INT TERM

run_case() {
    mode="$1"
    input_rel="$2"
    expected_rel="$3"
    work_file="$TMP_DIR/work.c"

    cp "$SRC_ROOT/$input_rel" "$work_file"
    "$MEMBERVARS_BIN" "$mode" "$work_file" >/dev/null
    cmp -s "$work_file" "$SRC_ROOT/$expected_rel"
}

run_case "-u" "examples/m_to_u_input.cpp" "examples/m_to_u_expected.cpp"
run_case "-m" "examples/u_to_m_input.cpp" "examples/u_to_m_expected.cpp"

echo "membervars tests passed"
