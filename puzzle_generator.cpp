#include "puzzle_generator.hpp"

#include "solver.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace {

constexpr std::size_t kUniquenessNodeBudget = 5000;

std::vector<int> shuffledGroupIndexes(int size, int groupSize,
                                      std::mt19937& randomEngine) {
  std::vector<int> groups(static_cast<std::size_t>(groupSize));
  std::iota(groups.begin(), groups.end(), 0);
  std::shuffle(groups.begin(), groups.end(), randomEngine);

  std::vector<int> indexes;
  indexes.reserve(static_cast<std::size_t>(size));
  for (const int group : groups) {
    std::vector<int> offsets(static_cast<std::size_t>(groupSize));
    std::iota(offsets.begin(), offsets.end(), 0);
    std::shuffle(offsets.begin(), offsets.end(), randomEngine);
    for (const int offset : offsets) {
      indexes.push_back(group * groupSize + offset);
    }
  }
  return indexes;
}

Board generateCompletedBoard(int size, std::mt19937& randomEngine) {
  Board solution(size);
  const int boxSize = solution.boxSize();
  const std::vector<int> rows =
      shuffledGroupIndexes(size, boxSize, randomEngine);
  const std::vector<int> columns =
      shuffledGroupIndexes(size, boxSize, randomEngine);

  std::vector<int> symbols(static_cast<std::size_t>(size));
  std::iota(symbols.begin(), symbols.end(), 1);
  std::shuffle(symbols.begin(), symbols.end(), randomEngine);
  const bool transpose =
      std::uniform_int_distribution<int>(0, 1)(randomEngine) != 0;

  for (int row = 0; row < size; ++row) {
    for (int column = 0; column < size; ++column) {
      const int sourceRow = transpose ? columns[static_cast<std::size_t>(column)]
                                      : rows[static_cast<std::size_t>(row)];
      const int sourceColumn = transpose ? rows[static_cast<std::size_t>(row)]
                                         : columns[static_cast<std::size_t>(column)];
      const int patternIndex =
          (sourceRow * boxSize + sourceRow / boxSize + sourceColumn) % size;
      solution.set(row, column, symbols[static_cast<std::size_t>(patternIndex)]);
    }
  }
  return solution;
}

}  // namespace

PuzzleGenerator::PuzzleGenerator(std::mt19937& randomEngine, int maxAttempts)
    : randomEngine_(randomEngine), maxAttempts_(maxAttempts) {
  if (maxAttempts < 0) {
    throw std::invalid_argument("maximum attempts cannot be negative");
  }
}

Puzzle PuzzleGenerator::generate(int size, Difficulty difficulty) {
  if (!Board::isSupportedSize(size)) {
    throw std::invalid_argument("puzzle size must be 4, 9, or 16");
  }

  const int target = targetClueCount(size, difficulty);
  const Solver solver;
  for (int attempt = 0; attempt < maxAttempts_; ++attempt) {
    const Board solution = generateCompletedBoard(size, randomEngine_);

    Board startingBoard = solution;
    std::vector<int> positions(static_cast<std::size_t>(size * size));
    std::iota(positions.begin(), positions.end(), 0);
    std::shuffle(positions.begin(), positions.end(), randomEngine_);

    for (const int position : positions) {
      if (startingBoard.clueCount() == target) {
        break;
      }

      const int row = position / size;
      const int column = position % size;
      const int previous = startingBoard.at(row, column);
      startingBoard.clear(row, column);
      const SolutionCountResult count = solver.countSolutionsWithBudget(
          startingBoard, 2, kUniquenessNodeBudget);
      if (count.budgetExceeded || count.count != 1) {
        startingBoard.set(row, column, previous);
      }
    }

    if (startingBoard.clueCount() == target) {
      const SolutionCountResult finalCount = solver.countSolutionsWithBudget(
          startingBoard, 2, kUniquenessNodeBudget);
      if (!finalCount.budgetExceeded && finalCount.count == 1) {
        return Puzzle{startingBoard, solution};
      }
    }
  }

  throw std::runtime_error("unable to generate a unique puzzle at the target difficulty");
}

int PuzzleGenerator::targetClueCount(int size, Difficulty difficulty) {
  if (!Board::isSupportedSize(size)) {
    throw std::invalid_argument("puzzle size must be 4, 9, or 16");
  }

  int percentage = 0;
  switch (difficulty) {
    case Difficulty::Easy:
      percentage = 60;
      break;
    case Difficulty::Medium:
      percentage = 50;
      break;
    case Difficulty::Hard:
      percentage = 40;
      break;
    default:
      throw std::invalid_argument("unsupported puzzle difficulty");
  }
  const int cells = size * size;
  return (cells * percentage + 99) / 100;
}
