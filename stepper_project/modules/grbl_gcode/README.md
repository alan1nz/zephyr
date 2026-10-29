# grbl_gcode

A Zephyr module containing the RS274/NGC g-code parser from
[Grbl v1.1h](https://github.com/gnea/grbl/tree/v1.1h.20190825), separated
from the rest of Grbl.

The parser keeps Grbl's behaviour: the same supported g-codes, modal-group
rules, error checks, unit and coordinate-system handling, arc-centre
calculation and `STATUS_*` error codes. It no longer drives hardware itself.
Instead it calls functions you supply in a `struct gc_machine_ops`. Every
callback is optional.

## License

Grbl is licensed under the **GNU GPL v3 or later**, and so is this module (see
`LICENSE`). If firmware links this module, distributing that firmware requires
releasing its complete source code under GPL-compatible terms.

## Using it

Make the module visible to the build. Either:

- add it to your west manifest, if it lives in its own repository:

  ```yaml
  - name: grbl_gcode
    url: <your repository URL>
    revision: main
    path: modules/lib/grbl_gcode
  ```

- or list its directory in `ZEPHYR_EXTRA_MODULES` in your application's
  `CMakeLists.txt`, before `find_package(Zephyr)`:

  ```cmake
  list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR}/path/to/grbl_gcode)
  ```

Then set `CONFIG_GRBL_GCODE=y` and drive it:

```c
#include <grbl/gcode.h>

static void do_line(void *user_data, const float target[GC_N_AXIS],
                    const struct gc_plan_data *pl)
{
	/* Queue a move to target (mm). pl->feed_rate is in mm/min, and
	 * PL_COND_FLAG_RAPID_MOTION is set in pl->condition for G0. */
}

static const struct gc_machine_ops ops = {
	.line = do_line,
};

static struct gc_parser gc;
static struct gc_line_reader reader;

void start(void)
{
	gc_init(&gc, &ops, NULL);
	gc_line_reader_init(&reader);
}

/* Call for every character received from the host. */
void on_char(char c)
{
	switch (gc_line_reader_feed(&reader, c)) {
	case GC_LINE_READY:
		reply(reader.line[0] ? gc_execute_line(&gc, reader.line) : STATUS_OK);
		break;
	case GC_LINE_OVERFLOW:
		reply(STATUS_OVERFLOW);
		break;
	default:
		break;
	}
}
```

To execute a single string, use `gc_execute_raw(&gc, "g1 x10 (cut) f100")`.

## Callbacks

Each `struct gc_machine_ops` member replaces a call into another part of Grbl:

| Callback | Grbl function | Used by | If you leave it NULL |
|---|---|---|---|
| `line` | `mc_line` | G0, G1, G28, G30 | Move is skipped |
| `arc` | `mc_arc` | G2, G3 | Command rejected (error 20) |
| `probe` | `mc_probe_cycle` | G38.2 to G38.5 | Command rejected (error 20) |
| `jog` | `jog_execute` | `$J=` | Command rejected (error 16) |
| `dwell` | `mc_dwell` | G4 | Skipped |
| `spindle_sync` | `spindle_sync` | M3, M4, M5, S | Skipped |
| `coolant_sync` | `coolant_sync` | M7, M8, M9 | Skipped |
| `read_coord_data` / `write_coord_data` | `settings_*_coord_data` (EEPROM) | G10, G28.1, G30.1, G54 to G59 | Kept in RAM in `struct gc_parser` |
| `get_position` | `system_convert_array_steps_to_mpos` | `gc_sync_position()`, probing | Skipped |
| `buffer_synchronize` | `protocol_buffer_synchronize` | M0, M2, M30 | Skipped |
| `program_pause` | feed hold | M0 | Skipped |
| `program_end` | `report_feedback_message` | M2, M30 | Skipped |
| `wco_changed` | `system_flag_wco_change` | G10, G43.1, G54 to G59, G92 | Skipped |

The `jog` and `probe` callbacks return a value. `jog` returns a `STATUS_*`
code. `probe` returns how the parser should update its position: one of
`GC_UPDATE_POS_*` or `GC_PROBE_*`.

## Supported g-code

This is Grbl v1.1h's set:

- **G-codes:** G0, G1, G2, G3, G4, G10 L2, G10 L20, G17, G18, G19, G20, G21,
  G28, G28.1, G30, G30.1, G38.2 to G38.5, G40, G43.1, G49, G53, G54 to G59,
  G61, G80, G90, G91, G91.1, G92, G92.1, G93, G94
- **M-codes:** M0, M1, M2, M30, M3, M4, M5, M7 (optional), M8, M9
- **Words:** F, I, J, K, L, N, P, R, S, T, X, Y, Z

## Changes from Grbl

- **Not included:** parking override control (M56), and resetting feed and
  spindle overrides at program end. Both belong to Grbl's machine-state code;
  your `program_end` callback can do the override reset if you need it.
- **Always enabled:** `VARIABLE_SPINDLE` and line-number tracking.
- **Laser mode** (Grbl's `$32`) and **check mode** (`$C`) are set with
  `gc_set_laser_mode()` and `gc_set_check_mode()`.
- **Compile-time options** from Grbl's `config.h` are Kconfig options:
  `CONFIG_GRBL_GCODE_LINE_BUFFER_SIZE`,
  `CONFIG_GRBL_GCODE_TOOL_LENGTH_OFFSET_AXIS` and `CONFIG_GRBL_GCODE_M7`.

## Tests

The ztest suite is in `tests/gcode`. Build and run it on `native_sim` with:

```
west build -b native_sim modules/grbl_gcode/tests/gcode -t run
```
