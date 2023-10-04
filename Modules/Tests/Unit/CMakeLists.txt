# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanRender

# ----------- Global Definitions -----------
SET(LIBRARY_NAME Unit)

# ------------- Library Setup --------------
SET(PRIVATE_MODULES
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Unit.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Placeholder.ixx
)

ADD_EXECUTABLE(${LIBRARY_NAME} ${PRIVATE_SOURCES} ${PRIVATE_MODULES})
SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

ADD_TEST(NAME ${LIBRARY_NAME} COMMAND $<TARGET_FILE:${LIBRARY_NAME}>)

FIND_PACKAGE(Boost REQUIRED COMPONENTS system)

TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE
					  Boost::system
)