/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * C entry points for the Zephyr stepper API, called from src/stepper.rs.
 *
 * The stepper API is made of static inline syscall wrappers, which Rust can't
 * link against directly, and zephyr-lang-rust doesn't generate bindings for it.
 * These functions give each call a real symbol with a plain C ABI.
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>

const struct device *app_stepper0(void)
{
	return DEVICE_DT_GET(DT_NODELABEL(stepper0));
}

bool app_device_is_ready(const struct device *dev)
{
	return device_is_ready(dev);
}

int app_stepper_enable(const struct device *dev)
{
	return stepper_enable(dev);
}

int app_stepper_disable(const struct device *dev)
{
	return stepper_disable(dev);
}

int app_stepper_set_microstep_interval(const struct device *dev, uint64_t interval_ns)
{
	return stepper_ctrl_set_microstep_interval(dev, interval_ns);
}

int app_stepper_move_to(const struct device *dev, int32_t micro_steps)
{
	return stepper_ctrl_move_to(dev, micro_steps);
}

int app_stepper_move_by(const struct device *dev, int32_t micro_steps)
{
	return stepper_ctrl_move_by(dev, micro_steps);
}

int app_stepper_stop(const struct device *dev)
{
	return stepper_ctrl_stop(dev);
}

int app_stepper_is_moving(const struct device *dev, bool *is_moving)
{
	return stepper_ctrl_is_moving(dev, is_moving);
}

int app_stepper_get_actual_position(const struct device *dev, int32_t *position)
{
	return stepper_ctrl_get_actual_position(dev, position);
}

int app_stepper_set_reference_position(const struct device *dev, int32_t position)
{
	return stepper_ctrl_set_reference_position(dev, position);
}
