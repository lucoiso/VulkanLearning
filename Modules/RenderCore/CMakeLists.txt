# Copyright Notices: [...]

# ----------- Global Definitions -----------
set(LIBRARY_NAME RenderCore)

# ------------- Library Setup --------------
set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanRender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanCommands.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanPipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanBufferManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanConfigurator.cpp
)

set(HEADER_FILES
    # Public Headers
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/${LIBRARY_NAME}/VulkanRender.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/${LIBRARY_NAME}/Window.h

    # Private Headers
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanEnumConverter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanCommands.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanPipeline.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanBufferManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VulkanConfigurator.h
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
)

target_include_directories(${LIBRARY_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
