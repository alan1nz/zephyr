#define DT_DRV_COMPAT my_led

#include "zephyr/device.h"
#include <app/drivers/myled.h>

static int myled_init(const struct device *device)
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
    return 0;
};

void run_blink(const struct device *dev)
{
    // 1. Ensure the device is ready
    if (!device_is_ready(dev))
    {
        return;
    }

    // 2. Cast dev->api to your vtable struct
    const struct my_led_apis *api = dev->api;

    const struct myled_config *cfg = dev->config;
    // 3. Call the backend function via the function pointer
    if (api && api->blink)
    {
        api->blink(dev);
    }

    k_msleep(cfg->blink_period);
}

static int set_led_period(const struct device *dev, uint8_t *state)
{
    return 1;
}

static void run_blink_imp(const struct device *dev)
{
    const struct myled_config *conf = (const struct myled_config *)dev->config;

    gpio_pin_toggle(conf->gpio_spec.port, conf->gpio_spec.pin);
}

static const struct my_led_apis apis = {
    .set = set_led_period,
    .blink = run_blink_imp,
};

#define MY_LED_DEFINE(inst)                                  \
    static struct myled_data myled_data_##inst;              \
    static struct myled_config myled_config_##inst = {       \
        .gpio_spec = GPIO_DT_SPEC_INST_GET(inst, gpios),     \
        .blink_period = DT_INST_PROP(inst, blink_period_ms), \
    };                                                       \
    DEVICE_DT_INST_DEFINE(inst,                              \
                          myled_init,                        \
                          NULL,                              \
                          &myled_data_##inst,                \
                          &myled_config_##inst,              \
                          POST_KERNEL,                       \
                          5,                                 \
                          &apis);

DT_INST_FOREACH_STATUS_OKAY(MY_LED_DEFINE)

#undef DT_DRV_COMPAT