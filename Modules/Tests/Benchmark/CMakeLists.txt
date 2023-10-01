# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanRender

# ----------- Global Definitions -----------
SET(LIBRARY_NAME Benchmark)

# ------------- Library Setup --------------
SET(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Benchmark.cpp
)

SET(INCLUDE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/Include/${LIBRARY_NAME}/Placeholder.h
)

ADD_EXECUTABLE(${LIBRARY_NAME} ${SOURCE_FILES} ${INCLUDE_FILES})
TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Include/${LIBRARY_NAME})

FIND_PACKAGE(benchmark CONFIG REQUIRED)

TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE
					  benchmark::benchmark_main
)
