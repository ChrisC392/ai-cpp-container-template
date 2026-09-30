# Spec: Configurable Command-Line Sudoku Game

## Objective

Build a terminal-based Sudoku game in C++ for programming practice. The player chooses a
4x4, 9x9, or 16x16 board and an Easy, Medium, or Hard difficulty. The application generates
a random puzzle with exactly one solution, then runs an interactive command loop.

The user is a C++ learner building this project for practice. Success means the complete game
supports the confirmed sizes and difficulties, generates only uniquely solvable puzzles,
handles terminal input safely, and passes the automated checks defined in this specification.
The project emphasizes C++ fundamentals, object-oriented design, backtracking, random-number
generation, input validation, and automated tests. It uses C++17 and the standard library
only.

## User Story

As a C++ learner,
I want to generate and play Sudoku puzzles of several sizes and difficulties in a terminal,
so that I can practice designing classes, implementing algorithms, validating input, and
testing a complete application.

## Problem Statement

The repository is currently a minimal C++ template. main.cpp does nothing, and there is no
Sudoku model, solver, generator, terminal interface, or meaningful test suite.

The implementation must do more than produce a filled grid. Each generated puzzle must obey
the selected board's row, column, and subgrid rules and have exactly one solution. Supporting
16x16 makes naive exhaustive search potentially slow, so the design must reduce unnecessary
search and always terminate generation after a bounded number of attempts.

## Solution Statement

Implement five focused components:

1. Board stores an N-by-N grid, validates dimensions and placements, and converts values to
   display symbols.
2. Solver uses recursive backtracking with minimum-remaining-values (MRV) cell selection to
   find a solution and an exact-cover counter to enumerate solutions up to a supplied limit.
3. PuzzleGenerator creates a completed board from a valid generalized Sudoku pattern and
   randomizes it through symbol, band/row, stack/column, and transpose permutations. It then
   removes clues in shuffled order and retains a removal only when the remaining puzzle still
   has exactly one solution.
4. Game handles rendering, menus, command parsing, immutable clues, and the play loop.
5. main.cpp creates the random engine and starts Game.

Cells are stored as integers, with 0 representing empty. The terminal displays 1 through 9
normally and uses A through G for values 10 through 16. Player coordinates are 1-based.

Difficulty is a documented clue-density approximation:

| Difficulty | Target given cells | Meaning |
|---|---:|---|
| Easy | ceil(N*N*0.60) | More starting information |
| Medium | ceil(N*N*0.50) | Moderate starting information |
| Hard | ceil(N*N*0.40) | Less starting information |

These labels do not claim to measure human solving techniques. Generation retries from a new
completed grid when one removal pass cannot reach the target. Retries must have a fixed limit;
failure produces a clear error instead of an infinite loop.

### Gameplay Contract

- The startup menu accepts sizes 4, 9, and 16 and difficulties Easy, Medium, and Hard.
- set <row> <column> <value> places or replaces a value in a non-given cell.
- clear <row> <column> clears a non-given cell.
- check reports whether the board is incomplete or solved without revealing the solution.
- help prints the command summary.
- quit exits cleanly.
- Invalid menus and commands explain the problem and allow another attempt.
- A move is rejected if it is out of range, targets an original clue, or causes a row,
  column, or subgrid duplicate.
- A locally legal value is allowed even when it differs from the hidden solution.
- Victory is announced only when the completed board equals the unique solution.

## Tech Stack

- Language: C++17
- Dependencies: C++ standard library only
- Random engine: std::mt19937, seeded from std::random_device during normal play and from an
  explicit seed during tests
- Tests: a lightweight assertion-based C++ executable and a shell CLI end-to-end test
- Compiler: g++ with strict warnings

## Commands

~~~bash
# Build the game
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror main.cpp board.cpp solver.cpp puzzle_generator.cpp game.cpp -o app

# Build unit tests
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. tests/test_main.cpp board.cpp solver.cpp puzzle_generator.cpp game.cpp -o test_app

# Run unit tests
./test_app

# Run the CLI end-to-end test
bash tests/e2e_cli.sh

# Run all automated validation
bash test_runner.sh

# Play manually
./app
~~~

## Project Structure

~~~text
main.cpp                    Thin application entry point
board.hpp / board.cpp       Grid storage, rules, candidates, and value symbols
solver.hpp / solver.cpp     MRV backtracking and bounded solution counting
puzzle_generator.hpp/.cpp   Random solved-board and unique-puzzle generation
game.hpp / game.cpp         Terminal UI, parsing, menus, and game session
tests/test_main.cpp         Unit and integration-style tests
tests/e2e_cli.sh            Scripted terminal behavior checks
test_runner.sh              Complete build-and-test entry point
README.md                   Build, play, controls, and architecture documentation
~~~

