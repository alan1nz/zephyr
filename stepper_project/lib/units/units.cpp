/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <app/lib/units.hpp>

using namespace units::literals;

/* Quantities store a float, not a double. */
static_assert(sizeof((1_v).value()) == sizeof(float), "quantities should store a float");

/* Compile-time checks that the literals produce the expected values/types.
 * Note: literals must be parenthesized before member access, since "_v.value"
 * would otherwise lex as a single preprocessing-number token.
 */
static_assert((48.0_v).value() == 48.0F, "48.0_v should hold 48.0");
static_assert((23_celsius).value() == 23.0F, "23_celsius should hold 23.0");
static_assert((23_celcius).value() == 23.0F, "23_celcius should hold 23.0");
static_assert((500_ms).value() == 0.5F, "500_ms should hold 0.5 seconds");

/* Long-form suffixes produce the same types as the short ones. */
static_assert((3.3_volt).value() == 3.3F, "3.3_volt should hold 3.3F");
static_assert((3_volt).value() == 3.0F, "3_volt should hold 3.0");
static_assert((1500_mv).value() == 1.5F, "1500_mv should hold 1.5 volts");
static_assert((250_ma).value() == 0.25F, "250_ma should hold 0.25 amps");
static_assert((2_khz).value() == 2000.0F, "2_khz should hold 2000 Hz");

/* Same-tag arithmetic is allowed. */
static_assert(((12_v) + (12_v)).value() == 24.0F, "same-unit addition should work");
static_assert(((2_a) - (1_a)).value() == 1.0F, "same-unit subtraction should work");

/* Scaling by a plain number keeps the unit, in either order and for any arithmetic type. */
static_assert(((1_volt) * 2.0).value() == 2.0F, "Volts * double should give Volts");
static_assert((2.0 * (1_volt)).value() == 2.0F, "double * Volts should give Volts");
static_assert((2.0F * (1_volt)).value() == 2.0F, "float * Volts should give Volts");
static_assert((2 * (1_volt)).value() == 2.0F, "int * Volts should give Volts");
static_assert(((1_volt) * 2U).value() == 2.0F, "Volts * unsigned should give Volts");
static_assert(((3_volt) / 2).value() == 1.5F, "Volts / int should give Volts");

/* The one defined cross-unit product: P = V * I. */
static_assert(((12_v) * (2_a)).value() == 24.0F, "Volts * Amps should give Watts");
static_assert(((2_a) * (12_v)).value() == 24.0F, "Amps * Volts should give Watts");

/* The following do NOT compile, by design (uncomment to verify):
 *   auto bad1 = 12_v + 2_a;        // error: use of deleted function
 *   auto bad2 = 12_v - 2_a;        // error: use of deleted function
 *   auto bad3 = 23_celsius * 5_mm; // error: use of deleted function
 */
