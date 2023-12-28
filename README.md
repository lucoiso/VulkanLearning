[![VulkanRenderer CMake Workflow](https://github.com/lucoiso/VulkanRenderer/actions/workflows/cmake-build.yml/badge.svg)](https://github.com/lucoiso/VulkanRenderer/actions/workflows/cmake-build.yml)

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

# Example

![image](https://github.com/lucoiso/VulkanRenderer/assets/77353979/d9aac4fe-3b0b-4862-a196-4dffda29763c)

- Renderer Application using this library: https://github.com/lucoiso/renderer-application
