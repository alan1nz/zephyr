/*
  nuts_bolts.h - Header file for shared definitions, variables, and functions
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

  Modified for Zephyr: reduced to the helpers used by the g-code parser.
*/

#ifndef GRBL_NUTS_BOLTS_H_
#define GRBL_NUTS_BOLTS_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <grbl/gcode.h>

// Axis array index values. Must start with 0 and be continuous.
#define N_AXIS GC_N_AXIS
#define X_AXIS GC_X_AXIS
#define Y_AXIS GC_Y_AXIS
#define Z_AXIS GC_Z_AXIS

#define N_COORDINATE_SYSTEM GC_N_COORDINATE_SYSTEM
#define SETTING_INDEX_G28   GC_COORD_INDEX_G28
#define SETTING_INDEX_G30   GC_COORD_INDEX_G30

#define TOOL_LENGTH_OFFSET_AXIS CONFIG_GRBL_GCODE_TOOL_LENGTH_OFFSET_AXIS

// Conversions
#define MM_PER_INCH (25.40)

// Useful macros
#define clear_vector(a) memset(a, 0, sizeof(a))
#define isequal_position_vector(a,b) !(memcmp(a, b, sizeof(float)*N_AXIS))

// Bit field and masking macros
#define bit(n) (1 << n)
#define bit_true(x,mask) (x) |= (mask)
#define bit_false(x,mask) (x) &= ~(mask)
#define bit_istrue(x,mask) ((x & mask) != 0)
#define bit_isfalse(x,mask) ((x & mask) == 0)

// Read a floating point value from a string. Line points to the input buffer, char_counter
// is the indexer pointing to the current character of the line, while float_ptr is
// a pointer to the result variable. Returns true when it succeeds
uint8_t read_float(const char *line, uint8_t *char_counter, float *float_ptr);

// Computes hypotenuse, avoiding avr-gcc's bloated version and the extra error checking.
float hypot_f(float x, float y);

#endif /* GRBL_NUTS_BOLTS_H_ */
