#pragma once

#include "board.hpp"

#include <random>

enum class Difficulty { Easy, Medium, Hard };

struct Puzzle {
  Board startingBoard;
  Board solution;
};

class PuzzleGenerator {
public:
  explicit PuzzleGenerator(std::mt19937& randomEngine, int maxAttempts = 20);

  Puzzle generate(int size, Difficulty difficulty);
  static int targetClueCount(int size, Difficulty difficulty);

private:
  std::mt19937& randomEngine_;
  int maxAttempts_;
};
