#include "game.hpp"

#include <charconv>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <system_error>

namespace {

bool parseSeed(const std::string& text, std::uint32_t& seed) {
  if (text.empty()) {
    return false;
  }

  std::uint64_t parsed = 0;
  const char* const first = text.data();
  const char* const last = first + text.size();
  const auto result = std::from_chars(first, last, parsed);
  if (result.ec != std::errc{} || result.ptr != last ||
      parsed > std::numeric_limits<std::uint32_t>::max()) {
    return false;
  }
  seed = static_cast<std::uint32_t>(parsed);
  return true;
}

}  // namespace

int main(int argc, char* argv[]) {
  std::uint32_t seed = 0;
  if (argc == 1) {
    seed = std::random_device{}();
  } else if (argc == 3 && std::string(argv[1]) == "--seed" &&
             parseSeed(argv[2], seed)) {
    // A supplied seed makes automated and manual reproductions deterministic.
  } else {
    std::cerr << "Usage: " << argv[0] << " [--seed <0-4294967295>]\n";
    return 2;
  }

  try {
    std::mt19937 randomEngine(seed);
    Game game(std::cin, std::cout, randomEngine);
    return game.run();
  } catch (const std::exception& error) {
    std::cerr << "Unable to start Sudoku: " << error.what() << '\n';
    return 1;
  }
}
