[![VulkanRenderer CMake Workflow](https://github.com/lucoiso/VulkanRenderer/actions/workflows/windows-x64-cmake-release.yml/badge.svg)](https://github.com/lucoiso/VulkanRenderer/actions/workflows/windows-x64-cmake-release.yml)  
_Note: Github Actions Workflow Failing until CMake 3.28 official release._

---

![image](https://github.com/lucoiso/VulkanRenderer/assets/77353979/260bc6be-76e9-4c70-8d8a-81e6df5d409e)

# Dependencies

1. Vcpkg (w/ ENVIRONMENT VARIABLE VCPKG_ROOT set to the vcpkg root directory)
2. CMake v3.28 (Officially supporting C++20 Modules)
3. Vulkan SDK v1.3.261 (Does not need any modules except the SDK core)
4. Git

# Setup

- Run these commands:

```
git clone https://github.com/lucoiso/VulkanRenderer.git
cd VulkanRenderer
git submodule update --init --recursive
```
