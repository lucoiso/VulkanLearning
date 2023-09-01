# Dependencies
1. Python 3.11 
2. Conan 2.0 
3. CMake 3.22 
4. Visual Studio 2022 Build Tools for C++ 
5. Windows 10/11 
6. x64 Arch 
7. Vulkan SDK 

# Setup
- Run these commands:  
```
git clone https://github.com/lucoiso/VulkanLearning.git
cd VulkanLearning
git submodule update --init --recursive
conan install . --build=missing -pr Profiles/Windows_x64_Release
conan install . --build=missing -pr Profiles/Windows_x64_Debug
```
