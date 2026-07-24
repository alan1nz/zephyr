#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>

int main()
{
	while (1)
	{
		printk("hello there\r\n");
		k_msleep(1);
	}
}