// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#ifndef RENDERCOREMODULE_H
#define RENDERCOREMODULE_H

#ifdef BUILD_DLL
#define RENDERCOREMODULE_API _declspec(dllexport)
#else
#define RENDERCOREMODULE_API _declspec(dllimport)
#endif
#endif
