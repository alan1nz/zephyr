/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <app/lib/units.hpp>

using namespace units::literals;

/* Compile-time checks that the literals produce the expected values/types.
 * Note: literals must be parenthesized before member access, since "_v.value"
 * would otherwise lex as a single preprocessing-number token.
 */
static_assert((48.0_v).value() == 48.0, "48.0_v should hold 48.0");
static_assert((23_celsius).value() == 23.0, "23_celsius should hold 23.0");
static_assert((23_celcius).value() == 23.0, "23_celcius should hold 23.0");
static_assert((500_ms).value() == 0.5, "500_ms should hold 0.5 seconds");

/* Same-tag arithmetic is allowed. */
static_assert(((12_v) + (12_v)).value() == 24.0, "same-unit addition should work");
static_assert(((2_a) - (1_a)).value() == 1.0, "same-unit subtraction should work");

/* The one defined cross-unit product: P = V * I. */
static_assert(((12_v) * (2_a)).value() == 24.0, "Volts * Amps should give Watts");
static_assert(((2_a) * (12_v)).value() == 24.0, "Amps * Volts should give Watts");

/* The following do NOT compile, by design (uncomment to verify):
 *   auto bad1 = 12_v + 2_a;        // error: use of deleted function
 *   auto bad2 = 12_v - 2_a;        // error: use of deleted function
 *   auto bad3 = 23_celsius * 5_mm; // error: use of deleted function
 */
