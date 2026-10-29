# SPDX-License-Identifier: GPL-3.0-or-later

if(CONFIG_GRBL_GCODE)
  zephyr_include_directories(include)

  zephyr_library()
  zephyr_library_sources(
    src/gcode.c
    src/line_reader.c
    src/nuts_bolts.c
  )

  # Grbl was written for AVR, where double is float, and uses unsuffixed
  # constants such as 0.0 and 25.40 throughout. Keep that math single precision
  # on targets with a real double type.
  zephyr_library_cc_option(-fsingle-precision-constant)
endif()
