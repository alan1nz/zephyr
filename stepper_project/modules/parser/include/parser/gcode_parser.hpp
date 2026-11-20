#pragma once

#include <cstdint>
#include <string_view>

namespace parser {

enum class Axis : std::uint8_t { X, Y, Z };

// A single linear move on one axis at a feed rate, from a line such as "G1 X10 F500".
struct LinearMove {
    Axis axis = Axis::X;
    double target = 0.0;     // position to move the axis to, in the caller's length units
    double feed_rate = 0.0;  // F word, in length units per minute
};

enum class ParseStatus : std::uint8_t {
    Ok,
    Empty,           // no axis word: nothing to move (a G1 or F word may still have set modal state)
    LineTooLong,     // longer than 256 characters (RS274/NGC section 3.3)
    InvalidCharacter,
    InvalidNumber,
    DuplicateWord,   // same word given twice, e.g. "X1 X2"
    MultipleAxes,    // more than one of X, Y, Z
    UnsupportedWord, // any G-code other than G1, any M-code, or any other word letter
    MissingCommand,  // axis word with no G1 on this line or earlier
    MissingFeedRate, // axis word with no F word on this line or earlier
    InvalidFeedRate, // F word not greater than zero
};

struct ParseResult {
    ParseStatus status = ParseStatus::Empty;
    LinearMove move{};  // only meaningful when status == ParseStatus::Ok
};

// Modal state carried from one line to the next, as in Grbl.
// The G1 command and the feed rate stay in effect until changed.
struct ModalState {
    bool motion_active = false;  // a G1 has been seen
    bool feed_set = false;       // an F word has been seen
    double feed_rate = 0.0;
};

// Parses one line of G-code.
// Follows Grbl's conventions: words may appear in any order, the G1 command and the F word are
// modal, letters are case-insensitive, spaces and tabs are ignored, comments are in parentheses
// or start with ';', and a leading N line number is accepted.
// The state is only updated when the line is accepted, so a rejected line changes nothing.
ParseResult parse_linear_move(std::string_view line, ModalState& state);

// Convenience overload that parses with no earlier modal state.
inline ParseResult parse_linear_move(std::string_view line) {
    ModalState state;
    return parse_linear_move(line, state);
}

const char* to_string(ParseStatus status);

}  // namespace parser
