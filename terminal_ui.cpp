#include "terminal_ui.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unistd.h>

TerminalKey readTerminalKey(std::istream& input) {
  const int rawCharacter = input.get();
  if (rawCharacter == std::char_traits<char>::eof()) {
    return TerminalKey{TerminalKeyType::EndOfInput};
  }

  const char character = static_cast<char>(rawCharacter);
  if (character == '\x1b') {
    const int prefix = input.get();
    if (prefix != '[') {
      return TerminalKey{TerminalKeyType::Unknown};
    }
    const int code = input.get();
    switch (code) {
      case 'A':
        return TerminalKey{TerminalKeyType::Up};
      case 'B':
        return TerminalKey{TerminalKeyType::Down};
      case 'C':
        return TerminalKey{TerminalKeyType::Right};
      case 'D':
        return TerminalKey{TerminalKeyType::Left};
      case '3':
        if (input.get() == '~') {
          return TerminalKey{TerminalKeyType::Clear};
        }
        return TerminalKey{TerminalKeyType::Unknown};
      default:
        return TerminalKey{TerminalKeyType::Unknown};
    }
  }

  if (character == '0' || character == '\b' || character == '\x7f') {
    return TerminalKey{TerminalKeyType::Clear};
  }
  if (character == '\r' || character == '\n') {
    return TerminalKey{TerminalKeyType::Check};
  }
  if (character == '?') {
    return TerminalKey{TerminalKeyType::Help};
  }
  if (character == 'q' || character == 'Q' || character == '\x03') {
    return TerminalKey{TerminalKeyType::Quit};
  }

  const unsigned char unsignedCharacter =
      static_cast<unsigned char>(character);
  const char upper = static_cast<char>(std::toupper(unsignedCharacter));
  if ((upper >= '1' && upper <= '9') ||
      (upper >= 'A' && upper <= 'G')) {
    return TerminalKey{TerminalKeyType::Value, character};
  }
  return TerminalKey{TerminalKeyType::Unknown};
}

bool standardStreamsAreInteractive() {
  return isatty(STDIN_FILENO) != 0 && isatty(STDOUT_FILENO) != 0;
}

void CellSelection::move(TerminalKeyType direction, int boardSize) {
  if (boardSize <= 0) {
    return;
  }
  switch (direction) {
    case TerminalKeyType::Up:
      row_ = std::max(0, row_ - 1);
      break;
    case TerminalKeyType::Down:
      row_ = std::min(boardSize - 1, row_ + 1);
      break;
    case TerminalKeyType::Left:
      column_ = std::max(0, column_ - 1);
      break;
    case TerminalKeyType::Right:
      column_ = std::min(boardSize - 1, column_ + 1);
      break;
    default:
      break;
  }
}

int CellSelection::row() const { return row_; }

int CellSelection::column() const { return column_; }

RawTerminalMode::RawTerminalMode() {
  if (!standardStreamsAreInteractive()) {
    throw std::runtime_error("interactive mode requires a terminal");
  }
  if (tcgetattr(STDIN_FILENO, &originalSettings_) != 0) {
    throw std::runtime_error("unable to read terminal settings");
  }

  termios rawSettings = originalSettings_;
  rawSettings.c_lflag &=
      static_cast<tcflag_t>(~(ICANON | ECHO | ISIG));
  rawSettings.c_iflag &= static_cast<tcflag_t>(~(IXON | ICRNL));
  rawSettings.c_cc[VMIN] = 1;
  rawSettings.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawSettings) != 0) {
    throw std::runtime_error("unable to enable raw terminal mode");
  }
  active_ = true;
}

RawTerminalMode::~RawTerminalMode() {
  if (active_) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalSettings_);
  }
}
