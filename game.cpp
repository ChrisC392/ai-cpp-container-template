#include "game.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace {

std::string lowercase(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](char character) {
    return static_cast<char>(
        std::tolower(static_cast<unsigned char>(character)));
  });
  return text;
}

std::vector<std::string> tokensFrom(const std::string& line) {
  std::istringstream stream(line);
  std::vector<std::string> tokens;
  std::string token;
  while (stream >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

bool parseInteger(const std::string& token, int& value) {
  if (token.empty()) {
    return false;
  }
  const char* const first = token.data();
  const char* const last = first + token.size();
  const auto result = std::from_chars(first, last, value);
  return result.ec == std::errc{} && result.ptr == last;
}

}  // namespace

Game::Game(std::istream& input, std::ostream& output,
           std::mt19937& randomEngine)
    : input_(input), output_(output), randomEngine_(randomEngine) {}

int Game::run() {
  output_ << "Command-Line Sudoku\n";
  int size = 0;
  if (!chooseSize(size)) {
    return 0;
  }

  Difficulty difficulty = Difficulty::Easy;
  if (!chooseDifficulty(difficulty)) {
    return 0;
  }

  output_ << "Generating puzzle...\n";
  PuzzleGenerator generator(randomEngine_);
  Puzzle puzzle = generator.generate(size, difficulty);
  if (&input_ == &std::cin && &output_ == &std::cout &&
      standardStreamsAreInteractive()) {
    RawTerminalMode rawTerminal;
    return playInteractive(puzzle);
  }
  return play(puzzle);
}

int Game::play(Puzzle puzzle) {
  Board current = puzzle.startingBoard;
  printHelp();
  render(current);

  std::string line;
  while (true) {
    output_ << "> ";
    if (!std::getline(input_, line)) {
      output_ << "\nGoodbye.\n";
      return 0;
    }

    const std::vector<std::string> tokens = tokensFrom(line);
    if (tokens.empty()) {
      output_ << "Enter a command, or type help.\n";
      continue;
    }

    const std::string command = lowercase(tokens[0]);
    if (command == "quit") {
      if (tokens.size() != 1) {
        output_ << "Usage: quit\n";
        continue;
      }
      output_ << "Goodbye.\n";
      return 0;
    }
    if (command == "help") {
      if (tokens.size() != 1) {
        output_ << "Usage: help\n";
      } else {
        printHelp();
      }
      continue;
    }
    if (command == "check") {
      if (tokens.size() != 1) {
        output_ << "Usage: check\n";
      } else if (current == puzzle.solution) {
        output_ << "Puzzle is solved.\n";
      } else {
        output_ << "Puzzle is incomplete.\n";
      }
      continue;
    }

    if (command == "set") {
      if (tokens.size() != 4) {
        output_ << "Usage: set <row> <column> <value>\n";
        continue;
      }

      int row = 0;
      int column = 0;
      if (!parseInteger(tokens[1], row) || !parseInteger(tokens[2], column) ||
          row < 1 || row > current.size() || column < 1 ||
          column > current.size()) {
        output_ << "Row and column must be between 1 and " << current.size()
                << ".\n";
        continue;
      }
      if (tokens[3].size() != 1) {
        output_ << "Value must be one symbol valid for this board.\n";
        continue;
      }
      const int value = Board::valueForSymbol(tokens[3][0]);
      if (value < 1 || value > current.size()) {
        output_ << "Value must be between 1 and "
                << Board::symbolForValue(current.size()) << ".\n";
        continue;
      }

      --row;
      --column;
      if (puzzle.startingBoard.at(row, column) != 0) {
        output_ << "That cell is an original clue and cannot be changed.\n";
        continue;
      }
      if (!current.isValidPlacement(row, column, value)) {
        output_ << "That value conflicts with its row, column, or box.\n";
        continue;
      }

      current.set(row, column, value);
      output_ << "Value placed.\n";
      render(current);
      if (current == puzzle.solution) {
        output_ << "Congratulations! You solved the puzzle.\n";
        return 0;
      }
      continue;
    }

    if (command == "clear") {
      if (tokens.size() != 3) {
        output_ << "Usage: clear <row> <column>\n";
        continue;
      }

      int row = 0;
      int column = 0;
      if (!parseInteger(tokens[1], row) || !parseInteger(tokens[2], column) ||
          row < 1 || row > current.size() || column < 1 ||
          column > current.size()) {
        output_ << "Row and column must be between 1 and " << current.size()
                << ".\n";
        continue;
      }

      --row;
      --column;
      if (puzzle.startingBoard.at(row, column) != 0) {
        output_ << "That cell is an original clue and cannot be changed.\n";
        continue;
      }
      current.clear(row, column);
      output_ << "Cell cleared.\n";
      render(current);
      continue;
    }

    output_ << "Unknown command. Type help for available commands.\n";
  }
}

bool Game::chooseSize(int& size) {
  std::string line;
  while (true) {
    output_ << "Choose board size (4, 9, or 16): ";
    if (!std::getline(input_, line)) {
      output_ << "\nGoodbye.\n";
      return false;
    }
    const std::vector<std::string> tokens = tokensFrom(line);
    if (tokens.size() == 1 && parseInteger(tokens[0], size) &&
        Board::isSupportedSize(size)) {
      return true;
    }
    output_ << "Size must be 4, 9, or 16.\n";
  }
}

bool Game::chooseDifficulty(Difficulty& difficulty) {
  std::string line;
  while (true) {
    output_ << "Choose difficulty (easy, medium, or hard): ";
    if (!std::getline(input_, line)) {
      output_ << "\nGoodbye.\n";
      return false;
    }
    const std::vector<std::string> tokens = tokensFrom(line);
    if (tokens.size() == 1) {
      const std::string choice = lowercase(tokens[0]);
      if (choice == "easy") {
        difficulty = Difficulty::Easy;
        return true;
      }
      if (choice == "medium") {
        difficulty = Difficulty::Medium;
        return true;
      }
      if (choice == "hard") {
        difficulty = Difficulty::Hard;
        return true;
      }
    }
    output_ << "Difficulty must be easy, medium, or hard.\n";
  }
}

void Game::render(const Board& board) {
  output_ << "\n    ";
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
      output_ << ' ' << Board::symbolForValue(board.at(row, column)) << ' ';
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
  output_ << '\n';
}

void Game::printHelp() {
  output_ << "Commands:\n"
          << "  set <row> <column> <value>\n"
          << "  clear <row> <column>\n"
          << "  check\n"
          << "  help\n"
          << "  quit\n";
}
