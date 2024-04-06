// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

#ifndef RENDERCORETESTSSHAREDUTILSMODULE_H
#define RENDERCORETESTSSHAREDUTILSMODULE_H

#ifdef BUILD_DLL
#define RENDERCORETESTSSHAREDUTILSMODULE_API _declspec(dllexport)
#else
#define RENDERCORETESTSSHAREDUTILSMODULE_API _declspec(dllimport)
#endif
#endif
