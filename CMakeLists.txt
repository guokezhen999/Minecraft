cmake_minimum_required(VERSION 3.27)
project(Minecraft)

set(CMAKE_CXX_STANDARD 17)

include_directories(glad/include)
include_directories(vendor)

include_directories(/opt/homebrew/Cellar/glfw/3.4/include)
link_directories(/opt/homebrew/Cellar/glfw/3.4/lib)

add_executable(Minecraft
        Main.cpp
        glad/src/glad.c
        vendor/stb_image/stb_image.cpp
        Game.h
        Game.cpp
        Config.h
        Context.h
        Context.cpp
        Util/NonCopyable.h
        Util/NonMovable.h
        Util/Singleton.h
        Util/Random.h
        Util/Random.cpp
        Shaders/Shader.cpp
        Shaders/Shader.h
        Textures/BasicTexture.cpp
        Textures/BasicTexture.h
        Camera.cpp
        Camera.h
        Renderer/CubeRenderer.cpp
        Renderer/CubeRenderer.h
        World/Block/BlockTypes/BlockType.cpp
        World/Block/BlockTypes/BlockType.h
        World/Block/BlockId.h
        World/Block/BlockData.cpp
        World/Block/BlockData.h
        World/WorldConstants.h
        Renderer/RenderInfo.h
        Renderer/RenderMaster.cpp
        Renderer/RenderMaster.h
        Textures/CubeTexture.cpp
        Textures/CubeTexture.h
        Shaders/BasicShader.cpp
        Shaders/BasicShader.h
        Entity.h
        Textures/TextureAtlas.cpp
        Textures/TextureAtlas.h
        World/Block/ChunkBlock.cpp
        World/Block/ChunkBlock.h
        ResourceManager.cpp
        ResourceManager.h
        World/Block/BlockDataBase.cpp
        World/Block/BlockDataBase.h
        Mesh.h
        Model.cpp
        Model.h
        Renderer/ChunkRenderer.cpp
        Renderer/ChunkRenderer.h
        Shaders/ChunkShader.cpp
        Shaders/ChunkShader.h
)

target_link_libraries(Minecraft
        glfw
)
