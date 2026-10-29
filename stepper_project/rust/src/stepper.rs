// SPDX-License-Identifier: Apache-2.0

//! Safe wrapper around the Zephyr stepper API (see `stepper_shim.c`).

use core::ffi::c_int;

use zephyr::error::{to_result_void, Result};
use zephyr::raw::device;

extern "C" {
    fn app_stepper0() -> *const device;
    fn app_device_is_ready(dev: *const device) -> bool;
    fn app_stepper_enable(dev: *const device) -> c_int;
    fn app_stepper_disable(dev: *const device) -> c_int;
    fn app_stepper_set_microstep_interval(dev: *const device, interval_ns: u64) -> c_int;
    fn app_stepper_move_to(dev: *const device, micro_steps: i32) -> c_int;
    fn app_stepper_move_by(dev: *const device, micro_steps: i32) -> c_int;
    fn app_stepper_stop(dev: *const device) -> c_int;
    fn app_stepper_is_moving(dev: *const device, is_moving: *mut bool) -> c_int;
    fn app_stepper_get_actual_position(dev: *const device, position: *mut i32) -> c_int;
    fn app_stepper_set_reference_position(dev: *const device, position: i32) -> c_int;
}

/// A stepper motion controller device.
pub struct Stepper {
    dev: *const device,
}

impl Stepper {
    /// The device at devicetree node label `stepper0`.
    pub fn stepper0() -> Self {
        // SAFETY: returns a pointer to a statically allocated device.
        Self { dev: unsafe { app_stepper0() } }
    }

    pub fn is_ready(&self) -> bool {
        // SAFETY: `self.dev` is a valid static device pointer.
        unsafe { app_device_is_ready(self.dev) }
    }

    pub fn enable(&self) -> Result<()> {
        // SAFETY: `self.dev` is a valid static device pointer.
        to_result_void(unsafe { app_stepper_enable(self.dev) })
    }

    pub fn disable(&self) -> Result<()> {
        // SAFETY: `self.dev` is a valid static device pointer.
        to_result_void(unsafe { app_stepper_disable(self.dev) })
    }

    pub fn set_microstep_interval(&self, interval_ns: u64) -> Result<()> {
        // SAFETY: `self.dev` is a valid static device pointer.
        to_result_void(unsafe { app_stepper_set_microstep_interval(self.dev, interval_ns) })
    }

    pub fn move_to(&self, micro_steps: i32) -> Result<()> {
        // SAFETY: `self.dev` is a valid static device pointer.
        to_result_void(unsafe { app_stepper_move_to(self.dev, micro_steps) })
    }

    pub fn move_by(&self, micro_steps: i32) -> Result<()> {
        // SAFETY: `self.dev` is a valid static device pointer.
        to_result_void(unsafe { app_stepper_move_by(self.dev, micro_steps) })
    }

    pub fn stop(&self) -> Result<()> {
        // SAFETY: `self.dev` is a valid static device pointer.
        to_result_void(unsafe { app_stepper_stop(self.dev) })
    }

    pub fn is_moving(&self) -> Result<bool> {
        let mut is_moving = false;
        // SAFETY: `self.dev` is a valid static device pointer and `is_moving`
        // outlives the call.
        to_result_void(unsafe { app_stepper_is_moving(self.dev, &mut is_moving) })?;
        Ok(is_moving)
    }

    pub fn position(&self) -> Result<i32> {
        let mut position = 0;
        // SAFETY: `self.dev` is a valid static device pointer and `position`
        // outlives the call.
        to_result_void(unsafe { app_stepper_get_actual_position(self.dev, &mut position) })?;
        Ok(position)
    }

    pub fn set_reference_position(&self, position: i32) -> Result<()> {
        // SAFETY: `self.dev` is a valid static device pointer.
        to_result_void(unsafe { app_stepper_set_reference_position(self.dev, position) })
    }
}
