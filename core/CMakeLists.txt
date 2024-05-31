cmake_minimum_required(VERSION 3.26)

CPMAddPackage("gh:hanickadot/compile-time-regular-expressions#main")

raoe_add_module(
    HEADERONLY
    NAME "core"
    NAMESPACE "raoe"
    DEPENDENCIES
    INTERFACE
    ctre::ctre
    COMPILE_DEFINITIONS
    "RAOE_CORE_USE_LIBASSERT=1"
    INCLUDE_DIRECTORIES
    PUBLIC
    "include"
    CPP_SOURCE_FILES
    "mod.ixx"
)
