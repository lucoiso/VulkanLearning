[![VulkanRender CMake Workflow](https://github.com/lucoiso/VulkanRender/actions/workflows/cmake-single-platform.yml/badge.svg)](https://github.com/lucoiso/VulkanRender/actions/workflows/cmake-single-platform.yml)

![image](https://github.com/lucoiso/VulkanRender/assets/77353979/119aef7f-0e49-45c1-81ba-b40dec441ea4)

# Dependencies

1. Vcpkg (w/ ENVIRONMENT VARIABLE VCPKG_ROOT set to the vcpkg root directory)
2. CMake 3.26.4
3. Vulkan SDK (Does not need any modules except the SDK core)
4. Git

# Setup

- Run these commands:

```
git clone https://github.com/lucoiso/VulkanRender.git
cd VulkanRender
git submodule update --init --recursive
```
