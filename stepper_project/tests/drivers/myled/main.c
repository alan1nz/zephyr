#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/drivers/gpio.h>
#include <myled_core.h>

DEFINE_FFF_GLOBALS;

/* 1. Declare FFF fake function for pin_configure */
FAKE_VALUE_FUNC(int, mock_gpio_pin_configure, const struct device *, gpio_pin_t, gpio_flags_t);

/* 2. Register mock API table in Zephyr's API section */
STRUCT_SECTION_ITERABLE(gpio_driver_api, fff_gpio_api) = {
    .pin_configure = mock_gpio_pin_configure,
};

/* 3. Extract device and config from Devicetree overlay */
static const struct gpio_dt_spec test_gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(test_led), gpios);

static const struct myled_config test_config = {
    .gpio_spec = test_gpio,
    .blink_period = 10,
};

static struct device test_dev = {
    .name = "MYLED_TEST_DEV",
    .config = &test_config,
    .api = NULL,
};

static void test_setup(void *fixture)
{
    RESET_FAKE(mock_gpio_pin_configure);
    FFF_RESET_HISTORY();

    /* Override DT device API pointer to point to our FFF mock API */
    ((struct device *)test_gpio.port)->api = &fff_gpio_api;
}

ZTEST(myled_suite, test_init_configure_failure)
{
    /* Tell FFF to return an error */
    mock_gpio_pin_configure_fake.return_val = -EIO;

    int ret = myled_core_init(&test_dev);

    zassert_equal(ret, 1, "Expected init to fail when configure fails");
    zassert_equal(mock_gpio_pin_configure_fake.call_count, 1, "Expected 1 call to pin_configure");
}

ZTEST(myled_suite, test_init_configure_success)
{
    /* Tell FFF to return fail */
    mock_gpio_pin_configure_fake.return_val = 1;

    int ret = myled_core_init(&test_dev);

    zassert_equal(ret, 0, "Expected init to succeed");
    zassert_equal(mock_gpio_pin_configure_fake.call_count, 1, "Expected 1 call to pin_configure");
}

ZTEST_SUITE(myled_suite, NULL, NULL, test_setup, NULL, NULL);