#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <app/drivers/myled.h>

int main()
{

	while (1)
	{
		run_blink();
	}
}
