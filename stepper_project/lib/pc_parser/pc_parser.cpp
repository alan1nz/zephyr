#include <algorithm>
#include <cstddef>
#include <expected>
#include <ranges>
#include <span>
#include <string_view>

namespace pc_parser
{

    enum class ParseError
    {
        InvalidNumber,
        InvalidCharacter,
        UnclosedComment,
        NestedComment,
        BufferTooSmall
    };

    // Copies line into out with comments removed and returns the number of characters written.
    // Parenthesised comments may appear several times on a line. A ';' comment runs to the end of the line.
    // On error, the contents of out are unspecified.
    std::expected<std::size_t, ParseError> remove_comments(std::string_view line, std::span<char> out)
    {
        std::size_t written = 0;

        for (std::size_t i = 0; i < line.size(); ++i)
        {
            const char c = line[i];

            // Semicolon comments run to end of line
            if (c == ';')
                break;

            // Handle parenthesized comments
            if (c == '(')
            {
                const auto close = line.find(')', i + 1);
                if (close == std::string_view::npos)
                    return std::unexpected(ParseError::UnclosedComment);

                // Check for nested parentheses in comment content
                const auto comment_content = line.substr(i + 1, close - i - 1);
                if (std::ranges::contains(comment_content, '('))
                    return std::unexpected(ParseError::NestedComment);

                i = close; // skip past ')'
                continue;
            }

            // An unmatched ')' is not valid outside a comment
            if (c == ')')
                return std::unexpected(ParseError::InvalidCharacter);

            // Copy non-comment character to output
            if (written == out.size())
                return std::unexpected(ParseError::BufferTooSmall);
            out[written++] = c;
        }

        return written;
    }

} // namespace pc_parser
