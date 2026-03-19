#!/bin/sh
set -eu

MEMBERVARS_BIN="$1"
SRC_ROOT="$2"
TESTS_DIR="$SRC_ROOT/tests"

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT INT TERM

run_case() {
    mode="$1"
    name="$2"
    work_file="$TMP_DIR/work.c"

    cp "$TESTS_DIR/${name}.in.cpp" "$work_file"
    "$MEMBERVARS_BIN" "$mode" "$work_file" >/dev/null
    cmp -s "$work_file" "$TESTS_DIR/${name}.out.cpp"
}

run_case "-u" "m_to_u_basic"
run_case "-m" "u_to_m_basic"
run_case "-u" "m_to_u_params"
run_case "-u" "m_to_u_several"
run_case "-m" "u_to_m_nested"

echo "membervars tests passed"
