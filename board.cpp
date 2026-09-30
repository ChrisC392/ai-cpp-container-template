#include "board.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

Board::Board(int size) : size_(size), boxSize_(0), cells_() {
  if (!isSupportedSize(size)) {
    throw std::invalid_argument("board size must be 4, 9, or 16");
  }

  boxSize_ = size == 4 ? 2 : (size == 9 ? 3 : 4);
  cells_.assign(static_cast<std::size_t>(size_ * size_), 0);
}

bool Board::isSupportedSize(int size) {
  return size == 4 || size == 9 || size == 16;
}

char Board::symbolForValue(int value) {
  if (value == 0) {
    return '.';
  }
  if (value >= 1 && value <= 9) {
    return static_cast<char>('0' + value);
  }
  if (value >= 10 && value <= 16) {
    return static_cast<char>('A' + value - 10);
  }
  throw std::invalid_argument("cell value has no display symbol");
}

int Board::valueForSymbol(char symbol) {
  const unsigned char raw = static_cast<unsigned char>(symbol);
  const char upper = static_cast<char>(std::toupper(raw));
  if (upper == '.' || upper == '0') {
    return 0;
  }
  if (upper >= '1' && upper <= '9') {
    return upper - '0';
  }
  if (upper >= 'A' && upper <= 'G') {
    return upper - 'A' + 10;
  }
  return -1;
}

int Board::size() const { return size_; }

int Board::boxSize() const { return boxSize_; }

int Board::at(int row, int column) const {
  return cells_[static_cast<std::size_t>(index(row, column))];
}

void Board::set(int row, int column, int value) {
  const int cellIndex = index(row, column);
  if (value < 0 || value > size_) {
    throw std::invalid_argument("cell value is outside the board range");
  }
  cells_[static_cast<std::size_t>(cellIndex)] = value;
}

void Board::clear(int row, int column) { set(row, column, 0); }

bool Board::isValidPlacement(int row, int column, int value) const {
  index(row, column);
  if (value < 1 || value > size_) {
    return false;
  }

  for (int offset = 0; offset < size_; ++offset) {
    if (offset != column && at(row, offset) == value) {
      return false;
    }
    if (offset != row && at(offset, column) == value) {
      return false;
    }
  }

  const int firstRow = (row / boxSize_) * boxSize_;
  const int firstColumn = (column / boxSize_) * boxSize_;
  for (int boxRow = firstRow; boxRow < firstRow + boxSize_; ++boxRow) {
    for (int boxColumn = firstColumn;
         boxColumn < firstColumn + boxSize_; ++boxColumn) {
      if ((boxRow != row || boxColumn != column) &&
          at(boxRow, boxColumn) == value) {
        return false;
      }
    }
  }

  return true;
}

std::uint32_t Board::candidateMask(int row, int column) const {
  if (at(row, column) != 0) {
    return 0;
  }

  std::uint32_t candidates = 0;
  for (int value = 1; value <= size_; ++value) {
    if (isValidPlacement(row, column, value)) {
      candidates |= std::uint32_t{1} << (value - 1);
    }
  }
  return candidates;
}

bool Board::isComplete() const {
  return std::none_of(cells_.begin(), cells_.end(),
                      [](int value) { return value == 0; });
}

bool Board::isValid() const {
  for (int row = 0; row < size_; ++row) {
    for (int column = 0; column < size_; ++column) {
      const int value = at(row, column);
      if (value != 0 && !isValidPlacement(row, column, value)) {
        return false;
      }
    }
  }
  return true;
}

int Board::clueCount() const {
  return static_cast<int>(std::count_if(cells_.begin(), cells_.end(),
                                        [](int value) { return value != 0; }));
}

bool Board::operator==(const Board& other) const {
  return size_ == other.size_ && cells_ == other.cells_;
}

bool Board::operator!=(const Board& other) const { return !(*this == other); }

int Board::index(int row, int column) const {
  if (row < 0 || row >= size_ || column < 0 || column >= size_) {
    throw std::out_of_range("cell coordinates are outside the board");
  }
  return row * size_ + column;
}
