/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_DRIVERS_MY_DRIVER_H_
#define APP_DRIVERS_MY_DRIVER_H_

#include <zephyr/device.h>
#include <zephyr/toolchain.h>

/**
 * @defgroup drivers_my_driver MyDriver drivers
 * @ingroup drivers
 * @{
 *
 * @brief Custom "my_driver" driver class.
 */

/** @brief MyDriver driver class operations */
__subsystem struct my_driver_driver_api {
	/* TODO: add operation callbacks here, e.g.:
	 * int (*do_something)(const struct device *dev);
	 */
	int (*do_something)(const struct device *dev);
};

/**
 * @brief TODO: describe this call.
 *
 * @param dev MyDriver device instance.
 *
 * @retval 0 if successful.
 * @retval -errno Negative errno code on failure.
 */
__syscall int my_driver_do_something(const struct device *dev);

static inline int z_impl_my_driver_do_something(const struct device *dev)
{
	__ASSERT_NO_MSG(DEVICE_API_IS(my_driver, dev));

	return DEVICE_API_GET(my_driver, dev)->do_something(dev);
}

#include <zephyr/syscalls/my_driver.h>

/** @} */

#endif /* APP_DRIVERS_MY_DRIVER_H_ */
