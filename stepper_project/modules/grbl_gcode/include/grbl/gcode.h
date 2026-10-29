/*
  gcode.h - rs274/ngc parser.
  Part of Grbl

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.

  SPDX-License-Identifier: GPL-3.0-or-later

  Modified for Zephyr: extracted from Grbl v1.1h as a standalone parser. All
  machine side effects (motion, spindle, coolant, coordinate storage, ...) are
  delegated to application callbacks in struct gc_machine_ops, and parser state
  lives in a struct gc_parser instance instead of globals.
*/

#ifndef GRBL_GCODE_H_
#define GRBL_GCODE_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Axis indexing. The parser supports the three linear axes X, Y and Z.
#define GC_N_AXIS 3
#define GC_X_AXIS 0
#define GC_Y_AXIS 1
#define GC_Z_AXIS 2

// Number of supported work coordinate systems G54-G59.
#define GC_N_COORDINATE_SYSTEM 6
// Coordinate data storage indices passed to the read/write_coord_data callbacks:
// 0-5 = G54-G59, then the G28 and G30 predefined positions.
#define GC_COORD_INDEX_G28 GC_N_COORDINATE_SYSTEM
#define GC_COORD_INDEX_G30 (GC_N_COORDINATE_SYSTEM + 1)
#define GC_N_COORD_DATA    (GC_N_COORDINATE_SYSTEM + 2)

// Status codes returned by the parser. Values match Grbl's report.h, so they can
// be reported to Grbl-compatible senders as "error:<code>".
#define STATUS_OK 0
#define STATUS_EXPECTED_COMMAND_LETTER 1
#define STATUS_BAD_NUMBER_FORMAT 2
#define STATUS_INVALID_STATEMENT 3
#define STATUS_NEGATIVE_VALUE 4
#define STATUS_SETTING_READ_FAIL 7
#define STATUS_OVERFLOW 11
#define STATUS_INVALID_JOG_COMMAND 16

#define STATUS_GCODE_UNSUPPORTED_COMMAND 20
#define STATUS_GCODE_MODAL_GROUP_VIOLATION 21
#define STATUS_GCODE_UNDEFINED_FEED_RATE 22
#define STATUS_GCODE_COMMAND_VALUE_NOT_INTEGER 23
#define STATUS_GCODE_AXIS_COMMAND_CONFLICT 24
#define STATUS_GCODE_WORD_REPEATED 25
#define STATUS_GCODE_NO_AXIS_WORDS 26
#define STATUS_GCODE_INVALID_LINE_NUMBER 27
#define STATUS_GCODE_VALUE_WORD_MISSING 28
#define STATUS_GCODE_UNSUPPORTED_COORD_SYS 29
#define STATUS_GCODE_G53_INVALID_MOTION_MODE 30
#define STATUS_GCODE_AXIS_WORDS_EXIST 31
#define STATUS_GCODE_NO_AXIS_WORDS_IN_PLANE 32
#define STATUS_GCODE_INVALID_TARGET 33
#define STATUS_GCODE_ARC_RADIUS_ERROR 34
#define STATUS_GCODE_NO_OFFSETS_IN_PLANE 35
#define STATUS_GCODE_UNUSED_WORDS 36
#define STATUS_GCODE_G43_DYNAMIC_AXIS_ERROR 37
#define STATUS_GCODE_MAX_VALUE_EXCEEDED 38

// Planner condition flags passed to motion callbacks in gc_plan_data.condition.
#define PL_COND_FLAG_RAPID_MOTION      (1 << 0)
#define PL_COND_FLAG_SYSTEM_MOTION     (1 << 1) // Single motion. Circumvents planner state. Used by home/park.
#define PL_COND_FLAG_NO_FEED_OVERRIDE  (1 << 2) // Motion does not honor feed override.
#define PL_COND_FLAG_INVERSE_TIME      (1 << 3) // Interprets feed rate value as inverse time when set.
#define PL_COND_FLAG_SPINDLE_CW        (1 << 4)
#define PL_COND_FLAG_SPINDLE_CCW       (1 << 5)
#define PL_COND_FLAG_COOLANT_FLOOD     (1 << 6)
#define PL_COND_FLAG_COOLANT_MIST      (1 << 7)

