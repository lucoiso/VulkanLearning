# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanRenderer

# -------------- Directories ---------------
ADD_SUBDIRECTORY(Application)
ADD_SUBDIRECTORY(RenderCore)
ADD_SUBDIRECTORY(cpp-submodules)
ADD_SUBDIRECTORY(ThirdParty)

IF (BUILD_TESTS)
	ADD_SUBDIRECTORY(Tests)
ENDIF (BUILD_TESTS)
