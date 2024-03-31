// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

#ifndef MACROS_H
#define MACROS_H

#ifdef _DEBUG
    #define PUSH_CALLSTACK() RuntimeInfo::Manager::Get().PushCallstack();
    #define PUSH_CALLSTACK_WITH_COUNTER() auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
#else
    #define PUSH_CALLSTACK()
    #define PUSH_CALLSTACK_WITH_COUNTER()
#endif

#endif
