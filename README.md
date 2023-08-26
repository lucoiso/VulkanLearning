# Dependencies
1. Python 3.11
2. CMake 3.22
3. Visual Studio 2022 Build Tools for C++
4. Windows 10/11
5. x64 Arch
6. Vulkan SDK

# Setup
- Run these commands:
```
git clone https://github.com/lucoiso/VulkanLearning.git
cd VulkanLearning
git submodule update --init --recursive
conan install . --build=missing -pr Profiles/Windows_x64_Release
conan install . --build=missing -pr Profiles/Windows_x64_Debug
```
