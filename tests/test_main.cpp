#include "board.hpp"
#include "game.hpp"
#include "puzzle_generator.hpp"
#include "solver.hpp"

#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using Test = std::pair<std::string, std::function<void()>>;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Board boardFromRows(const std::vector<std::string>& rows) {
  Board board(static_cast<int>(rows.size()));
  for (int row = 0; row < board.size(); ++row) {
    expect(static_cast<int>(rows[static_cast<std::size_t>(row)].size()) ==
               board.size(),
           "fixture row width must match board size");
    for (int column = 0; column < board.size(); ++column) {
      const int value = Board::valueForSymbol(
          rows[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)]);
      board.set(row, column, value);
    }
  }
  return board;
}

template <typename Exception, typename Function>
void expectThrows(Function function, const std::string& message) {
  try {
    function();
  } catch (const Exception&) {
    return;
  }
  throw std::runtime_error(message);
}

int runTests(const std::vector<Test>& tests) {
  int failures = 0;

  for (const auto& [name, test] : tests) {
    try {
      test();
      std::cout << "PASS: " << name << '\n';
    } catch (const std::exception& error) {
      ++failures;
      std::cerr << "FAIL: " << name << ": " << error.what() << '\n';
    }
  }

  std::cout << tests.size() - static_cast<std::size_t>(failures) << "/"
            << tests.size() << " tests passed\n";
  return failures == 0 ? 0 : 1;
}

}  // namespace

