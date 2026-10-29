// SPDX-License-Identifier: Apache-2.0
//
// Rust version of app/src/main.cpp.

#![no_std]

pub mod stepper;
pub mod units;

use zephyr::printkln;
use zephyr::time::{sleep, Duration};
// Routes Rust panics to k_panic().
use zephyr_panic as _;

use stepper::Stepper;
use units::{A, V};

#[no_mangle]
extern "C" fn rust_main() {
    let stepper_dev = Stepper::stepper0();

    if !stepper_dev.is_ready() {
        panic!("stepper0 is not ready");
    }

    let v = 48.0 * V;
    let _t = 1 * A;
    let a = v - 1.0 * V;

    // As in main.cpp, driver errors from these calls are ignored.

    // /* Enable driver and set motion bounds */
    let _ = stepper_dev.enable();
    let _ = stepper_dev.set_microstep_interval(500_000);

    // /* 1. Move to absolute position +6400 (blocking) */
    let _ = stepper_dev.move_to(200 * 8 * 69);

    sleep(Duration::millis(1000));

    // stepper_dev.move_to(0);

    // /* 2. Move to absolute position 0 (returns to origin) */
    // stepper_dev.move_to(0);

    loop {
        printkln!("gay\r");
        printkln!("v = {:.6}", v.value());
        printkln!("a = {:.6}", a.value());
        sleep(Duration::millis(1000));

        // run_blink();
    }
}
