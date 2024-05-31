cmake_minimum_required(VERSION 3.26)

macro(raoe_add_module)
    set(options STATIC SHARED MODULE HEADERONLY)
    set(oneValueArgs NAME NAMESPACE CXX_STANDARD TARGET_VARIABLE)
    set(multiValueArgs CPP_SOURCE_FILES INCLUDE_DIRECTORIES COMPILE_DEFINITIONS DEPENDENCIES)

    cmake_parse_arguments(raoe_add_module "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT raoe_add_module_CXX_STANDARD)
        set(raoe_add_module_CXX_STANDARD 20)
    endif()

    set(CMAKE_CXX_STANDARD ${raoe_add_module_CXX_STANDARD})

    # Validate the Input Arguments
    if(NOT raoe_add_module_NAME)
        message(FATAL_ERROR "This module must have a name")
    endif()

    if(NOT raoe_add_module_NAMESPACE)
        string(REPLACE "::" "-" MODULE_LIBRARY_NAME "raoe-module-${raoe_add_module_NAME}")
    else()
        string(REPLACE "::" "-" MODULE_LIBRARY_NAME "${raoe_add_module_NAMESPACE}-${raoe_add_module_NAME}")
    endif()

    macro(raoe_add_module_configure_module)
        if(NOT raoe_add_module_HEADERONLY AND NOT raoe_add_module_MODULE)
            target_include_directories(${PROJECT_NAME}
                ${raoe_add_module_INCLUDE_DIRECTORIES}
            )
        endif()

        if(NOT raoe_add_module_HEADERONLY)
            # add compile definitions.
            string(TOUPPER "${raoe_add_module_NAME}_API" module_api)
            string(REPLACE "-" "_" module_api ${raoe_add_module_NAME})
            target_compile_definitions(${PROJECT_NAME}
                PRIVATE

                # "${module_api}=1"
                # "RAOE_MODULE_NAME=${raoe_add_module_NAME}"
                ${raoe_add_module_COMPILE_DEFINITIONS}
            )
        endif()

        if(NOT raoe_add_module_NAMESPACE)
            set(raoe_add_module_alias "raoe::module::${raoe_add_module_NAME}${raoe_alias_sufix}")
        else()
            set(raoe_add_module_alias "${raoe_add_module_NAMESPACE}::${raoe_add_module_NAME}${raoe_alias_sufix}")
        endif()

        add_library("${raoe_add_module_alias}" ALIAS ${PROJECT_NAME})

        target_link_libraries(
            ${PROJECT_NAME}
            ${raoe_add_module_DEPENDENCIES}
        )
    endmacro()

    if(raoe_add_module_HEADERONLY)
        set(raoe_alias_sufix "-headeronly")
        project("${MODULE_LIBRARY_NAME}${raoe_alias_sufix}" LANGUAGES CXX)

        add_library("${PROJECT_NAME}" INTERFACE)
        target_include_directories("${PROJECT_NAME}"
            INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        )

        set(raoe_alias_sufix "")
        raoe_add_module_configure_module()
    endif()

    if(raoe_add_module_MODULE)
        set(raoe_alias_sufix "-MODULE")
        project("${MODULE_LIBRARY_NAME}${raoe_alias_sufix}" LANGUAGES CXX)

        # turn on the dynamic depends for ninja
        set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
        set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "2182bf5c-ef0d-489a-91da-49dbc3090d2a")

        add_library("${PROJECT_NAME}")
        target_sources("${PROJECT_NAME}"
            PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES FILES
            ${raoe_add_module_CPP_SOURCE_FILES}
        )
        raoe_add_module_configure_module()
    endif()

    if(raoe_add_module_STATIC)
        if(raoe_add_module_HEADERONLY)
            message(FATAL_ERROR "A module cannot be headeronly and static")
        endif()

        set(raoe_alias_sufix "")
        project("${MODULE_LIBRARY_NAME}-static" LANGUAGES CXX)

        add_library("${PROJECT_NAME}"
            STATIC
            ${raoe_add_module_CPP_SOURCE_FILES}
        )
        raoe_add_module_configure_module()
    endif()

    if(raoe_add_module_SHARED)
        if(raoe_add_module_HEADERONLY)
            message(FATAL_ERROR "A module cannot be headeronly and static")
        endif()

        message(FATAL_ERROR "Shared modules are not supported yet")
    endif()

    if(DEFINED raoe_add_module_TARGET_VARIABLE)
        set(${raoe_add_module_TARGET_VARIABLE} ${PROJECT_NAME})
    endif()

    # if this module has a third_party directory, include it now
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/CMakeLists.txt")
        add_subdirectory("third_party")
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
        add_subdirectory("test")
    endif()
endmacro()

macro(raoe_add_test)
    set(options)
    set(oneValueArgs NAME TARGET_VARIABLE)
    set(multiValueArgs CPP_SOURCE_FILES INCLUDE_DIRECTORIES COMPILE_DEFINITIONS DEPENDENCIES)

    cmake_parse_arguments(raoe_add_test "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT raoe_add_test_NAME)
        message(FATAL_ERROR "This module must have a name")
    endif()

    set(CMAKE_CXX_STANDARD 23) # turn on the dynamic depends for ninja

    # set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
    # set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "2182bf5c-ef0d-489a-91da-49dbc3090d2a")
    project("raoe-test-${raoe_add_test_NAME}" CXX)

    add_executable(${PROJECT_NAME}
        ${raoe_add_test_CPP_SOURCE_FILES}
    )

    enable_testing()

    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        SEQUENTIAL_PROCESSING=0
        ${raoe_add_test_COMPILE_DEFINITIONS}
    )

    CPMAddPackage("gh:catchorg/Catch2@3.3.2")
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
        Catch2::Catch2WithMain
        ${raoe_add_test_DEPENDENCIES}
    )

    if(DEFINED raoe_add_test_TARGET_VARIABLE)
        set(${raoe_add_test_TARGET_VARIABLE} ${PROJECT_NAME})
    endif()
endmacro()