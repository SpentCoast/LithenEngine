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

FetchContent_Declare(
	imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG v1.92.9b
)
FetchContent_MakeAvailable(imgui)
set(IMGUI_DIR ${imgui_SOURCE_DIR})

find_package(slang CONFIG HINTS "$ENV{VULKAN_SDK}/lib/cmake")
find_package(Vulkan REQUIRED)

find_program(SLANGC_EXECUTABLE slangc HINTS "$ENV{VULKAN_SDK}/Bin")
if(NOT SLANGC_EXECUTABLE)
    message(FATAL_ERROR "Could not find slangc executable. Please check your Vulkan SDK installation.")
endif()

add_library(Lithen
	"Source/Core/Engine.cpp"
	"Source/Core/Window.cpp"
	"Source/Core/Input.cpp"

	"Source/Graphics/VulkanContext.cpp"
	"Source/Graphics/Renderer.cpp"
	"Source/Graphics/Swapchain.cpp"
	"Source/Graphics/Buffer.cpp"
	"Source/Graphics/Texture.cpp"
	"Source/Graphics/GraphicsPipeline.cpp"
	"Source/Graphics/DescriptorManager.cpp"
	"Source/Graphics/Model.cpp"
	"Source/Graphics/Camera.cpp"

	"Source/External/stb_image.cpp"

	"${IMGUI_DIR}/imgui.cpp"
	"${IMGUI_DIR}/imgui_demo.cpp"
	"${IMGUI_DIR}/imgui_draw.cpp"
	"${IMGUI_DIR}/imgui_tables.cpp"
	"${IMGUI_DIR}/imgui_widgets.cpp"
	"${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
	"${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
)

target_include_directories(Lithen PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/Include"
	"${CMAKE_CURRENT_SOURCE_DIR}/Include/Lithen"
	"${CMAKE_CURRENT_SOURCE_DIR}/Include/Lithen/External"

	"${IMGUI_DIR}"
	"${IMGUI_DIR}/backends"
)

target_compile_definitions(Lithen PUBLIC IMGUI_DEFINE_MATH_OPERATORS)
target_compile_definitions(Lithen PRIVATE VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS)

target_link_libraries(Lithen PUBLIC Vulkan::Vulkan glfw tinygltf ktx)
target_precompile_headers(Lithen PRIVATE "Include/Lithen/LithenPCH.hpp")
