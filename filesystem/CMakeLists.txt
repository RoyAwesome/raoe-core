cmake_minimum_required(VERSION 3.26)

raoe_add_module(
    NAME "filesystem"
    NAMESPACE "raoe"
    INCLUDE_DIRECTORIES
    PUBLIC
        "include"
    CPP_SOURCE_FILES
        "src/filesystem.cpp"
    DEPENDENCIES
    PUBLIC
        physfs-static
)