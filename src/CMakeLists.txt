set(IMGUI_SOURCES
	engine/imgui/imgui.cpp
	engine/imgui/imgui_draw.cpp
)

set(STB_HEADER
	engine/graphics/stb_image.h
)

set(SHOOTER_SOURCES
	Main.cpp
	Shooter.cpp
	Player.cpp
	PowerUp.cpp
	Level.cpp
	Enemy.cpp
	Bullet.cpp
	engine/Camera.cpp
	engine/Logger.cpp
	engine/ResourceManager.cpp
	engine/SdlWindow.cpp
	engine/gl_core_4_5.c
	engine/Transform.cpp
	engine/ImGuiHelper.cpp
	engine/graphics/Entity.cpp
	engine/graphics/Fog.cpp
	engine/graphics/GlUtils.cpp
	engine/graphics/IndexedMeshImpl.cpp
	engine/graphics/Light.cpp
	engine/graphics/Material.cpp
	engine/graphics/MaterialFactory.cpp
	engine/graphics/MeshFactory.cpp
	engine/graphics/MeshImpl.cpp
	engine/graphics/PointLight.cpp
	engine/graphics/PostProcessorImpl.cpp
	engine/graphics/Shader.cpp
	engine/graphics/Skybox.cpp
	engine/graphics/SpotLight.cpp
	engine/graphics/Sprite.cpp
	engine/graphics/Tex2dImpl.cpp
	engine/graphics/TexPerlinImpl.cpp
	engine/graphics/TexSkyboxImpl.cpp
	engine/audio/Chunk.cpp
	engine/audio/Music.cpp
	engine/audio/SdlMixer.cpp
)

add_executable(${CMAKE_PROJECT_NAME} ${SHOOTER_SOURCES} ${IMGUI_SOURCES} ${STB_HEADER})
target_link_libraries(${CMAKE_PROJECT_NAME} ${SHOOTER_LIBS})