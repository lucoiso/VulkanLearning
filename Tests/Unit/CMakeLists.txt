# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanRenderer

# ----------- Global Definitions -----------
SET(LIBRARY_NAME RenderCoreUnit)

# ------------- Library Setup --------------
SET(PRIVATE_MODULES
	Modules/Private/RenderCoreUnit.cpp
	Modules/Private/RenderCore.ixx
	Modules/Private/Helpers.ixx
)

ADD_EXECUTABLE(${LIBRARY_NAME} ${PRIVATE_MODULES})
SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

ADD_TEST(NAME ${LIBRARY_NAME} COMMAND $<TARGET_FILE:${LIBRARY_NAME}>)

FIND_PACKAGE(Catch2 CONFIG REQUIRED)

TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE
					  Catch2::Catch2
					  RenderCore
					  RenderCoreTestsSharedUtils
)

IF (BUILD_TYPE STREQUAL "Debug")
	FIND_PACKAGE(Boost REQUIRED COMPONENTS log)
	TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE Boost::log)
ENDIF (BUILD_TYPE STREQUAL "Debug")