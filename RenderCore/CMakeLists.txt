# Author: Lucas Vilas-Boas
# Year: 2024
# Repo: https://github.com/lucoiso/vulkan-renderer

# ----------- Global Definitions -----------
SET(LIBRARY_NAME RenderCore)

# ------------- Library Setup --------------
SET(PRIVATE_MODULES_BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source/Private)
SET(PUBLIC_MODULES_BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source/Public)
SET(SHADERS_RESOURCES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders)

SET(PRIVATE_MODULES
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Rendering/Renderer.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Rendering/Core/Command.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Rendering/Core/Device.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Rendering/Core/Instance.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Rendering/Core/Memory.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Rendering/Core/Model.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Rendering/Core/Offscreen.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Rendering/Core/Pipeline.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Rendering/Core/Scene.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Rendering/Core/ShaderCompiler.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Rendering/Core/SwapChain.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Rendering/Core/Synchronization.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Rendering/Factories/MeshFactory.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Rendering/Factories/TextureFactory.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Types/Allocation.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Types/Camera.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Types/Illumination.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Types/Mesh.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Types/Object.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Types/Resource.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Types/Texture.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Utils/Debug/DebugHelpers.cxx"
        "${PRIVATE_MODULES_BASE_DIRECTORY}/Utils/Library/Helpers.cxx"
)

SET(PUBLIC_MODULES
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/Renderer.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/Core/Command.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/Core/Device.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/Core/Instance.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/Core/Memory.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/Core/Model.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/Core/Offscreen.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/Core/Pipeline.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/Core/Scene.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/Core/ShaderCompiler.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/Core/SwapChain.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/Core/Synchronization.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/Factories/MeshFactory.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Rendering/Factories/TextureFactory.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Types/Allocation.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Types/Camera.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Types/Illumination.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Types/Mesh.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Types/Object.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Types/Resource.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Types/Texture.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Types/Material.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Types/RendererStateFlags.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Types/UniformBufferObject.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Types/Vertex.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Types/SurfaceProperties.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Types/Transform.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Utils/Debug/DebugHelpers.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Utils/Enum/EnumConverter.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Utils/Enum/EnumHelpers.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Utils/Library/Constants.ixx"
        "${PUBLIC_MODULES_BASE_DIRECTORY}/Utils/Library/Helpers.ixx"
)

SET(PUBLIC_HEADERS
        "${PUBLIC_MODULES_BASE_DIRECTORY}/RenderCoreModule.hpp"
)

ADD_LIBRARY(${LIBRARY_NAME} STATIC ${PRIVATE_MODULES})
SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

TARGET_SOURCES(${LIBRARY_NAME}
        PUBLIC
        FILE_SET cxx_public_modules
        TYPE CXX_MODULES
        BASE_DIRS ${PUBLIC_MODULES_BASE_DIRECTORY}
        FILES ${PUBLIC_MODULES}
)

TARGET_SOURCES(${LIBRARY_NAME}
        PUBLIC
        FILE_SET cxx_public_headers
        TYPE HEADERS
        BASE_DIRS ${PUBLIC_MODULES_BASE_DIRECTORY}
        FILES ${PUBLIC_HEADERS}
)

TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PRIVATE ${PRIVATE_MODULES_BASE_DIRECTORY})
TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PUBLIC ${PUBLIC_MODULES_BASE_DIRECTORY})
TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PRIVATE BUILD_DLL)

FIND_PACKAGE(Boost REQUIRED COMPONENTS log)
FIND_PACKAGE(tinygltf CONFIG REQUIRED)

FIND_PACKAGE(glfw3 CONFIG REQUIRED)
FIND_PACKAGE(meshoptimizer REQUIRED)

TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PUBLIC
        Boost::log
        TinyGLTF::TinyGLTF
        glfw
        meshoptimizer::meshoptimizer

        Timer
        ThreadPool
)

SET(VULKAN_LIBS_TO_LINK
        volk
        SPIRV-Tools
        SPIRV-Tools-opt
        GenericCodeGen
        glslang
        glslang-default-resource-limits
        MachineIndependent
        OSDependent
        SPIRV
)

FOREACH (VkLib ${VULKAN_LIBS_TO_LINK})
    IF (CMAKE_BUILD_TYPE MATCHES "Debug")
        TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE $ENV{VULKAN_SDK}/Lib/${VkLib}d.lib)
    ELSE ()
        TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE $ENV{VULKAN_SDK}/Lib/${VkLib}.lib)
    ENDIF (CMAKE_BUILD_TYPE MATCHES "Debug")
ENDFOREACH ()

TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PUBLIC $ENV{VULKAN_SDK}/Include)

TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PRIVATE
        # Assets directory (relative to binaries)
        DEFAULT_VERTEX_SHADER="Shaders/DEFAULT_SHADER.vert"
        DEFAULT_FRAGMENT_SHADER="Shaders/DEFAULT_SHADER.frag"
        DEFAULT_TASK_SHADER="Shaders/DEFAULT_SHADER.task"
        DEFAULT_MESH_SHADER="Shaders/DEFAULT_SHADER.mesh"
)

TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PUBLIC
        GPU_API_DUMP=0
)

IF (WIN32)
    SET(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)

    TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PUBLIC
            VK_USE_PLATFORM_WIN32_KHR
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _WIN32_WINNT=0x0A00
    )
ELSEIF (MACOS)
    SET(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)

    TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PUBLIC
            VK_USE_PLATFORM_MACOS_MVK
    )
ELSEIF (UNIX)
    SET(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XCB_KHR)

    TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PUBLIC
            VK_USE_PLATFORM_XCB_KHR
    )
ENDIF (WIN32)

INCLUDE(FetchContent)

FETCHCONTENT_DECLARE(
        stringzilla
        GIT_REPOSITORY https://github.com/ashvardanian/StringZilla
        GIT_TAG main
)
FETCHCONTENT_MAKEAVAILABLE(stringzilla)

IF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    TARGET_COMPILE_OPTIONS(${LIBRARY_NAME} PUBLIC -Wno-unknown-pragmas)
ELSEIF (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    TARGET_COMPILE_OPTIONS(${LIBRARY_NAME} PUBLIC /wd4068)
ENDIF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PUBLIC ${stringzilla_SOURCE_DIR}/include)

SET(PCH_FILE ${PUBLIC_MODULES_BASE_DIRECTORY}/PCH.hpp)
TARGET_PRECOMPILE_HEADERS(${LIBRARY_NAME} PUBLIC ${PCH_FILE})

ADD_CUSTOM_TARGET(RENDERCORE_COPY_SHADERS
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${SHADERS_RESOURCES_DIRECTORY}" "${CMAKE_INSTALL_BINDIR}/Shaders"
        COMMENT "[CMake Command] [RenderCore]: Copying renderer default shaders"
)