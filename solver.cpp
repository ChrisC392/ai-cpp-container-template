#include "solver.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

int bitCount(std::uint32_t mask) {
  int count = 0;
  while (mask != 0) {
    count += static_cast<int>(mask & 1U);
    mask >>= 1U;
  }
  return count;
}

class SearchState {
public:
  explicit SearchState(const Board& source)
      : board(source),
        fullMask((std::uint32_t{1} << source.size()) - 1U),
        rowMasks(static_cast<std::size_t>(source.size()), 0),
        columnMasks(static_cast<std::size_t>(source.size()), 0),
        boxMasks(static_cast<std::size_t>(source.size()), 0) {
    for (int row = 0; row < board.size(); ++row) {
      for (int column = 0; column < board.size(); ++column) {
        const int value = board.at(row, column);
        if (value != 0) {
          add(row, column, value);
        }
      }
    }
  }

  std::uint32_t candidates(int row, int column) const {
    const std::uint32_t used =
        rowMasks[static_cast<std::size_t>(row)] |
        columnMasks[static_cast<std::size_t>(column)] |
        boxMasks[static_cast<std::size_t>(boxIndex(row, column))];
    return fullMask & ~used;
  }

  void place(int row, int column, int value) {
    board.set(row, column, value);
    add(row, column, value);
  }

  void remove(int row, int column, int value) {
    const std::uint32_t bit = std::uint32_t{1} << (value - 1);
    rowMasks[static_cast<std::size_t>(row)] &= ~bit;
    columnMasks[static_cast<std::size_t>(column)] &= ~bit;
    boxMasks[static_cast<std::size_t>(boxIndex(row, column))] &= ~bit;
    board.clear(row, column);
  }

  Board board;

private:
  int boxIndex(int row, int column) const {
    return (row / board.boxSize()) * board.boxSize() +
           column / board.boxSize();
  }

  void add(int row, int column, int value) {
    const std::uint32_t bit = std::uint32_t{1} << (value - 1);
    rowMasks[static_cast<std::size_t>(row)] |= bit;
    columnMasks[static_cast<std::size_t>(column)] |= bit;
    boxMasks[static_cast<std::size_t>(boxIndex(row, column))] |= bit;
  }

  std::uint32_t fullMask;
  std::vector<std::uint32_t> rowMasks;
  std::vector<std::uint32_t> columnMasks;
  std::vector<std::uint32_t> boxMasks;
};

// Array indexes act as Dancing Links pointers. Header 0 owns the horizontal
// column list; each candidate assignment links one node into each of Sudoku's
// four exact-cover constraints. cover/uncover must restore every link exactly.
class ExactCoverCounter {
public:
  ExactCoverCounter(const Board& board, int limit, std::size_t nodeBudget)
      : limit_(limit), solutions_(0), nodeBudget_(nodeBudget), visitedNodes_(0),
        budgetExceeded_(false), columnSizes_(), left_(), right_(), up_(),
        down_(), columns_() {
    const int constraintCount = 4 * board.size() * board.size();
    initializeHeaders(constraintCount);
    addCandidates(board);
  }

  int count() {
    search();
    return solutions_;
  }

  bool budgetExceeded() const { return budgetExceeded_; }

private:
  void initializeHeaders(int constraintCount) {
    const int headerCount = constraintCount + 1;
    columnSizes_.assign(static_cast<std::size_t>(headerCount), 0);
    left_.resize(static_cast<std::size_t>(headerCount));
    right_.resize(static_cast<std::size_t>(headerCount));
    up_.resize(static_cast<std::size_t>(headerCount));
    down_.resize(static_cast<std::size_t>(headerCount));
    columns_.resize(static_cast<std::size_t>(headerCount));

    for (int header = 0; header < headerCount; ++header) {
      left_[static_cast<std::size_t>(header)] =
          header == 0 ? constraintCount : header - 1;
      right_[static_cast<std::size_t>(header)] =
          header == constraintCount ? 0 : header + 1;
      up_[static_cast<std::size_t>(header)] = header;
      down_[static_cast<std::size_t>(header)] = header;
      columns_[static_cast<std::size_t>(header)] = header;
    }
  }

