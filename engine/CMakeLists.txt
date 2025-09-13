cmake_minimum_required(VERSION 3.26)

raoe_add_module(
        STATIC
        NAME "engine"
        NAMESPACE "raoe"
        INCLUDE_DIRECTORIES
        PUBLIC
        "include"
        PRIVATE
        "src"
        CPP_SOURCE_FILES
        "src/engine.cpp"
        "src/render.cpp"
        "src/asset.cpp"
        "src/sys/window.cpp"
        "src/sys/pack.cpp"
        COMPILE_DEFINITIONS
        PRIVATE
        "TOML_EXCEPTIONS=0" # disable exceptions in toml
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

raoe_add_asset_pack(
        NAME "core"
        DIRECTORY "assets"
)
