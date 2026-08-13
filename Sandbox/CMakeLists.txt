# CMakeList.txt : CMake project for Sandbox, include source and define
# project specific logic here.
#
add_executable(Sandbox 
    "Source/Main.cpp"
)

target_link_libraries(Sandbox PRIVATE Lithen)

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
    "$<TARGET_FILE_DIR:Sandbox>"
    COMMENT "Syncing Resources to build directory..."
)

add_slang_shader_target(Shader SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Shaders/Shader.slang")
add_dependencies(Sandbox Shader)
add_dependencies(Sandbox CopyResources)
