/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Tests for the Grbl g-code parser module, using a mock machine that records
 * every callback.
 */

#include <string.h>

#include <zephyr/ztest.h>

#include <grbl/gcode.h>

#define TOL 1e-4f

enum call_kind {
	CALL_LINE,
	CALL_ARC,
	CALL_JOG,
	CALL_DWELL,
	CALL_SPINDLE,
	CALL_COOLANT,
	CALL_PROGRAM_END,
};

struct call {
	enum call_kind kind;
	float target[GC_N_AXIS];
	float offset[GC_N_AXIS];
	float radius;
	bool clockwise;
	struct gc_plan_data pl;
	uint8_t state;
	float value;
};

static struct call calls[16];
static size_t n_calls;
static struct gc_parser gc;

static struct call *record(enum call_kind kind)
{
	zassert_true(n_calls < ARRAY_SIZE(calls), "too many callbacks");
	struct call *c = &calls[n_calls++];

	memset(c, 0, sizeof(*c));
	c->kind = kind;
	return c;
}

static void mock_line(void *user_data, const float target[GC_N_AXIS],
		      const struct gc_plan_data *pl)
{
	struct call *c = record(CALL_LINE);

	memcpy(c->target, target, sizeof(c->target));
	c->pl = *pl;
}

static void mock_arc(void *user_data, const float target[GC_N_AXIS],
		     const struct gc_plan_data *pl, const float position[GC_N_AXIS],
		     const float offset[GC_N_AXIS], float radius, uint8_t axis_0,
		     uint8_t axis_1, uint8_t axis_linear, bool is_clockwise)
{
	struct call *c = record(CALL_ARC);

	memcpy(c->target, target, sizeof(c->target));
	memcpy(c->offset, offset, sizeof(c->offset));
	c->radius = radius;
	c->clockwise = is_clockwise;
	c->pl = *pl;
}

static uint8_t mock_jog(void *user_data, const float target[GC_N_AXIS],
			const struct gc_plan_data *pl)
{
	struct call *c = record(CALL_JOG);

	memcpy(c->target, target, sizeof(c->target));
	c->pl = *pl;
	return STATUS_OK;
}

static void mock_dwell(void *user_data, float seconds)
{
	record(CALL_DWELL)->value = seconds;
}

static void mock_spindle(void *user_data, uint8_t state, float rpm)
{
	struct call *c = record(CALL_SPINDLE);

	c->state = state;
	c->value = rpm;
}

static void mock_coolant(void *user_data, uint8_t state)
{
	record(CALL_COOLANT)->state = state;
}

static void mock_program_end(void *user_data, uint8_t program_flow)
{
	record(CALL_PROGRAM_END)->state = program_flow;
}

static const struct gc_machine_ops ops = {
	.line = mock_line,
	.arc = mock_arc,
	.jog = mock_jog,
	.dwell = mock_dwell,
	.spindle_sync = mock_spindle,
	.coolant_sync = mock_coolant,
	.program_end = mock_program_end,
};

static void assert_xyz(const float v[GC_N_AXIS], float x, float y, float z)
{
	zassert_within(v[GC_X_AXIS], x, TOL, "x = %f, expected %f", (double)v[0], (double)x);
	zassert_within(v[GC_Y_AXIS], y, TOL, "y = %f, expected %f", (double)v[1], (double)y);
	zassert_within(v[GC_Z_AXIS], z, TOL, "z = %f, expected %f", (double)v[2], (double)z);
}

static uint8_t run(const char *raw)
{
	n_calls = 0;
	return gc_execute_raw(&gc, raw);
}

static void before(void *fixture)
{
	zassert_equal(gc_init(&gc, &ops, NULL), STATUS_OK);
	n_calls = 0;
}

ZTEST_SUITE(grbl_gcode, NULL, NULL, before, NULL, NULL);

ZTEST(grbl_gcode, test_rapid_move)
{
	zassert_equal(run("G0 X10 Y20"), STATUS_OK);
	zassert_equal(n_calls, 1);
	zassert_equal(calls[0].kind, CALL_LINE);
	assert_xyz(calls[0].target, 10, 20, 0);
	zassert_true(calls[0].pl.condition & PL_COND_FLAG_RAPID_MOTION);
	assert_xyz(gc.state.position, 10, 20, 0);
}

