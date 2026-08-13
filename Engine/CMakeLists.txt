# CMakeList.txt : CMake project for Engine, include source and define
# project specific logic here.
#
include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG 3.5.1
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
	tinygltf
	GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
	GIT_TAG v2.9.7
)
FetchContent_MakeAvailable(tinygltf)

FetchContent_Declare(
	ktx
	GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
	GIT_TAG v4.4.2
)
set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ktx)

find_package(slang CONFIG HINTS "$ENV{VULKAN_SDK}/lib/cmake")
find_package(Vulkan REQUIRED)

find_program(SLANGC_EXECUTABLE slangc HINTS "$ENV{VULKAN_SDK}/Bin")
if(NOT SLANGC_EXECUTABLE)
    message(FATAL_ERROR "Could not find slangc executable. Please check your Vulkan SDK installation.")
endif()

add_executable(Engine
 "Source/Core/Main.cpp"
 "Include/Lithen.hpp"

 "Source/Core/Engine.cpp"
 "Source/Core/Window.cpp"

 "Source/Graphics/VulkanContext.cpp"
 "Source/Graphics/Renderer.cpp"
 "Source/Graphics/Swapchain.cpp"
 "Source/Graphics/Buffer.cpp"
 "Source/Graphics/Texture.cpp"
 "Source/Graphics/GraphicsPipeline.cpp"
 "Source/Graphics/DescriptorManager.cpp"
 "Source/Graphics/Model.cpp"

 "Source/External/stb_image.cpp")

target_include_directories(Engine PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/Include"
	"${CMAKE_CURRENT_SOURCE_DIR}/Include/Lithen"
	"${CMAKE_CURRENT_SOURCE_DIR}/Include/Lithen/External"
)

target_compile_definitions(Engine PRIVATE VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS)

target_link_libraries(Engine PRIVATE
	Vulkan::Vulkan
	glfw
	tinygltf
	ktx
)

target_precompile_headers(Engine PRIVATE "Include/Lithen/LithenPCH.hpp")

function(add_slang_shader_target TARGET)
	cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})
	set(SHADERS_DIR "${CMAKE_CURRENT_BINARY_DIR}/Shaders")
	set(ENTRY_POINTS -entry vertMain -entry fragMain)
	add_custom_command(
		OUTPUT ${SHADERS_DIR}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
	)
	add_custom_command(
		OUTPUT ${SHADERS_DIR}/slang.spv
		COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o slang.spv
		WORKING_DIRECTORY ${SHADERS_DIR}
		DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
		COMMENT "Compiling Slang Shaders"
		VERBATIM
	)
	add_custom_target(${TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)
endfunction()

add_custom_target(CopyResources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
    "$<TARGET_FILE_DIR:Engine>"
    COMMENT "Syncing Resources to build directory..."
)

add_slang_shader_target(Shader SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Shaders/Shader.slang")
add_dependencies(Engine	Shader)
add_dependencies(Engine	CopyResources)
