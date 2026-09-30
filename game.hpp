#pragma once

#include "puzzle_generator.hpp"

#include <istream>
#include <ostream>
#include <random>

class Game {
public:
  Game(std::istream& input, std::ostream& output,
       std::mt19937& randomEngine);

  int run();
  int play(Puzzle puzzle);

private:
  bool chooseSize(int& size);
  bool chooseDifficulty(Difficulty& difficulty);
  void render(const Board& board);
  void printHelp();

  std::istream& input_;
  std::ostream& output_;
  std::mt19937& randomEngine_;
};
