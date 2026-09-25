# Copyright (c) 2024
# SPDX-License-Identifier: Apache-2.0

'''gen_driver_class.py

West extension that scaffolds a new out-of-tree custom driver class,
following the same file/folder layout as the "blink" driver class:

  drivers/<class>/CMakeLists.txt
  drivers/<class>/Kconfig
  drivers/<class>/Kconfig.<impl>
  drivers/<class>/<impl>.c
  dts/bindings/<class>/<class>-<impl>.yaml
  include/app/drivers/<class>.h

It also wires the new class into drivers/CMakeLists.txt and drivers/Kconfig.
'''

import argparse
from pathlib import Path

from west.commands import WestCommand

CMAKE_LISTS_TMPL = '''\
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_{CLASS}_{IMPL} {impl}.c)
'''

KCONFIG_TMPL = '''\
# SPDX-License-Identifier: Apache-2.0

menuconfig {CLASS}
	bool "{Class} device drivers"
	help
	  This option enables the {class} custom driver class.

if {CLASS}

config {CLASS}_INIT_PRIORITY
	int "{Class} device drivers init priority"
	default KERNEL_INIT_PRIORITY_DEVICE
	help
	  {Class} device drivers init priority.

module = {CLASS}
module-str = {class}
source "subsys/logging/Kconfig.template.log_config"

rsource "Kconfig.{impl}"

endif # {CLASS}
'''

KCONFIG_IMPL_TMPL = '''\
# SPDX-License-Identifier: Apache-2.0

config {CLASS}_{IMPL}
	bool "{Impl} {class} driver"
	default y
	depends on DT_HAS_{CLASS}_{IMPL}_ENABLED
	help
	  Enable this option to use the {impl} {class} driver.
'''

BINDING_TMPL = '''\
# SPDX-License-Identifier: Apache-2.0

description: |
  A binding for the {impl} implementation of the {class} driver class.

  Example definition in devicetree:

    {compat} {{
        compatible = "{compat}";
    }};

compatible: "{compat}"

include: base.yaml

properties:
  # TODO: add implementation specific properties here.
'''

DRIVER_SRC_TMPL = '''\
/*
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT {compat_ident}

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <app/drivers/{class}.h>

LOG_MODULE_REGISTER({class}_{impl}, CONFIG_{CLASS}_LOG_LEVEL);

struct {class}_{impl}_data {{
	/* TODO: add per-instance runtime data here. */
}};

struct {class}_{impl}_config {{
	/* TODO: add per-instance devicetree-derived config here. */
}};

static DEVICE_API({class}, {class}_{impl}_api) = {{
	/* TODO: assign operation callbacks here. */
}};

static int {class}_{impl}_init(const struct device *dev)
{{
	ARG_UNUSED(dev);

	/* TODO: initialize the device here. */

	return 0;
}}

#define {CLASS}_{IMPL}_DEFINE(inst)                                            \\
	static struct {class}_{impl}_data data##inst;                          \\
                                                                                \\
	static const struct {class}_{impl}_config config##inst = {{             \\
	}};                                                                     \\
                                                                                \\
	DEVICE_DT_INST_DEFINE(inst, {class}_{impl}_init, NULL, &data##inst,    \\
			      &config##inst, POST_KERNEL,                      \\
			      CONFIG_{CLASS}_INIT_PRIORITY,                    \\
			      &{class}_{impl}_api);

DT_INST_FOREACH_STATUS_OKAY({CLASS}_{IMPL}_DEFINE)
'''

HEADER_TMPL = '''\
/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_DRIVERS_{CLASS}_H_
#define APP_DRIVERS_{CLASS}_H_

#include <zephyr/device.h>
#include <zephyr/toolchain.h>

/**
 * @defgroup drivers_{class} {Class} drivers
 * @ingroup drivers
 * @{{
 *
 * @brief Custom "{class}" driver class.
 */

/** @brief {Class} driver class operations */
__subsystem struct {class}_driver_api {{
	/* TODO: add operation callbacks here, e.g.:
	 * int (*do_something)(const struct device *dev);
	 */
	int (*do_something)(const struct device *dev);
}};

/**
 * @brief TODO: describe this call.
 *
 * @param dev {Class} device instance.
 *
 * @retval 0 if successful.
 * @retval -errno Negative errno code on failure.
 */
__syscall int {class}_do_something(const struct device *dev);

static inline int z_impl_{class}_do_something(const struct device *dev)
{{
	__ASSERT_NO_MSG(DEVICE_API_IS({class}, dev));

	return DEVICE_API_GET({class}, dev)->do_something(dev);
}}

#include <zephyr/syscalls/{class}.h>

/** @}} */

#endif /* APP_DRIVERS_{CLASS}_H_ */
'''


