// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

#ifndef RENDERGRAPHRPSMODULE_H
#define RENDERGRAPHRPSMODULE_H

#ifdef BUILD_DLL
    #define RENDERGRAPHRPSMODULE_API _declspec(dllexport)
#else
    #define RENDERGRAPHRPSMODULE_API _declspec(dllimport)
#endif
#endif
