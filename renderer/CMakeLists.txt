cmake_minimum_required(VERSION 3.26)

raoe_add_module(
        STATIC
        NAME "renderer"
        NAMESPACE "raoe"
        INCLUDE_DIRECTORIES
        PUBLIC
        "include"
        CPP_SOURCE_FILES
        "src/render.cpp"
        "src/buffer.cpp"
        "src/mesh.cpp"
        "src/shader.cpp"
        "src/texture.cpp"
        "src/immediate.cpp"
        COMPILE_DEFINITIONS
        PRIVATE
        "GLM_ENABLE_EXPERIMENTAL"
        DEPENDENCIES
        PUBLIC
        raoe::core
        glm::glm
        glad
)