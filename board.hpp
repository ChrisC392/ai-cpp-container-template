#pragma once

#include <cstdint>
#include <vector>

class Board {
public:
  explicit Board(int size);

  static bool isSupportedSize(int size);
  static char symbolForValue(int value);
  static int valueForSymbol(char symbol);

  int size() const;
  int boxSize() const;
  int at(int row, int column) const;
  void set(int row, int column, int value);
  void clear(int row, int column);

  bool isValidPlacement(int row, int column, int value) const;
  std::uint32_t candidateMask(int row, int column) const;
  bool isComplete() const;
  bool isValid() const;
  int clueCount() const;

  bool operator==(const Board& other) const;
  bool operator!=(const Board& other) const;

private:
  int index(int row, int column) const;

  int size_;
  int boxSize_;
  std::vector<int> cells_;
};
