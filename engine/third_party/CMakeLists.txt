CPMAddPackage("gh:SanderMertens/flecs#master")
CPMAddPackage("gh:hanickadot/compile-time-regular-expressions#main")
CPMAddPackage("gh:nlohmann/json#v3.11.3")
CPMAddPackage("gh:marzer/tomlplusplus#master")
CPMAddPackage("gh:g-truc/glm#master")


CPMAddPackage(
        NAME GLFW
        GITHUB_REPOSITORY glfw/glfw
        GIT_TAG 3.4
        OPTIONS
        "GLFW_BUILD_TESTS OFF"
        "GLFW_BUILD_EXAMPLES OFF"
        "GLFW_BULID_DOCS OFF"
)


CPMAddPackage(
        NAME imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG "v1.91.9b"
        DOWNLOAD_ONLY YES
)
if (imgui_ADDED)
    add_library(imgui
                STATIC
                "${imgui_SOURCE_DIR}/imgui.cpp"
                "${imgui_SOURCE_DIR}/imgui_demo.cpp"
                "${imgui_SOURCE_DIR}/imgui_draw.cpp"
                "${imgui_SOURCE_DIR}/imgui_tables.cpp"
                "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
                "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp"
    )

    target_include_directories(imgui
                               PUBLIC
                               ${imgui_SOURCE_DIR}
    )
endif ()

if (imgui_ADDED)
    add_library(imgui-backend
                STATIC
                "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
                "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp")

    target_link_libraries(imgui-backend
                          PUBLIC
                          imgui
                          glad
                          glfw
    )


endif ()


CPMAddPackage(
        NAME lua
        GIT_REPOSITORY https://github.com/lua/lua.git
        GIT_TAG "v5.4.7"
        DOWNLOAD_ONLY YES
)

if (lua_ADDED)
    # lua has no CMake support, so we create our own target
    FILE(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
    list(REMOVE_ITEM lua_sources "${lua_SOURCE_DIR}/lua.c" "${lua_SOURCE_DIR}/luac.c")
    add_library(lua STATIC ${lua_sources})

    target_include_directories(lua
                               PUBLIC
                               $<BUILD_INTERFACE:${lua_SOURCE_DIR}>
    )

    if (LINUX)
        target_compile_definitions(lua
                                   PUBLIC
                                   LUA_USE_LINUX
        )
    elseif (APPLE)
        target_compile_definitions(lua
                                   PUBLIC
                                   LUA_USE_MACOSX
        )
    elseif (WIN32)
        target_compile_definitions(lua
                                   PUBLIC
                                   LUA_USE_WINDOWS
        )
    endif ()
endif ()