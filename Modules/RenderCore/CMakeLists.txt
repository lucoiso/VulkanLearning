# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanRender

# ----------- Global Definitions -----------
SET(LIBRARY_NAME RenderCore)

# ------------- Library Setup --------------
SET(PRIVATE_MODULES
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Window.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/VulkanRenderCore.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/VulkanShaderManager.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/VulkanCommandsManager.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/VulkanPipelineManager.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/VulkanBufferManager.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/VulkanDeviceManager.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Utils/RenderCoreHelpers.mxx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Utils/VulkanDebugHelpers.mxx

	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/VulkanRenderCore.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/VulkanShaderManager.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/VulkanCommandsManager.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/VulkanPipelineManager.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/VulkanBufferManager.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Managers/VulkanDeviceManager.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Utils/RenderCoreHelpers.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Utils/VulkanDebugHelpers.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Utils/VulkanConstants.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Utils/VulkanEnumConverter.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Utils/EnumHelpers.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Types/DeviceProperties.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Private/Types/TextureData.ixx
)

SET(PUBLIC_MODULES
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Public/Window.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Public/Types/VulkanVertex.ixx
	${CMAKE_CURRENT_SOURCE_DIR}/Modules/Public/Types/VulkanUniformBufferObject.ixx
)

ADD_LIBRARY(${LIBRARY_NAME} SHARED ${PRIVATE_MODULES} ${PUBLIC_MODULES})

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
