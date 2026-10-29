// SPDX-License-Identifier: Apache-2.0

//! Strongly-typed physical quantities.
//!
//! Rust counterpart of include/app/lib/units.hpp. Rust has no user-defined
//! literal suffixes, so each C++ suffix is a unit constant to multiply by:
//!
//! ```ignore
//! use units::{CELSIUS, V, VOLT};
//!
//! let voltage = 48.0 * V;         // Volts, like 48.0_v
//! let supply = 3.3 * VOLT;        // Volts, like 3.3_volt
//! let temperature = 23 * CELSIUS; // Celsius, like 23_celsius
//! ```
//!
//! As in C++, unit mix-ups are compile errors: same-unit quantities can be
//! added/subtracted and compared, any quantity can be scaled by a plain number
//! (in either order), and the only cross-unit product defined is
//! Volts * Amps -> Watts.

use core::marker::PhantomData;
use core::ops::{Add, Div, Mul, Sub};

/// A scalar quantity tagged with a unit type to prevent mix-ups.
#[derive(Debug, Clone, Copy, PartialEq, PartialOrd)]
pub struct Quantity<Tag> {
    value: f32,
    _unit: PhantomData<Tag>,
}

impl<Tag> Quantity<Tag> {
    pub const fn new(value: f32) -> Self {
        Self {
            value,
            _unit: PhantomData,
        }
    }

    pub const fn value(self) -> f32 {
        self.value
    }
}

// Only same-unit operands may be added/subtracted.
impl<Tag> Add for Quantity<Tag> {
    type Output = Self;

    fn add(self, rhs: Self) -> Self {
        Self::new(self.value + rhs.value)
    }
}

impl<Tag> Sub for Quantity<Tag> {
    type Output = Self;

    fn sub(self, rhs: Self) -> Self {
        Self::new(self.value - rhs.value)
    }
}

// Scaling by a plain number keeps the unit, in either operand order. Only one
// float and one integer type are supported so that untyped literals such as
// `48.0 * V` and `2 * V` infer unambiguously.
macro_rules! impl_scalar {
    ($($scalar:ty),*) => {$(
        impl<Tag> Mul<$scalar> for Quantity<Tag> {
            type Output = Self;

            fn mul(self, rhs: $scalar) -> Self {
                Self::new(self.value * rhs as f32)
            }
        }

        impl<Tag> Mul<Quantity<Tag>> for $scalar {
            type Output = Quantity<Tag>;

            fn mul(self, rhs: Quantity<Tag>) -> Quantity<Tag> {
                Quantity::new(self as f32 * rhs.value)
            }
        }

        impl<Tag> Div<$scalar> for Quantity<Tag> {
            type Output = Self;

            fn div(self, rhs: $scalar) -> Self {
                Self::new(self.value / rhs as f32)
            }
        }
    )*};
}

impl_scalar!(f32, i32);

pub mod tags {
    #[derive(Debug, Clone, Copy, PartialEq, PartialOrd)]
    pub struct Volts;
    #[derive(Debug, Clone, Copy, PartialEq, PartialOrd)]
    pub struct Amps;
    #[derive(Debug, Clone, Copy, PartialEq, PartialOrd)]
    pub struct Watts;
    #[derive(Debug, Clone, Copy, PartialEq, PartialOrd)]
    pub struct Celsius;
    #[derive(Debug, Clone, Copy, PartialEq, PartialOrd)]
    pub struct Millimeters;
    #[derive(Debug, Clone, Copy, PartialEq, PartialOrd)]
    pub struct Hertz;
    #[derive(Debug, Clone, Copy, PartialEq, PartialOrd)]
    pub struct Seconds;
}

pub type Volts = Quantity<tags::Volts>;
pub type Amps = Quantity<tags::Amps>;
pub type Watts = Quantity<tags::Watts>;
pub type Celsius = Quantity<tags::Celsius>;
pub type Millimeters = Quantity<tags::Millimeters>;
pub type Hertz = Quantity<tags::Hertz>;
pub type Seconds = Quantity<tags::Seconds>;

// The only cross-unit multiplication currently defined: P = V * I.
impl Mul<Amps> for Volts {
    type Output = Watts;

    fn mul(self, current: Amps) -> Watts {
        Watts::new(self.value * current.value)
    }
}

impl Mul<Volts> for Amps {
    type Output = Watts;

    fn mul(self, voltage: Volts) -> Watts {
        voltage * self
    }
}

// Unit constants mirroring the C++ literal suffixes. Each holds one of its
// unit, expressed in the quantity's base unit (e.g. MS is 0.001 seconds).
pub const V: Volts = Volts::new(1.0);
pub const VOLT: Volts = V;
pub const MV: Volts = Volts::new(1e-3);
pub const MILLIVOLT: Volts = MV;

pub const A: Amps = Amps::new(1.0);
pub const AMP: Amps = A;
pub const MA: Amps = Amps::new(1e-3);
pub const MILLIAMP: Amps = MA;

pub const W: Watts = Watts::new(1.0);
pub const WATT: Watts = W;

pub const CELSIUS: Celsius = Celsius::new(1.0);
/// Common misspelling kept as an alias so either name works.
pub const CELCIUS: Celsius = CELSIUS;

pub const MM: Millimeters = Millimeters::new(1.0);
pub const MILLIMETER: Millimeters = MM;

pub const HZ: Hertz = Hertz::new(1.0);
pub const HERTZ: Hertz = HZ;
pub const KHZ: Hertz = Hertz::new(1e3);

pub const S: Seconds = Seconds::new(1.0);
pub const SECOND: Seconds = S;
pub const MS: Seconds = Seconds::new(1e-3);
pub const MILLISECOND: Seconds = MS;
pub const US: Seconds = Seconds::new(1e-6);
