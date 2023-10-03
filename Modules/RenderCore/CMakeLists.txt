# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanRender

# ----------- Global Definitions -----------
SET(LIBRARY_NAME RenderCore)

# ------------- Library Setup --------------
SET(PRIVATE_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/RenderCoreModule.h
)

SET(PRIVATE_MODULES
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Window.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/EngineCore.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/ShaderManager.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/CommandsManager.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/PipelineManager.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/BufferManager.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/DeviceManager.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Utils/Helpers.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Utils/DebugHelpers.mxx

	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/EngineCore.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/ShaderManager.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/CommandsManager.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/PipelineManager.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/BufferManager.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/DeviceManager.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Utils/Helpers.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Utils/DebugHelpers.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Utils/Constants.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Utils/EnumConverter.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Utils/EnumHelpers.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Types/DeviceProperties.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Types/TextureData.ixx
)

SET(PUBLIC_MODULES
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Public/Window.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Public/Types/Vertex.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Public/Types/UniformBufferObject.ixx
)

ADD_LIBRARY(${LIBRARY_NAME} SHARED)

TARGET_SOURCES(${LIBRARY_NAME}
			   PRIVATE
			   FILE_SET cxx_private_headers_file_set
			   TYPE HEADERS
			   FILES ${PRIVATE_HEADERS}
)

TARGET_SOURCES(${LIBRARY_NAME}
			   PRIVATE
			   FILE_SET cxx_private_modules_file_set
			   TYPE CXX_MODULES
			   FILES ${PRIVATE_MODULES}
)

TARGET_SOURCES(${LIBRARY_NAME}
			   PUBLIC
			   FILE_SET cxx_public_modules_file_set
			   TYPE CXX_MODULES
			   FILES ${PUBLIC_MODULES}
)

TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Modules/Public)
TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private)

TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PRIVATE BUILD_DLL)

SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

FIND_PACKAGE(Boost REQUIRED COMPONENTS log)
FIND_PACKAGE(glm REQUIRED)
FIND_PACKAGE(Stb REQUIRED)
FIND_PACKAGE(assimp CONFIG REQUIRED)

FIND_PACKAGE(VulkanHeaders CONFIG REQUIRED)
FIND_PACKAGE(volk CONFIG REQUIRED)
FIND_PACKAGE(VulkanMemoryAllocator CONFIG REQUIRED)
FIND_PACKAGE(glslang CONFIG REQUIRED)

FIND_PACKAGE(glfw3 CONFIG REQUIRED)
FIND_PACKAGE(imgui CONFIG REQUIRED)

TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PRIVATE
					  Boost::log

					  glm::glm
					  assimp::assimp

					  Vulkan::Headers
					  volk::volk_headers
					  GPUOpen::VulkanMemoryAllocator

					  glslang::glslang
					  glslang::glslang-default-resource-limits
					  glslang::SPIRV

					  glfw
					  imgui::imgui

					  Timer
					  Configuration
)

TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PRIVATE
						   ${Stb_INCLUDE_DIR}
)

TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PRIVATE
						   # Assets directory (relative to binaries)
						   DEBUG_SHADER_VERT="Resources/Shaders/TRIANGLE_VERT_SHADER.vert"
						   DEBUG_SHADER_FRAG="Resources/Shaders/TRIANGLE_FRAG_SHADER.frag"
						   DEBUG_MODEL_OBJ="Resources/Assets/VIKING_ROOM_OBJ.obj"
						   DEBUG_MODEL_TEX="Resources/Assets/VIKING_ROOM_TEX.png"
						   EMPTY_TEX="Resources/Assets/EMPTY_TEX.png"
)

TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PUBLIC
						   GPU_API_DUMP=0
						   VK_NO_PROTOTYPES
)

IF (WIN32)
	TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PUBLIC
							   VK_USE_PLATFORM_WIN32_KHR
							   WIN32_LEAN_AND_MEAN
							   NOMINMAX
							   _WIN32_WINNT=0x0601
	)
ELSEIF (MACOS)
	TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PUBLIC
							   VK_USE_PLATFORM_MACOS_MVK
	)
ELSEIF (UNIX)
	TARGET_COMPILE_DEFINITIONS(${LIBRARY_NAME} PUBLIC
							   VK_USE_PLATFORM_XCB_KHR
	)
ENDIF (WIN32)