  void addCandidates(const Board& board) {
    const int size = board.size();
    for (int row = 0; row < size; ++row) {
      for (int column = 0; column < size; ++column) {
        const int fixedValue = board.at(row, column);
        for (int value = 1; value <= size; ++value) {
          if ((fixedValue != 0 && value != fixedValue) ||
              (fixedValue == 0 &&
               !board.isValidPlacement(row, column, value))) {
            continue;
          }
          addAssignment(board, row, column, value);
        }
      }
    }
  }

  void addAssignment(const Board& board, int row, int column, int value) {
    const int size = board.size();
    const int valueIndex = value - 1;
    const int box = (row / board.boxSize()) * board.boxSize() +
                    column / board.boxSize();
    const int cellConstraint = row * size + column;
    const int rowConstraint = size * size + row * size + valueIndex;
    const int columnConstraint =
        2 * size * size + column * size + valueIndex;
    const int boxConstraint = 3 * size * size + box * size + valueIndex;
    addRow({cellConstraint, rowConstraint, columnConstraint, boxConstraint});
  }

  void addRow(const std::vector<int>& constraintIndexes) {
    int firstNode = -1;
    int previousNode = -1;
    for (const int constraintIndex : constraintIndexes) {
      const int header = constraintIndex + 1;
      const int node = static_cast<int>(columns_.size());
      columns_.push_back(header);
      columnSizes_.push_back(0);
      left_.push_back(node);
      right_.push_back(node);
      up_.push_back(up_[static_cast<std::size_t>(header)]);
      down_.push_back(header);

      down_[static_cast<std::size_t>(up_[static_cast<std::size_t>(header)])] =
          node;
      up_[static_cast<std::size_t>(header)] = node;
      ++columnSizes_[static_cast<std::size_t>(header)];

      if (firstNode == -1) {
        firstNode = node;
      } else {
        left_[static_cast<std::size_t>(node)] = previousNode;
        right_[static_cast<std::size_t>(previousNode)] = node;
      }
      previousNode = node;
    }
    left_[static_cast<std::size_t>(firstNode)] = previousNode;
    right_[static_cast<std::size_t>(previousNode)] = firstNode;
  }

  void cover(int header) {
    right_[static_cast<std::size_t>(left_[static_cast<std::size_t>(header)])] =
        right_[static_cast<std::size_t>(header)];
    left_[static_cast<std::size_t>(right_[static_cast<std::size_t>(header)])] =
        left_[static_cast<std::size_t>(header)];
    for (int rowNode = down_[static_cast<std::size_t>(header)];
         rowNode != header;
         rowNode = down_[static_cast<std::size_t>(rowNode)]) {
      for (int node = right_[static_cast<std::size_t>(rowNode)];
           node != rowNode; node = right_[static_cast<std::size_t>(node)]) {
        down_[static_cast<std::size_t>(up_[static_cast<std::size_t>(node)])] =
            down_[static_cast<std::size_t>(node)];
        up_[static_cast<std::size_t>(down_[static_cast<std::size_t>(node)])] =
            up_[static_cast<std::size_t>(node)];
        --columnSizes_[static_cast<std::size_t>(
            columns_[static_cast<std::size_t>(node)])];
      }
    }
  }

  void uncover(int header) {
    for (int rowNode = up_[static_cast<std::size_t>(header)];
         rowNode != header;
         rowNode = up_[static_cast<std::size_t>(rowNode)]) {
      for (int node = left_[static_cast<std::size_t>(rowNode)];
           node != rowNode; node = left_[static_cast<std::size_t>(node)]) {
        ++columnSizes_[static_cast<std::size_t>(
            columns_[static_cast<std::size_t>(node)])];
        down_[static_cast<std::size_t>(up_[static_cast<std::size_t>(node)])] =
            node;
        up_[static_cast<std::size_t>(down_[static_cast<std::size_t>(node)])] =
            node;
      }
    }
    right_[static_cast<std::size_t>(left_[static_cast<std::size_t>(header)])] =
        header;
    left_[static_cast<std::size_t>(right_[static_cast<std::size_t>(header)])] =
        header;
  }

  int smallestColumn() const {
    int best = right_[0];
    for (int header = right_[0]; header != 0;
         header = right_[static_cast<std::size_t>(header)]) {
      if (columnSizes_[static_cast<std::size_t>(header)] <
          columnSizes_[static_cast<std::size_t>(best)]) {
        best = header;
      }
    }
    return best;
  }