ZTEST(grbl_gcode, test_feed_move_requires_feed_rate)
{
	zassert_equal(run("G1 X5"), STATUS_GCODE_UNDEFINED_FEED_RATE);
	zassert_equal(n_calls, 0);

	zassert_equal(run("G1 X5 F300"), STATUS_OK);
	zassert_equal(calls[0].kind, CALL_LINE);
	zassert_within(calls[0].pl.feed_rate, 300, TOL);
	zassert_false(calls[0].pl.condition & PL_COND_FLAG_RAPID_MOTION);

	/* Motion and feed rate are modal. */
	zassert_equal(run("Y7"), STATUS_OK);
	assert_xyz(calls[0].target, 5, 7, 0);
	zassert_within(calls[0].pl.feed_rate, 300, TOL);
}

ZTEST(grbl_gcode, test_inches_converted_to_mm)
{
	zassert_equal(run("G20 G1 X1 F10"), STATUS_OK);
	assert_xyz(calls[0].target, 25.4f, 0, 0);
	zassert_within(calls[0].pl.feed_rate, 254, TOL);
}

ZTEST(grbl_gcode, test_incremental_distance)
{
	zassert_equal(run("G0 X10"), STATUS_OK);
	zassert_equal(run("G91 G0 X5 Z-1"), STATUS_OK);
	assert_xyz(calls[0].target, 15, 0, -1);
}

ZTEST(grbl_gcode, test_comments_whitespace_and_case)
{
	zassert_equal(run("  g1 x5 (move to five) f100 ; trailing comment"), STATUS_OK);
	zassert_equal(n_calls, 1);
	assert_xyz(calls[0].target, 5, 0, 0);

	zassert_equal(run("(only a comment)"), STATUS_OK);
	zassert_equal(n_calls, 0);
}

ZTEST(grbl_gcode, test_errors_leave_state_unchanged)
{
	zassert_equal(run("G17 G18"), STATUS_GCODE_MODAL_GROUP_VIOLATION);
	zassert_equal(run("G0 G1 X1"), STATUS_GCODE_AXIS_COMMAND_CONFLICT);
	zassert_equal(run("G0 X1 X2"), STATUS_GCODE_WORD_REPEATED);
	zassert_equal(run("G0 X1 I1"), STATUS_GCODE_UNUSED_WORDS);
	zassert_equal(run("G1 F-10 X1"), STATUS_NEGATIVE_VALUE);
	zassert_equal(run("G5 X1"), STATUS_GCODE_UNSUPPORTED_COMMAND);
	zassert_equal(run("X"), STATUS_BAD_NUMBER_FORMAT);
	zassert_equal(run("1"), STATUS_EXPECTED_COMMAND_LETTER);
	zassert_equal(n_calls, 0);
	assert_xyz(gc.state.position, 0, 0, 0);
	zassert_equal(gc.state.modal.motion, MOTION_MODE_SEEK);
}

ZTEST(grbl_gcode, test_arc_center_offset)
{
	/* Quarter circle from (10,0) to (0,10) around the origin. */
	zassert_equal(run("G0 X10"), STATUS_OK);
	zassert_equal(run("G3 X0 Y10 I-10 J0 F100"), STATUS_OK);
	zassert_equal(calls[0].kind, CALL_ARC);
	assert_xyz(calls[0].target, 0, 10, 0);
	assert_xyz(calls[0].offset, -10, 0, 0);
	zassert_within(calls[0].radius, 10, TOL);
	zassert_false(calls[0].clockwise);
}

ZTEST(grbl_gcode, test_arc_radius)
{
	/* Clockwise quarter circle from (0,0) to (10,10) with R10: centre (10,0). */
	zassert_equal(run("G2 X10 Y10 R10 F100"), STATUS_OK);
	zassert_equal(calls[0].kind, CALL_ARC);
	zassert_true(calls[0].clockwise);
	assert_xyz(calls[0].offset, 10, 0, 0);

	/* Radius too small to reach the target. */
	zassert_equal(run("G2 X100 Y0 R10"), STATUS_GCODE_ARC_RADIUS_ERROR);
}

ZTEST(grbl_gcode, test_arc_without_callback_rejected)
{
	static const struct gc_machine_ops line_only = { .line = mock_line };

	gc_init(&gc, &line_only, NULL);
	zassert_equal(run("G2 X10 Y10 R10 F100"), STATUS_GCODE_UNSUPPORTED_COMMAND);
}

ZTEST(grbl_gcode, test_g92_offset)
{
	zassert_equal(run("G0 X10"), STATUS_OK);
	/* Current position becomes X0 in work coordinates. */
	zassert_equal(run("G92 X0"), STATUS_OK);
	zassert_equal(run("G0 X5"), STATUS_OK);
	assert_xyz(calls[0].target, 15, 0, 0);

	zassert_equal(run("G92.1"), STATUS_OK);
	zassert_equal(run("G0 X5"), STATUS_OK);
	assert_xyz(calls[0].target, 5, 0, 0);
}