class GenDriverClass(WestCommand):

    def __init__(self):
        super().__init__(
            'gen-driver-class',
            'scaffold a new out-of-tree custom driver class',
            '''\
Generate the file and folder structure for a new out-of-tree custom driver
class, modeled after the existing "blink" driver class:

  drivers/<class>/CMakeLists.txt
  drivers/<class>/Kconfig
  drivers/<class>/Kconfig.<impl>
  drivers/<class>/<impl>.c
  dts/bindings/<class>/<class>-<impl>.yaml
  include/app/drivers/<class>.h

It also registers the new class in drivers/CMakeLists.txt and
drivers/Kconfig.''')

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(self.name,
                                          help=self.help,
                                          description=self.description)
        parser.add_argument('class_name',
                             help='name of the driver class, e.g. "fan"')
        parser.add_argument('-i', '--impl', default='gpio',
                             help='name of the first implementation '
                                  '(default: "gpio")')
        parser.add_argument('-f', '--force', action='store_true',
                             help='overwrite existing files')
        return parser

    def do_run(self, args, unknown_args):
        proj_root = Path(__file__).resolve().parent.parent

        cls = args.class_name.strip().lower().replace('-', '_')
        impl = args.impl.strip().lower().replace('-', '_')

        subs = {
            'class': cls,
            'Class': cls.replace('_', ' ').title().replace(' ', ''),
            'CLASS': cls.upper(),
            'impl': impl,
            'Impl': impl.replace('_', ' ').title().replace(' ', ''),
            'IMPL': impl.upper(),
            'compat': f"{cls}-{impl}".replace('_', '-'),
            'compat_ident': f"{cls}_{impl}",
        }

        driver_dir = proj_root / 'drivers' / cls
        binding_dir = proj_root / 'dts' / 'bindings' / cls
        header_path = proj_root / 'include' / 'app' / 'drivers' / f'{cls}.h'

        files = {
            driver_dir / 'CMakeLists.txt': CMAKE_LISTS_TMPL.format(**subs),
            driver_dir / 'Kconfig': KCONFIG_TMPL.format(**subs),
            driver_dir / f'Kconfig.{impl}': KCONFIG_IMPL_TMPL.format(**subs),
            driver_dir / f'{impl}.c': DRIVER_SRC_TMPL.format(**subs),
            binding_dir / f'{subs["compat"]}.yaml': BINDING_TMPL.format(**subs),
            header_path: HEADER_TMPL.format(**subs),
        }

        for path, content in files.items():
            if path.exists() and not args.force:
                self.die(f'{path} already exists (use --force to overwrite)')

        for path, content in files.items():
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content)
            self.inf(f'created {path.relative_to(proj_root)}')

        self._patch_drivers_cmake(proj_root, cls)
        self._patch_drivers_kconfig(proj_root, cls)

        self.inf('')
        self.inf(f'Driver class "{cls}" scaffolded with "{impl}" '
                  'implementation. Remaining TODOs are marked in the '
                  'generated files.')

    def _patch_drivers_cmake(self, proj_root, cls):
        path = proj_root / 'drivers' / 'CMakeLists.txt'
        line = f'add_subdirectory_ifdef(CONFIG_{cls.upper()} {cls})\n'
        text = path.read_text()
        if line.strip() in text:
            return
        path.write_text(text.rstrip('\n') + '\n' + line)
        self.inf(f'updated {path.relative_to(proj_root)}')

    def _patch_drivers_kconfig(self, proj_root, cls):
        path = proj_root / 'drivers' / 'Kconfig'
        line = f'rsource "{cls}/Kconfig"\n'
        text = path.read_text()
        if line.strip() in text:
            return
        marker = 'endmenu'
        if marker in text:
            text = text.replace(marker, line + marker, 1)
        else:
            text = text.rstrip('\n') + '\n' + line
        path.write_text(text)
        self.inf(f'updated {path.relative_to(proj_root)}')