// Define modal group internal numbers for checking multiple command violations and tracking the
// type of command that is called in the block. A modal group is a group of g-code commands that are
// mutually exclusive, or cannot exist on the same line, because they each toggle a state or execute
// a unique motion. These are defined in the NIST RS274-NGC v3 g-code standard, available online,
// and are similar/identical to other g-code interpreters by manufacturers (Haas,Fanuc,Mazak,etc).
// NOTE: Modal group define values must be sequential and starting from zero.
#define MODAL_GROUP_G0 0 // [G4,G10,G28,G28.1,G30,G30.1,G53,G92,G92.1] Non-modal
#define MODAL_GROUP_G1 1 // [G0,G1,G2,G3,G38.2,G38.3,G38.4,G38.5,G80] Motion
#define MODAL_GROUP_G2 2 // [G17,G18,G19] Plane selection
#define MODAL_GROUP_G3 3 // [G90,G91] Distance mode
#define MODAL_GROUP_G4 4 // [G91.1] Arc IJK distance mode
#define MODAL_GROUP_G5 5 // [G93,G94] Feed rate mode
#define MODAL_GROUP_G6 6 // [G20,G21] Units
#define MODAL_GROUP_G7 7 // [G40] Cutter radius compensation mode. G41/42 NOT SUPPORTED.
#define MODAL_GROUP_G8 8 // [G43.1,G49] Tool length offset
#define MODAL_GROUP_G12 9 // [G54,G55,G56,G57,G58,G59] Coordinate system selection
#define MODAL_GROUP_G13 10 // [G61] Control mode

#define MODAL_GROUP_M4 11  // [M0,M1,M2,M30] Stopping
#define MODAL_GROUP_M7 12 // [M3,M4,M5] Spindle turning
#define MODAL_GROUP_M8 13 // [M7,M8,M9] Coolant control

// Define command actions for within execution-type modal groups (motion, stopping, non-modal). Used
// internally by the parser to know which command to execute.

// Modal Group G0: Non-modal actions
#define NON_MODAL_NO_ACTION 0 // (Default: Must be zero)
#define NON_MODAL_DWELL 4 // G4 (Do not alter value)
#define NON_MODAL_SET_COORDINATE_DATA 10 // G10 (Do not alter value)
#define NON_MODAL_GO_HOME_0 28 // G28 (Do not alter value)
#define NON_MODAL_SET_HOME_0 38 // G28.1 (Do not alter value)
#define NON_MODAL_GO_HOME_1 30 // G30 (Do not alter value)
#define NON_MODAL_SET_HOME_1 40 // G30.1 (Do not alter value)
#define NON_MODAL_ABSOLUTE_OVERRIDE 53 // G53 (Do not alter value)
#define NON_MODAL_SET_COORDINATE_OFFSET 92 // G92 (Do not alter value)
#define NON_MODAL_RESET_COORDINATE_OFFSET 102 //G92.1 (Do not alter value)

// Modal Group G1: Motion modes
#define MOTION_MODE_SEEK 0 // G0 (Default: Must be zero)
#define MOTION_MODE_LINEAR 1 // G1 (Do not alter value)
#define MOTION_MODE_CW_ARC 2  // G2 (Do not alter value)
#define MOTION_MODE_CCW_ARC 3  // G3 (Do not alter value)
#define MOTION_MODE_PROBE_TOWARD 140 // G38.2 (Do not alter value)
#define MOTION_MODE_PROBE_TOWARD_NO_ERROR 141 // G38.3 (Do not alter value)
#define MOTION_MODE_PROBE_AWAY 142 // G38.4 (Do not alter value)
#define MOTION_MODE_PROBE_AWAY_NO_ERROR 143 // G38.5 (Do not alter value)
#define MOTION_MODE_NONE 80 // G80 (Do not alter value)

// Modal Group G2: Plane select
#define PLANE_SELECT_XY 0 // G17 (Default: Must be zero)
#define PLANE_SELECT_ZX 1 // G18 (Do not alter value)
#define PLANE_SELECT_YZ 2 // G19 (Do not alter value)

// Modal Group G3: Distance mode
#define DISTANCE_MODE_ABSOLUTE 0 // G90 (Default: Must be zero)
#define DISTANCE_MODE_INCREMENTAL 1 // G91 (Do not alter value)

// Modal Group G4: Arc IJK distance mode
#define DISTANCE_ARC_MODE_INCREMENTAL 0 // G91.1 (Default: Must be zero)

