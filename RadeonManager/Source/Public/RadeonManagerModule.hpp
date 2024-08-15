// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

#ifndef RADEONMANAGERMODULE_H
#define RADEONMANAGERMODULE_H

#ifdef BUILD_DLL
    #define RADEONMANAGERMODULE_API _declspec(dllexport)
#else
    #define RADEONMANAGERMODULE_API _declspec(dllimport)
#endif
#endif
