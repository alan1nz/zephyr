/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * MicroPython bindings for the Zephyr stepper driver API.
 *
 *     import stepper
 *
 *     dev = stepper.Stepper("stepper0")  # devicetree node label or device name
 *     dev.enable()
 *     dev.set_microstep_interval(500_000) # nanoseconds
 *     dev.move_to(6400)
 *
 * Driver errors are raised as OSError(errno).
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "zephyr_device.h"

typedef struct _stepper_obj_t {
	mp_obj_base_t base;
	const struct device *dev;
} stepper_obj_t;

static void check_ret(int ret)
{
	if (ret < 0) {
		mp_raise_OSError(-ret);
	}
}

static mp_obj_t stepper_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw,
				 const mp_obj_t *args)
{
	mp_arg_check_num(n_args, n_kw, 1, 1, false);

	stepper_obj_t *self = mp_obj_malloc(stepper_obj_t, type);
	self->dev = zephyr_device_find(args[0]);
	return MP_OBJ_FROM_PTR(self);
}

static void stepper_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
	stepper_obj_t *self = MP_OBJ_TO_PTR(self_in);
	mp_printf(print, "<Stepper %s>", self->dev->name);
}

static mp_obj_t stepper_is_ready(mp_obj_t self_in)
{
	stepper_obj_t *self = MP_OBJ_TO_PTR(self_in);
	return mp_obj_new_bool(device_is_ready(self->dev));
}
static MP_DEFINE_CONST_FUN_OBJ_1(stepper_is_ready_obj, stepper_is_ready);

static mp_obj_t stepper_enable_(mp_obj_t self_in)
{
	stepper_obj_t *self = MP_OBJ_TO_PTR(self_in);
	check_ret(stepper_enable(self->dev));
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(stepper_enable_obj, stepper_enable_);

static mp_obj_t stepper_disable_(mp_obj_t self_in)
{
	stepper_obj_t *self = MP_OBJ_TO_PTR(self_in);
	check_ret(stepper_disable(self->dev));
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(stepper_disable_obj, stepper_disable_);

static mp_obj_t stepper_set_microstep_interval(mp_obj_t self_in, mp_obj_t interval_ns_in)
{
	stepper_obj_t *self = MP_OBJ_TO_PTR(self_in);
	uint64_t interval_ns = (uint64_t)mp_obj_get_ll(interval_ns_in);

	check_ret(stepper_ctrl_set_microstep_interval(self->dev, interval_ns));
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(stepper_set_microstep_interval_obj,
				 stepper_set_microstep_interval);

static mp_obj_t stepper_move_to(mp_obj_t self_in, mp_obj_t micro_steps_in)
{
	stepper_obj_t *self = MP_OBJ_TO_PTR(self_in);
	check_ret(stepper_ctrl_move_to(self->dev, mp_obj_get_int(micro_steps_in)));
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(stepper_move_to_obj, stepper_move_to);

static mp_obj_t stepper_move_by(mp_obj_t self_in, mp_obj_t micro_steps_in)
{
	stepper_obj_t *self = MP_OBJ_TO_PTR(self_in);
	check_ret(stepper_ctrl_move_by(self->dev, mp_obj_get_int(micro_steps_in)));
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(stepper_move_by_obj, stepper_move_by);

static mp_obj_t stepper_stop(mp_obj_t self_in)
{
	stepper_obj_t *self = MP_OBJ_TO_PTR(self_in);
	check_ret(stepper_ctrl_stop(self->dev));
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(stepper_stop_obj, stepper_stop);

static mp_obj_t stepper_is_moving(mp_obj_t self_in)
{
	stepper_obj_t *self = MP_OBJ_TO_PTR(self_in);
	bool is_moving;

	check_ret(stepper_ctrl_is_moving(self->dev, &is_moving));
	return mp_obj_new_bool(is_moving);
}
static MP_DEFINE_CONST_FUN_OBJ_1(stepper_is_moving_obj, stepper_is_moving);

static mp_obj_t stepper_position(mp_obj_t self_in)
{
	stepper_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int32_t position;

	check_ret(stepper_ctrl_get_actual_position(self->dev, &position));
	return mp_obj_new_int(position);
}
static MP_DEFINE_CONST_FUN_OBJ_1(stepper_position_obj, stepper_position);

static mp_obj_t stepper_set_reference_position(mp_obj_t self_in, mp_obj_t position_in)
{
	stepper_obj_t *self = MP_OBJ_TO_PTR(self_in);
	check_ret(stepper_ctrl_set_reference_position(self->dev, mp_obj_get_int(position_in)));
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(stepper_set_reference_position_obj,
				 stepper_set_reference_position);

static const mp_rom_map_elem_t stepper_locals_dict_table[] = {
	{MP_ROM_QSTR(MP_QSTR_is_ready), MP_ROM_PTR(&stepper_is_ready_obj)},
	{MP_ROM_QSTR(MP_QSTR_enable), MP_ROM_PTR(&stepper_enable_obj)},
	{MP_ROM_QSTR(MP_QSTR_disable), MP_ROM_PTR(&stepper_disable_obj)},
	{MP_ROM_QSTR(MP_QSTR_set_microstep_interval),
	 MP_ROM_PTR(&stepper_set_microstep_interval_obj)},
	{MP_ROM_QSTR(MP_QSTR_move_to), MP_ROM_PTR(&stepper_move_to_obj)},
	{MP_ROM_QSTR(MP_QSTR_move_by), MP_ROM_PTR(&stepper_move_by_obj)},
	{MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&stepper_stop_obj)},
	{MP_ROM_QSTR(MP_QSTR_is_moving), MP_ROM_PTR(&stepper_is_moving_obj)},
	{MP_ROM_QSTR(MP_QSTR_position), MP_ROM_PTR(&stepper_position_obj)},
	{MP_ROM_QSTR(MP_QSTR_set_reference_position),
	 MP_ROM_PTR(&stepper_set_reference_position_obj)},
};
static MP_DEFINE_CONST_DICT(stepper_locals_dict, stepper_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
	stepper_type,
	MP_QSTR_Stepper,
	MP_TYPE_FLAG_NONE,
	make_new, stepper_make_new,
	print, stepper_print,
	locals_dict, &stepper_locals_dict
	);

static const mp_rom_map_elem_t stepper_module_globals_table[] = {
	{MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_stepper)},
	{MP_ROM_QSTR(MP_QSTR_Stepper), MP_ROM_PTR(&stepper_type)},
};
static MP_DEFINE_CONST_DICT(stepper_module_globals, stepper_module_globals_table);

const mp_obj_module_t stepper_module = {
	.base = {&mp_type_module},
	.globals = (mp_obj_dict_t *)&stepper_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_stepper, stepper_module);
