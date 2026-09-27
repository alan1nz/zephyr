#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <app/drivers/myled.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <app/lib/units.hpp>

using namespace units::literals;

static const struct device *stepper_dev = DEVICE_DT_GET(DT_NODELABEL(stepper0));


int main()
{

	bool ret = device_is_ready(stepper_dev);

	if(!ret){
        k_panic();
    }


    auto v = 48.0;
    auto t = 1_a;
    auto a = v - 1.0_v;

    // /* Enable driver and set motion bounds */
    stepper_enable(stepper_dev);
    stepper_ctrl_set_microstep_interval(stepper_dev, 500e3);

    // /* 1. Move to absolute position +6400 (blocking) */
    stepper_ctrl_move_to(stepper_dev, 200*8*69);

    k_msleep(1000);

	
    // stepper_ctrl_move_to(stepper_dev, 0);

    // /* 2. Move to absolute position 0 (returns to origin) */
    // stepper_move_to(stepper_dev, 0, NULL);

	while (1)
	{
        printk("gay\r\n");
        printk("v = %f\n", v.value());
        printk("a = %f\n", a.value());
        k_msleep(1000);


        
		// run_blink();
	}
}