## Code Style

- Classes use PascalCase; functions and variables use camelCase.
- Constants use kPascalCase.
- Prefer values, references, std::vector, and RAII; do not use owning raw pointers.
- Keep terminal input/output out of Board, Solver, and PuzzleGenerator.
- Only 4, 9, and 16 are valid board dimensions.
- Pass random engines by reference so tests can reproduce generated puzzles.
- Read complete input lines with std::getline and parse them without leaving std::cin in a
  failed state.

Representative interface:

~~~cpp
class Solver {
public:
    bool solve(Board& board, std::mt19937* randomEngine = nullptr) const;
    int countSolutions(const Board& board, int limit = 2) const;

private:
    bool solveRecursive(Board& board, std::mt19937* randomEngine) const;
};
~~~

## Testing Strategy

### Unit Tests

- Board: size validation, box-size derivation, bounds, cell mutations, row/column/subgrid
  conflicts, empty cells, and 1-9/A-G symbol conversion.
- Solver: completed, partial, invalid, unsolvable, unique, and ambiguous boards for the
  supported dimensions; verify that solution counting stops at its limit.
- Generator: solution validity, given/solution consistency, uniqueness, clue targets,
  deterministic seeds, bounded failures, and all nine size/difficulty combinations.
- Game: menu parsing, command parsing, immutable clues, conflict rejection, clear/check/help/
  quit, EOF, malformed input, and victory output.

Deterministic fixed seeds are the correctness baseline. A small repeated-random smoke test may
supplement them but must not be the only proof.

### End-to-End Test

tests/e2e_cli.sh launches the real executable with a fixed test seed, sends a short command
transcript, and checks stable output phrases. It covers valid startup, help, one malformed
command, and clean quit. Victory is covered through an injected-stream Game test instead of a
brittle shell transcript tied to one random puzzle.

### Edge Cases

- Unsupported sizes and difficulty values.
- Empty input, whitespace, EOF, extra tokens, negative numbers, and numeric overflow.
- Lowercase and uppercase A-G on a 16x16 board.
- Values 0, H, or values larger than the selected board.
- Clearing or replacing an original clue.
- Replacing an editable value with itself.
- Independent row, column, and subgrid conflicts.
- Full valid, full invalid, empty, unsolvable, and multiple-solution boards.
- A clue removal that introduces a second solution.
- Failure to reach a target before the generator's retry limit.
- Minimum and maximum supported test seed values.
- 16x16 separator and symbol alignment.

## Boundaries

### Always Do

- Validate every menu choice, command, coordinate, and cell value.
- Preserve original clues throughout a game.
- Stop uniqueness counting after finding two solutions.
- Bound puzzle-generation retries.
- Add tests with every behavioral slice.
- Compile with warnings treated as errors.

### Ask First

- Add a third-party library, test framework, or build system.
- Change supported board sizes or symbols.
- Add save/load, undo, hints, timers, scores, or technique-based difficulty.

### Never Do

- Accept a generated puzzle without verifying exactly one solution.
- Change a given clue during play.
- Reject an otherwise rule-valid guess merely because it differs from the stored solution.
- Use unbounded generation attempts.
- Commit generated executables or secrets.

## Relevant Files

### Existing Files

- main.cpp: replace the empty entry point with thin application composition.
- test_runner.sh: replace the interactive-only run with explicit build and automated checks.
- README.md: document setup, build, test, play, controls, symbols, and architecture.
- tests/README.md: describe unit and end-to-end test responsibilities.
- .gitignore: verify app and test_app are ignored, updating only if necessary.

### New Files

- board.hpp and board.cpp
- solver.hpp and solver.cpp
- puzzle_generator.hpp and puzzle_generator.cpp
- game.hpp and game.cpp
- tests/test_main.cpp
- tests/e2e_cli.sh

## Architecture Decisions

- Board is independent of terminal I/O.
- A generated Puzzle contains a starting Board and its solved Board. Nonzero cells in the
  starting board define the immutable clues.
- Solver chooses the empty cell with the fewest candidates (MRV) when finding a solution.
- Solution counting uses an indexed Dancing Links exact-cover matrix and stops at the caller's
  limit. This keeps repeated 16x16 uniqueness checks practical while remaining exact.
- Candidate sets use a 32-bit mask, sufficient for values 1 through 16.
- Completed boards use validity-preserving randomized permutations instead of searching an
  empty 16x16 grid. Solution counting stays deterministic.
