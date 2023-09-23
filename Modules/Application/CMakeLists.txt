# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanLearning

# ----------- Global Definitions -----------
set(LIBRARY_NAME Application)

# ------------- Library Setup --------------

set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Main.cpp
)

find_package(Qt${QT_LIB_VERSION}Core CONFIG REQUIRED)
find_package(Qt${QT_LIB_VERSION}Gui CONFIG REQUIRED)
find_package(Qt${QT_LIB_VERSION}Quick CONFIG REQUIRED)

qt_add_executable(${LIBRARY_NAME} ${SOURCE_FILES})

target_link_libraries(${LIBRARY_NAME} PRIVATE 
    RenderCore

    Qt${QT_LIB_VERSION}::Core
    Qt${QT_LIB_VERSION}::Gui
    Qt${QT_LIB_VERSION}::Quick
)

include(GNUInstallDirs)

install(TARGETS ${LIBRARY_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_custom_command(TARGET ${LIBRARY_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${VCPKG_QT_PLUGINS_DIR} $<TARGET_FILE_DIR:${LIBRARY_NAME}>/plugins
)

add_custom_command(TARGET ${LIBRARY_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${VCPKG_QT_QML_DIR} $<TARGET_FILE_DIR:${LIBRARY_NAME}>/qml
)

add_custom_command(TARGET ${LIBRARY_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${PROJECT_RESOURCES_DIRECTORY} $<TARGET_FILE_DIR:${LIBRARY_NAME}>/Resources
)