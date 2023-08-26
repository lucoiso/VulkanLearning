// Copyright Notice: [...]

#ifndef RENDERMODULE_H
#define RENDERMODULE_H

#ifdef BUILD_DLL
#define RENDERCOREMODULE_API _declspec(dllexport)
#else
#define RENDERCOREMODULE_API _declspec(dllimport)
#endif
#endif
