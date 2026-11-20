/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr threads declared as a plain array.
 *
 * Each entry names a function, its scheduling settings and its stack size.
 * A period of 0 runs the function once; any other period calls it repeatedly
 * at that rate:
 *
 *     static void blink() { ... }
 *     static void run_program() { ... }
 *
 *     static constexpr std::array threads{
 *         rtos::ThreadEntry{.name = "blink", .fn = blink, .priority = 10, .stack_size = 512, .period_ms = 500},
 *         rtos::ThreadEntry{.name = "program", .fn = run_program, .priority = 5, .stack_size = 4096},
 *     };
 *
 *     int main() { rtos::start_threads<threads>(); }
 *
 * The array must be `static constexpr`: start_threads reads it at compile
 * time to allocate a stack of the right size for each entry, and rejects
 * invalid entries with a compile error.
 *
 * Threads run in kernel mode only (no CONFIG_USERSPACE support).
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>
#include <type_traits>
#include <utility>

#include <zephyr/kernel.h>

namespace rtos
{

    /// Maximum thread name length, including the terminating '\0'.
    inline constexpr std::size_t kMaxThreadNameLen = 32;

    /// One thread: what it runs and how it is scheduled.
    struct ThreadEntry
    {
        const char *name;
        void (*fn)(); // function or captureless lambda
        int priority; // lower number = higher priority
        std::size_t stack_size;

        /// 0 runs fn once and the thread then exits. Otherwise fn is called
        /// every period_ms, at fixed deadlines that do not drift with fn's run time.
        std::uint32_t period_ms = 0;

        std::int32_t start_delay_ms = 0;

        /// Periodic threads only, optional: called when fn ran past one or more
        /// deadlines. Those calls are skipped, not caught up.
        void (*on_overrun)(std::uint32_t missed) = nullptr;
    };

    namespace detail
    {
        /// Thread object and stack passed to k_thread_create.
        struct ThreadStorageRef
        {
            k_thread *thread;
            k_thread_stack_t *stack;
            std::size_t stack_size;
        };

        /// Kernel thread object and stack for one entry (all zero, so it goes in .bss).
        template <std::size_t StackSize>
        struct ThreadStorage
        {
            k_thread thread{};
            // Zephyr's macro applies the stack alignment and guard size required by the kernel config.
            K_KERNEL_STACK_MEMBER(stack, StackSize){};

            ThreadStorageRef ref() { return {&thread, stack, K_KERNEL_STACK_SIZEOF(stack)}; }
        };

        /// Storage for entry I of Table: one separate object per entry.
        template <const auto &Table, std::size_t I>
        inline ThreadStorage<Table[I].stack_size> storage{};

        /// Creates and starts one thread. Defined in thread.cpp.
        void start_thread(const ThreadEntry &entry, const ThreadStorageRef &storage);

        consteval bool name_fits(const char *name)
        {
            return std::string_view(name).size() < kMaxThreadNameLen;
        }

        /// Compile-time checks for entry I. A failure names the problem, and the
        /// compiler's "required from" note shows the entry index.
        template <const auto &Table, std::size_t I>
        consteval bool check_entry()
        {
            constexpr const ThreadEntry &e = Table[I];

            static_assert(e.name != nullptr, "thread table: entry has no name");
            static_assert(e.name == nullptr || name_fits(e.name),
                          "thread table: name longer than 31 characters");
            static_assert(e.fn != nullptr, "thread table: entry has no function");
            static_assert(e.priority >= K_HIGHEST_APPLICATION_THREAD_PRIO &&
                              e.priority <= K_LOWEST_APPLICATION_THREAD_PRIO,
                          "thread table: priority out of range for this kernel config");
            static_assert(e.stack_size > 0, "thread table: stack size must not be 0");
            static_assert(e.start_delay_ms >= 0, "thread table: start delay must not be negative");
            static_assert(e.on_overrun == nullptr || e.period_ms > 0,
                          "thread table: on_overrun is only used by periodic threads");
            return true;
        }
    } // namespace detail

    /// Starts every thread in Table, in array order. Call once, e.g. from main().
    template <const auto &Table>
    void start_threads()
    {
        static_assert(std::is_same_v<std::remove_cvref_t<decltype(Table[0])>, ThreadEntry>,
                      "thread table must be an array of rtos::ThreadEntry");

        // The stacks belong to the table, so starting it twice would reuse running stacks.
        static bool started = false;
        __ASSERT(!started, "thread table started twice");
        if (started)
            return;
        started = true;

        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            static_assert((detail::check_entry<Table, I>() && ...));

            const std::array<detail::ThreadStorageRef, sizeof...(I)> storage{
                detail::storage<Table, I>.ref()...};

            for (std::size_t i = 0; i < storage.size(); ++i)
                detail::start_thread(Table[i], storage[i]);
        }(std::make_index_sequence<std::size(Table)>{});
    }

} // namespace rtos
