cmake_minimum_required(VERSION 3.26)
function(raoe_recursively_get_dependencies targets out_var)
    foreach (target IN ITEMS ${targets})
        if (TARGET ${target})
            list(APPEND all_deps ${target})
            get_target_property(deps ${target} LINK_LIBRARIES)
            foreach (dep IN ITEMS ${deps})
                if (TARGET ${dep})
                    list(APPEND all_deps ${dep})
                endif ()
            endforeach ()

            foreach (dep IN ITEMS ${deps})
                if (TARGET ${dep})
                    raoe_recursively_get_dependencies(${dep} dep_deps)
                    list(APPEND all_deps ${dep_deps})
                endif ()
            endforeach ()
        endif ()
    endforeach ()

    list(REMOVE_DUPLICATES all_deps)
    set(${out_var} ${all_deps} PARENT_SCOPE)
endfunction()

macro(raoe_add_module)
    set(options STATIC SHARED MODULE HEADERONLY EXECUTABLE)
    set(oneValueArgs NAME NAMESPACE CXX_STANDARD TARGET_VARIABLE PACK_DIRECTORY)
    set(multiValueArgs CPP_SOURCE_FILES INCLUDE_DIRECTORIES COMPILE_DEFINITIONS DEPENDENCIES SYMLINK_IN_DEV)

    cmake_parse_arguments(raoe_add_module "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT raoe_add_module_STATIC
        AND NOT raoe_add_module_SHARED
        AND NOT raoe_add_module_MODULE
        AND NOT raoe_add_module_HEADERONLY
        AND NOT raoe_add_module_EXECUTABLE)
        message(FATAL_ERROR "raoe_add_module called without a valid module mode (must be STATIC, SHARED, MODULE, HEADERONLY, EXECUTABLE)")
    endif ()

    if (NOT raoe_add_module_EXECUTABLE AND DEFINED raoe_add_module_PACK_DIRECTORY)
        message(FATAL_ERROR "Only executable modules can have a PACK_DIRECTORY")
    endif ()

    if (NOT raoe_add_module_CXX_STANDARD)
        if (NOT CMAKE_CXX_STANDARD)
            set(raoe_add_module_CXX_STANDARD 20)
        else ()
            set(raoe_add_module_CXX_STANDARD ${CMAKE_CXX_STANDARD})
        endif ()
    endif ()

    set(CMAKE_CXX_STANDARD ${raoe_add_module_CXX_STANDARD})

    # Validate the Input Arguments
    if (NOT raoe_add_module_NAME)
        message(FATAL_ERROR "This module must have a name")
    endif ()

    if (NOT raoe_add_module_NAMESPACE)
        set(raoe_add_module_NAMESPACE "raoe")
    endif ()


    string(REPLACE "::" "-" MODULE_LIBRARY_NAME "${raoe_add_module_NAMESPACE}-${raoe_add_module_NAME}")

    macro(raoe_add_module_configure_module)
        if (NOT raoe_add_module_HEADERONLY AND NOT raoe_add_module_MODULE)
            if (raoe_add_module_INCLUDE_DIRECTORIES)
                target_include_directories(${PROJECT_NAME}
                                           ${raoe_add_module_INCLUDE_DIRECTORIES}
                )
            endif ()
        endif ()

        if (NOT raoe_add_module_HEADERONLY)
            # add compile definitions.
            string(TOUPPER "${raoe_add_module_NAME}_API" module_api)
            string(REPLACE "-" "_" module_api ${raoe_add_module_NAME})
            target_compile_definitions(${PROJECT_NAME}
                                       PRIVATE

                                       # "${module_api}=1"
                                       # "RAOE_MODULE_NAME=${raoe_add_module_NAME}"
                                       ${raoe_add_module_COMPILE_DEFINITIONS}
            )
        endif ()

        if (NOT raoe_add_module_EXECUTABLE)
            set(raoe_add_module_alias "${raoe_add_module_NAMESPACE}::${raoe_add_module_NAME}${raoe_alias_sufix}")
            add_library("${raoe_add_module_alias}" ALIAS ${PROJECT_NAME})
        endif ()

        target_link_libraries(
                ${PROJECT_NAME}
                ${raoe_add_module_DEPENDENCIES}
        )
        set_target_properties(${PROJECT_NAME} PROPERTIES PACK_INFO "")
    endmacro()

    if (raoe_add_module_HEADERONLY)
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
    elseif (raoe_add_module_MODULE)
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

    elseif (raoe_add_module_STATIC)
        set(raoe_alias_sufix "")
        project("${MODULE_LIBRARY_NAME}-static" LANGUAGES CXX)

        add_library("${PROJECT_NAME}"
                    STATIC
                    ${raoe_add_module_CPP_SOURCE_FILES}
        )
        raoe_add_module_configure_module()
    elseif (raoe_add_module_SHARED)
        message(FATAL_ERROR "Shared modules are not supported yet")
    elseif (raoe_add_module_EXECUTABLE)
        set(raoe_alias_sufix "")
        project("${MODULE_LIBRARY_NAME}-exe" LANGUAGES CXX)
        add_executable(${PROJECT_NAME}
                       ${raoe_add_module_CPP_SOURCE_FILES}
        )

        if (DEFINED raoe_add_module_PACK_DIRECTORY)
            set(output_pack_dir "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${raoe_add_module_PACK_DIRECTORY}")
            # remove the pack directory to clean it up
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                               COMMAND ${CMAKE_COMMAND} -E remove_directory "${output_pack_dir}"
                               COMMENT "Cleaning up pack directory ${output_pack_dir}"
                               VERBATIM
            )
            #collect all the dependencies of this module, so that we can copy their asset packs to the output directory
            raoe_recursively_get_dependencies("${raoe_add_module_DEPENDENCIES}" raoe_add_module_ALL_DEPENDENCIES)
            # go through each of the dependencies and append their asset directories to a list
            if (DEFINED raoe_add_module_ZIP_ASSET_PACKS)
                list(APPEND raoe_add_module_ALL_PACK_DIRECTORIES ${raoe_add_module_ZIP_ASSET_PACKS})
            endif ()
            foreach (dep IN ITEMS ${raoe_add_module_ALL_DEPENDENCIES})
                get_target_property(pack_info ${dep} PACK_INFO)
                if (NOT pack_info)
                    continue ()
                endif ()
                STRING(REPLACE ";" "|" pack_info "${pack_info}")
                list(APPEND raoe_add_module_ALL_PACK_DIRECTORIES "${dep}.${pack_info}")
            endforeach ()
            # after building the executable, copy all the asset packs to the output directory

            foreach (pack_pair IN ITEMS ${raoe_add_module_ALL_PACK_DIRECTORIES})
                string(REPLACE "." ";" pack_pair_split ${pack_pair})
                list(GET pack_pair_split 0 pack_target)
                list(GET pack_pair_split 1 pack_rest)

                string(REPLACE "," ";" pack_pair_split ${pack_rest})
                list(GET pack_pair_split 0 pack_name)
                list(GET pack_pair_split 1 pack_dir)
                list(GET pack_pair_split 2 pack_options)

                string(REPLACE "[" ";" split_options "${pack_options}")
                foreach (option IN ITEMS ${split_options})
                    string(REPLACE "=" ";" option_split "${option}")
                    list(GET option_split 0 option_key)
                    list(GET option_split 1 option_value)
                    set(${option_key} ${option_value})
                endforeach ()


                if (SYMLINK_IN_DEV AND CMAKE_BUILD_TYPE STREQUAL "Debug")
                    # in dev mode, just symlink the directory
                    if (WIN32)
                        set(raoe_add_module_EXECUTE_STRING cmd /C mklink /J "${output_pack_dir}/${pack_name}" "${pack_dir}")
                    else ()
                        set(raoe_add_module_EXECUTE_STRING ln -sf "${pack_dir}" "${output_pack_dir}/${pack_name}")
                    endif ()
                    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                                       COMMAND ${CMAKE_COMMAND} -E make_directory "${output_pack_dir}"
                                       COMMAND ${raoe_add_module_EXECUTE_STRING}
                                       COMMENT "Symlinking asset pack ${pack_dir} to ${output_pack_dir}/${pack_name}"
                    )
                else ()
                    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                                       COMMAND ${CMAKE_COMMAND} -E make_directory "${output_pack_dir}"
                                       COMMAND ${CMAKE_COMMAND} -E tar "cf" "${output_pack_dir}/${pack_name}.zip" --format=zip -- "."
                                       WORKING_DIRECTORY "${pack_dir}"
                                       COMMENT "Extracting asset pack ${pack_dir}.zip to ${output_pack_dir}"
                                       VERBATIM
                    )
                endif ()


            endforeach ()
        endif ()

        raoe_add_module_configure_module()
    elseif ()
        message(FATAL_ERROR "raoe_add_module called without a valid module mode (must be STATIC, SHARED, MODULE, HEADERONLY, EXECUTABLE)")
    endif ()

    if (DEFINED raoe_add_module_TARGET_VARIABLE)
        set(${raoe_add_module_TARGET_VARIABLE} ${PROJECT_NAME})
    endif ()

    # if this module wants to copy out a directory post build, set up those commands here
    if (DEFINED raoe_add_module_SYMLINK_IN_DEV)
        foreach (raoe_add_module_ITEM IN ITEMS ${raoe_add_module_SYMLINK_IN_DEV})
            set(raoe_add_module_DIRLINK_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${raoe_add_module_ITEM}")
            set(raoe_add_module_DIRLINK_TARGET_PATH "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${raoe_add_module_ITEM}")

            if (WIN32)
                set(raoe_add_module_EXECUTE_STRING cmd /C mklink /J "${raoe_add_module_REAL_SRC_PATH}" "${raoe_add_module_DIRLINK_TARGET_PATH}")
            else ()
                set(raoe_add_module_EXECUTE_STRING ln -sf "${raoe_add_module_DIRLINK_SOURCE_PATH}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/")
            endif ()

            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                               COMMAND ${raoe_add_module_EXECUTE_STRING}
            )
        endforeach ()
    endif ()

    # if this module has a third_party directory, include it now
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/CMakeLists.txt" AND NOT INCLUDED_THIRD_PARTY_IN_THIS_FILE)
        set(INCLUDED_THIRD_PARTY_IN_THIS_FILE TRUE)
        add_subdirectory("third_party")
    endif ()

    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt" AND NOT INCLUDED_TEST_IN_THIS_FILE)
        set(INCLUDED_TEST_IN_THIS_FILE TRUE)
        add_subdirectory("test")
    endif ()
