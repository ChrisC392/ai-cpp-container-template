#include "game.hpp"

#include <iomanip>
#include <ostream>
#include <string>

namespace {

class CursorVisibilityGuard {
public:
  explicit CursorVisibilityGuard(std::ostream& output) : output_(output) {
    output_ << "\x1b[?25l";
  }

  ~CursorVisibilityGuard() {
    output_ << "\x1b[0m\x1b[?25h" << std::flush;
  }

  CursorVisibilityGuard(const CursorVisibilityGuard&) = delete;
  CursorVisibilityGuard& operator=(const CursorVisibilityGuard&) = delete;

private:
  std::ostream& output_;
};

}  // namespace

int Game::playInteractive(Puzzle puzzle) {
  Board current = puzzle.startingBoard;
  CellSelection selection;
  std::string status = "Use the arrow keys to choose a cell.";
  CursorVisibilityGuard cursorGuard(output_);

  while (true) {
    renderInteractive(current, puzzle.startingBoard, selection, status);
    const TerminalKey key = readTerminalKey(input_);

    if (key.type == TerminalKeyType::EndOfInput ||
        key.type == TerminalKeyType::Quit) {
      output_ << "Goodbye.\n";
      return 0;
    }

    if (key.type == TerminalKeyType::Up ||
        key.type == TerminalKeyType::Down ||
        key.type == TerminalKeyType::Left ||
        key.type == TerminalKeyType::Right) {
      selection.move(key.type, current.size());
      status = "Selected row " + std::to_string(selection.row() + 1) +
               ", column " + std::to_string(selection.column() + 1) + ".";
      continue;
    }

    const int row = selection.row();
    const int column = selection.column();
    if (key.type == TerminalKeyType::Value) {
      const int value = Board::valueForSymbol(key.character);
      if (value < 1 || value > current.size()) {
        status = "That value is not valid for this board.";
      } else if (puzzle.startingBoard.at(row, column) != 0) {
        status = "That cell is an original clue and cannot be changed.";
      } else if (!current.isValidPlacement(row, column, value)) {
        status = "That value conflicts with its row, column, or box.";
      } else {
        current.set(row, column, value);
        status = "Value placed.";
        if (current == puzzle.solution) {
          renderInteractive(current, puzzle.startingBoard, selection,
                            "Puzzle solved.");
          output_ << "Congratulations! You solved the puzzle.\n";
          return 0;
        }
      }
      continue;
    }

    if (key.type == TerminalKeyType::Clear) {
      if (puzzle.startingBoard.at(row, column) != 0) {
        status = "That cell is an original clue and cannot be changed.";
      } else {
        current.clear(row, column);
        status = "Cell cleared.";
      }
      continue;
    }

    if (key.type == TerminalKeyType::Check) {
      status = current == puzzle.solution ? "Puzzle is solved."
                                          : "Puzzle is incomplete.";
      continue;
    }

    if (key.type == TerminalKeyType::Help) {
      status = "Arrow keys move; 1-9/A-G enter; Backspace clears; "
               "Enter checks; Q quits.";
      continue;
    }

    status = "Unsupported key. Press ? for help.";
  }
}

void Game::renderInteractive(const Board& board, const Board& startingBoard,
                             const CellSelection& selection,
                             const std::string& status) {
  output_ << "\x1b[2J\x1b[H"
          << "Sudoku  |  Arrow keys move  |  Values enter  |  Q quits\n\n"
          << "    ";
  for (int column = 1; column <= board.size(); ++column) {
    output_ << std::setw(2) << column << ' ';
    if (column % board.boxSize() == 0 && column != board.size()) {
      output_ << ' ';
    }
  }
  output_ << '\n';

  for (int row = 0; row < board.size(); ++row) {
    if (row % board.boxSize() == 0) {
      output_ << "   +";
      for (int box = 0; box < board.boxSize(); ++box) {
        output_ << std::string(static_cast<std::size_t>(board.boxSize() * 3),
                               '-')
                << '+';
      }
      output_ << '\n';
    }
    output_ << std::setw(2) << row + 1 << " |";
    for (int column = 0; column < board.size(); ++column) {
      const char symbol = Board::symbolForValue(board.at(row, column));
      if (row == selection.row() && column == selection.column()) {
        output_ << "\x1b[44m\x1b[97m[" << symbol << "]\x1b[0m";
      } else {
        output_ << ' ' << symbol << ' ';
      }
      if ((column + 1) % board.boxSize() == 0) {
        output_ << '|';
      }
    }
    output_ << '\n';
  }

  output_ << "   +";
  for (int box = 0; box < board.boxSize(); ++box) {
    output_ << std::string(static_cast<std::size_t>(board.boxSize() * 3), '-')
            << '+';
  }
  const bool isClue =
      startingBoard.at(selection.row(), selection.column()) != 0;
  output_ << "\n\nSelected: row " << selection.row() + 1 << ", column "
          << selection.column() + 1
          << (isClue ? " (original clue)" : " (editable)")
          << "\nStatus: " << status
          << "\nControls: arrows move | 1-9/A-G enter | Backspace/Delete/0 clear"
          << " | Enter check | ? help | Q quit\n"
          << std::flush;
}
