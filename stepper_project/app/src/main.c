#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <app/drivers/myled.h>

#define MY_LED DT_NODELABEL(my_led1)
static const struct device *led = DEVICE_DT_GET(MY_LED);
// static const struct ;

int main()
{

	while (1)
	{
		// printk("hello there gay boi\r\n");
		run_blink(led);
		// k_msleep(20);
	}
}
