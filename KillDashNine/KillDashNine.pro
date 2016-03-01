TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS_DEBUG += -Wall

#INCLUDEPATH += /home/lateralus/work/glm

LIBS += -lSDL2 -lX11 -lGL -lSDL2_mixer

SOURCES += \
    Main.cpp \
    engine/graphics/Fog.cpp \
    extlibs/gl_core_4_5.c \
    extlibs/imgui/imgui.cpp \
    extlibs/imgui/imgui_demo.cpp \
    extlibs/imgui/imgui_draw.cpp \
    engine/DataLogger.cpp \
    engine/Transform.cpp \
    engine/WavefrontObjectLoader.cpp \
    engine/Camera.cpp \
    engine/SdlManager.cpp \
    engine/ResourceManager.cpp \
    engine/graphics/GlUtils.cpp \
    engine/graphics/Material.cpp \
    engine/graphics/Shader.cpp \
    engine/graphics/Light.cpp \
    engine/graphics/PointLight.cpp \
    engine/graphics/SpotLight.cpp \
    ImGuiHelper.cpp \
    KillDashNine.cpp \
    engine/graphics/Entity.cpp \
    engine/graphics/MaterialFactory.cpp \
    engine/graphics/Tex2dImpl.cpp \
    Player.cpp \
    engine/graphics/TexSkyboxImpl.cpp \
    engine/graphics/MeshImpl.cpp \
    engine/graphics/PostProcessorImpl.cpp \
    engine/graphics/Skybox.cpp \
    engine/graphics/Sprite.cpp \
    engine/graphics/TexPerlinNoise2dImpl.cpp \
    LevelGenerator.cpp \
    engine/graphics/IndexedMeshImpl.cpp \
    engine/graphics/MeshFactory.cpp \
    engine/audio/SdlMixer.cpp \
    PowerUp.cpp \
    engine/audio/Chunk.cpp \
    engine/audio/Music.cpp \
    Enemy.cpp \
    TitleState.cpp \
    PlayState.cpp \
    MenuState.cpp \
	StateMap.cpp

HEADERS += \
    engine/graphics/Fog.hpp \
    extlibs/gl_core_4_5.h \
    extlibs/stb_image.h \
    extlibs/imgui/imconfig.h \
    extlibs/imgui/imgui.h \
    extlibs/imgui/imgui_internal.h \
    extlibs/imgui/stb_image.h \
    extlibs/imgui/stb_rect_pack.h \
    extlibs/imgui/stb_textedit.h \
    extlibs/imgui/stb_truetype.h \
    engine/graphics/IMesh.hpp \
    engine/graphics/ITexture.hpp \
    engine/DataLogger.hpp \
    engine/AABB.hpp \
    engine/Config.hpp \
    engine/Transform.hpp \
    engine/Utils.hpp \
    engine/Vertex.hpp \
    engine/WavefrontObjectLoader.hpp \
    engine/Camera.hpp \
    engine/SdlManager.hpp \
    engine/ResourceManager.hpp \
    engine/graphics/GlUtils.hpp \
    engine/graphics/Material.hpp \
    engine/graphics/Shader.hpp \
    IState.hpp \
    engine/graphics/Attenuation.hpp \
    engine/graphics/Light.hpp \
    engine/graphics/PointLight.hpp \
    engine/graphics/SpotLight.hpp \
    IApplication.hpp \
    ImGuiHelper.hpp \
    ResourcePaths.hpp \
    ResourceIds.hpp \
    KillDashNine.hpp \
    engine/graphics/Entity.hpp \
    engine/graphics/MaterialFactory.hpp \
    engine/graphics/Tex2dImpl.hpp \
    Player.hpp \
    engine/graphics/TexSkyboxImpl.hpp \
    engine/graphics/MeshImpl.hpp \
    engine/graphics/IFramebuffer.hpp \
    engine/graphics/PostProcessorImpl.hpp \
    engine/graphics/Skybox.hpp \
    engine/graphics/Sprite.hpp \
    engine/graphics/TexPerlinNoise2dImpl.hpp \
    LevelGenerator.hpp \
    engine/graphics/IndexedMeshImpl.hpp \
    engine/graphics/MeshFactory.hpp \
    ResourceLevels.hpp \
    engine/audio/SdlMixer.hpp \
    PowerUp.hpp \
    engine/audio/Chunk.hpp \
    engine/audio/Music.hpp \
    Enemy.hpp \
    TitleState.hpp \
    PlayState.hpp \
    MenuState.hpp \
	StateMap.hpp

DISTFILES += \
    ../resources/shaders/level.frag.glsl \
    ../resources/shaders/level.vert.glsl \
    ../resources/shaders/skybox.frag.glsl \
    ../resources/shaders/skybox.vert.glsl \
    ../resources/shaders/effects.frag.glsl \
    ../resources/shaders/effects.vert.glsl \
    ../resources/shaders/sprite.frag.glsl \
    ../resources/shaders/sprite.geom.glsl \
    ../resources/shaders/sprite.vert.glsl

