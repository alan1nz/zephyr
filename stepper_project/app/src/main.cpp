#include <math.h>

#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <atomic>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <app/lib/units.hpp>
#include <app/lib/thread.hpp>
#include <grbl/gcode.h>

using namespace units::literals;

#define STEPPER_NODE DT_NODELABEL(stepper0)

/* Motion controller: generates step/dir pulses and tracks position. */
static const struct device *stepper_dev = DEVICE_DT_GET(STEPPER_NODE);

/* Stepper driver chip (e.g. TMC2209) linked from stepper0, if the board has one. */
#if DT_NODE_HAS_PROP(STEPPER_NODE, stepper_driver)
static const struct device *stepper_driver_dev =
    DEVICE_DT_GET(DT_PHANDLE(STEPPER_NODE, stepper_driver));
#endif

/* stepper0 drives the X axis. Adjust these to the machine's mechanics. */
constexpr float kFullStepsPerRev = 200.0f;
constexpr float kMicrosteps = 8.0f;
constexpr float kMmPerRev = 8.0f; /* T8 lead screw */
constexpr float kMicrostepsPerMm = kFullStepsPerRev * kMicrosteps / kMmPerRev;

/* Speed used for G0 rapid moves. */
constexpr float kRapidFeedMmPerMin = 1500.0f;

/* G-code program to run at startup. */
static const char *const program[] = {
    "G21 G90",      /* Millimetres, absolute positioning */
    "G1 X10 F1500", /* Move to X = 10 mm at 1500 mm/min */
    "G91 G1 X-2.5", /* Move 2.5 mm back, relative to the current position */
    "G90 G0 X0",    /* Rapid back to X = 0 */
};

static float current_x_mm;

static void wait_until_stopped()
{
    bool moving = true;

    while ((stepper_ctrl_is_moving(stepper_dev, &moving) == 0) && moving)
    {
        k_msleep(10);
    }
}

/* Called by the g-code parser for G0/G1 moves. Target is in machine mm. */
static void gcode_line(void *user_data, const float target[GC_N_AXIS],
                       const struct gc_plan_data *pl)
{
    ARG_UNUSED(user_data);

    float target_x_mm = target[GC_X_AXIS];
    float feed_mm_per_min;

    if (pl->condition & PL_COND_FLAG_RAPID_MOTION)
    {
        feed_mm_per_min = kRapidFeedMmPerMin;
    }
    else if (pl->condition & PL_COND_FLAG_INVERSE_TIME)
    {
        /* G93: F is moves per minute, so the speed depends on the distance. */
        feed_mm_per_min = fabsf(target_x_mm - current_x_mm) * pl->feed_rate;
    }
    else
    {
        feed_mm_per_min = pl->feed_rate;
    }

    if (feed_mm_per_min > 0.0f)
    {
        uint64_t interval_ns =
            (uint64_t)(60e9f / (feed_mm_per_min * kMicrostepsPerMm));
        stepper_ctrl_set_microstep_interval(stepper_dev, interval_ns);
    }

    int32_t target_steps = (int32_t)lroundf(target_x_mm * kMicrostepsPerMm);

    /* printk has no float support by default, so print micrometres. */
    printk("move X to %ld um (%d microsteps)\n", lroundf(target_x_mm * 1000.0f),
           target_steps);

    /* The parser expects each move to finish before the next block runs. */
    stepper_ctrl_move_to(stepper_dev, target_steps);
    wait_until_stopped();
    current_x_mm = target_x_mm;
}

static struct gc_machine_ops make_gcode_ops()
{
    struct gc_machine_ops ops = {};

    ops.line = gcode_line;
    return ops;
}

static const struct gc_machine_ops gcode_ops = make_gcode_ops();

/* The board's user LED (LD3): blinks while the program runs, stays on when it is done. */
static const struct gpio_dt_spec status_led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

enum class ProgramState
{
    Idle,
    Running,
    Done,
};

/* Written by the gcode thread, read by the status LED thread. */
static std::atomic<ProgramState> program_state{ProgramState::Idle};

/* Runs the g-code program once, then the thread exits. */
static void run_gcode_program()
{
    static struct gc_parser parser;

    gc_init(&parser, &gcode_ops, NULL);
    program_state = ProgramState::Running;

    for (const char *line : program)
    {
        printk("> %s\n", line);
        uint8_t status = gc_execute_raw(&parser, line);
        if (status != STATUS_OK)
        {
            printk("error:%d\n", status);
        }
    }

    program_state = ProgramState::Done;
    printk("program done\n");
}

/* Drives the status LED from the program state. Only this thread touches the LED. */
static void update_status_led()
{
    switch (program_state.load())
    {
    case ProgramState::Running:
        gpio_pin_toggle_dt(&status_led);
        break;
    case ProgramState::Done:
        gpio_pin_set_dt(&status_led, 1);
        break;
    case ProgramState::Idle:
        break;
    }
}

/* Prints the stepper position if it changed since the last call. */
static void print_position_if_changed()
{
    static int32_t last_position = 0;
    int32_t position;

    if (stepper_ctrl_get_actual_position(stepper_dev, &position) == 0 &&
        position != last_position)
    {
        printk("position: %d microsteps\n", position);
        last_position = position;
    }
}

/* Prints the uptime in seconds; called once a second. */
static void print_heartbeat()
{
    static uint32_t seconds = 0;

    printk("heartbeat: %u s\n", ++seconds);
}

/* Reports heartbeat calls skipped because the thread ran late. */
static void report_overrun(uint32_t missed)
{
    printk("heartbeat: %u missed ticks\n", missed);
}

/* All application threads. Lower priority number = higher priority. */
static constexpr std::array threads{
    rtos::ThreadEntry{.name = "position", .fn = print_position_if_changed, .priority = 4,
                      .stack_size = 1024, .period_ms = 100},
    /* runs once; the parser needs a large stack */
    rtos::ThreadEntry{.name = "gcode", .fn = run_gcode_program, .priority = 5,
                      .stack_size = 4096},
    rtos::ThreadEntry{.name = "heartbeat", .fn = print_heartbeat, .priority = 10,
                      .stack_size = 1024, .period_ms = 1000, .start_delay_ms = 500,
                      .on_overrun = report_overrun},
    /* 100 ms period: the LED blinks at 5 Hz while the program runs */
    rtos::ThreadEntry{.name = "status_led", .fn = update_status_led, .priority = 11,
                      .stack_size = 1024, .period_ms = 100},
};

int main()
{
    if (!device_is_ready(stepper_dev))
    {
        k_panic();
    }

    /* Enable the driver chip. stepper_enable() belongs to the driver API, so it must
     * be called on the driver device, not on the stepper0 motion controller.
     */
#if DT_NODE_HAS_PROP(STEPPER_NODE, stepper_driver)
    if (!device_is_ready(stepper_driver_dev))
    {
        k_panic();
    }
    stepper_enable(stepper_driver_dev);
#endif

    /* The LED is only a status indicator, so the program still runs without it. */
    if (!gpio_is_ready_dt(&status_led) ||
        gpio_pin_configure_dt(&status_led, GPIO_OUTPUT_INACTIVE) != 0)
    {
        printk("status LED not available\n");
    }

    rtos::start_threads<threads>();

    /* main's work is done; returning ends the main thread while the others keep running. */
    return 0;
}
