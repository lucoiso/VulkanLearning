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
        self.requires("glfw/3.4")

        # https://conan.io/center/recipes/imgui
        self.requires("imgui/1.91.0-docking")

        # https://conan.io/center/recipes/boost
        self.requires("boost/1.85.0")

        # https://conan.io/center/recipes/tinygltf
        self.requires("tinygltf/2.9.0")

        # https://conan.io/center/recipes/benchmark
        self.requires("benchmark/1.8.4")

        # https://conan.io/center/recipes/catch2
        self.requires("catch2/3.6.0")

        # https://conan.io/center/recipes/meshoptimizer
        self.requires("meshoptimizer/0.21")

        # https://conan.io/center/recipes/easy_profiler
        self.requires("easy_profiler/2.1.0")

    def configure(self):
        self.options["glfw/*"].shared = False
        self.options["imgui/*"].shared = True
        self.options["boost/*"].shared = True
        self.options["boost/*"].without_cobalt = True
        self.options["benchmark/*"].shared = True
        self.options["catch2/*"].shared = True
        self.options["catch2/*"].enable_exceptions = True
        self.options["meshoptimizer/*"].shared = True
        self.options["easy_profiler/*"].shared = True

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.28]")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self, generator="Ninja")
        tc.generate()