// Modal Group M4: Program flow
#define PROGRAM_FLOW_RUNNING 0 // (Default: Must be zero)
#define PROGRAM_FLOW_PAUSED 3 // M0
#define PROGRAM_FLOW_OPTIONAL_STOP 1 // M1 NOTE: Not supported, but valid and ignored.
#define PROGRAM_FLOW_COMPLETED_M2  2 // M2 (Do not alter value)
#define PROGRAM_FLOW_COMPLETED_M30 30 // M30 (Do not alter value)

// Modal Group G5: Feed rate mode
#define FEED_RATE_MODE_UNITS_PER_MIN  0 // G94 (Default: Must be zero)
#define FEED_RATE_MODE_INVERSE_TIME   1 // G93 (Do not alter value)

// Modal Group G6: Units mode
#define UNITS_MODE_MM 0 // G21 (Default: Must be zero)
#define UNITS_MODE_INCHES 1 // G20 (Do not alter value)

// Modal Group G7: Cutter radius compensation mode
#define CUTTER_COMP_DISABLE 0 // G40 (Default: Must be zero)

// Modal Group G13: Control mode
#define CONTROL_MODE_EXACT_PATH 0 // G61 (Default: Must be zero)

// Modal Group M7: Spindle control
#define SPINDLE_DISABLE 0 // M5 (Default: Must be zero)
#define SPINDLE_ENABLE_CW   PL_COND_FLAG_SPINDLE_CW // M3 (NOTE: Uses planner condition bit flag)
#define SPINDLE_ENABLE_CCW  PL_COND_FLAG_SPINDLE_CCW // M4 (NOTE: Uses planner condition bit flag)

// Modal Group M8: Coolant control
#define COOLANT_DISABLE 0 // M9 (Default: Must be zero)
#define COOLANT_FLOOD_ENABLE  PL_COND_FLAG_COOLANT_FLOOD // M8 (NOTE: Uses planner condition bit flag)
#define COOLANT_MIST_ENABLE   PL_COND_FLAG_COOLANT_MIST  // M7 (NOTE: Uses planner condition bit flag)

// Modal Group G8: Tool length offset
#define TOOL_LENGTH_OFFSET_CANCEL 0 // G49 (Default: Must be zero)
#define TOOL_LENGTH_OFFSET_ENABLE_DYNAMIC 1 // G43.1

// Modal Group G12: Active work coordinate system
// N/A: Stores coordinate system value (54-59) to change to.

// Define g-code parser position updating flags, returned by the probe callback.
#define GC_UPDATE_POS_TARGET   0 // Must be zero
#define GC_UPDATE_POS_SYSTEM   1
#define GC_UPDATE_POS_NONE     2

// Define probe cycle exit states and assign proper position updating.
#define GC_PROBE_FOUND      GC_UPDATE_POS_SYSTEM
#define GC_PROBE_ABORT      GC_UPDATE_POS_NONE
#define GC_PROBE_FAIL_INIT  GC_UPDATE_POS_NONE
#define GC_PROBE_FAIL_END   GC_UPDATE_POS_TARGET

// Define gcode parser flags for handling special cases. Passed to the probe callback.
#define GC_PARSER_NONE                  0 // Must be zero.
#define GC_PARSER_JOG_MOTION            (1 << 0)
#define GC_PARSER_CHECK_MANTISSA        (1 << 1)
#define GC_PARSER_ARC_IS_CLOCKWISE      (1 << 2)
#define GC_PARSER_PROBE_IS_AWAY         (1 << 3)
#define GC_PARSER_PROBE_IS_NO_ERROR     (1 << 4)
#define GC_PARSER_LASER_FORCE_SYNC      (1 << 5)
#define GC_PARSER_LASER_DISABLE         (1 << 6)
#define GC_PARSER_LASER_ISMOTION        (1 << 7)


// NOTE: When this struct is zeroed, the above defines set the defaults for the system.
typedef struct {
  uint8_t motion;          // {G0,G1,G2,G3,G38.2,G80}
  uint8_t feed_rate;       // {G93,G94}
  uint8_t units;           // {G20,G21}
  uint8_t distance;        // {G90,G91}
  // uint8_t distance_arc; // {G91.1} NOTE: Don't track. Only default supported.
  uint8_t plane_select;    // {G17,G18,G19}
  // uint8_t cutter_comp;  // {G40} NOTE: Don't track. Only default supported.
  uint8_t tool_length;     // {G43.1,G49}
  uint8_t coord_select;    // {G54,G55,G56,G57,G58,G59}
  // uint8_t control;      // {G61} NOTE: Don't track. Only default supported.
  uint8_t program_flow;    // {M0,M1,M2,M30}
  uint8_t coolant;         // {M7,M8,M9}
  uint8_t spindle;         // {M3,M4,M5}
} gc_modal_t;

