#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

// #define LED DT_ALIAS(led0)
// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED, gpios);

int main()
{
	// if (!gpio_is_ready_dt(&led))
	// {
	// 	printk("LED is not ready");
	// 	return 1;
	// }

	// if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) != 0)
	// {
	// 	printk("LED failed to configure");
	// 	return 1;
	// }

	while (1)
	{
		printk("hello there gay boi\r\n");
		// gpio_pin_toggle_dt(&led);
		k_msleep(5000);
	}
}