ZTEST(grbl_gcode, test_work_coordinate_systems)
{
	/* Set G55 to X100 (kept in the parser's RAM storage) and switch to it. */
	zassert_equal(run("G10 L2 P2 X100"), STATUS_OK);
	zassert_equal(run("G55 G0 X1"), STATUS_OK);
	assert_xyz(calls[0].target, 101, 0, 0);

	/* G53 moves in machine coordinates for that block only. */
	zassert_equal(run("G53 G0 X1"), STATUS_OK);
	assert_xyz(calls[0].target, 1, 0, 0);
	zassert_equal(run("G0 X2"), STATUS_OK);
	assert_xyz(calls[0].target, 102, 0, 0);
}

ZTEST(grbl_gcode, test_spindle_coolant_dwell)
{
	zassert_equal(run("M3 S1000"), STATUS_OK);
	zassert_equal(calls[0].kind, CALL_SPINDLE);
	zassert_equal(calls[0].state, SPINDLE_ENABLE_CW);
	zassert_within(calls[0].value, 1000, TOL);

	zassert_equal(run("M7"), STATUS_OK);
	zassert_equal(calls[0].kind, CALL_COOLANT);
	zassert_equal(calls[0].state, COOLANT_MIST_ENABLE);

	zassert_equal(run("G4 P0.5"), STATUS_OK);
	zassert_equal(calls[0].kind, CALL_DWELL);
	zassert_within(calls[0].value, 0.5f, TOL);

	zassert_equal(run("G4"), STATUS_GCODE_VALUE_WORD_MISSING);
}

ZTEST(grbl_gcode, test_program_end_resets_modes)
{
	zassert_equal(run("G91 G1 F100 M3 S500 M8"), STATUS_OK);
	zassert_equal(run("M2"), STATUS_OK);

	/* Spindle and coolant stop, then the program end notification. */
	zassert_equal(n_calls, 3);
	zassert_equal(calls[0].kind, CALL_SPINDLE);
	zassert_equal(calls[0].state, SPINDLE_DISABLE);
	zassert_equal(calls[1].kind, CALL_COOLANT);
	zassert_equal(calls[1].state, COOLANT_DISABLE);
	zassert_equal(calls[2].kind, CALL_PROGRAM_END);
	zassert_equal(calls[2].state, PROGRAM_FLOW_COMPLETED_M2);

	zassert_equal(gc.state.modal.distance, DISTANCE_MODE_ABSOLUTE);
	zassert_equal(gc.state.modal.motion, MOTION_MODE_LINEAR);
	zassert_equal(gc.state.modal.spindle, SPINDLE_DISABLE);
	zassert_equal(gc.state.modal.coolant, COOLANT_DISABLE);
}

ZTEST(grbl_gcode, test_jog)
{
	zassert_equal(run("$J=G91 X2 F500"), STATUS_OK);
	zassert_equal(calls[0].kind, CALL_JOG);
	assert_xyz(calls[0].target, 2, 0, 0);
	zassert_within(calls[0].pl.feed_rate, 500, TOL);

	/* Jogging doesn't change the modal state... */
	zassert_equal(gc.state.modal.distance, DISTANCE_MODE_ABSOLUTE);
	/* ...but does track position, and requires a feed rate. */
	assert_xyz(gc.state.position, 2, 0, 0);
	zassert_equal(run("$J=X1"), STATUS_GCODE_UNDEFINED_FEED_RATE);
	zassert_equal(run("$J=M3 X1 F10"), STATUS_INVALID_JOG_COMMAND);
}

ZTEST(grbl_gcode, test_line_reader_overflow)
{
	struct gc_line_reader reader;
	enum gc_line_result result = GC_LINE_INCOMPLETE;

	gc_line_reader_init(&reader);
	for (int i = 0; i < CONFIG_GRBL_GCODE_LINE_BUFFER_SIZE + 5; i++) {
		result = gc_line_reader_feed(&reader, 'X');
		zassert_equal(result, GC_LINE_INCOMPLETE);
	}
	zassert_equal(gc_line_reader_feed(&reader, '\n'), GC_LINE_OVERFLOW);

	/* The reader recovers for the next line. */
	for (const char *p = "g0x1\r"; *p; p++) {
		result = gc_line_reader_feed(&reader, *p);
	}
	zassert_equal(result, GC_LINE_READY);
	zassert_str_equal(reader.line, "G0X1");
}
