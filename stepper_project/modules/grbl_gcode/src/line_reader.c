/*
  line_reader.c - g-code line clean-up, from Grbl's protocol.c line handling.
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

  Modified for Zephyr: the character handling of protocol_main_loop() as a
  standalone, reentrant line reader.
*/

#include "nuts_bolts.h"

// Define line flags. Includes comment type tracking and line overflow detection.
#define LINE_FLAG_OVERFLOW bit(0)
#define LINE_FLAG_COMMENT_PARENTHESES bit(1)
#define LINE_FLAG_COMMENT_SEMICOLON bit(2)

void gc_line_reader_init(struct gc_line_reader *reader)
{
  reader->char_counter = 0;
  reader->flags = 0;
  reader->line[0] = 0;
}

enum gc_line_result gc_line_reader_feed(struct gc_line_reader *reader, char c)
{
  uint8_t line_flags = reader->flags;
  uint8_t char_counter = reader->char_counter;
  char *line = reader->line;

  if ((c == '\n') || (c == '\r')) { // End of line reached
    line[char_counter] = 0; // Set string termination character.

    // Reset tracking data for next line.
    reader->flags = 0;
    reader->char_counter = 0;

    if (line_flags & LINE_FLAG_OVERFLOW) {
      line[0] = 0;
      return GC_LINE_OVERFLOW;
    }
    return GC_LINE_READY;
  }

  if (line_flags) {
    // Throw away all (except EOL) comment characters and overflow characters.
    if (c == ')') {
      // End of '()' comment. Resume line allowed.
      if (line_flags & LINE_FLAG_COMMENT_PARENTHESES) { line_flags &= ~(LINE_FLAG_COMMENT_PARENTHESES); }
    }
  } else {
    if ((unsigned char)c <= ' ') {
      // Throw away whitepace and control characters
    } else if (c == '/') {
      // Block delete NOT SUPPORTED. Ignore character.
      // NOTE: If supported, would simply need to check the system if block delete is enabled.
    } else if (c == '(') {
      // Enable comments flag and ignore all characters until ')' or EOL.
      // NOTE: This doesn't follow the NIST definition exactly, but is good enough for now.
      // In the future, we could simply remove the items within the comments, but retain the
      // comment control characters, so that the g-code parser can error-check it.
      line_flags |= LINE_FLAG_COMMENT_PARENTHESES;
    } else if (c == ';') {
      // NOTE: ';' comment to EOL is a LinuxCNC definition. Not NIST.
      line_flags |= LINE_FLAG_COMMENT_SEMICOLON;
    } else if (char_counter >= (CONFIG_GRBL_GCODE_LINE_BUFFER_SIZE-1)) {
      // Detect line buffer overflow and set flag.
      line_flags |= LINE_FLAG_OVERFLOW;
    } else if (c >= 'a' && c <= 'z') { // Upcase lowercase
      line[char_counter++] = c-'a'+'A';
    } else {
      line[char_counter++] = c;
    }
  }

  reader->flags = line_flags;
  reader->char_counter = char_counter;
  return GC_LINE_INCOMPLETE;
}
