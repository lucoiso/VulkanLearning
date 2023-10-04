[![VulkanRender CMake Workflow](https://github.com/lucoiso/VulkanRender/actions/workflows/windows-x64-cmake-release.yml/badge.svg)](https://github.com/lucoiso/VulkanRender/actions/workflows/windows-x64-cmake-release.yml)

![image](https://github.com/lucoiso/VulkanRender/assets/77353979/cee86fe8-dfe2-4561-b378-1ba3925a7ce4)

# Dependencies

1. Vcpkg (w/ ENVIRONMENT VARIABLE VCPKG_ROOT set to the vcpkg root directory)
2. CMake 3.28 (Officially supporting C++20 Modules)
3. Vulkan SDK (Does not need any modules except the SDK core)
4. Git

# Setup

- Run these commands:

```
git clone https://github.com/lucoiso/VulkanRender.git
cd VulkanRender
git submodule update --init --recursive
```
