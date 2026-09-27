/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Unit tests for the "blink" gpio_led driver using Catch2 instead of ztest.
 * The device instance below is constructed entirely from devicetree (see
 * app.overlay), exactly as it would be on real hardware.
 */

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>

#include <app/drivers/blink.h>

namespace {

const struct device *led_dev = DEVICE_DT_GET(DT_NODELABEL(my_led1));
const struct gpio_dt_spec led_gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(my_led1), led_gpios);

} /* namespace */

SCENARIO("blink device readiness", "[blink]")
{
	GIVEN("a blink device constructed from devicetree")
	{
		THEN("it is ready")
		{
			REQUIRE(device_is_ready(led_dev));
		}
	}
}

SCENARIO("turning the blink LED off", "[blink]")
{
	GIVEN("a ready blink device")
	{
		WHEN("blink_off is called")
		{
			REQUIRE(blink_off(led_dev) == 0);
			k_msleep(50);

			THEN("the LED gpio is driven inactive")
			{
				REQUIRE(gpio_emul_output_get_dt(&led_gpio) == 0);
			}
		}
	}
}

SCENARIO("blinking the LED at a configured period", "[blink]")
{
	GIVEN("a ready blink device")
	{
		WHEN("blink_set_period_ms is called with a non-zero period")
		{
			REQUIRE(blink_set_period_ms(led_dev, 10) == 0);

			THEN("the LED gpio toggles over time")
			{
				int last = gpio_emul_output_get_dt(&led_gpio);
				bool toggled = false;

				/* Poll for a change in output level within a generous timeout. */
				for (int i = 0; i < 50 && !toggled; i++) {
					k_msleep(5);
					toggled = gpio_emul_output_get_dt(&led_gpio) != last;
				}

				REQUIRE(toggled);
			}

			AND_WHEN("blink_off is called afterwards")
			{
				THEN("it stops the blinking")
				{
					REQUIRE(blink_off(led_dev) == 0);
				}
			}
		}
	}
}

int main(void)
{
	int result = Catch::Session().run();

	/* native_sim is a real host process; returning from main() here does
	 * not terminate it (the idle thread just keeps the process alive), so
	 * exit explicitly instead.
	 */
	std::exit(result);
}