- Generator uniqueness checks have a fixed search-node budget. Budget exhaustion is treated
  as inconclusive, so the clue is restored and never accepted as uniquely safe.
- Solver counts only up to two solutions during uniqueness checks.
- Difficulty targets live in PuzzleGenerator, not Game.
- Game receives std::istream and std::ostream references for testability.
- A small fixed command parser is preferred over a generic command framework.

## Implementation Plan

### Phase 1: Foundation

Create a repeatable automated test command, then implement the generalized Board and symbol
codec. End with clean strict-warning builds and passing rule tests for every size.

### Phase 2: Core Algorithms

Implement the MRV solver and bounded solution counter before the generator. Then implement
random complete-board creation and uniqueness-preserving clue removal. Validate all nine
size/difficulty combinations using fixed seeds.

### Phase 3: Integration

Implement terminal rendering, menus, parsing, edit protection, and completion. Wire the
components through main.cpp, add a scripted end-to-end test, update documentation, and run the
full validation suite.

## Step-by-Step Tasks

### Task 1: Establish the Test Scaffold

Description: Add a minimal assertion-based runner and make test_runner.sh compile production
and test executables explicitly, stopping on any failure.

Acceptance criteria:

- Production and test builds use C++17 and strict warnings.
- Named tests report failures and return a nonzero exit status.
- test_runner.sh does not wait for interactive input.

Verification:

- Run bash test_runner.sh with an initial smoke test.
- Confirm a deliberately failing assertion returns nonzero, then restore it.

Dependencies: None

Files: test_runner.sh, tests/test_main.cpp

Estimated scope: Small (2 files)

### Task 2: Implement the Generalized Board

Description: Add grid storage, supported-size checks, coordinate/value validation,
row/column/subgrid rules, empty-cell inspection, candidate masks, and display symbols.

Acceptance criteria:

- Sizes 4, 9, and 16 derive box sizes 2, 3, and 4.
- Placement checks detect all three kinds of duplicate.
- Cells remain in the range 0 through size.
- Symbols round-trip values 1-9 and A-G.

Verification:

- Test dimensions, bounds, mutation, clearing, conflicts, candidates, and symbols for all
  sizes.
- Run bash test_runner.sh.

Dependencies: Task 1

Files: board.hpp, board.cpp, tests/test_main.cpp

Estimated scope: Medium (3 files)

### Checkpoint: Board Foundation

- All Board tests pass.
- Board has no terminal I/O or random behavior.
- Unsupported dimensions fail in a documented and tested way.

### Task 3: Implement Solving and Solution Counting

Description: Add recursive MRV search. Support solving with optional randomized candidate
order and counting solutions up to a caller-supplied limit.

Acceptance criteria:

- Known boards of each size are solved correctly.
- Invalid or unsolvable boards report failure without corrupting caller state.
- Counting distinguishes zero, one, and at least two solutions.
- Counting stops once it reaches the supplied limit.

Verification:

- Test solved, partial, invalid, unsolvable, unique, and ambiguous boards.
- Test reproducibility with a fixed random seed.
- Run bash test_runner.sh.

Dependencies: Task 2

Files: solver.hpp, solver.cpp, tests/test_main.cpp

Estimated scope: Medium (3 files)

### Task 4: Generate Random Unique Puzzles

Description: Add Difficulty, Puzzle, and PuzzleGenerator. Randomize a valid completed-board
pattern, remove shuffled cells one at a time, retain only removals proven unique within the
search budget, and retry from a new solution when necessary.

Acceptance criteria:

- Every size/difficulty pair yields a valid puzzle and matching valid solution.
- Every puzzle has exactly one solution.
- Every given matches the stored solution.
- The documented clue target is met or a bounded, clear generation error is returned.
- Equal explicit seeds produce equal puzzles.

Verification:

- Generate every configuration with several fixed seeds and check range, target, consistency,
  validity, and uniqueness.
- Exercise the attempt-limit failure path deterministically.
- Run bash test_runner.sh.

Dependencies: Task 3

Files: puzzle_generator.hpp, puzzle_generator.cpp, tests/test_main.cpp

Estimated scope: Medium (3 files)

### Checkpoint: Puzzle Engine

- Solver and generator tests pass repeatedly.
- Every configuration has deterministic uniqueness coverage.
- 16x16 generation terminates reliably before UI work begins.

### Task 5: Implement the Terminal Game

Description: Add rendering, size/difficulty selection, line-based command parsing, the game
loop, clue protection, validation messages, and victory detection.

Acceptance criteria:

- Menus re-prompt on invalid input.
- Rendering aligns all supported sizes and shows subgrid separators.
- set, clear, check, help, and quit satisfy the gameplay contract.
- Malformed commands and clue edits never alter the board.
- Solving the puzzle prints a victory message and ends cleanly.

Verification:

- Test through string streams: bad menus, malformed commands, extra tokens, range errors,
  lowercase symbols, clue edits, conflicts, clear, check, help, quit, EOF, and victory.
- Run bash test_runner.sh.

Dependencies: Task 4

Files: game.hpp, game.cpp, main.cpp, tests/test_main.cpp

Estimated scope: Medium (4 files)

### Task 6: Add CLI E2E Coverage and Documentation

Description: Script representative input through the compiled application and document all
build, test, play, control, symbol, and architecture details.

Acceptance criteria:

- The E2E test covers startup, help, invalid input, and clean quit.
- A test-only deterministic seed mechanism does not alter normal random startup.
- README commands work in the documented container.
- Generated executables remain untracked.

Verification:

- Run bash tests/e2e_cli.sh.
- Run every README command.
- Run git status --short and confirm no binaries appear.

Dependencies: Task 5

Files: tests/e2e_cli.sh, README.md, tests/README.md, .gitignore

Estimated scope: Medium (4 files)

### Task 7: Complete Validation and Manual Play Checks

Description: Run all automated validation and manually exercise representative small and
large boards to identify display or interaction problems.

Acceptance criteria:

- All validation exits successfully with no compiler warnings.
- A 4x4 puzzle can be completed in the terminal.
- A 16x16 board is legible and accepts A-G case-insensitively.
- Repeated generated puzzles remain valid and unique.

Verification: Execute every command in Validation Commands.

Dependencies: Tasks 1-6

Files: None unless validation exposes a defect

Estimated scope: Small

### Checkpoint: Complete

- All acceptance criteria are met.
- Automated and manual checks pass.
- No generated executable is tracked.
- The implementation is ready for code review.

## Success Criteria

- The program builds as C++17 using only the standard library and emits no strict-warning
  diagnostics.
- Players can choose 4x4, 9x9, or 16x16 and Easy, Medium, or Hard.
- Every generated puzzle is valid and has exactly one solution.
- Difficulty uses the documented clue-density targets.
- The display supports 1-9 and A-G with visible subgrid separators.
- set, clear, check, help, and quit work with clear error messages.
- Original clues are immutable, and invalid/conflicting moves do not change the board.
- Locally valid guesses are allowed; victory requires the unique completed solution.
- Randomness is seed-injectable for tests.
- Generator retries are bounded and failures are reported.
- Unit tests cover Board, Solver, PuzzleGenerator, and Game.
- A separate E2E script tests the compiled interactive application.
- bash test_runner.sh runs all automated checks successfully.

## Validation Commands

Run from the repository root in this order:

~~~bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror main.cpp board.cpp solver.cpp puzzle_generator.cpp game.cpp -o app
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. tests/test_main.cpp board.cpp solver.cpp puzzle_generator.cpp game.cpp -o test_app
./test_app
bash tests/e2e_cli.sh
bash test_runner.sh
git status --short
./app
~~~

During manual validation, generate every size/difficulty combination, fully solve one 4x4
puzzle, and inspect/input values on one 16x16 puzzle. Automated tests, not manual play, remain
the proof of uniqueness.

## Risks and Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| 16x16 uniqueness checks are slow | High | Use candidate masks, MRV, a solution limit of two, timing checks, and bounded retries. |
| Clue count poorly predicts human difficulty | Medium | State the limitation and defer technique-based ratings. |
| One solved grid cannot reach the target | Medium | Retry with a new randomized solution up to a fixed maximum. |
| Random tests become flaky | Medium | Inject fixed seeds and make deterministic cases the baseline. |
| CLI parsing becomes tangled with algorithms | Medium | Keep streams in Game and domain classes independent of I/O. |
| The custom test harness grows unnecessarily | Low | Support only named checks, failure reporting, and exit status. |

## Notes

- Save/load, undo, hints, timers, scoring, symmetry, and advanced difficulty grading are future
  extensions and are out of scope.
- If measured 16x16 performance is still poor, maintain row/column/box masks incrementally
  inside Solver. Measure before adding that complexity.
- Implementation was authorized after plan approval and completed against this specification.

## Open Questions

None. The user confirmed the board sizes, unique-solution requirement, random generation,
difficulty selection, and the v1 exclusions documented above, then separately authorized
implementation. Any later scope change must update this specification before code changes
begin.