typedef struct {
  float f;         // Feed
  float ijk[3];    // I,J,K Axis arc offsets
  uint8_t l;       // G10 or canned cycles parameters
  int32_t n;       // Line number
  float p;         // G10 or dwell parameters
  // float q;      // G82 peck drilling
  float r;         // Arc radius
  float s;         // Spindle speed
  uint8_t t;       // Tool selection
  float xyz[3];    // X,Y,Z Translational axes
} gc_values_t;


typedef struct {
  gc_modal_t modal;

  float spindle_speed;          // RPM
  float feed_rate;              // Millimeters/min
  uint8_t tool;                 // Tracks tool number. NOT USED.
  int32_t line_number;          // Last line number sent

  float position[GC_N_AXIS];       // Where the interpreter considers the tool to be at this point in the code

  float coord_system[GC_N_AXIS];    // Current work coordinate system (G54+). Stores offset from absolute machine
                                    // position in mm. Loaded from coordinate storage when called.
  float coord_offset[GC_N_AXIS];    // Retains the G92 coordinate offset (work coordinates) relative to
                                    // machine zero in mm. Non-persistent. Cleared upon reset and boot.
  float tool_length_offset;         // Tracks tool length offset value when enabled.
} parser_state_t;


typedef struct {
  uint8_t non_modal_command;
  gc_modal_t modal;
  gc_values_t values;
} parser_block_t;


/** @brief Motion data passed to the motion callbacks (Grbl's plan_line_data_t). */
struct gc_plan_data {
  float feed_rate;      // Desired feed rate for line motion. Value is ignored, if rapid motion.
  float spindle_speed;  // Desired spindle speed through line motion.
  uint8_t condition;    // Bitflag variable to indicate planner conditions. See PL_COND_FLAG_*.
  int32_t line_number;  // Desired line number to report when executing.
};

/**
 * @brief Machine actions performed by the parser.
 *
 * All positions are absolute machine coordinates in mm, feed rates in mm/min
 * (or inverse time when PL_COND_FLAG_INVERSE_TIME is set). Every callback is
 * optional:
 * - A g-code that needs a missing motion callback (arc, probe, jog) is rejected
 *   with STATUS_GCODE_UNSUPPORTED_COMMAND / STATUS_INVALID_JOG_COMMAND.
 * - Without read/write_coord_data, coordinate data (G54-G59, G28, G30) is kept
 *   in RAM inside struct gc_parser.
 * - Any other missing callback is simply skipped.
 */
struct gc_machine_ops {
  /** Linear move to target (Grbl mc_line). */
  void (*line)(void *user_data, const float target[GC_N_AXIS], const struct gc_plan_data *pl);

  /** Arc from position to target around position+offset (Grbl mc_arc). */
  void (*arc)(void *user_data, const float target[GC_N_AXIS], const struct gc_plan_data *pl,
              const float position[GC_N_AXIS], const float offset[GC_N_AXIS], float radius,
              uint8_t axis_0, uint8_t axis_1, uint8_t axis_linear, bool is_clockwise);

  /**
   * G38.x probing cycle towards target (Grbl mc_probe_cycle). May update target
   * with the probed position. Returns one of GC_UPDATE_POS_* / GC_PROBE_*.
   */
  uint8_t (*probe)(void *user_data, float target[GC_N_AXIS], const struct gc_plan_data *pl,
                   uint8_t parser_flags);

  /** "$J=" jog move to target (Grbl jog_execute). Returns a STATUS_* code. */
  uint8_t (*jog)(void *user_data, const float target[GC_N_AXIS], const struct gc_plan_data *pl);

  /** G4 dwell (Grbl mc_dwell). */
  void (*dwell)(void *user_data, float seconds);

  /** Synchronised spindle state/speed change, state is SPINDLE_* (Grbl spindle_sync). */
  void (*spindle_sync)(void *user_data, uint8_t state, float rpm);

  /** Synchronised coolant change, state is a COOLANT_* bitmask (Grbl coolant_sync). */
  void (*coolant_sync)(void *user_data, uint8_t state);

