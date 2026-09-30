#pragma once

#include "board.hpp"

#include <cstddef>
#include <random>

struct SolutionCountResult {
  int count;
  bool budgetExceeded;
};

class Solver {
public:
  bool solve(Board& board, std::mt19937* randomEngine = nullptr) const;
  int countSolutions(const Board& board, int limit = 2) const;
  SolutionCountResult countSolutionsWithBudget(const Board& board, int limit,
                                               std::size_t nodeBudget) const;
};
