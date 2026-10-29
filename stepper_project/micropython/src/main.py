# SPDX-License-Identifier: Apache-2.0
#
# MicroPython version of app/src/main.cpp. Frozen into the firmware and run
# automatically at boot.

import time

import stepper
from units import amp, volt

stepper_dev = stepper.Stepper("stepper0")


def main():
    if not stepper_dev.is_ready():
        raise RuntimeError("stepper0 is not ready")

    v = volt(48.0)
    t = amp(1)
    a = v - volt(1.0)

    # /* Enable driver and set motion bounds */
    stepper_dev.enable()
    stepper_dev.set_microstep_interval(500_000)

    # /* 1. Move to absolute position +6400 (blocking) */
    stepper_dev.move_to(200 * 8 * 69)

    time.sleep_ms(1000)

    # stepper_dev.move_to(0)

    # /* 2. Move to absolute position 0 (returns to origin) */
    # stepper_dev.move_to(0)

    while True:
        print("gay")
        print("v = %f" % v.value())
        print("a = %f" % a.value())
        time.sleep_ms(1000)

        # run_blink()


main()
