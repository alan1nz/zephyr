#include "myled_core.h"
#include <zephyr/drivers/gpio.h>

static const struct device *mDev = NULL;

int myled_core_init(const struct device *device)
{
    int ret;

    const struct myled_config *conf = (const struct myled_config *)device->config;

    if (!gpio_is_ready_dt(&conf->gpio_spec))
    {
        printk("GPIO not ready");
        return 1;
    }

    if (gpio_pin_configure_dt(&conf->gpio_spec, GPIO_OUTPUT_ACTIVE) != 0)
    {
        printk("LED failed to configure");
        return 1;
    }

    mDev = device;
    return 0;
};

void myled_core_run_blink()
{
    if (mDev == NULL)
    {
        return;
    }

    // 1. Ensure the device is ready
    if (!device_is_ready(mDev))
    {
        return;
    }

    // 2. Cast dev->api to your vtable struct
    const struct my_led_apis *api = mDev->api;
    const struct myled_config *cfg = mDev->config;

    gpio_pin_toggle(cfg->gpio_spec.port, cfg->gpio_spec.pin);

    k_msleep(cfg->blink_period);
}

int myled_core_set_led_period()
{
    return 1;
}
