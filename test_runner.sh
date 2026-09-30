#!/usr/bin/env bash

set -euo pipefail

readonly compiler_flags=(
  -std=c++17
  -Wall
  -Wextra
  -Wpedantic
  -Werror
)

g++ "${compiler_flags[@]}" main.cpp board.cpp solver.cpp puzzle_generator.cpp \
  game.cpp interactive_game.cpp terminal_ui.cpp -o app
g++ "${compiler_flags[@]}" -I. tests/test_main.cpp board.cpp solver.cpp \
  puzzle_generator.cpp game.cpp interactive_game.cpp terminal_ui.cpp -o test_app

./test_app
bash tests/e2e_cli.sh
