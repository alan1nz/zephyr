#include "parser/gcode_parser.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <string>

using parser::Axis;
using parser::ParseStatus;
using parser::parse_linear_move;
using Catch::Matchers::WithinAbs;

TEST_CASE("valid G1 moves on each axis") {
    SECTION("X axis") {
        const auto r = parse_linear_move("G1 X10 F500");
        REQUIRE(r.status == ParseStatus::Ok);
        CHECK(r.move.axis == Axis::X);
        CHECK_THAT(r.move.target, WithinAbs(10.0, 1e-9));
        CHECK_THAT(r.move.feed_rate, WithinAbs(500.0, 1e-9));
    }
    SECTION("Y axis") {
        const auto r = parse_linear_move("G1 Y-2.5 F100");
        REQUIRE(r.status == ParseStatus::Ok);
        CHECK(r.move.axis == Axis::Y);
        CHECK_THAT(r.move.target, WithinAbs(-2.5, 1e-9));
    }
    SECTION("Z axis") {
        const auto r = parse_linear_move("G1 Z0.25 F30");
        REQUIRE(r.status == ParseStatus::Ok);
        CHECK(r.move.axis == Axis::Z);
        CHECK_THAT(r.move.target, WithinAbs(0.25, 1e-9));
    }
}

TEST_CASE("word order does not matter") {
    const auto r = parse_linear_move("F250 Z5 G1");
    REQUIRE(r.status == ParseStatus::Ok);
    CHECK(r.move.axis == Axis::Z);
    CHECK_THAT(r.move.target, WithinAbs(5.0, 1e-9));
    CHECK_THAT(r.move.feed_rate, WithinAbs(250.0, 1e-9));
}

TEST_CASE("case, spaces and tabs are ignored") {
    const auto r = parse_linear_move("g01x+0. 12 34\tf 5 0 0");
    REQUIRE(r.status == ParseStatus::Ok);
    CHECK(r.move.axis == Axis::X);
    CHECK_THAT(r.move.target, WithinAbs(0.1234, 1e-9));
    CHECK_THAT(r.move.feed_rate, WithinAbs(500.0, 1e-9));
}

TEST_CASE("comments and line numbers are accepted") {
    CHECK(parse_linear_move("N100 G1 X1 F10").status == ParseStatus::Ok);
    CHECK(parse_linear_move("G1 X1 F10 (move the axis)").status == ParseStatus::Ok);
    CHECK(parse_linear_move("G1 X1 F10 ; trailing comment").status == ParseStatus::Ok);
    CHECK(parse_linear_move("G1 (ignored) X1 (ignored) F10").status == ParseStatus::Ok);
}

TEST_CASE("blank, comment-only and line-number-only input is empty") {
    CHECK(parse_linear_move("").status == ParseStatus::Empty);
    CHECK(parse_linear_move("   \t ").status == ParseStatus::Empty);
    CHECK(parse_linear_move("(just a comment)").status == ParseStatus::Empty);
    CHECK(parse_linear_move("; only a comment").status == ParseStatus::Empty);
    CHECK(parse_linear_move("N10").status == ParseStatus::Empty);
}

