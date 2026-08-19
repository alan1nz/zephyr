#include <zephyr/device.h>
#include <zephyr/toolchain.h>
#include <zephyr/drivers/gpio.h>

struct myled_data
{
    int something;
};

struct myled_config
{
    uint16_t blink_period;
    struct gpio_dt_spec gpio_spec;
};

struct my_led_apis
{
    int (*set)(const struct device *dev, uint8_t *state);
    void (*blink)(const struct device *dev);
};

void run_blink(const struct device *dev);
