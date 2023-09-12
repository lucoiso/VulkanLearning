# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanLearning

# ----------- Global Definitions -----------
set(LIBRARY_NAME RenderCore)

# ------------- Library Setup --------------
set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanRender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Managers/VulkanShaderManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Managers/VulkanCommandsManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Managers/VulkanPipelineManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Managers/VulkanBufferManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Managers/VulkanDeviceManager.cpp
)

set(HEADER_FILES
    # Public Headers
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/${LIBRARY_NAME}/VulkanRender.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/${LIBRARY_NAME}/Window.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/${LIBRARY_NAME}/Types/VulkanVertex.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/${LIBRARY_NAME}/Types/VulkanUniformBufferObject.h
    
    # Private Headers
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Utils/RenderCoreHelpers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Utils/VulkanDebugHelpers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Utils/VulkanEnumConverter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Utils/VulkanConstants.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Types/DeviceProperties.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Managers/VulkanShaderManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Managers/VulkanCommandsManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Managers/VulkanPipelineManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Managers/VulkanBufferManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Managers/VulkanDeviceManager.h
)

add_library(${LIBRARY_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_include_directories(${LIBRARY_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/${LIBRARY_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_compile_definitions(${LIBRARY_NAME} PRIVATE BUILD_DLL)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Boost REQUIRED CONFIG COMPONENTS log)
find_package(stb REQUIRED)
find_package(assimp REQUIRED)
find_package(Vulkan REQUIRED)

target_link_libraries(${LIBRARY_NAME} PRIVATE
    glfw
    glm::glm
    Boost::log
    stb::stb
    assimp::assimp
    ${Vulkan_LIBRARIES}
    glslang
    glslang-default-resource-limits
    SPIRV
    GPUOpen::VulkanMemoryAllocator
)

target_include_directories(${LIBRARY_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})

target_compile_definitions(${LIBRARY_NAME} PRIVATE 
    DEBUG_SHADER_VERT="${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Triangle.vert"
    DEBUG_SHADER_FRAG="${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Triangle.frag"
    DEBUG_MODEL_OBJ="${CMAKE_CURRENT_SOURCE_DIR}/Resources/viking_room.obj"
    DEBUG_MODEL_TEX="${CMAKE_CURRENT_SOURCE_DIR}/Resources/viking_room.png"
)

target_compile_definitions(${LIBRARY_NAME} PUBLIC
    WIN32_LEAN_AND_MEAN
    VK_USE_PLATFORM_WIN32_KHR
    NOMINMAX
)