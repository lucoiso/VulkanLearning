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

find_package(Boost REQUIRED CONFIG COMPONENTS log)

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
