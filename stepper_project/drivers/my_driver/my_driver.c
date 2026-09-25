/*
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT my_driver_my_driver

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <app/drivers/my_driver.h>

LOG_MODULE_REGISTER(my_driver_my_driver, CONFIG_MY_DRIVER_LOG_LEVEL);

struct my_driver_my_driver_data {
	/* TODO: add per-instance runtime data here. */
};

struct my_driver_my_driver_config {
	/* TODO: add per-instance devicetree-derived config here. */
};

static DEVICE_API(my_driver, my_driver_my_driver_api) = {
	/* TODO: assign operation callbacks here. */
};

static int my_driver_my_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* TODO: initialize the device here. */

	return 0;
}

#define MY_DRIVER_MY_DRIVER_DEFINE(inst)                                            \
	static struct my_driver_my_driver_data data##inst;                          \
                                                                                \
	static const struct my_driver_my_driver_config config##inst = {             \
	};                                                                     \
                                                                                \
	DEVICE_DT_INST_DEFINE(inst, my_driver_my_driver_init, NULL, &data##inst,    \
			      &config##inst, POST_KERNEL,                      \
			      CONFIG_MY_DRIVER_INIT_PRIORITY,                    \
			      &my_driver_my_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MY_DRIVER_MY_DRIVER_DEFINE)
