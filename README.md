[![VulkanLearning CMake Workflow](https://github.com/lucoiso/VulkanLearning/actions/workflows/cmake-single-platform.yml/badge.svg)](https://github.com/lucoiso/VulkanLearning/actions/workflows/cmake-single-platform.yml)

![image](https://github.com/lucoiso/VulkanLearning/assets/77353979/9a63384b-6add-4d37-ae2c-bcde3fe271e8)

# Dependencies
1.  Python 3.11 
2.  Conan 2.0 
3.  CMake 3.22 
4.  Visual Studio 2022 Build Tools for C++ 
5.  Windows 10/11 
6.  x64 Arch 
7.  Vulkan SDK
8.  Git

# Setup
-   Run these commands:  
```
git clone https://github.com/lucoiso/VulkanLearning.git
cd VulkanLearning
git submodule update --init --recursive
conan install . --build=missing -pr Profiles/Windows_x64_Release
conan install . --build=missing -pr Profiles/Windows_x64_Debug
```
