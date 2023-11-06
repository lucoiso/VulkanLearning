//
// Created by User on 30.10.2023.
//

module;

#include "RenderCoreModule.h"

export module RenderCore.Types.Coroutine;

export import <coroutine>;
import <algorithm>;

namespace RenderCore
{
    export template<typename T>
        requires(!std::is_void_v<T>)
    struct RENDERCOREMODULE_API AsyncOperation
    {
        struct promise_type
        {
        private:
            std::exception_ptr m_Exception;
            T m_Value;

        public:
            promise_type()  = default;
            ~promise_type() = default;

            std::suspend_never initial_suspend()
            {
                return {};
            }

            std::suspend_never final_suspend() noexcept
            {
                return {};
            }

            template<std::convertible_to<T> ValueTy>
            std::suspend_always yield_value(ValueTy&& Value)
            {
                m_Value = std::forward<ValueTy>(Value);
                return {};
            }

            void return_value(T&& Value)
            {
                m_Value = std::forward<T>(Value);
            }

            auto get_return_object()
            {
                return AsyncOperation {std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            void unhandled_exception()
            {
                m_Exception = std::current_exception();
            }

            T get()
            {
                return m_Value;
            }
        };

        struct awaitable
        {
            promise_type& promise;

            bool await_ready() noexcept
            {
                return promise.final_suspend().await_ready();
            }

            void await_suspend(std::coroutine_handle<> awaiting_coroutine)
            {
                promise.final_suspend().await_suspend(awaiting_coroutine);
            }

            T await_resume()
            {
                return promise.get();
            }
        };

        awaitable operator co_await() noexcept
        {
            return awaitable {m_CoroutineHandle.promise()};
        }

    private:
        std::coroutine_handle<promise_type> m_CoroutineHandle;

    public:
        AsyncOperation(std::coroutine_handle<promise_type> Handle)
            : m_CoroutineHandle(Handle)
        {
        }

        ~AsyncOperation()
        {
            if (m_CoroutineHandle && m_CoroutineHandle.done())
            {
                m_CoroutineHandle.destroy();
            }
        }

        T Get()
        {
            return m_CoroutineHandle.promise().get();
        }
    };

    export struct RENDERCOREMODULE_API AsyncTask
    {
        struct promise_type
        {
        private:
            std::exception_ptr m_Exception;

        public:
            promise_type()  = default;
            ~promise_type() = default;

            std::suspend_never initial_suspend()
            {
                return {};
            }

            std::suspend_never final_suspend() noexcept
            {
                return {};
            }

            void return_void()
            {
            }

            auto get_return_object()
            {
                return AsyncTask {std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            void unhandled_exception()
            {
                m_Exception = std::current_exception();
            }

            void get()
            {
            }
        };

        struct awaitable
        {
            promise_type& promise;

            bool await_ready() noexcept
            {
                return promise.final_suspend().await_ready();
            }

            void await_suspend(std::coroutine_handle<> awaiting_coroutine)
            {
                promise.final_suspend().await_suspend(awaiting_coroutine);
            }

            void await_resume()
            {
                promise.get();
            }
        };

        awaitable operator co_await() noexcept
        {
            return awaitable {m_CoroutineHandle.promise()};
        }

    private:
        std::coroutine_handle<promise_type> m_CoroutineHandle;

    public:
        AsyncTask(std::coroutine_handle<promise_type> Handle)
            : m_CoroutineHandle(Handle)
        {
        }

        ~AsyncTask()
        {
            if (m_CoroutineHandle && m_CoroutineHandle.done())
            {
                m_CoroutineHandle.destroy();
            }
        }

        void Get()
        {
            return m_CoroutineHandle.promise().get();
        }
    };
}// namespace RenderCore