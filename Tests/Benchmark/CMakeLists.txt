# Author: Lucas Vilas-Boas
# Year: 2024
# Repo: https://github.com/lucoiso/vulkan-renderer

# ----------- Global Definitions -----------
SET(LIBRARY_NAME RenderCoreBenchmark)

# ------------- Library Setup --------------
SET(PRIVATE_MODULES_BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source/Private)

SET(PRIVATE_MODULES
    ${PRIVATE_MODULES_BASE_DIRECTORY}/RenderCoreBenchmark.cpp
    ${PRIVATE_MODULES_BASE_DIRECTORY}/RenderCore.hpp
)

ADD_EXECUTABLE(${LIBRARY_NAME} ${PRIVATE_MODULES})
SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PRIVATE ${PRIVATE_MODULES_BASE_DIRECTORY})

FIND_PACKAGE(benchmark CONFIG REQUIRED)

TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE
                      benchmark::benchmark_main
                      RenderCore
                      SharedUtils
)

ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${LIBRARY_NAME}> $<TARGET_FILE_DIR:${LIBRARY_NAME}>
                   COMMAND_EXPAND_LISTS
)