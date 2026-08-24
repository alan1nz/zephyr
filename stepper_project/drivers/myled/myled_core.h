#ifndef MY_LED_CORE_H
#define MY_LED_CORE_H

#include <zephyr/device.h>
#include <app/drivers/myled.h>

int myled_core_init(const struct device *device);
void myled_core_run_blink();
int myled_core_set_led_period();
// void myled_core_run_blink_imp();

#endif