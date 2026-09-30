#pragma once

#include <istream>
#include <termios.h>

enum class TerminalKeyType {
  Up,
  Down,
  Left,
  Right,
  Value,
  Clear,
  Check,
  Help,
  Quit,
  Unknown,
  EndOfInput,
};

struct TerminalKey {
  TerminalKeyType type;
  char character = '\0';
};

TerminalKey readTerminalKey(std::istream& input);
bool standardStreamsAreInteractive();

class CellSelection {
public:
  void move(TerminalKeyType direction, int boardSize);
  int row() const;
  int column() const;

private:
  int row_ = 0;
  int column_ = 0;
};

class RawTerminalMode {
public:
  RawTerminalMode();
  ~RawTerminalMode();

  RawTerminalMode(const RawTerminalMode&) = delete;
  RawTerminalMode& operator=(const RawTerminalMode&) = delete;

private:
  termios originalSettings_{};
  bool active_ = false;
};