  void search() {
    if (solutions_ >= limit_ || budgetExceeded_) {
      return;
    }
    if (visitedNodes_ >= nodeBudget_) {
      budgetExceeded_ = true;
      return;
    }
    ++visitedNodes_;
    if (right_[0] == 0) {
      ++solutions_;
      return;
    }

    const int header = smallestColumn();
    if (columnSizes_[static_cast<std::size_t>(header)] == 0) {
      return;
    }
    cover(header);
    for (int rowNode = down_[static_cast<std::size_t>(header)];
         rowNode != header && solutions_ < limit_ && !budgetExceeded_;
         rowNode = down_[static_cast<std::size_t>(rowNode)]) {
      for (int node = right_[static_cast<std::size_t>(rowNode)];
           node != rowNode; node = right_[static_cast<std::size_t>(node)]) {
        cover(columns_[static_cast<std::size_t>(node)]);
      }
      search();
      for (int node = left_[static_cast<std::size_t>(rowNode)];
           node != rowNode; node = left_[static_cast<std::size_t>(node)]) {
        uncover(columns_[static_cast<std::size_t>(node)]);
      }
    }
    uncover(header);
  }

  int limit_;
  int solutions_;
  std::size_t nodeBudget_;
  std::size_t visitedNodes_;
  bool budgetExceeded_;
  std::vector<int> columnSizes_;
  std::vector<int> left_;
  std::vector<int> right_;
  std::vector<int> up_;
  std::vector<int> down_;
  std::vector<int> columns_;
};

bool findBestEmptyCell(const SearchState& state, int& row, int& column,
                       std::uint32_t& candidates) {
  int bestCount = std::numeric_limits<int>::max();
  bool foundEmpty = false;
  for (int candidateRow = 0; candidateRow < state.board.size(); ++candidateRow) {
    for (int candidateColumn = 0;
         candidateColumn < state.board.size(); ++candidateColumn) {
      if (state.board.at(candidateRow, candidateColumn) != 0) {
        continue;
      }

      foundEmpty = true;
      const std::uint32_t mask =
          state.candidates(candidateRow, candidateColumn);
      const int count = bitCount(mask);
      if (count < bestCount) {
        bestCount = count;
        row = candidateRow;
        column = candidateColumn;
        candidates = mask;
        if (count == 0) {
          return true;
        }
      }
    }
  }
  return foundEmpty;
}

std::vector<int> valuesFromMask(std::uint32_t mask, int size) {
  std::vector<int> values;
  for (int value = 1; value <= size; ++value) {
    if ((mask & (std::uint32_t{1} << (value - 1))) != 0) {
      values.push_back(value);
    }
  }
  return values;
}

bool solveRecursive(SearchState& state, std::mt19937* randomEngine) {
  int row = -1;
  int column = -1;
  std::uint32_t candidates = 0;
  if (!findBestEmptyCell(state, row, column, candidates)) {
    return true;
  }
  if (candidates == 0) {
    return false;
  }

  std::vector<int> values = valuesFromMask(candidates, state.board.size());
  if (randomEngine != nullptr) {
    std::shuffle(values.begin(), values.end(), *randomEngine);
  }
  for (const int value : values) {
    state.place(row, column, value);
    if (solveRecursive(state, randomEngine)) {
      return true;
    }
    state.remove(row, column, value);
  }
  return false;
}

}  // namespace

bool Solver::solve(Board& board, std::mt19937* randomEngine) const {
  if (!board.isValid()) {
    return false;
  }
  SearchState state(board);
  if (!solveRecursive(state, randomEngine)) {
    return false;
  }
  board = state.board;
  return true;
}

int Solver::countSolutions(const Board& board, int limit) const {
  const SolutionCountResult result = countSolutionsWithBudget(
      board, limit, std::numeric_limits<std::size_t>::max());
  return result.count;
}

SolutionCountResult Solver::countSolutionsWithBudget(
    const Board& board, int limit, std::size_t nodeBudget) const {
  if (limit <= 0) {
    throw std::invalid_argument("solution count limit must be positive");
  }
  if (!board.isValid()) {
    return SolutionCountResult{0, false};
  }
  ExactCoverCounter counter(board, limit, nodeBudget);
  const int count = counter.count();
  return SolutionCountResult{count, counter.budgetExceeded()};
}
