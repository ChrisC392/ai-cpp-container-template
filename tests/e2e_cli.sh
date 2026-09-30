#!/usr/bin/env bash

set -euo pipefail

output="$({
  printf '%s\n' '5' '4' 'impossible' 'easy' 'help' 'set' 'quit'
} | ./app --seed 5)"

require_output() {
  local expected="$1"
  if [[ "$output" != *"$expected"* ]]; then
    echo "E2E failure: expected output containing: $expected" >&2
    exit 1
  fi
}

require_output "Size must be 4, 9, or 16"
require_output "Difficulty must be easy, medium, or hard"
require_output "set <row> <column> <value>"
require_output "Usage: set"
require_output "Goodbye"

eof_output="$(printf '%s\n' '4' 'easy' | ./app --seed 5)"
if [[ "$eof_output" != *"Goodbye"* ]]; then
  echo "E2E failure: EOF did not exit cleanly" >&2
  exit 1
fi

if ./app --seed invalid >/dev/null 2>&1; then
  echo "E2E failure: invalid seed unexpectedly succeeded" >&2
  exit 1
fi

printf '' | ./app --seed 0 >/dev/null
printf '' | ./app --seed 4294967295 >/dev/null

if ./app --seed 1 extra >/dev/null 2>&1; then
  echo "E2E failure: extra arguments unexpectedly succeeded" >&2
  exit 1
fi

echo "PASS: command-line end-to-end flow"