int main() {
  const std::vector<Test> tests{
      {"board accepts supported sizes", [] {
         const Board small(4);
         const Board classic(9);
         const Board large(16);

         expect(small.size() == 4 && small.boxSize() == 2,
                "4x4 board must use 2x2 boxes");
         expect(classic.size() == 9 && classic.boxSize() == 3,
                "9x9 board must use 3x3 boxes");
         expect(large.size() == 16 && large.boxSize() == 4,
                "16x16 board must use 4x4 boxes");
       }},
      {"board rejects unsupported sizes", [] {
         expectThrows<std::invalid_argument>([] { Board board(8); },
                                             "8x8 board must be rejected");
       }},
      {"board stores and clears values", [] {
         Board board(4);
         board.set(1, 2, 3);
         expect(board.at(1, 2) == 3, "stored value must be readable");
         board.clear(1, 2);
         expect(board.at(1, 2) == 0, "cleared cell must be empty");
       }},
      {"board rejects invalid coordinates and values", [] {
         Board board(4);
         expectThrows<std::out_of_range>([&] { board.at(4, 0); },
                                         "row outside board must fail");
         expectThrows<std::out_of_range>([&] { board.set(0, -1, 1); },
                                         "negative column must fail");
         expectThrows<std::invalid_argument>([&] { board.set(0, 0, 5); },
                                             "value above size must fail");
       }},
      {"board detects row column and box conflicts", [] {
         Board board(4);
         board.set(0, 0, 1);

         expect(!board.isValidPlacement(0, 3, 1),
                "duplicate in row must be rejected");
         expect(!board.isValidPlacement(3, 0, 1),
                "duplicate in column must be rejected");
         expect(!board.isValidPlacement(1, 1, 1),
                "duplicate in box must be rejected");
         expect(board.isValidPlacement(2, 2, 1),
                "same value outside row, column, and box must be allowed");
       }},
      {"board allows validating the current value", [] {
         Board board(4);
         board.set(0, 0, 2);
         expect(board.isValidPlacement(0, 0, 2),
                "a cell must not conflict with itself");
       }},
      {"board provides candidate masks", [] {
         Board board(4);
         board.set(0, 0, 1);
         board.set(0, 1, 2);
         board.set(1, 2, 3);

         const std::uint32_t candidates = board.candidateMask(0, 2);
         expect(candidates == (std::uint32_t{1} << 3),
                "only value 4 should be available");
       }},
      {"board reports valid complete state", [] {
         Board board(4);
         const std::vector<int> values{
             1, 2, 3, 4,
             3, 4, 1, 2,
             2, 1, 4, 3,
             4, 3, 2, 1,
         };
         for (int row = 0; row < board.size(); ++row) {
           for (int column = 0; column < board.size(); ++column) {
             board.set(row, column,
                       values[static_cast<std::size_t>(row * board.size() + column)]);
           }
         }

         expect(board.isComplete(), "filled board must be complete");
         expect(board.isValid(), "known solution must be valid");
       }},
      {"board converts display symbols", [] {
         expect(Board::symbolForValue(1) == '1', "1 must display as 1");
         expect(Board::symbolForValue(9) == '9', "9 must display as 9");
         expect(Board::symbolForValue(10) == 'A', "10 must display as A");
         expect(Board::symbolForValue(16) == 'G', "16 must display as G");
         expect(Board::valueForSymbol('g') == 16,
                "lowercase symbols must be accepted");
         expect(Board::valueForSymbol('.') == 0, "dot must represent empty");
         expect(Board::valueForSymbol('H') == -1,
                "unsupported symbol must be rejected");
       }},
      {"solver solves a unique 4x4 puzzle", [] {
         Board puzzle = boardFromRows({".234", "3.12", "21.3", "432."});
         const Board solution =
             boardFromRows({"1234", "3412", "2143", "4321"});
         const Solver solver;

         expect(solver.countSolutions(puzzle) == 1,
                "fixture must have one solution");
         expect(solver.solve(puzzle), "unique puzzle must be solvable");
         expect(puzzle == solution, "solver must produce expected solution");
       }},
      {"solver solves a known 9x9 puzzle", [] {
         Board puzzle = boardFromRows({
             "53..7....", "6..195...", ".98....6.", "8...6...3",
             "4..8.3..1", "7...2...6", ".6....28.", "...419..5",
             "....8..79",
         });
         const Board solution = boardFromRows({
             "534678912", "672195348", "198342567", "859761423",
             "426853791", "713924856", "961537284", "287419635",
             "345286179",
         });
         const Solver solver;

         expect(solver.countSolutions(puzzle) == 1,
                "classic fixture must have one solution");
         expect(solver.solve(puzzle), "classic puzzle must be solvable");
         expect(puzzle == solution, "9x9 solution must match fixture");
       }},
      {"solver handles a 16x16 puzzle", [] {
         Board puzzle = boardFromRows({
             ".23456789ABCDEFG", "5.789ABCDEFG1234", "9A.CDEFG12345678",
             "DEF.123456789ABC", "2345.789ABCDEFG1", "6789A.CDEFG12345",
             "ABCDEF.123456789", "EFG1234.6789ABCD", "3456789A.CDEFG12",
             "789ABCDEF.123456", "BCDEFG1234.6789A", "FG123456789.BCDE",
             "456789ABCDEF.123", "89ABCDEFG1234.67", "CDEFG123456789.B",
             "G123456789ABCDE.",
         });
         const Board solution = boardFromRows({
             "123456789ABCDEFG", "56789ABCDEFG1234", "9ABCDEFG12345678",
             "DEFG123456789ABC", "23456789ABCDEFG1", "6789ABCDEFG12345",
             "ABCDEFG123456789", "EFG123456789ABCD", "3456789ABCDEFG12",
             "789ABCDEFG123456", "BCDEFG123456789A", "FG123456789ABCDE",
             "456789ABCDEFG123", "89ABCDEFG1234567", "CDEFG123456789AB",
             "G123456789ABCDEF",
         });
         const Solver solver;

         expect(solver.solve(puzzle), "16x16 fixture must be solvable");
         expect(puzzle == solution, "16x16 solution must match fixture");
       }},
      {"solver bounds ambiguous solution counting", [] {
         const Board empty(4);
         const Solver solver;
         expect(solver.countSolutions(empty, 2) == 2,
                "empty 4x4 board must stop after two solutions");
         expect(solver.countSolutions(empty, 1) == 1,
                "solution count must respect limit one");
         expectThrows<std::invalid_argument>(
             [&] { solver.countSolutions(empty, 0); },
             "nonpositive count limit must be rejected");
       }},
      {"solver reports an exhausted solution-count budget", [] {
         const Board empty(16);
         const Solver solver;
         const SolutionCountResult result =
             solver.countSolutionsWithBudget(empty, 2, 0);

         expect(result.budgetExceeded,
                "zero node budget must report an inconclusive count");
         expect(result.count == 0,
                "no solutions can be proven without visiting a node");
       }},
      {"solver rejects invalid boards without mutation", [] {
         Board invalid =
             boardFromRows({"1134", "3412", "2143", "4321"});
         const Board original = invalid;
         const Solver solver;

         expect(!solver.solve(invalid), "invalid board must not solve");
         expect(invalid == original, "failed solve must preserve caller state");
         expect(solver.countSolutions(invalid) == 0,
                "invalid board must have zero solutions");
         expect(invalid == original, "counting must preserve caller state");
       }},
      {"solver rejects a locally valid unsolvable board", [] {
         Board unsolvable = boardFromRows({
             "531.7....", "6..195...", ".98....6.", "8...6...3",
             "4..8.3..1", "7...2...6", ".6....28.", "...419..5",
             "....8..79",
         });
         const Board original = unsolvable;
         const Solver solver;

         expect(unsolvable.isValid(),
                "fixture must have no immediate Sudoku conflicts");
         expect(!solver.solve(unsolvable),
                "locally valid contradiction must not solve");
         expect(unsolvable == original,
                "unsolvable search must preserve caller state");
         expect(solver.countSolutions(unsolvable) == 0,
                "unsolvable fixture must have zero solutions");
       }},
      {"randomized solving is reproducible", [] {
         Board first(4);
         Board second(4);
         std::mt19937 firstEngine(42);
         std::mt19937 secondEngine(42);
         const Solver solver;

         expect(solver.solve(first, &firstEngine), "empty board must solve");
         expect(solver.solve(second, &secondEngine), "empty board must solve");
         expect(first == second, "equal seeds must produce equal solutions");
       }},
      {"generator computes exact clue targets", [] {
         expect(PuzzleGenerator::targetClueCount(4, Difficulty::Easy) == 10,
                "4x4 easy target must be 10");
         expect(PuzzleGenerator::targetClueCount(4, Difficulty::Medium) == 8,
                "4x4 medium target must be 8");
         expect(PuzzleGenerator::targetClueCount(4, Difficulty::Hard) == 7,
                "4x4 hard target must be 7");
         expect(PuzzleGenerator::targetClueCount(9, Difficulty::Easy) == 49,
                "9x9 easy target must be 49");
         expect(PuzzleGenerator::targetClueCount(9, Difficulty::Medium) == 41,
                "9x9 medium target must be 41");
         expect(PuzzleGenerator::targetClueCount(9, Difficulty::Hard) == 33,
                "9x9 hard target must be 33");
         expect(PuzzleGenerator::targetClueCount(16, Difficulty::Easy) == 154,
                "16x16 easy target must be 154");
         expect(PuzzleGenerator::targetClueCount(16, Difficulty::Medium) == 128,
                "16x16 medium target must be 128");
         expect(PuzzleGenerator::targetClueCount(16, Difficulty::Hard) == 103,
                "16x16 hard target must be 103");
         expectThrows<std::invalid_argument>(
             [] {
               PuzzleGenerator::targetClueCount(
                   4, static_cast<Difficulty>(99));
             },
             "unknown difficulty must be rejected");
         expectThrows<std::invalid_argument>(
             [] {
               PuzzleGenerator::targetClueCount(8, Difficulty::Easy);
             },
             "unsupported generator size must be rejected");
       }},
      {"generator creates a unique 4x4 puzzle", [] {
         std::mt19937 engine(0);
         PuzzleGenerator generator(engine);
         const Puzzle puzzle = generator.generate(4, Difficulty::Easy);
         const Solver solver;

         expect(puzzle.startingBoard.clueCount() == 10,
                "easy puzzle must meet exact clue target");
         expect(puzzle.solution.isComplete() && puzzle.solution.isValid(),
                "stored solution must be complete and valid");
         expect(solver.countSolutions(puzzle.startingBoard, 2) == 1,
                "generated puzzle must have one solution");
         for (int row = 0; row < 4; ++row) {
           for (int column = 0; column < 4; ++column) {
             const int clue = puzzle.startingBoard.at(row, column);
             expect(clue == 0 || clue == puzzle.solution.at(row, column),
                    "every clue must match the solution");
           }
         }
       }},
      {"generator is reproducible for equal seeds", [] {
         std::mt19937 firstEngine(42);
         std::mt19937 secondEngine(42);
         PuzzleGenerator firstGenerator(firstEngine);
         PuzzleGenerator secondGenerator(secondEngine);

         const Puzzle first = firstGenerator.generate(4, Difficulty::Medium);
         const Puzzle second = secondGenerator.generate(4, Difficulty::Medium);
         expect(first.startingBoard == second.startingBoard,
                "equal seeds must produce equal puzzles");
         expect(first.solution == second.solution,
                "equal seeds must produce equal solutions");
       }},
      {"generator reports bounded failure", [] {
         std::mt19937 engine(7);
         PuzzleGenerator generator(engine, 0);
         expectThrows<std::runtime_error>(
             [&] { generator.generate(4, Difficulty::Easy); },
             "zero permitted attempts must fail immediately");
       }},
      {"generator supports every size and difficulty", [] {
         struct Configuration {
           int size;
           Difficulty difficulty;
           std::uint32_t seed;
         };
         const std::vector<Configuration> configurations{
             {4, Difficulty::Easy, 0},
             {4, Difficulty::Medium, 1},
             {4, Difficulty::Hard,
              std::numeric_limits<std::uint32_t>::max()},
             {9, Difficulty::Easy, 7},
             {9, Difficulty::Medium, 42},
             {9, Difficulty::Hard, 20260930},
             {16, Difficulty::Easy, 17},
             {16, Difficulty::Medium, 430},
             {16, Difficulty::Hard, 0x00C0FFEE},
             {16, Difficulty::Hard, 14},
         };
         const Solver solver;

         for (const Configuration& configuration : configurations) {
           std::mt19937 engine(configuration.seed);
           PuzzleGenerator generator(engine);
           const Puzzle puzzle =
               generator.generate(configuration.size, configuration.difficulty);

           expect(puzzle.startingBoard.clueCount() ==
                      PuzzleGenerator::targetClueCount(configuration.size,
                                                       configuration.difficulty),
                  "generated puzzle must meet its exact clue target");
           expect(puzzle.solution.isComplete() && puzzle.solution.isValid(),
                  "generated solution must be complete and valid");
           expect(solver.countSolutions(puzzle.startingBoard, 2) == 1,
                  "every generated puzzle must be unique");
         }
       }},
      {"game completes a fixed puzzle", [] {
         const Board starting =
             boardFromRows({".234", "3412", "2143", "4321"});
         const Board solution =
             boardFromRows({"1234", "3412", "2143", "4321"});
         std::istringstream input("set 1 1 1\n");
         std::ostringstream output;
         std::mt19937 engine(1);
         Game game(input, output, engine);

         expect(game.play(Puzzle{starting, solution}) == 0,
                "completed session must exit successfully");
         expect(output.str().find("Congratulations") != std::string::npos,
                "completed puzzle must print a victory message");
       }},
      {"game protects clues and rejects conflicts", [] {
         const Board starting =
             boardFromRows({".234", "3412", "2143", "4321"});
         const Board solution =
             boardFromRows({"1234", "3412", "2143", "4321"});
         std::istringstream input("set 1 2 2\nset 1 1 2\nquit\n");
         std::ostringstream output;
         std::mt19937 engine(2);
         Game game(input, output, engine);

         game.play(Puzzle{starting, solution});
         expect(output.str().find("original clue") != std::string::npos,
                "editing a clue must be rejected");
         expect(output.str().find("conflicts") != std::string::npos,
                "duplicate value must be rejected");
       }},
      {"game accepts a locally legal wrong guess", [] {
         const Board starting = boardFromRows({
             "53..7....", "6..195...", ".98....6.", "8...6...3",
             "4..8.3..1", "7...2...6", ".6....28.", "...419..5",
             "....8..79",
         });
         const Board solution = boardFromRows({
             "534678912", "672195348", "198342567", "859761423",
             "426853791", "713924856", "961537284", "287419635",
             "345286179",
         });
         std::istringstream input("set 1 3 1\ncheck\nclear 1 3\nquit\n");
         std::ostringstream output;
         std::mt19937 engine(3);
         Game game(input, output, engine);

         game.play(Puzzle{starting, solution});
         expect(output.str().find("Value placed") != std::string::npos,
                "locally legal guess must be accepted");
         expect(output.str().find("Puzzle is incomplete") != std::string::npos,
                "check must not reveal the solution");
         expect(output.str().find("Cell cleared") != std::string::npos,
                "editable value must be clearable");
       }},
      {"game explains malformed commands and help", [] {
         const Board starting =
             boardFromRows({".234", "3412", "2143", "4321"});
         const Board solution =
             boardFromRows({"1234", "3412", "2143", "4321"});
         std::istringstream input("help\nset 1\nunknown\nquit\n");
         std::ostringstream output;
         std::mt19937 engine(4);
         Game game(input, output, engine);

         game.play(Puzzle{starting, solution});
         expect(output.str().find("set <row> <column> <value>") !=
                    std::string::npos,
                "help must describe set syntax");
         expect(output.str().find("Usage: set") != std::string::npos,
                "malformed set must print usage");
         expect(output.str().find("Unknown command") != std::string::npos,
                "unknown command must be explained");
       }},
      {"game menus re-prompt and start a session", [] {
         std::istringstream input(
             "5\n4 extra\n999999999999999999999\n4\n"
             "impossible\neasy extra\neasy\nquit\n");
         std::ostringstream output;
         std::mt19937 engine(5);
         Game game(input, output, engine);

         expect(game.run() == 0, "normal quit must return success");
         expect(output.str().find("Size must be 4, 9, or 16") !=
                    std::string::npos,
                "invalid size must be explained");
         expect(output.str().find("Difficulty must be easy, medium, or hard") !=
                    std::string::npos,
                "invalid difficulty must be explained");
         expect(output.str().find("Goodbye") != std::string::npos,
                "quit must print a goodbye message");
       }},
      {"game rejects malformed command arguments", [] {
         const Board starting =
             boardFromRows({".234", "3412", "2143", "4321"});
         const Board solution =
             boardFromRows({"1234", "3412", "2143", "4321"});
         std::istringstream input(
             "\nset 1 1 1 extra\nset -1 1 1\n"
             "set 999999999999999999999 1 1\nset 1 1 0\nset 1 1 H\n"
             "clear 1 2\nclear 1 1 extra\nquit extra\nquit\n");
         std::ostringstream output;
         std::mt19937 engine(7);
         Game game(input, output, engine);

         game.play(Puzzle{starting, solution});
         const std::string transcript = output.str();
         expect(transcript.find("Enter a command") != std::string::npos,
                "empty command must be explained");
         expect(transcript.find("Usage: set") != std::string::npos,
                "set with trailing tokens must be rejected");
         expect(transcript.find("Row and column must be between") !=
                    std::string::npos,
                "negative or overflowed coordinates must be rejected");
         expect(transcript.find("Value must be between") != std::string::npos,
                "zero and invalid symbols must be rejected");
         expect(transcript.find("original clue") != std::string::npos,
                "clearing an original clue must be rejected");
         expect(transcript.find("Usage: clear") != std::string::npos,
                "clear with trailing tokens must be rejected");
         expect(transcript.find("Usage: quit") != std::string::npos,
                "quit with trailing tokens must be rejected");
       }},
      {"game accepts lowercase 16x16 symbols", [] {
         const Board starting = boardFromRows({
             "123456789ABCDEF.", "56789ABCDEFG1234", "9ABCDEFG12345678",
             "DEFG123456789ABC", "23456789ABCDEFG1", "6789ABCDEFG12345",
             "ABCDEFG123456789", "EFG123456789ABCD", "3456789ABCDEFG12",
             "789ABCDEFG123456", "BCDEFG123456789A", "FG123456789ABCDE",
             "456789ABCDEFG123", "89ABCDEFG1234567", "CDEFG123456789AB",
             "G123456789ABCDEF",
         });
         const Board solution = boardFromRows({
             "123456789ABCDEFG", "56789ABCDEFG1234", "9ABCDEFG12345678",
             "DEFG123456789ABC", "23456789ABCDEFG1", "6789ABCDEFG12345",
             "ABCDEFG123456789", "EFG123456789ABCD", "3456789ABCDEFG12",
             "789ABCDEFG123456", "BCDEFG123456789A", "FG123456789ABCDE",
             "456789ABCDEFG123", "89ABCDEFG1234567", "CDEFG123456789AB",
             "G123456789ABCDEF",
         });
         std::istringstream input("set 1 16 g\n");
         std::ostringstream output;
         std::mt19937 engine(6);
         Game game(input, output, engine);

         game.play(Puzzle{starting, solution});
         expect(output.str().find("Congratulations") != std::string::npos,
                "lowercase G must place value 16 and complete the puzzle");

         std::istringstream rendered(output.str());
         std::string line;
         std::size_t rowWidth = 0;
         int renderedRows = 0;
         while (std::getline(rendered, line)) {
           if (line.find('|') == std::string::npos) {
             continue;
           }
           if (rowWidth == 0) {
             rowWidth = line.size();
           }
           expect(line.size() == rowWidth,
                  "every rendered 16x16 data row must align");
           ++renderedRows;
         }
         expect(renderedRows == 32,
                "16 rows must render before and after the winning move");
       }},
  };

  return runTests(tests);
}
