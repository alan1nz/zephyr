#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>

int main()
{
	while (1)
	{

		printk("hello there gay boi\r\n");
		k_msleep(1);
	}
}