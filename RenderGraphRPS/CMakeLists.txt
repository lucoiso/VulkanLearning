# Author: Lucas Vilas-Boas
# Year: 2024
# Repo: https://github.com/lucoiso/VulkanRenderer

# ----------- Global Definitions -----------
SET(LIBRARY_NAME RenderGraphRPS)

# ------------- Library Setup --------------
SET(PRIVATE_MODULES_BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source/Private)
SET(PUBLIC_MODULES_BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source/Public)

SET(PRIVATE_MODULES
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Manager.cxx
)

SET(PUBLIC_MODULES
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Manager.ixx
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Utils.ixx
)

ADD_LIBRARY(${LIBRARY_NAME} SHARED ${PRIVATE_MODULES})
SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

TARGET_SOURCES(${LIBRARY_NAME}
        PUBLIC
        FILE_SET cxx_public_modules
        TYPE CXX_MODULES
        BASE_DIRS ${PUBLIC_MODULES_BASE_DIRECTORY}
        FILES ${PUBLIC_MODULES}
)

TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PRIVATE ${PRIVATE_MODULES_BASE_DIRECTORY})
TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PRIVATE BUILD_DLL)

IF (CMAKE_BUILD_TYPE MATCHES "Debug")
    TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE $ENV{VULKAN_SDK}/Lib/volkd.lib)
ELSE ()
    TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE $ENV{VULKAN_SDK}/Lib/volk.lib)
ENDIF ()

TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PRIVATE $ENV{VULKAN_SDK}/Include)

OPTION(RpsEnableVulkan "Enable Vulkan backend" ON)
OPTION(RpsEnableImGui "Enable ImGui" OFF)
OPTION(RpsBuildTests "Enable unit test targets" OFF)
OPTION(RpsBuildTools "Enable tool targets" OFF)
OPTION(RpsBuildExamples "Enable example targets" OFF)
OPTION(RpsEnableDXAgilitySDK "Enable DX12 Agility SDK" OFF)
OPTION(RpsPackagingIncludeStaticLibs "Include prebuilt static libs during packaging" OFF)
OPTION(RpsEnableDefaultDeviceImpl "Enable default allocator & printer support" OFF)

INCLUDE(FetchContent)

FETCHCONTENT_DECLARE(
        renderpipelineshaders
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders
        GIT_TAG main
)
FETCHCONTENT_MAKEAVAILABLE(renderpipelineshaders)

TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PRIVATE ${renderpipelineshaders_SOURCE_DIR}/include)
TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE rps_core rps_runtime rps_runtime_vk rps_runtime_vkdyn)
