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
 *     auto supply = 3.3_volt;     // units::Volts (long-form suffix)
 *     auto temperature = 23_celsius; // units::Celsius
 *
 * Each quantity stores a single-precision `float` (see `units::value_type`),
 * which keeps arithmetic on the hardware FPU of single-precision MCUs such as
 * the Cortex-M4F instead of falling back to software double emulation.
 *
 * Each quantity implicitly converts to `float` so it can be passed straight
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

/** @brief Underlying numeric type stored by every quantity. */
using value_type = float;

/** @brief A scalar quantity tagged with a unit type to prevent mix-ups. */
template <typename Tag>
class Quantity {
public:
	constexpr explicit Quantity(value_type value) : value_(value) {}

	constexpr value_type value() const { return value_; }
	constexpr operator value_type() const { return value_; }

private:
	value_type value_;
};

namespace detail {

/*
 * Minimal stand-ins for std::is_arithmetic / std::enable_if, so this header
 * doesn't depend on <type_traits> (not available without a full libc++).
 */
template <typename T> struct is_scalar {
	static constexpr bool value = false;
};
template <> struct is_scalar<short> {
	static constexpr bool value = true;
};
template <> struct is_scalar<unsigned short> {
	static constexpr bool value = true;
};
template <> struct is_scalar<int> {
	static constexpr bool value = true;
};
template <> struct is_scalar<unsigned int> {
	static constexpr bool value = true;
};
template <> struct is_scalar<long> {
	static constexpr bool value = true;
};
template <> struct is_scalar<unsigned long> {
	static constexpr bool value = true;
};
template <> struct is_scalar<long long> {
	static constexpr bool value = true;
};
template <> struct is_scalar<unsigned long long> {
	static constexpr bool value = true;
};
template <> struct is_scalar<float> {
	static constexpr bool value = true;
};
template <> struct is_scalar<double> {
	static constexpr bool value = true;
};
template <> struct is_scalar<long double> {
	static constexpr bool value = true;
};

template <bool Cond> struct enable_if {};
template <> struct enable_if<true> {
	using type = void;
};

template <typename Scalar>
using if_scalar = typename enable_if<is_scalar<Scalar>::value>::type;

} /* namespace detail */

/*
 * Scaling by a plain number keeps the unit, in either operand order. The
 * scalar type is a template parameter (rather than a fixed `value_type`) so
 * the call is an exact match for any arithmetic type: with a fixed type, a
 * `double` or `int` scalar would need a conversion and become ambiguous with
 * the built-in operators reachable through `operator value_type()`.
 */
template <typename Tag, typename Scalar, typename = detail::if_scalar<Scalar>>
constexpr Quantity<Tag> operator*(Quantity<Tag> quantity, Scalar scalar)
{
	return Quantity<Tag>(quantity.value() * static_cast<value_type>(scalar));
}

template <typename Tag, typename Scalar, typename = detail::if_scalar<Scalar>>
constexpr Quantity<Tag> operator*(Scalar scalar, Quantity<Tag> quantity)
{
	return Quantity<Tag>(static_cast<value_type>(scalar) * quantity.value());
}

template <typename Tag, typename Scalar, typename = detail::if_scalar<Scalar>>
constexpr Quantity<Tag> operator/(Quantity<Tag> quantity, Scalar scalar)
{
	return Quantity<Tag>(quantity.value() / static_cast<value_type>(scalar));
}

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
 * implicit `operator value_type()` conversion and the compiler's built-in
 * arithmetic operators. These templates are an exact match (no conversion
 * needed) so overload resolution always prefers them over the built-ins;
 * being deleted, that makes cross-unit arithmetic a compile error instead.
 * Partial ordering still picks the same-tag templates above/below when
 * Tag1 == Tag2, and any non-template overload (e.g. Volts * Amps) below
 * still wins over these for the combinations it defines.
 */
template <typename Tag1, typename Tag2>
constexpr value_type operator+(Quantity<Tag1>, Quantity<Tag2>) = delete;

template <typename Tag1, typename Tag2>
constexpr value_type operator-(Quantity<Tag1>, Quantity<Tag2>) = delete;

template <typename Tag1, typename Tag2>
constexpr value_type operator*(Quantity<Tag1>, Quantity<Tag2>) = delete;

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

/*
 * Defines both the floating-point (3.3_volt) and integer (3_volt) forms of a
 * literal suffix. `scale` converts the literal into the quantity's base unit,
 * e.g. 500_ms -> Seconds(0.5). The scaling is done at the literal's full
 * precision and rounded to `value_type` once at the end.
 */
#define UNITS_DEFINE_LITERAL(type, suffix, scale)                                                  \
	constexpr type operator""_##suffix(long double value)                                       \
	{                                                                                          \
		return type(static_cast<value_type>(value * (scale)));                             \
	}                                                                                          \
	constexpr type operator""_##suffix(unsigned long long value)                                \
	{                                                                                          \
		return type(static_cast<value_type>(static_cast<double>(value) * (scale)));        \
	}

UNITS_DEFINE_LITERAL(Volts, v, 1.0)
UNITS_DEFINE_LITERAL(Volts, volt, 1.0)
UNITS_DEFINE_LITERAL(Volts, mv, 1e-3)
UNITS_DEFINE_LITERAL(Volts, millivolt, 1e-3)

UNITS_DEFINE_LITERAL(Amps, a, 1.0)
UNITS_DEFINE_LITERAL(Amps, amp, 1.0)
UNITS_DEFINE_LITERAL(Amps, ma, 1e-3)
UNITS_DEFINE_LITERAL(Amps, milliamp, 1e-3)

UNITS_DEFINE_LITERAL(Watts, w, 1.0)
UNITS_DEFINE_LITERAL(Watts, watt, 1.0)

UNITS_DEFINE_LITERAL(Celsius, celsius, 1.0)
/* Common misspelling kept as an alias so either suffix works. */
UNITS_DEFINE_LITERAL(Celsius, celcius, 1.0)

UNITS_DEFINE_LITERAL(Millimeters, mm, 1.0)
UNITS_DEFINE_LITERAL(Millimeters, millimeter, 1.0)

UNITS_DEFINE_LITERAL(Hertz, hz, 1.0)
UNITS_DEFINE_LITERAL(Hertz, hertz, 1.0)
UNITS_DEFINE_LITERAL(Hertz, khz, 1e3)

UNITS_DEFINE_LITERAL(Seconds, s, 1.0)
UNITS_DEFINE_LITERAL(Seconds, second, 1.0)
UNITS_DEFINE_LITERAL(Seconds, ms, 1e-3)
UNITS_DEFINE_LITERAL(Seconds, millisecond, 1e-3)
UNITS_DEFINE_LITERAL(Seconds, us, 1e-6)

#undef UNITS_DEFINE_LITERAL

} /* namespace literals */

} /* namespace units */

#endif /* APP_LIB_UNITS_HPP_ */
