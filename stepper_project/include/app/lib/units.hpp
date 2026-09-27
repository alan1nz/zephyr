/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_LIB_UNITS_HPP_
#define APP_LIB_UNITS_HPP_

/**
 * @brief Strongly-typed physical quantities with C++ user-defined literals.
 *
 * Usage:
 *
 *     using namespace units::literals;
 *
 *     auto voltage = 48.0_v;      // units::Volts
 *     auto temperature = 23_celsius; // units::Celsius
 *
 * Each quantity implicitly converts to `double` so it can be passed straight
 * into existing C driver APIs, while still catching unit mix-ups at compile
 * time in C++ code: same-tag quantities can be added/subtracted, and the
 * only cross-unit product defined is Volts * Amps -> Watts. Any other
 * mixed-unit +, -, or * (e.g. `voltage - current`, `celsius * millimeters`)
 * is a compile error.
 *
 * Note: parenthesize a literal before calling a member function on it
 * directly, e.g. `(48.0_v).value()`, not `48.0_v.value()` -- otherwise the
 * `.value` gets lexed as part of the same preprocessing-number token as the
 * suffix.
 */
namespace units {

/** @brief A scalar quantity tagged with a unit type to prevent mix-ups. */
template <typename Tag>
class Quantity {
public:
	constexpr explicit Quantity(double value) : value_(value) {}

	constexpr double value() const { return value_; }
	constexpr operator double() const { return value_; }

	constexpr Quantity operator*(double scalar) const { return Quantity(value_ * scalar); }
	constexpr Quantity operator/(double scalar) const { return Quantity(value_ / scalar); }

private:
	double value_;
};

/* Only same-tag operands may be added/subtracted. */
template <typename Tag>
constexpr Quantity<Tag> operator+(Quantity<Tag> lhs, Quantity<Tag> rhs)
{
	return Quantity<Tag>(lhs.value() + rhs.value());
}

template <typename Tag>
constexpr Quantity<Tag> operator-(Quantity<Tag> lhs, Quantity<Tag> rhs)
{
	return Quantity<Tag>(lhs.value() - rhs.value());
}

/*
 * Mismatched-tag +/- and * would otherwise silently compile via the
 * implicit `operator double()` conversion and the compiler's built-in
 * arithmetic operators. These templates are an exact match (no conversion
 * needed) so overload resolution always prefers them over the built-ins;
 * being deleted, that makes cross-unit arithmetic a compile error instead.
 * Partial ordering still picks the same-tag templates above/below when
 * Tag1 == Tag2, and any non-template overload (e.g. Volts * Amps) below
 * still wins over these for the combinations it defines.
 */
template <typename Tag1, typename Tag2>
constexpr double operator+(Quantity<Tag1>, Quantity<Tag2>) = delete;

template <typename Tag1, typename Tag2>
constexpr double operator-(Quantity<Tag1>, Quantity<Tag2>) = delete;

template <typename Tag1, typename Tag2>
constexpr double operator*(Quantity<Tag1>, Quantity<Tag2>) = delete;

namespace tags {
struct Volts {};
struct Amps {};
struct Watts {};
struct Celsius {};
struct Millimeters {};
struct Hertz {};
struct Seconds {};
} /* namespace tags */

using Volts = Quantity<tags::Volts>;
using Amps = Quantity<tags::Amps>;
using Watts = Quantity<tags::Watts>;
using Celsius = Quantity<tags::Celsius>;
using Millimeters = Quantity<tags::Millimeters>;
using Hertz = Quantity<tags::Hertz>;
using Seconds = Quantity<tags::Seconds>;

/* The only cross-unit multiplication currently defined: P = V * I. */
constexpr Watts operator*(Volts voltage, Amps current)
{
	return Watts(voltage.value() * current.value());
}

constexpr Watts operator*(Amps current, Volts voltage) { return voltage * current; }

inline namespace literals {

constexpr Volts operator""_v(long double value) { return Volts(static_cast<double>(value)); }
constexpr Volts operator""_v(unsigned long long value) { return Volts(static_cast<double>(value)); }

constexpr Amps operator""_a(long double value) { return Amps(static_cast<double>(value)); }
constexpr Amps operator""_a(unsigned long long value) { return Amps(static_cast<double>(value)); }

constexpr Celsius operator""_celsius(long double value) { return Celsius(static_cast<double>(value)); }
constexpr Celsius operator""_celsius(unsigned long long value)
{
	return Celsius(static_cast<double>(value));
}

/* Common misspelling kept as an alias so either suffix works. */
constexpr Celsius operator""_celcius(long double value) { return Celsius(static_cast<double>(value)); }
constexpr Celsius operator""_celcius(unsigned long long value)
{
	return Celsius(static_cast<double>(value));
}

constexpr Millimeters operator""_mm(long double value) { return Millimeters(static_cast<double>(value)); }
constexpr Millimeters operator""_mm(unsigned long long value)
{
	return Millimeters(static_cast<double>(value));
}

constexpr Hertz operator""_hz(long double value) { return Hertz(static_cast<double>(value)); }
constexpr Hertz operator""_hz(unsigned long long value) { return Hertz(static_cast<double>(value)); }

constexpr Seconds operator""_s(long double value) { return Seconds(static_cast<double>(value)); }
constexpr Seconds operator""_s(unsigned long long value) { return Seconds(static_cast<double>(value)); }

constexpr Seconds operator""_ms(long double value) { return Seconds(static_cast<double>(value) / 1000.0); }
constexpr Seconds operator""_ms(unsigned long long value)
{
	return Seconds(static_cast<double>(value) / 1000.0);
}

constexpr Watts operator""_w(long double value) { return Watts(static_cast<double>(value)); }
constexpr Watts operator""_w(unsigned long long value) { return Watts(static_cast<double>(value)); }

} /* namespace literals */

} /* namespace units */

#endif /* APP_LIB_UNITS_HPP_ */
