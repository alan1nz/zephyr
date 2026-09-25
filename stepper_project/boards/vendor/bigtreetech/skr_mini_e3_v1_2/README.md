# BIGTREETECH SKR Mini E3 V1.2

Zephyr board port for the [BIGTREETECH SKR Mini E3 V1.2](https://github.com/bigtreetech/BIGTREETECH-SKR-mini-E3)
3D printer control board.

## Hardware

- MCU: STM32F103RCT6 (Cortex-M3 @ 72 MHz, 256 KiB flash, 48 KiB RAM)
- 8 MHz HSE crystal
- 4x onboard TMC2209 stepper drivers (X, Y, Z, E0), UART configured for
  microstepping/current but only the hardware `enable` pin is wired up here;
  step/dir are driven with `zephyr,gpio-step-dir-stepper-ctrl`
- Hotend/bed thermistor inputs on ADC1 (PA0, PC3)
- Hotend/bed heater PWM outputs (TIM3 CH3/CH4, full remap) and a
  part-cooling fan PWM output (TIM1 CH1)
- X/Y/Z endstops, BLTouch/probe input and E0 filament runout input as
  `gpio-keys`
- Onboard micro-SD card slot on SPI1
- Console/shell UART on the TFT header (USART2, PA2/PA3)

## Programming and Debugging

Flash and debug via the SWD header using J-Link or OpenOCD, e.g.:

```shell
west build -b btt_skr_mini_e3_v1_2 app
west flash
```

Note that many of these boards ship with the stock BTT bootloader relocating
the application to `0x08007000`; when flashing a stock board via SD card
instead of SWD, the image must be built and offset accordingly.
