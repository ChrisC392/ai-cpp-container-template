# Spec: Interactive Terminal Sudoku UI

## Objective

Upgrade real terminal sessions from command entry to a keyboard-driven Sudoku
board. Players navigate with arrow keys, see the selected cell with a blue
background and brackets, enter values directly, clear editable cells, and keep
all existing Sudoku validation and completion behavior.

Piped or redirected input continues to use the existing line-command interface
so automated scripts and non-interactive environments remain supported.

## Controls

- Arrow keys: move selection one cell, clamped at board edges
- 1-9 and A-G: enter a value supported by the selected board
- Backspace, Delete, or 0: clear the selected editable cell
- Enter: check puzzle completion
- ?: show control help
- Q or Ctrl-C: quit cleanly

Original clues cannot be changed. Invalid values and Sudoku conflicts leave the
board unchanged and display a status message.

## Terminal Behavior

- Raw mode is enabled only when stdin and stdout are TTYs.
- The original terminal settings are restored through RAII on every normal
  return and exception path.
- Signal-generating input is disabled while raw mode is active so Ctrl-C can be
  handled as a normal quit key and terminal restoration still runs.
- Each frame clears and redraws the board from the home position.
- The selected cell uses ANSI blue background plus visible brackets; selection
  therefore does not rely on color alone.
- The text cursor is hidden during play and restored on exit.
- If raw mode cannot be enabled, the program reports the issue instead of
  leaving a partially configured terminal.

## Project Structure

- terminal_ui.hpp/.cpp: key decoding, TTY detection, and raw-mode RAII
- game.hpp/.cpp: shared game interface and redirected-input command mode
- interactive_game.cpp: keyboard-driven session, ANSI rendering, and Sudoku
  actions
- tests/test_main.cpp: key, movement, rendering, validation, and restoration
  behavior tests
- tests/e2e_cli.sh: existing redirected-input compatibility tests

## Testing Strategy

- Unit-test ANSI escape decoding for all arrows and Delete.
- Unit-test direct values, Enter, help, quit, clear, unknown input, and EOF.
- Drive the interactive session through injected string streams.
- Verify movement clamps at edges and changes the highlighted cell.
- Verify blue ANSI styling and bracket selection are emitted together.
- Verify clues and conflicts are rejected without board mutation.
- Verify valid input, clearing, checking, quitting, and victory.
- Keep every existing line-mode and generator test green.
- Perform a real PTY smoke test to verify raw mode and terminal restoration.

## Boundaries

- Always: restore terminal state, preserve non-TTY fallback, validate every
  value, and keep selection visible without color.
- Ask first: add ncurses or another dependency, change puzzle rules, or remove
  the command-mode fallback.
- Never: leave the terminal in raw mode, interpret partial escape sequences as
  values, or permit edits to original clues.

## Success Criteria

- Arrow keys move a visible selection between cells.
- The selected cell has a blue indicator and brackets.
- Direct entry and clearing work on editable cells.
- Invalid edits produce feedback without changing the board.
- Q, Ctrl-C, EOF, and victory all restore terminal state.
- Existing automation remains compatible and all tests pass.

## Open Questions

None. The implementation uses the confirmed arrow-key and blue-selection
interaction with the control conflict resolved by assigning Enter to check.
