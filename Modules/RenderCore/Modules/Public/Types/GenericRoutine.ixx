//
// Created by User on 30.10.2023.
//

module;

#include "RenderCoreModule.h"

export module RenderCore.Types.GenericRoutine;

export import <coroutine>;
import <algorithm>;

namespace RenderCore
{
    export template<typename T>
        requires(!std::is_void_v<T>)
    struct RENDERCOREMODULE_API GenericAsyncOperation
    {
        struct promise_type
        {
        private:
            T m_Result;

        public:
            auto get_return_object()
            {
                return GenericAsyncOperation {std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_never initial_suspend()
            {
                return {};
            }

            std::suspend_never final_suspend() noexcept
            {
                return {};
            }

            void return_value(T&& Value)
            {
                m_Result = std::move(Value);
            }

            void unhandled_exception()
            {
            }

            T& get()
            {
                return m_Result;
            }
        };

    private:
        std::coroutine_handle<promise_type> m_CoroutineHandle;

    public:
        GenericAsyncOperation(std::coroutine_handle<promise_type> Handle)
            : m_CoroutineHandle(Handle)
        {
        }

        ~GenericAsyncOperation()
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

    export struct RENDERCOREMODULE_API GenericAsyncTask
    {
        struct promise_type
        {
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
                return GenericAsyncTask {std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            void unhandled_exception()
            {
            }

            void get()
            {
            }
        };

        using handle_type = std::coroutine_handle<promise_type>;

    private:
        handle_type m_CoroutineHandle;

    public:
        GenericAsyncTask(handle_type Handle)
            : m_CoroutineHandle(Handle)
        {
        }

        ~GenericAsyncTask()
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