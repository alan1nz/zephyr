# SPDX-License-Identifier: Apache-2.0
"""Strongly-typed physical quantities.

MicroPython counterpart of include/app/lib/units.hpp. Python has no
user-defined literals, so each C++ suffix is a constructor function of the
same name:

    from units import volt, celsius

    voltage = volt(48.0)        # units.Volts, like 48.0_v / 48.0_volt
    temperature = celsius(23)   # units.Celsius, like 23_celsius

Same-unit quantities can be added/subtracted, any quantity can be scaled by a
plain number (in either order), and the only cross-unit product defined is
Volts * Amps -> Watts. Any other mixed-unit +, -, or * raises TypeError. In
C++ these mix-ups are compile errors; in Python they are caught at runtime.
"""


class Quantity:
    """A scalar quantity tagged with a unit type to prevent mix-ups."""

    __slots__ = ("_value",)

    def __init__(self, value):
        self._value = float(value)

    def value(self):
        return self._value

    def _same_unit(self, other, op):
        if type(other) is not type(self):
            raise TypeError(
                "unsupported operand types for %s: '%s' and '%s'"
                % (op, type(self).__name__, type(other).__name__)
            )

    # Only same-unit operands may be added/subtracted.
    def __add__(self, other):
        self._same_unit(other, "+")
        return type(self)(self._value + other._value)

    def __sub__(self, other):
        self._same_unit(other, "-")
        return type(self)(self._value - other._value)

    # Scaling by a plain number keeps the unit, in either operand order.
    def __mul__(self, other):
        if isinstance(other, (int, float)):
            return type(self)(self._value * other)
        product = _PRODUCTS.get((type(self), type(other)))
        if product is None:
            raise TypeError(
                "unsupported operand types for *: '%s' and '%s'"
                % (type(self).__name__, type(other).__name__)
            )
        return product(self._value * other._value)

    def __rmul__(self, other):
        if isinstance(other, (int, float)):
            return type(self)(other * self._value)
        return NotImplemented

    def __truediv__(self, other):
        if isinstance(other, (int, float)):
            return type(self)(self._value / other)
        return NotImplemented

    def __eq__(self, other):
        return type(other) is type(self) and self._value == other._value

    def __lt__(self, other):
        self._same_unit(other, "<")
        return self._value < other._value

    def __le__(self, other):
        self._same_unit(other, "<=")
        return self._value <= other._value

    def __gt__(self, other):
        self._same_unit(other, ">")
        return self._value > other._value

    def __ge__(self, other):
        self._same_unit(other, ">=")
        return self._value >= other._value

    def __repr__(self):
        return "%s(%r)" % (type(self).__name__, self._value)


class Volts(Quantity):
    pass


class Amps(Quantity):
    pass


class Watts(Quantity):
    pass


class Celsius(Quantity):
    pass


class Millimeters(Quantity):
    pass


class Hertz(Quantity):
    pass


class Seconds(Quantity):
    pass


# The only cross-unit multiplication currently defined: P = V * I.
_PRODUCTS = {
    (Volts, Amps): Watts,
    (Amps, Volts): Watts,
}


# Constructors mirroring the C++ literal suffixes. `scale` converts the given
# number into the quantity's base unit, e.g. ms(500) -> Seconds(0.5).
def _unit(cls, scale=1.0):
    def make(value):
        return cls(value * scale)

    return make


v = volt = _unit(Volts)
mv = millivolt = _unit(Volts, 1e-3)

a = amp = _unit(Amps)
ma = milliamp = _unit(Amps, 1e-3)

w = watt = _unit(Watts)

# "celcius" is a common misspelling kept as an alias so either name works.
celsius = celcius = _unit(Celsius)

mm = millimeter = _unit(Millimeters)

hz = hertz = _unit(Hertz)
khz = _unit(Hertz, 1e3)

s = second = _unit(Seconds)
ms = millisecond = _unit(Seconds, 1e-3)
us = _unit(Seconds, 1e-6)
