# Copyright Notices: [...]

from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMakeToolchain


class VulkanRendererRecipe(ConanFile):
    name = "vulkan-renderer"
    version = "0.0.1"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        # https://conan.io/center/recipes/boost
        self.requires("boost/1.86.0")

        # https://conan.io/center/recipes/tinygltf
        self.requires("tinygltf/2.9.0")

        # https://conan.io/center/recipes/meshoptimizer
        self.requires("meshoptimizer/0.21")

    def configure(self):
        self.options["boost/*"].shared = True
        self.options["boost/*"].without_cobalt = True
        self.options["meshoptimizer/*"].shared = True

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.28]")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self, generator="Ninja")
        tc.generate()
