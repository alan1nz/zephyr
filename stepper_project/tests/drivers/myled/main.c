#include <limits.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/gpio.h>
#include <myled_core.h>

/* 1. Mock GPIO pin configuration function returning an error */
static int mock_gpio_pin_configure_fail(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
    ARG_UNUSED(port);
    ARG_UNUSED(pin);
    ARG_UNUSED(flags);
    return -EIO;
}

/* 2. Register mock API in the official GPIO linker section */
STRUCT_SECTION_ITERABLE(gpio_driver_api, mock_gpio_api) = {
    .pin_configure = mock_gpio_pin_configure_fail,
};

/* 3. Mock GPIO device state, dummy config, and driver data */
static struct device_state mock_gpio_state = {
    .initialized = true,
};

static const struct gpio_driver_config mock_gpio_config = {
    .port_pin_mask = BIT_MASK(32),
};

static struct gpio_driver_data mock_gpio_data;

/* 4. Mock GPIO controller device structure */
static const struct device mock_gpio_port = {
    .name = "MOCK_GPIO_PORT",
    .api = &mock_gpio_api,
    .state = &mock_gpio_state,
    .config = &mock_gpio_config,
    .data = &mock_gpio_data,
};

/* 5. Driver test configuration and target device */
static struct myled_config test_config = {
    .gpio_spec = {
        .port = &mock_gpio_port,
        .pin = 1,
        .dt_flags = GPIO_OUTPUT_ACTIVE,
    },
    .blink_period = 10,
};

static const struct device test_dev = {
    .name = "MYLED_TEST_DEV",
    .config = &test_config,
    .api = NULL,
};

/* 6. Test logic */
ZTEST(myled_suite, test_init_gpio_configure_failure)
{
    int ret = myled_core_init(&test_dev);
    zassert_equal(ret, 1, "Expected init to return 1 when gpio_pin_configure_dt fails");
}

ZTEST_SUITE(myled_suite, NULL, NULL, NULL, NULL, NULL);