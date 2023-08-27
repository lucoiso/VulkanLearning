// Copyright Notice: [...]

#ifndef RENDERCOREMODULE_H
#define RENDERCOREMODULE_H

#ifdef BUILD_DLL
#define RENDERCOREMODULE_API _declspec(dllexport)
#else
#define RENDERCOREMODULE_API _declspec(dllimport)
#endif
#endif
