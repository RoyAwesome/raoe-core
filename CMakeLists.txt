cmake_minimum_required(VERSION 3.26)

include(cmake/add-module.cmake)

add_subdirectory("core")

option(RAOE_CORE_INCLUDE_FILESYSTEM "Include the filesystem module" ON)
option(RAOE_CORE_INCLUDE_RENDERER "Include the renderer module" ON)
option(RAOE_CORE_INCLUDE_ENGINE "Include the engine module" ON)

if (RAOE_CORE_INCLUDE_FILESYSTEM)
    add_subdirectory("filesystem")
endif ()

if (RAOE_CORE_INCLUDE_RENDERER)
    add_subdirectory("renderer")
endif ()

if (RAOE_CORE_INCLUDE_ENGINE)
    add_subdirectory("engine")
endif ()