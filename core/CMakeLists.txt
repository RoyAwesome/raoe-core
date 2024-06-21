cmake_minimum_required(VERSION 3.26)


set(CPM_DOWNLOAD_VERSION 0.40.0)

if(CPM_SOURCE_CACHE)
    set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
    set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()

if(NOT(EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD
        https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
        ${CPM_DOWNLOAD_LOCATION}
    )
endif()

include(${CPM_DOWNLOAD_LOCATION})

#set the default values for using libassert and spdlog
if(NOT DEFINED RAOE_CORE_USE_LIBASSERT)
    set(RAOE_CORE_USE_LIBASSERT 1)
endif()

if(NOT DEFINED RAOE_CORE_USE_SPDLOG)
	set(RAOE_CORE_USE_SPDLOG 1)
endif()

if(NOT DEFINED RAOE_CORE_EMIT_MODULE)
    set(RAOE_CORE_EMIT_MODULE 0)
endif()


#build the dependencies for the core module based on the configuration
set(RAOE_CORE_COMPILE_DEFINITIONS "")
set(RAOE_CORE_INTERFACES "")
set(RAOE_CORE_TRANSITIVE "")
list(APPEND RAOE_CORE_INTERFACES "ctre::ctre")

if(RAOE_CORE_USE_LIBASSERT)
	CPMAddPackage("gh:jeremy-rifkin/libassert#v1.1")
    list(APPEND RAOE_CORE_COMPILE_DEFINITIONS "RAOE_CORE_USE_LIBASSERT=1")
    list(APPEND RAOE_CORE_TRANSITIVE "assert")
endif()

if(RAOE_CORE_USE_SPDLOG)
    CPMAddPackage("gh:gabime/spdlog#v1.11.0")
    list(APPEND RAOE_CORE_COMPILE_DEFINITIONS "RAOE_CORE_USE_SPDLOG=1")
    list(APPEND RAOE_CORE_INTERFACES "spdlog::spdlog_header_only")
endif()

CPMAddPackage("gh:hanickadot/compile-time-regular-expressions#main")




if(RAOE_CORE_EMIT_MODULE) #EXPERIMENTAL: Export this library as a cpp20 module
    raoe_add_module(
        MODULE
        NAME "core"
        NAMESPACE "raoe"
        DEPENDENCIES
        INTERFACE
            ${RAOE_CORE_INTERFACES}
        PUBLIC
            ${RAOE_CORE_TRANSITIVE}
        COMPILE_DEFINITIONS
            PUBLIC
              RAOE_CORE_COMPILE_DEFINITIONS
        INCLUDE_DIRECTORIES
            PUBLIC
                "include"
         CPP_SOURCE_FILES
            "mod.ixx"
    )
else() #STANDARD: Export this library as a header only library
    raoe_add_module(
        HEADERONLY
        NAME "core"
        NAMESPACE "raoe"
        TARGET_VARIABLE
            RAOE_CORE_TARGET
        DEPENDENCIES 
			INTERFACE
				${RAOE_CORE_INTERFACES}
        COMPILE_DEFINITIONS
            PUBLIC
              RAOE_CORE_COMPILE_DEFINITIONS
        INCLUDE_DIRECTORIES
            PUBLIC
                "include"
        CPP_SOURCE_FILES
            "mod.ixx"
    )

    foreach(RAOE_CORE_TRANSITIVE_ITEM IN ITEMS ${RAOE_CORE_TRANSITIVE})
        target_link_libraries(${RAOE_CORE_TARGET} INTERFACE ${RAOE_CORE_TRANSITIVE_ITEM} $<TARGET_OBJECTS:${RAOE_CORE_TRANSITIVE_ITEM}>)
    endforeach()

endif()