# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanRenderer

# ----------- Global Definitions -----------
SET(LIBRARY_NAME Application)

# ------------- Library Setup --------------
ADD_EXECUTABLE(${LIBRARY_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Main.cpp)
SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE RenderCore)

INCLUDE(GNUInstallDirs)
INSTALL(TARGETS ${LIBRARY_NAME}
		BUNDLE DESTINATION .
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

ADD_CUSTOM_COMMAND(TARGET ${LIBRARY_NAME} POST_BUILD
				   COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${PROJECT_RESOURCES_DIRECTORY}/Shaders" "$<TARGET_FILE_DIR:${LIBRARY_NAME}>/Resources/Shaders"
				   COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${PROJECT_RESOURCES_DIRECTORY}/Assets/glTF-Sample-Assets/Models"
				   "$<TARGET_FILE_DIR:${LIBRARY_NAME}>/Resources/Assets"
)