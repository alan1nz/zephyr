/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <app/lib/thread.hpp>

namespace rtos::detail
{

    namespace
    {
        void run_periodic(const ThreadEntry &e)
        {
            const auto period = static_cast<k_ticks_t>(k_ms_to_ticks_ceil64(e.period_ms));
            k_ticks_t next = k_uptime_ticks() + period;

            for (;;)
            {
                // Sleep to an absolute deadline so fn's run time does not shift the period.
                // The loop also covers an early wake-up by k_wakeup().
                while (k_uptime_ticks() < next)
                    k_sleep(K_TIMEOUT_ABS_TICKS(next));

                e.fn();

                next += period;
                const k_ticks_t now = k_uptime_ticks();
                if (now >= next)
                {
                    // fn ran past one or more deadlines: skip them rather than run a burst of late calls.
                    const auto missed = static_cast<std::uint32_t>((now - next) / period + 1);
                    next += static_cast<k_ticks_t>(missed) * period;
                    if (e.on_overrun != nullptr)
                        e.on_overrun(missed);
                }
            }
        }

        void thread_main(void *entry, void *, void *)
        {
            const auto &e = *static_cast<const ThreadEntry *>(entry);

            if (e.period_ms == 0)
                e.fn(); // one-shot: the thread exits when fn returns
            else
                run_periodic(e);
        }
    } // namespace

    void start_thread(const ThreadEntry &e, const ThreadStorageRef &s)
    {
        // The entry is only read; k_thread_create just takes a non-const void*.
        void *entry = const_cast<ThreadEntry *>(&e);

        if (e.start_delay_ms == 0)
        {
            // Create suspended so the name is set before a higher-priority thread can run.
            const k_tid_t tid = k_thread_create(s.thread, s.stack, s.stack_size, thread_main, entry,
                                                nullptr, nullptr, e.priority, 0, K_FOREVER);
            k_thread_name_set(tid, e.name);
            k_thread_start(tid);
        }
        else
        {
            const k_tid_t tid = k_thread_create(s.thread, s.stack, s.stack_size, thread_main, entry,
                                                nullptr, nullptr, e.priority, 0,
                                                K_MSEC(e.start_delay_ms));
            k_thread_name_set(tid, e.name);
        }
    }

} // namespace rtos::detail
