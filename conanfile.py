# Copyright Notices: [...]

from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMakeToolchain


class VulkanRendererRecipe(ConanFile):
    name = "vulkan-renderer"
    version = "0.0.1"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        # https://conan.io/center/recipes/glfw
        self.requires("glfw/[>=3.3]")

        # https://conan.io/center/recipes/imgui
        self.requires("imgui/1.90.1-docking")

        # https://conan.io/center/recipes/boost
        self.requires("boost/[>=1.84]",
                      options={
                          "without_cobalt": True
                      })

        # https://conan.io/center/recipes/benchmark
        self.requires("benchmark/[>=1.8]")

        # https://conan.io/center/recipes/catch2
        self.requires("catch2/[>=3.4]")

        # https://conan.io/center/recipes/glm
        self.requires("glm/[>=0.9]")

        # https://conan.io/center/recipes/tinygltf
        self.requires("tinygltf/[>=2.8]")

        # https://conan.io/center/recipes/glslang
        # self.requires("glslang/11.7.0") # Uncomment when glslang 14.0 is available

        # https://conan.io/center/recipes/volk
        self.requires("volk/[>=1.3]")

        # https://conan.io/center/recipes/vulkan-memory-allocator
        self.requires("vulkan-memory-allocator/cci.20231120")

        # https://conan.io/center/recipes/vulkan-headers
        self.requires("vulkan-headers/[>=1.3]", override=True)

        # https://conan.io/center/recipes/spirv-tools
        # self.requires("spirv-tools/[>=1.3]", override=True) # Uncomment when glslang 14.0 is available

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.28]")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self, generator="Ninja")
        tc.generate()
