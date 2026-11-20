#include "parser/gcode_parser.hpp"

#include <array>
#include <cstddef>

namespace parser {
namespace {

constexpr std::size_t kMaxLineLength = 256;  // RS274/NGC section 3.3
constexpr unsigned kMaxLineNumber = 99999;   // RS274/NGC section 3.3.1

bool is_digit(char c) { return c >= '0' && c <= '9'; }

char to_upper(char c) { return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c; }

// Upper-cased copy of a line with spaces, tabs and comments removed.
struct CleanLine {
    std::array<char, kMaxLineLength> text{};
    std::size_t length = 0;
};

ParseStatus clean_line(std::string_view line, CleanLine& out) {
    if (line.size() > kMaxLineLength) {
        return ParseStatus::LineTooLong;
    }
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (c == ';') {
            break;  // comment to end of line
        }
        if (c == '(') {
            const std::size_t close = line.find(')', i + 1);
            if (close == std::string_view::npos) {
                return ParseStatus::InvalidCharacter;
            }
            i = close;
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            continue;
        }
        out.text[out.length++] = to_upper(c);
    }
    return ParseStatus::Ok;
}

// Decimal number: optional sign, digits, optional decimal point, digits.
// At least one digit is required, and the whole token must be consumed.
bool parse_number(const char* s, std::size_t n, double& out) {
    std::size_t i = 0;
    bool negative = false;
    if (i < n && (s[i] == '+' || s[i] == '-')) {
        negative = (s[i] == '-');
        ++i;
    }

    double value = 0.0;
    bool has_digits = false;
    while (i < n && is_digit(s[i])) {
        value = value * 10.0 + (s[i] - '0');
        has_digits = true;
        ++i;
    }
    if (i < n && s[i] == '.') {
        ++i;
        double scale = 1.0;
        while (i < n && is_digit(s[i])) {
            scale /= 10.0;
            value += (s[i] - '0') * scale;
            has_digits = true;
            ++i;
        }
    }

    if (!has_digits || i != n) {
        return false;
    }
    out = negative ? -value : value;
    return true;
}

// Accepts only whole non-negative values that fit in an unsigned int.
bool as_integer(double value, unsigned& out) {
    if (value < 0.0 || value > 1e9) {
        return false;
    }
    out = static_cast<unsigned>(value);
    return static_cast<double>(out) == value;
}

ParseResult fail(ParseStatus status) { return ParseResult{status, LinearMove{}}; }

}  // namespace

ParseResult parse_linear_move(std::string_view line, ModalState& state) {
    CleanLine clean;
    if (const ParseStatus s = clean_line(line, clean); s != ParseStatus::Ok) {
        return fail(s);
    }
    if (clean.length == 0) {
        return fail(ParseStatus::Empty);
    }

    const char* text = clean.text.data();
    bool has_line_number = false;
    bool has_command = false;
    bool has_axis = false;
    bool has_feed = false;
    std::size_t motion_words = 0;  // words other than N
    LinearMove move{};

    std::size_t i = 0;
    while (i < clean.length) {
        const char letter = text[i++];
        if (letter < 'A' || letter > 'Z') {
            return fail(ParseStatus::InvalidCharacter);
        }

        const std::size_t start = i;
        while (i < clean.length && (is_digit(text[i]) || text[i] == '+' || text[i] == '-' || text[i] == '.')) {
            ++i;
        }
        double value = 0.0;
        if (!parse_number(text + start, i - start, value)) {
            return fail(ParseStatus::InvalidNumber);
        }

        if (letter == 'N') {
            unsigned number = 0;
            if (has_line_number) {
                return fail(ParseStatus::DuplicateWord);
            }
            if (!as_integer(value, number) || number > kMaxLineNumber) {
                return fail(ParseStatus::InvalidNumber);
            }
            has_line_number = true;
            continue;
        }

        ++motion_words;
        switch (letter) {
            case 'G': {
                unsigned code = 0;
                if (has_command) {
                    return fail(ParseStatus::DuplicateWord);
                }
                if (!as_integer(value, code) || code != 1) {
                    return fail(ParseStatus::UnsupportedWord);
                }
                has_command = true;
                break;
            }
            case 'F':
                if (has_feed) {
                    return fail(ParseStatus::DuplicateWord);
                }
                move.feed_rate = value;
                has_feed = true;
                break;
            case 'X':
            case 'Y':
            case 'Z': {
                const Axis axis = (letter == 'X') ? Axis::X : (letter == 'Y') ? Axis::Y : Axis::Z;
                if (has_axis) {
                    return fail(move.axis == axis ? ParseStatus::DuplicateWord : ParseStatus::MultipleAxes);
                }
                move.axis = axis;
                move.target = value;
                has_axis = true;
                break;
            }
            default:
                return fail(ParseStatus::UnsupportedWord);
        }
    }

    if (motion_words == 0) {
        return fail(ParseStatus::Empty);  // e.g. "N10" alone
    }
    if (has_feed && move.feed_rate <= 0.0) {
        return fail(ParseStatus::InvalidFeedRate);
    }

    // Lines without an axis word only change modal state (as in Grbl, "G1" or "F500" alone is valid).
    if (!has_axis) {
        if (has_command) {
            state.motion_active = true;
        }
        if (has_feed) {
            state.feed_set = true;
            state.feed_rate = move.feed_rate;
        }
        return fail(ParseStatus::Empty);
    }

    // Axis words use the modal G1 and feed rate when this line does not set them.
    if (!has_command && !state.motion_active) {
        return fail(ParseStatus::MissingCommand);
    }
    if (!has_feed && !state.feed_set) {
        return fail(ParseStatus::MissingFeedRate);
    }
    if (!has_feed) {
        move.feed_rate = state.feed_rate;
    }

    state.motion_active = true;
    if (has_feed) {
        state.feed_set = true;
        state.feed_rate = move.feed_rate;
    }
    return ParseResult{ParseStatus::Ok, move};
}

const char* to_string(ParseStatus status) {
    switch (status) {
        case ParseStatus::Ok: return "ok";
        case ParseStatus::Empty: return "empty line";
        case ParseStatus::LineTooLong: return "line too long";
        case ParseStatus::InvalidCharacter: return "invalid character";
        case ParseStatus::InvalidNumber: return "invalid number";
        case ParseStatus::DuplicateWord: return "duplicate word";
        case ParseStatus::MultipleAxes: return "more than one axis word";
        case ParseStatus::UnsupportedWord: return "unsupported word";
        case ParseStatus::MissingCommand: return "no G1 active";
        case ParseStatus::MissingFeedRate: return "no feed rate set";
        case ParseStatus::InvalidFeedRate: return "feed rate must be greater than zero";
    }
    return "unknown";
}

}  // namespace parser