endmacro()

macro(raoe_add_test)
    set(options)
    set(oneValueArgs NAME TARGET_VARIABLE PACK_DIRECTORY)
    set(multiValueArgs CPP_SOURCE_FILES INCLUDE_DIRECTORIES COMPILE_DEFINITIONS DEPENDENCIES)

    cmake_parse_arguments(raoe_add_test "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT raoe_add_test_NAME)
        message(FATAL_ERROR "This module must have a name")
    endif ()

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
            PACK_DIRECTORY
            ${raoe_add_test_PACK_DIRECTORY}
    )
endmacro()

macro(raoe_add_asset_pack)
    set(options PACK_IN_DEV)
    set(oneValueArgs NAME DIRECTORY)
    set(multiValueArgs)
    cmake_parse_arguments(raoe_add_asset_pack "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT raoe_add_asset_pack_NAME)
        message(FATAL_ERROR "This asset pack must have a name")
    endif ()

    if (NOT raoe_add_asset_pack_DIRECTORY)
        set(raoe_add_asset_pack_DIRECTORY "assets/")
    endif ()

    set(raoe_add_asset_pack_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${raoe_add_asset_pack_DIRECTORY}/${raoe_add_asset_pack_NAME}")

    if (NOT EXISTS "${raoe_add_asset_pack_DIRECTORY}/${raoe_add_asset_pack_NAME}.toml")
        message(FATAL_ERROR "The provided path for the asset pack (${raoe_add_asset_pack_DIRECTORY}/${raoe_add_asset_pack_NAME}.toml) is invalid or does not contain a ${raoe_add_asset_pack_NAME}.toml file")
    endif ()

    message(STATUS "Current Project: ${PROJECT_NAME}")

    # get the pack name list from the target properties of the module this pack is attached to
    get_target_property(existing_pack_info ${PROJECT_NAME} PACK_INFO)
    set(SYMLINK_IN_DEV "$<IF:$<BOOL:${raoe_add_asset_pack_PACK_IN_DEV}>,TRUE,FALSE>")
    list(APPEND existing_pack_info " ${raoe_add_asset_pack_NAME},${raoe_add_asset_pack_DIRECTORY},SYMLINK_IN_DEV=${SYMLINK_IN_DEV}")

    # set the pack names property on the module target
    set_target_properties(${PROJECT_NAME} PROPERTIES
                          PACK_INFO "${existing_pack_info}"
                          ASSET_PACKS "${existing_pack_dirs}"
    )
endmacro()

function(raoe_process_packs)
    function(_get_all_cmake_targets out_var current_dir)
        get_property(targets DIRECTORY ${current_dir} PROPERTY BUILDSYSTEM_TARGETS)
        get_property(subdirs DIRECTORY ${current_dir} PROPERTY SUBDIRECTORIES)

        foreach (subdir ${subdirs})
            _get_all_cmake_targets(subdir_targets ${subdir})
            list(APPEND targets ${subdir_targets})
        endforeach ()

        set(${out_var} ${targets} PARENT_SCOPE)
    endfunction()

    # Run at end of top-level CMakeLists
    _get_all_cmake_targets(all_targets ${CMAKE_SOURCE_DIR})
    message(STATUS "Test all_targets:${all_targets}")

    foreach (target IN ITEMS ${all_targets})
        get_target_property(PACK_DIRECTORY ${target} PACK_DIRECTORY)
        if (PACK_DIRECTORY)
            message(STATUS "Target ${target} has PACK_DIRECTORY: ${PACK_DIRECTORY}")
            add_custom_command(TARGET ${target} POST_BUILD
                               COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target}>/${PACK_DIRECTORY}"
                               COMMENT "Creating pack directory for ${target}"
                               VERBATIM
            )

        endif ()
    endforeach ()

endfunction()