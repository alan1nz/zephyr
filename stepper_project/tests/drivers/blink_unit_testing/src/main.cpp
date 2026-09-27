/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Pure unit test of the "blink" gpio_led driver logic on the unit_testing
 * board: no kernel, no devicetree, no driver model are built here, so every
 * dependency the driver relies on is faked by hand, and the driver source is
 * pulled in directly (its functions are static) instead of linked as a
 * library.
 */

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>

#include <zephyr/fff.h>

DEFINE_FFF_GLOBALS;

extern "C" {

/* device.c isn't built for unit_testing; the driver only needs readiness. */
FAKE_VALUE_FUNC(bool, device_is_ready, const struct device *);

/* kernel/timer.c isn't built either; fake the whole k_timer surface used by
 * the driver instead of a real, ticking timer.
 */
FAKE_VOID_FUNC(k_timer_init, struct k_timer *, k_timer_expiry_t, k_timer_stop_t);
FAKE_VOID_FUNC(k_timer_start, struct k_timer *, k_timeout_t, k_timeout_t);
FAKE_VOID_FUNC(k_timer_stop, struct k_timer *);
FAKE_VOID_FUNC(k_timer_user_data_set, struct k_timer *, void *);
FAKE_VALUE_FUNC(void *, k_timer_user_data_get, const struct k_timer *);

/* Fakes standing in for the GPIO driver's own low-level pin operations. */
FAKE_VALUE_FUNC(int, fake_gpio_pin_configure, const struct device *, gpio_pin_t, gpio_flags_t);
FAKE_VALUE_FUNC(int, fake_gpio_port_set_bits_raw, const struct device *, gpio_port_pins_t);
FAKE_VALUE_FUNC(int, fake_gpio_port_clear_bits_raw, const struct device *, gpio_port_pins_t);
FAKE_VALUE_FUNC(int, fake_gpio_port_toggle_bits, const struct device *, gpio_port_pins_t);

} /* extern "C" */

/* Register the fakes above as the (only) GPIO driver API implementation. */
STRUCT_SECTION_ITERABLE(gpio_driver_api, fake_gpio_api) = {
	.pin_configure = fake_gpio_pin_configure,
	.port_set_bits_raw = fake_gpio_port_set_bits_raw,
	.port_clear_bits_raw = fake_gpio_port_clear_bits_raw,
	.port_toggle_bits = fake_gpio_port_toggle_bits,
};

/* Pull in the driver under test: its functions are `static`, so this is the
 * only way to reach them without a devicetree-generated instance.
 */
#include "../../../../drivers/blink/gpio_led.c"

namespace {

struct device_state ready_state = {
	.init_res = 0,
	.initialized = true,
};

struct device gpio_port_dev = {
	.name = "fake_gpio_port",
	.state = &ready_state,
};

struct device led_dev = {
	.name = "led0",
};

struct gpio_dt_spec led_gpio = {
	.port = &gpio_port_dev,
	.pin = 0,
	.dt_flags = GPIO_ACTIVE_HIGH,
};

struct blink_gpio_led_config led_config = {
	.led = led_gpio,
	.period_ms = 0,
};

struct blink_gpio_led_data led_data{};

void reset_fakes()
{
	RESET_FAKE(device_is_ready);
	RESET_FAKE(k_timer_init);
	RESET_FAKE(k_timer_start);
	RESET_FAKE(k_timer_stop);
	RESET_FAKE(k_timer_user_data_set);
	RESET_FAKE(k_timer_user_data_get);
	RESET_FAKE(fake_gpio_pin_configure);
	RESET_FAKE(fake_gpio_port_set_bits_raw);
	RESET_FAKE(fake_gpio_port_clear_bits_raw);
	RESET_FAKE(fake_gpio_port_toggle_bits);
	FFF_RESET_HISTORY();

	device_is_ready_fake.return_val = true;
	fake_gpio_pin_configure_fake.return_val = 0;
	fake_gpio_port_set_bits_raw_fake.return_val = 0;
	fake_gpio_port_clear_bits_raw_fake.return_val = 0;
	fake_gpio_port_toggle_bits_fake.return_val = 0;

	led_data = {};
	led_dev.data = &led_data;
	led_dev.config = &led_config;
	led_dev.api = &blink_gpio_led_api;
}

} /* namespace */

SCENARIO("blink device readiness", "[blink]")
{
	reset_fakes();

	GIVEN("a blink device constructed by hand instead of devicetree")
	{
		WHEN("the driver is initialized")
		{
			REQUIRE(blink_gpio_led_init(&led_dev) == 0);

			THEN("it configures the underlying LED gpio")
			{
				REQUIRE(fake_gpio_pin_configure_fake.call_count == 1);
			}
		}
	}
}

SCENARIO("turning the blink LED off", "[blink]")
{
	reset_fakes();
	REQUIRE(blink_gpio_led_init(&led_dev) == 0);

	GIVEN("an initialized blink device")
	{
		WHEN("blink_off is called")
		{
			REQUIRE(blink_off(&led_dev) == 0);

			THEN("the LED gpio is driven inactive and the timer is stopped")
			{
				REQUIRE(fake_gpio_port_clear_bits_raw_fake.call_count == 1);
				REQUIRE(k_timer_stop_fake.call_count == 1);
			}
		}
	}
}

SCENARIO("blinking the LED at a configured period", "[blink]")
{
	reset_fakes();
	REQUIRE(blink_gpio_led_init(&led_dev) == 0);

	GIVEN("an initialized blink device")
	{
		WHEN("blink_set_period_ms is called with a non-zero period")
		{
			REQUIRE(blink_set_period_ms(&led_dev, 10) == 0);

			THEN("the timer is (re)started with that period")
			{
				REQUIRE(k_timer_start_fake.call_count == 1);
			}

			AND_WHEN("the timer fires (simulated directly: there is no real "
				 "scheduler on unit_testing)")
			{
				blink_gpio_led_on_timer_expire(&led_data.timer);

				THEN("the LED gpio is toggled")
				{
					REQUIRE(fake_gpio_port_toggle_bits_fake.call_count == 1);
				}
			}

			AND_WHEN("blink_off is called afterwards")
			{
				THEN("it stops the blinking")
				{
					REQUIRE(blink_off(&led_dev) == 0);
				}
			}
		}
	}
}

int main(void)
{
	int result = Catch::Session().run();

	/* unit_testing produces a normal host binary; make sure the process
	 * actually reports the result via its exit code.
	 */
	std::exit(result);
}
