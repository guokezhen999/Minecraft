cmake_minimum_required(VERSION 3.27)
project(Minecraft)

set(CMAKE_CXX_STANDARD 17)

include_directories(glad/include)
include_directories(vendor)

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(Minecraft
        Main.cpp
        glad/src/glad.c
        vendor/stb_image/stb_image.cpp
        Game.h
        Game.cpp
        Config.h
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
        World/Chunk/Chunk.h
        World/Chunk/Chunk.cpp
        World/Chunk/ChunkSection.h
        World/Chunk/ChunkSection.cpp
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
        World/Chunk/IChunk.h
        World/Chunk/ChunkMesh.cpp
        World/Chunk/ChunkMesh.h
        Util/Frustum.h
        Physics/AABB.h
        Physics/RayCast.h
        Physics/RayCast.cpp
        Physics/Player.h
        Physics/Player.cpp
        Renderer/OutlineRenderer.h
        Renderer/OutlineRenderer.cpp
        Renderer/CrosshairRenderer.h
        Renderer/CrosshairRenderer.cpp
        Renderer/HotbarRenderer.h
        Renderer/HotbarRenderer.cpp
        Renderer/SkyRenderer.h
        Renderer/SkyRenderer.cpp
        UI/Hotbar.h
        UI/Hotbar.cpp
        World/TerrainGenerator.h
        World/TerrainGenerator.cpp
        World/World.h
        World/World.cpp
)

find_package(Threads REQUIRED)

target_link_libraries(Minecraft
        glfw
        OpenGL::GL
        Threads::Threads
)

add_custom_command(TARGET Minecraft POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    ${CMAKE_CURRENT_BINARY_DIR}/resources
    COMMENT "Copying resources to build directory"
)