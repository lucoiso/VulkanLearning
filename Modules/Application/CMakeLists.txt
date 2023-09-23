# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanLearning

# ----------- Global Definitions -----------
set(LIBRARY_NAME Application)

# ------------- Library Setup --------------

set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Main.cpp
)

add_executable(${LIBRARY_NAME} ${SOURCE_FILES})

find_package(Boost REQUIRED COMPONENTS log)

target_link_libraries(${LIBRARY_NAME} PRIVATE
    Boost::log
    RenderCore
)

include(GNUInstallDirs)

install(TARGETS ${LIBRARY_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

configure_file(${PROJECT_CONFIG_DIRECTORY}/qt.conf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/qt.conf COPYONLY)

set(VCPKG_QT_PLUGINS_DIR ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/${CMAKE_BUILD_TYPE}/Qt${QT_LIB_VERSION}/plugins)
add_custom_command(TARGET ${LIBRARY_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${VCPKG_QT_PLUGINS_DIR} $<TARGET_FILE_DIR:${LIBRARY_NAME}>/plugins
)

add_custom_command(TARGET ${LIBRARY_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${PROJECT_RESOURCES_DIRECTORY} $<TARGET_FILE_DIR:${LIBRARY_NAME}>/Resources
)