TEST_CASE("invalid lines are rejected") {
    struct Case {
        const char* line;
        ParseStatus expected;
    };
    const Case cases[] = {
        {"G1 X10", ParseStatus::MissingFeedRate},
        {"G1 X10 F0", ParseStatus::InvalidFeedRate},
        {"G1 X10 F-5", ParseStatus::InvalidFeedRate},
        {"X10 F100", ParseStatus::MissingCommand},
        {"G1 X1 Y2 F100", ParseStatus::MultipleAxes},
        {"G1 X1 X2 F100", ParseStatus::DuplicateWord},
        {"G1 X1 F10 F20", ParseStatus::DuplicateWord},
        {"G1 G1 X1 F10", ParseStatus::DuplicateWord},
        {"N1 N2 G1 X1 F10", ParseStatus::DuplicateWord},
        {"G0 X10 F100", ParseStatus::UnsupportedWord},
        {"G1 X1 F10 M3", ParseStatus::UnsupportedWord},
        {"G1 X1 F10 I2", ParseStatus::UnsupportedWord},
        {"G1 X1.2.3 F10", ParseStatus::InvalidNumber},
        {"G1 X F10", ParseStatus::InvalidNumber},
        {"G1 X1 F+ ", ParseStatus::InvalidNumber},
        {"G1 X1 F10 @", ParseStatus::InvalidCharacter},
        {"G1 X1 F10 (unterminated", ParseStatus::InvalidCharacter},
        {"N100000 G1 X1 F10", ParseStatus::InvalidNumber},
    };
    for (const auto& c : cases) {
        INFO("line: " << c.line);
        const auto r = parse_linear_move(c.line);
        CHECK(r.status == c.expected);
    }
}

TEST_CASE("G1 and F words without an axis only set modal state") {
    parser::ModalState state;
    CHECK(parser::parse_linear_move("G1 F100", state).status == ParseStatus::Empty);
    CHECK(state.motion_active);
    CHECK(state.feed_set);
    CHECK_THAT(state.feed_rate, WithinAbs(100.0, 1e-9));

    CHECK(parser::parse_linear_move("F250", state).status == ParseStatus::Empty);
    CHECK_THAT(state.feed_rate, WithinAbs(250.0, 1e-9));
}

TEST_CASE("G1 and feed rate persist across lines") {
    parser::ModalState state;
    REQUIRE(parser::parse_linear_move("G1 X10 F500", state).status == ParseStatus::Ok);

    // Axis word alone uses the modal G1 and the last feed rate.
    const auto r = parser::parse_linear_move("Y-3", state);
    REQUIRE(r.status == ParseStatus::Ok);
    CHECK(r.move.axis == Axis::Y);
    CHECK_THAT(r.move.target, WithinAbs(-3.0, 1e-9));
    CHECK_THAT(r.move.feed_rate, WithinAbs(500.0, 1e-9));

    // A new F word changes the feed rate for later lines.
    REQUIRE(parser::parse_linear_move("F120", state).status == ParseStatus::Empty);
    const auto s = parser::parse_linear_move("Z1", state);
    REQUIRE(s.status == ParseStatus::Ok);
    CHECK_THAT(s.move.feed_rate, WithinAbs(120.0, 1e-9));
}

TEST_CASE("axis words need modal G1 and feed rate from earlier lines") {
    parser::ModalState state;
    CHECK(parser::parse_linear_move("X5", state).status == ParseStatus::MissingCommand);
    CHECK(parser::parse_linear_move("G1 X5", state).status == ParseStatus::MissingFeedRate);
    CHECK_FALSE(state.motion_active);  // rejected line changed nothing
    CHECK_FALSE(state.feed_set);
}

TEST_CASE("rejected lines do not change modal state") {
    parser::ModalState state;
    REQUIRE(parser::parse_linear_move("G1 X1 F10", state).status == ParseStatus::Ok);

    CHECK(parser::parse_linear_move("F99 X1 Y1", state).status == ParseStatus::MultipleAxes);
    CHECK_THAT(state.feed_rate, WithinAbs(10.0, 1e-9));

    CHECK(parser::parse_linear_move("F0 X2", state).status == ParseStatus::InvalidFeedRate);
    CHECK_THAT(state.feed_rate, WithinAbs(10.0, 1e-9));
}

TEST_CASE("lines longer than 256 characters are rejected") {
    std::string line = "G1 X1 F10 (";
    line.append(250, 'c');
    line += ")";
    REQUIRE(line.size() > 256);
    CHECK(parse_linear_move(line).status == ParseStatus::LineTooLong);
}

TEST_CASE("to_string describes every status") {
    CHECK(std::string(parser::to_string(ParseStatus::Ok)) == "ok");
    CHECK(std::string(parser::to_string(ParseStatus::MissingFeedRate)) == "no feed rate set");
}
