cmake_minimum_required(VERSION 3.21)
project(pp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_FILES 
	"src/main.cpp"
	"src/audio.cpp"
	"src/Chunk.cpp"
	"src/Directory.cpp"
	"src/File.cpp"
	"src/Framebuffer.cpp"
	"src/GPUBuffer.cpp"
	"src/Math5D.cpp"
	"src/QuadRenderer.cpp"
	"src/QuadRendererBasic.cpp"
	"src/Shader.cpp"
	"src/StateManager.cpp"
	"src/StateControls.cpp"
	"src/StateCreateWorld.cpp"
	"src/StateGame.cpp"
	"src/StateGeneratingWorld.cpp"
	"src/StateLoadWorld.cpp"
	"src/StateTitleScreen.cpp"
	"src/TexRenderer.cpp"
	"src/TextRenderer.cpp"
	"src/Texture.cpp"
	"src/TextureBuffer.cpp"
	"src/World.cpp"
	"src/ui.cpp"
)

set(HEADER_FILES 
	"include/audio.h"
	"include/Block.h"
	"include/Chunk.h"
	"include/Directory.h"
	"include/File.h"
	"include/Framebuffer.h"
	"include/GPUBuffer.h"
	"include/MashPool.h"
	"include/Math5D.h"
	"include/OpenSimplexNoise.hpp"
	"include/QuadRenderer.h"
	"include/QuadRendererBasic.h"
	"include/Shader.h"
	"include/State.h"
	"include/StateManager.h"
	"include/StateControls.h"
	"include/StateCreateWorld.h"
	"include/StateGame.h"
	"include/StateGeneratingWorld.h"
	"include/StateLoadWorld.h"
	"include/StateTitleScreen.h"
	"include/stb_image.h"
	"include/TexRenderer.h"
	"include/TextRenderer.h"
	"include/Texture.h"
	"include/TextureBuffer.h"
	"include/World.h"
	"include/tuple_hash.h"
	"include/ui.h"
	"include/utils.h"
	"include/vec5.h"
	"include/vec6.h"
)

include_directories(include)

add_executable(pp ${SRC_FILES} ${HEADER_FILES} "src/icon.rc")

set_target_properties(pp PROPERTIES OUTPUT_NAME "Penteract Placer")

find_package(OpenGL REQUIRED)
target_link_libraries(pp PRIVATE ${OPENGL_LIBRARIES})

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(pp PRIVATE glfw)

find_package(glm CONFIG REQUIRED)
target_link_libraries(pp PRIVATE glm::glm-header-only)
target_compile_definitions(pp PRIVATE GLM_EXPERIMENTAL)

find_package(Boost REQUIRED)
target_link_libraries(pp PRIVATE Boost::headers)

add_subdirectory("libs/ndtf")
target_link_libraries(pp PRIVATE ndtf)

add_subdirectory("libs/soloud/contrib")
target_link_libraries(pp PRIVATE SoLoud)

add_subdirectory("libs/glad")
target_link_libraries(pp PRIVATE glad)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

if(WIN32)
	set_target_properties(pp PROPERTIES WIN32_EXECUTABLE "$<NOT:$<CONFIG:Debug>>")

	install(IMPORTED_RUNTIME_ARTIFACTS glfw DESTINATION .)

	install(TARGETS pp RUNTIME DESTINATION .)

	install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets DESTINATION .)

elseif(LINUX)
	install(TARGETS pp RUNTIME DESTINATION lib/pp RENAME "Penteract Placer")
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets DESTINATION lib/pp)

	configure_file(
			${CMAKE_CURRENT_SOURCE_DIR}/cmake/dev.tr1ngle.pp.desktop
			${CMAKE_CURRENT_BINARY_DIR}/dev.tr1ngle.pp.desktop
			@ONLY
	)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dev.tr1ngle.pp.desktop
			DESTINATION share/applications
	)

	install(FILES ${CMAKE_SOURCE_DIR}/LICENSE.txt DESTINATION share/licenses/pp)
	install(FILES ${CMAKE_SOURCE_DIR}/assets/textures/icon16.png DESTINATION share/icons/hicolor/16x16/apps RENAME dev.tr1ngle.pp.png)
	install(FILES ${CMAKE_SOURCE_DIR}/assets/textures/icon32.png DESTINATION share/icons/hicolor/32x32/apps RENAME dev.tr1ngle.pp.png)
	install(FILES ${CMAKE_SOURCE_DIR}/assets/textures/icon48.png DESTINATION share/icons/hicolor/48x48/apps RENAME dev.tr1ngle.pp.png)
	install(FILES ${CMAKE_SOURCE_DIR}/assets/textures/icon64.png DESTINATION share/icons/hicolor/64x64/apps RENAME dev.tr1ngle.pp.png)
endif()
