# Copyright Notices: [...]

# ----------- Global Definitions -----------
set(LIBRARY_NAME Benchmark)

# ------------- Library Setup --------------
set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Benchmark.cpp
)

set(INCLUDE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/${LIBRARY_NAME}/Window.h
)

add_executable(${LIBRARY_NAME} ${SOURCE_FILES} ${INCLUDE_FILES})
target_include_directories(${LIBRARY_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Include/${LIBRARY_NAME})

find_package(benchmark REQUIRED CONFIG COMPONENTS benchmark_main)

target_link_libraries(${LIBRARY_NAME} PRIVATE
    benchmark::benchmark_main
    RenderCore
)
