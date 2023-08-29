# Copyright Notices: [...]

# ----------- Global Definitions -----------
set(LIBRARY_NAME RenderCore)

# ------------- Library Setup --------------
set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanRender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanShaderCompiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanCommandsManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanPipelineManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanBufferManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanDeviceManager.cpp
)

set(HEADER_FILES
    # Public Headers
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/${LIBRARY_NAME}/VulkanRender.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/${LIBRARY_NAME}/Window.h

    # Private Headers
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanDebugHelpers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanEnumConverter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanConstants.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanShaderCompiler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanCommandsManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanPipelineManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanBufferManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanDeviceManager.h
)

add_library(${LIBRARY_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_include_directories(${LIBRARY_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Include/${LIBRARY_NAME})
target_compile_definitions(${LIBRARY_NAME} PRIVATE BUILD_DLL)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Boost REQUIRED CONFIG COMPONENTS log)

target_link_libraries(${LIBRARY_NAME} PRIVATE
    glfw
    ${Vulkan_LIBRARIES}
    Boost::log
    glslang
    glslang-default-resource-limits
    SPIRV
)

target_include_directories(${LIBRARY_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})

# Testing only - Debugging Shaders
target_compile_definitions(${LIBRARY_NAME} PRIVATE DEBUG_SHADER_VERT="${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Triangle.vert")
target_compile_definitions(${LIBRARY_NAME} PRIVATE DEBUG_SHADER_FRAG="${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Triangle.frag")
