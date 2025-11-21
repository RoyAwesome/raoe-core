/*
    Copyright 2022-2025 Roy Awesome's Open Engine (RAOE)
     Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

 */
#pragma once

#include <any>
#include <array>
#include <coroutine>
#include <exception>
#include <variant>

#include "core/types.hpp"

#include <chrono>
#include <utility>

namespace raoe::engine
{
    template<typename T>
    concept sequence_waiter = requires(T t) {
        { t.ready() } -> std::convertible_to<bool>;
        { t.run_inline() } -> std::convertible_to<bool>;
        { t.terminate() } -> std::convertible_to<bool>;
    };

    struct coro
    {
      private:
        struct waiter_storage
        {
            struct vtable
            {
                bool (*ready)(const std::any& handle);
                bool (*run_inline)(const std::any& handle);
                bool (*terminate)(const std::any& handle);
            };

            template<sequence_waiter waiter_t>
            explicit waiter_storage(const waiter_t& waiter)
                : m_waiter(waiter)
            {
                m_vtable = {
                    .ready = [](const std::any& w) -> bool { return std::any_cast<waiter_t>(w).ready(); },
                    .run_inline = [](const std::any& w) -> bool { return std::any_cast<waiter_t>(w).run_inline(); },
                    .terminate = [](const std::any& w) -> bool { return std::any_cast<waiter_t>(w).terminate(); },
                };
            }

            template<sequence_waiter waiter_t>
            explicit waiter_storage(waiter_t&& waiter) noexcept
                : m_waiter(std::forward<waiter_t>(waiter))
            {
                m_vtable = {
                    .ready = [](const std::any& w) -> bool { return std::any_cast<waiter_t>(w).ready(); },
                    .run_inline = [](const std::any& w) -> bool { return std::any_cast<waiter_t>(w).run_inline(); },
                    .terminate = [](const std::any& w) -> bool { return std::any_cast<waiter_t>(w).terminate(); },
                };
            }

            bool ready() const { return m_vtable.ready(m_waiter); }
            bool run_inline() const { return m_vtable.run_inline(m_waiter); }
            bool terminate() const { return m_vtable.terminate(m_waiter); }

            vtable m_vtable;
            std::any m_waiter;
        };
        struct result_discriminator
        {
            constexpr static uint8 empty = 0;
            constexpr static uint8 data = 1;
            constexpr static uint8 exception = 2;
        };

      public:
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;

        struct promise_type
        {
            std::variant<std::monostate, waiter_storage, std::exception_ptr> result;

            coro get_return_object() { return coro(handle_type::from_promise(*this)); }

            std::suspend_never initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            void unhandled_exception() noexcept
            {
                result.emplace<result_discriminator::exception>(std::current_exception());
            }

            template<sequence_waiter From>
            std::suspend_always yield_value(From&& from)
            {
                result.emplace<result_discriminator::data>(std::forward<From>(from));
                return {};
            }

            void return_void() {}
        };

        handle_type handle;

        explicit coro(const handle_type in_handle)
            : handle(in_handle)
        {
        }
        ~coro()
        {
            if(handle)
            {
                handle.destroy();
            }
        }

        coro(const coro&) = delete;
        coro& operator=(const coro&) = delete;
        coro(coro&& other) noexcept
            : handle(std::move(std::exchange(other.handle, {})))
            , waiting_move(other.waiting_move) {};
        coro& operator=(coro&& other) noexcept
        {
            handle = std::move(std::exchange(other.handle, {}));
            waiting_move = other.waiting_move;
            return *this;
        }

        explicit operator bool() const { return handle && !handle.done(); }

        void operator()() { try_move_next(); }

      private:
        bool has_waiter() const { return handle.promise().result.index() != result_discriminator::empty; }

        bool waiting_move = false;
        void try_move_next()
        {
            if(const waiter_storage* waiter = std::get_if<result_discriminator::data>(&handle.promise().result);
               waiter == nullptr || waiter->ready())
            {
                do
                {
                    handle();

                    if(handle.promise().result.index() == result_discriminator::exception)
                    {
                        std::rethrow_exception(
                            std::get<result_discriminator::exception>(std::move(handle.promise().result)));
                    }
                    if(handle.done())
                    {
                        return;
                    }

                    waiter = std::get_if<result_discriminator::data>(&handle.promise().result);

                    if(waiter == nullptr || waiter->terminate())
                    {
                        handle.destroy();
                        return;
                    }

                } while(waiter->run_inline());
            }

            if(!waiting_move)
            {
                handle();
                if(handle.promise().result.index() == result_discriminator::exception)
                {
                    std::rethrow_exception(
                        std::get<result_discriminator::exception>(std::move(handle.promise().result)));
                }
                waiting_move = true;
            }
        }
    };

    namespace wait
    {
        inline auto for_next_tick()
        {
            struct waiter
            {
                [[nodiscard]] bool ready() const { return true; }
                [[nodiscard]] bool run_inline() const { return false; }
                [[nodiscard]] bool terminate() const { return false; }
            };

            return waiter {};
        }
        inline auto for_duration(const std::chrono::duration<uint64> duration)
        {
            struct waiter
            {
                explicit waiter(const std::chrono::duration<uint64> duration)
                    : start(std::chrono::high_resolution_clock::now())
                    , duration(duration)
                {
                }
                [[nodiscard]] bool ready() const
                {
                    const auto time_point = std::chrono::high_resolution_clock::now();
                    return (duration <= (time_point - start));
                }
                [[nodiscard]] bool run_inline() const { return false; }
                [[nodiscard]] bool terminate() const { return false; }

                std::chrono::time_point<std::chrono::high_resolution_clock> start;
                std::chrono::duration<uint64> duration;
            };

            return waiter(duration);
        }
    }
}
