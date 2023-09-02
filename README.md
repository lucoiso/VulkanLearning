[![VulkanLearning CMake Workflow](https://github.com/lucoiso/VulkanLearning/actions/workflows/cmake-single-platform.yml/badge.svg)](https://github.com/lucoiso/VulkanLearning/actions/workflows/cmake-single-platform.yml)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/121e8e09a8b84b6d854fc12fff70ae38)](https://app.codacy.com/gh/lucoiso/VulkanLearning/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

# Dependencies
1.  Python 3.11 
2.  Conan 2.0 
3.  CMake 3.22 
4.  Visual Studio 2022 Build Tools for C++ 
5.  Windows 10/11 
6.  x64 Arch 
7.  Vulkan SDK 

# Setup
-   Run these commands:  
```
git clone https://github.com/lucoiso/VulkanLearning.git
cd VulkanLearning
git submodule update --init --recursive
conan install . --build=missing -pr Profiles/Windows_x64_Release
conan install . --build=missing -pr Profiles/Windows_x64_Debug
```
