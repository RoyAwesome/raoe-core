cmake_minimum_required(VERSION 3.26)

include(cmake/add-module.cmake)

add_subdirectory("core")

option(RAOE_CORE_INCLUDE_FILESYSTEM "Include the filesystem module" TRUE)

if(RAOE_CORE_INCLUDE_FILESYSTEM)
    add_subdirectory("filesystem")
endif()