  /** Load/store coordinate data, index is 0-5 (G54-G59) or GC_COORD_INDEX_G28/G30. */
  bool (*read_coord_data)(void *user_data, uint8_t index, float data[GC_N_AXIS]);
  bool (*write_coord_data)(void *user_data, uint8_t index, const float data[GC_N_AXIS]);

  /** Current machine position in mm, used by gc_sync_position(). */
  void (*get_position)(void *user_data, float position[GC_N_AXIS]);

  /** Block until all buffered motion has completed (Grbl protocol_buffer_synchronize). */
  void (*buffer_synchronize)(void *user_data);

  /** M0 program pause, e.g. enter feed hold (Grbl EXEC_FEED_HOLD). */
  void (*program_pause)(void *user_data);

  /** M2/M30 program end, after modal state has been reset. */
  void (*program_end)(void *user_data, uint8_t program_flow);

  /** Work coordinate offset changed (Grbl system_flag_wco_change). */
  void (*wco_changed)(void *user_data);
};

/** @brief A g-code parser instance. Treat as opaque except for `state`. */
struct gc_parser {
  parser_state_t state;
  parser_block_t block;

  const struct gc_machine_ops *ops;
  void *user_data;

  bool laser_mode;   // Grbl $32 laser mode.
  bool check_mode;   // Grbl $C check mode: no spindle/coolant/coordinate side effects at program end.

  float coord_data[GC_N_COORD_DATA][GC_N_AXIS]; // Used without read/write_coord_data callbacks.
};

/**
 * @brief Initialize the parser.
 *
 * @param ops Machine callbacks, may be NULL. Must outlive the parser.
 * @param user_data Passed to every callback.
 * @return STATUS_OK or STATUS_SETTING_READ_FAIL if G54 couldn't be loaded.
 */
uint8_t gc_init(struct gc_parser *gc, const struct gc_machine_ops *ops, void *user_data);

/** @brief Enable/disable laser mode (Grbl setting $32). */
void gc_set_laser_mode(struct gc_parser *gc, bool enable);

/** @brief Enable/disable check mode (Grbl $C). */
void gc_set_check_mode(struct gc_parser *gc, bool enable);

/**
 * @brief Execute one line of 0-terminated G-Code.
 *
 * The line is assumed to contain only uppercase characters and signed floating
 * point values (no whitespace), with comments removed. Use struct
 * gc_line_reader or gc_execute_raw() to clean up raw input. A line starting
 * with "$J=" is executed as a jog command.
 *
 * @return STATUS_OK or a STATUS_* error code. On error the parser state is
 *         unchanged.
 */
uint8_t gc_execute_line(struct gc_parser *gc, const char *line);

/**
 * @brief Clean up and execute one raw line of G-Code, e.g. "g1 x10 (cut) f100".
 *
 * Applies the same processing as struct gc_line_reader. Empty and
 * comment-only lines return STATUS_OK.
 */
uint8_t gc_execute_raw(struct gc_parser *gc, const char *raw_line);

/** @brief Set the parser position from the machine position (get_position callback). */
void gc_sync_position(struct gc_parser *gc);


/**
 * @brief Incremental g-code line reader (Grbl's protocol line handling).
 *
 * Feed it characters as they arrive. It strips whitespace, control characters,
 * '(...)' and ';' comments and block delete '/', and converts to upper case.
 */
struct gc_line_reader {
  char line[CONFIG_GRBL_GCODE_LINE_BUFFER_SIZE];
  uint8_t char_counter;
  uint8_t flags;
};

enum gc_line_result {
  GC_LINE_INCOMPLETE, // More characters needed.
  GC_LINE_READY,      // reader->line holds a complete, cleaned-up line (may be empty).
  GC_LINE_OVERFLOW,   // Line was longer than CONFIG_GRBL_GCODE_LINE_BUFFER_SIZE - 1 and was dropped.
};

/** @brief Reset the reader, discarding any partial line. */
void gc_line_reader_init(struct gc_line_reader *reader);

/**
 * @brief Process one input character. '\n' or '\r' completes the line.
 *
 * After GC_LINE_READY or GC_LINE_OVERFLOW the reader starts a new line on the
 * next call, so reader->line must be consumed before feeding more characters.
 */
enum gc_line_result gc_line_reader_feed(struct gc_line_reader *reader, char c);

#ifdef __cplusplus
}
#endif

#endif /* GRBL_GCODE_H_ */
