# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanRender

# ----------- Global Definitions -----------
SET(LIBRARY_NAME Benchmark)

# ------------- Library Setup --------------
SET(PRIVATE_MODULES
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Benchmark.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Placeholder.ixx
)

SET(PUBLIC_MODULES
)

ADD_EXECUTABLE(${LIBRARY_NAME} ${PRIVATE_MODULES} ${PUBLIC_MODULES})

TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Modules/Public/)
TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/)

SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

FIND_PACKAGE(benchmark CONFIG REQUIRED)

TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE
					  benchmark::benchmark_main
)
