# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_MY_DRIVER_MY_DRIVER my_driver.c)
