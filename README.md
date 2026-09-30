# Command-Line Sudoku

A C++17 terminal Sudoku game that generates random puzzles with exactly one
solution.

## Features

- 4x4, 9x9, and 16x16 boards
- Easy, Medium, and Hard clue-density settings
- Random puzzle generation with an exact uniqueness check
- Values 1-9 and A-G on larger boards
- Input validation, immutable clues, and conflict detection
- Deterministic seeds for reproducing games and running tests

Difficulty is based on how many clues remain:

| Difficulty | Given cells |
|---|---:|
| Easy | 60% |
| Medium | 50% |
| Hard | 40% |

Clue density is only an approximation of human difficulty; the game does not
yet grade puzzles by required solving techniques.

## Build and Run

With g++ installed:

~~~bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror main.cpp board.cpp solver.cpp puzzle_generator.cpp game.cpp -o app
./app
~~~

To replay deterministic generation:

~~~bash
./app --seed 42
~~~

The accepted seed range is 0 through 4294967295.

This repository is also compatible with
[cpp-container](https://github.com/ChicoState/cpp-container):

~~~bash
docker run -v "$(pwd)":/usr/src -it cpp-container sh
~~~

Run the build commands from /usr/src inside the container.

## Playing

Choose a board size and difficulty at startup. Coordinates shown by the game
are 1-based.

~~~text
set <row> <column> <value>  Place or replace an editable value
clear <row> <column>        Clear an editable cell
check                       Check whether the puzzle is complete
help                        Show the command list
quit                        Exit the game
~~~

For 16x16 boards, use A through G for values 10 through 16. Letter input is
case-insensitive. A move that duplicates a value in its row, column, or
subgrid is rejected. A locally legal guess is allowed even if it is not the
final answer.

## Tests

Run the complete build, unit/integration suite, and CLI end-to-end flow:

~~~bash
bash test_runner.sh
~~~

Or run the checks separately:

~~~bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror main.cpp board.cpp solver.cpp puzzle_generator.cpp game.cpp -o app
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. tests/test_main.cpp board.cpp solver.cpp puzzle_generator.cpp game.cpp -o test_app
./test_app
bash tests/e2e_cli.sh
~~~

## Architecture

- Board stores cells, validates Sudoku rules, and converts display symbols.
- Solver uses MRV backtracking to find solutions and indexed Dancing Links to
  count solutions efficiently.
- PuzzleGenerator creates a randomized complete board and removes clues only
  while exact solution counting proves uniqueness.
- Game owns terminal rendering, menus, parsing, and session state.
- main.cpp parses an optional seed and starts the game.

The detailed design and acceptance criteria are in
[specs/sudoku-cli-plan.md](specs/sudoku-cli-plan.md).
