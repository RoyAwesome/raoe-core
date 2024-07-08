cmake_minimum_required(VERSION 3.26)

macro(raoe_add_module)
    set(options STATIC SHARED MODULE HEADERONLY EXECUTABLE)
    set(oneValueArgs NAME NAMESPACE CXX_STANDARD TARGET_VARIABLE)
    set(multiValueArgs CPP_SOURCE_FILES INCLUDE_DIRECTORIES COMPILE_DEFINITIONS DEPENDENCIES COPY_DIRECTORY)

    cmake_parse_arguments(raoe_add_module "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT raoe_add_module_CXX_STANDARD)
        if(NOT CMAKE_CXX_STANDARD)
            set(raoe_add_module_CXX_STANDARD 20)
        else()
            set(raoe_add_module_CXX_STANDARD ${CMAKE_CXX_STANDARD})
        endif()
    endif()

    set(CMAKE_CXX_STANDARD ${raoe_add_module_CXX_STANDARD})

    # Validate the Input Arguments
    if(NOT raoe_add_module_NAME)
        message(FATAL_ERROR "This module must have a name")
    endif()

    if(NOT raoe_add_module_NAMESPACE)
        set(raoe_add_module_NAMESPACE "raoe")
    endif()

    string(REPLACE "::" "-" MODULE_LIBRARY_NAME "${raoe_add_module_NAMESPACE}-${raoe_add_module_NAME}")

    macro(raoe_add_module_configure_module)
        if(NOT raoe_add_module_HEADERONLY AND NOT raoe_add_module_MODULE)
            if(raoe_add_module_INCLUDE_DIRECTORIES)
                target_include_directories(${PROJECT_NAME}
                    ${raoe_add_module_INCLUDE_DIRECTORIES}
                )
            endif()
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

        if(NOT raoe_add_module_EXECUTABLE)
            set(raoe_add_module_alias "${raoe_add_module_NAMESPACE}::${raoe_add_module_NAME}${raoe_alias_sufix}")
            add_library("${raoe_add_module_alias}" ALIAS ${PROJECT_NAME})
        endif()

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
    elseif(raoe_add_module_MODULE)
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

    elseif(raoe_add_module_STATIC)
        set(raoe_alias_sufix "")
        project("${MODULE_LIBRARY_NAME}-static" LANGUAGES CXX)

        add_library("${PROJECT_NAME}"
                STATIC
                ${raoe_add_module_CPP_SOURCE_FILES}
        )
        raoe_add_module_configure_module()
    elseif(raoe_add_module_SHARED)
        message(FATAL_ERROR "Shared modules are not supported yet")
    elseif(raoe_add_module_EXECUTABLE)
        set(raoe_alias_sufix "")
        project("${MODULE_LIBRARY_NAME}-exe" LANGUAGES CXX)
        add_executable(${PROJECT_NAME}
            ${raoe_add_module_CPP_SOURCE_FILES}
        )

        raoe_add_module_configure_module()
    elseif()
        message(FATAL_ERROR "raoe_add_module called without a valid module mode (must be STATIC, SHARED, MODULE, HEADERONLY, EXECUTABLE)")
    endif()

    if(DEFINED raoe_add_module_TARGET_VARIABLE)
        set(${raoe_add_module_TARGET_VARIABLE} ${PROJECT_NAME})
    endif()

    # if this module wants to copy out a directory post build, set up those commands here
    if(DEFINED raoe_add_module_COPY_DIRECTORY)
        foreach(raoe_add_module_ITEM IN ITEMS ${raoe_add_module_COPY_DIRECTORY})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -e copy_directory
                ${CMAKE_SOURCE_DIR}/${raoe_add_module_ITEM}
                $<TARGET_FILE_DIR:${PROJECT_NAME}>${raoe_add_module_ITEM}
            )
        endforeach()
    endif()

    # if this module has a third_party directory, include it now
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/CMakeLists.txt" AND NOT INCLUDED_THIRD_PARTY_IN_THIS_FILE)
        set(INCLUDED_THIRD_PARTY_IN_THIS_FILE TRUE)
        add_subdirectory("third_party")
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt" AND NOT INCLUDED_TEST_IN_THIS_FILE)
        set(INCLUDED_TEST_IN_THIS_FILE TRUE)
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

    CPMAddPackage("gh:catchorg/Catch2@3.3.2")

    raoe_add_module(
        EXECUTABLE
        NAME ${raoe_add_test_NAME}
        NAMESPACE "raoe::test"
        TARGET_VARIABLE ${TARGET_VARIABLE}
        CPP_SOURCE_FILES
            ${raoe_add_test_CPP_SOURCE_FILES}
        INCLUDE_DIRECTORIES
            ${raoe_add_test_INCLUDE_DIRECTORIES}
        COMPILE_DEFINITIONS
            ${raoe_add_test_COMPILE_DEFINITIONS}
        DEPENDENCIES
            PUBLIC
            Catch2::Catch2WithMain
        ${raoe_add_test_DEPENDENCIES}
    )
endmacro()