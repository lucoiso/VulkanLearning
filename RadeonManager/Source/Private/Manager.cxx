// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <ADLX.h>
#include <string_view>

module RadeonManager.Manager;

using namespace RadeonManager;

adlx_handle        g_DLLHandle { nullptr };
adlx::IADLXSystem *g_SystemServices { nullptr };

ADLXQueryFullVersion_Fn        g_QueryFullVersionFn { nullptr };
ADLXQueryVersion_Fn            g_QueryVersionFn { nullptr };
ADLXInitializeWithCallerAdl_Fn g_InitializeWithCallerAdlFn { nullptr };
ADLXInitialize_Fn              g_InitializeFn { nullptr };
ADLXTerminate_Fn               g_TerminateFn { nullptr };

adlx_handle ADLX_CDECL_CALL CrossPlatformLoadLibrary(std::string_view const Filename)
{
    #ifdef _WIN32
    return ::LoadLibraryEx(std::data(Filename),
                           nullptr,
                           LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
                           LOAD_LIBRARY_SEARCH_SYSTEM32);
    #else
    return ::dlopen(Filename, RTLD_LAZY);
    #endif
}

int ADLX_CDECL_CALL CrossPlatformFreeLibrary(adlx_handle ModuleHandle)
{
    #ifdef _WIN32
    return FreeLibrary(static_cast<HMODULE>(ModuleHandle)) == TRUE;
    #else
    return ::dlclose(ModuleHandle) == 0;
    #endif
}

void * ADLX_CDECL_CALL CrossPlatformGetProcAddress(adlx_handle ModuleHandle, std::string_view const ProcName)
{
    #ifdef _WIN32
    return reinterpret_cast<void *>(GetProcAddress(static_cast<HMODULE>(ModuleHandle), std::data(ProcName)));
    #else
    return ::dlsym(ModuleHandle, std::data(ProcName));
    #endif
}

bool IsADLXLibraryLoaded()
{
    return g_DLLHandle;
}

bool IsADLXLoaded()
{
    return IsADLXLibraryLoaded() && g_QueryFullVersionFn && g_QueryVersionFn && g_InitializeWithCallerAdlFn && g_InitializeFn && g_TerminateFn;
}

bool LoadADLXLibrary()
{
    if (IsADLXLoaded())
    {
        return true;
    }

    if (g_DLLHandle = CrossPlatformLoadLibrary(ADLX_DLL_NAME);
        g_DLLHandle)
    {
        g_QueryFullVersionFn = static_cast<ADLXQueryFullVersion_Fn>(CrossPlatformGetProcAddress(g_DLLHandle, ADLX_QUERY_FULL_VERSION_FUNCTION_NAME));
        g_QueryVersionFn = static_cast<ADLXQueryVersion_Fn>(CrossPlatformGetProcAddress(g_DLLHandle, ADLX_QUERY_VERSION_FUNCTION_NAME));
        g_InitializeWithCallerAdlFn = static_cast<ADLXInitializeWithCallerAdl_Fn>(CrossPlatformGetProcAddress(g_DLLHandle,
            ADLX_INIT_WITH_CALLER_ADL_FUNCTION_NAME));
        g_InitializeFn = static_cast<ADLXInitialize_Fn>(CrossPlatformGetProcAddress(g_DLLHandle, ADLX_INIT_FUNCTION_NAME));
        g_TerminateFn  = static_cast<ADLXTerminate_Fn>(CrossPlatformGetProcAddress(g_DLLHandle, ADLX_TERMINATE_FUNCTION_NAME));
    }

    return IsADLXLoaded();
}

bool RadeonManager::IsRunning()
{
    return g_SystemServices != nullptr;
}

bool RadeonManager::IsLoaded()
{
    return IsADLXLoaded();
}

bool RadeonManager::Start()
{
    if (!IsADLXLoaded())
    {
        if (!LoadADLXLibrary())
        {
            return false;
        }
    }

    if (IsRunning())
    {
        return true;
    }

    return g_InitializeFn(ADLX_FULL_VERSION, &g_SystemServices) == ADLX_OK;
}

void RadeonManager::Stop()
{
    if (!IsADLXLibraryLoaded())
    {
        return;
    }

    if (IsRunning())
    {
        g_TerminateFn();
    }

    g_DLLHandle                 = nullptr;
    g_SystemServices            = nullptr;
    g_QueryFullVersionFn        = nullptr;
    g_QueryVersionFn            = nullptr;
    g_InitializeWithCallerAdlFn = nullptr;
    g_InitializeFn              = nullptr;
    g_TerminateFn               = nullptr;

    CrossPlatformFreeLibrary(g_DLLHandle);
}

adlx::IADLXSystem *RadeonManager::GetADLXSystemServices()
{
    return g_SystemServices;
}
