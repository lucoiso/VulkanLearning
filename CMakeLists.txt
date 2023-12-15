# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanRenderer

# ----------- Global Definitions -----------
PROJECT(VulkanRenderer VERSION 0.1.0 LANGUAGES CXX)
CMAKE_MINIMUM_REQUIRED(VERSION 3.28)

SET(BUILD_TESTS $ENV{BUILD_TESTS})

# -------------- Directories ---------------
ADD_SUBDIRECTORY(RenderCore)
ADD_SUBDIRECTORY(cpp-submodules)
ADD_SUBDIRECTORY(ThirdParty)

IF (BUILD_TESTS)
	INCLUDE(CTest)
	ADD_SUBDIRECTORY(Tests)
ENDIF (BUILD_TESTS)
