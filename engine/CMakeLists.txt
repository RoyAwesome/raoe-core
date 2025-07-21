cmake_minimum_required(VERSION 3.26)

raoe_add_module(
        STATIC
        NAME "engine"
        NAMESPACE "raoe"
        INCLUDE_DIRECTORIES
        PUBLIC
        "include"
        CPP_SOURCE_FILES
        "src/engine.cpp"
        "src/sys/window.cpp"
        DEPENDENCIES
        PUBLIC
        raoe::core
        raoe::renderer
        raoe::filesystem
        spdlog
        flecs::flecs_static
        glm::glm
        glfw
        nlohmann_json::nlohmann_json
        glad
        imgui
        imgui-backend
        lua
        tomlplusplus::tomlplusplus
)