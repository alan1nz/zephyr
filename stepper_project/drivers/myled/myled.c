#define DT_DRV_COMPAT my_led

#include "zephyr/device.h"
#include <app/drivers/myled.h>
#include "myled_core.h"

static const struct my_led_apis apis = {
    // .set = myled_core_set_led_period,
    // .blink = myled_core_run_blink,
};

void run_blink()
{
    myled_core_run_blink();
};

#define MY_LED_DEFINE(inst)                                  \
    static struct myled_data myled_data_##inst;              \
    static struct myled_config myled_config_##inst = {       \
        .gpio_spec = GPIO_DT_SPEC_INST_GET(inst, gpios),     \
        .blink_period = DT_INST_PROP(inst, blink_period_ms), \
    };                                                       \
    DEVICE_DT_INST_DEFINE(inst,                              \
                          myled_core_init,                   \
                          NULL,                              \
                          &myled_data_##inst,                \
                          &myled_config_##inst,              \
                          POST_KERNEL,                       \
                          5,                                 \
                          &apis);

DT_INST_FOREACH_STATUS_OKAY(MY_LED_DEFINE)

#undef DT_DRV_COMPAT