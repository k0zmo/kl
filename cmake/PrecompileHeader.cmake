function(__filter_source_files_pch _target _excluded _out)
    get_target_property(source_files ${_target} SOURCES)
    foreach(source_file IN ITEMS ${source_files})
        get_source_file_property(is_header ${source_file} HEADER_FILE_ONLY)
        get_source_file_property(is_external ${source_file} EXTERNAL_OBJECT)
        get_source_file_property(is_symbolic ${source_file} SYMBOLIC)
        if(NOT is_header AND NOT is_external AND NOT is_symbolic)
            set(is_excluded OFF)
            foreach(excluded_source_file IN ITEMS ${_excluded})
                if(excluded_source_file STREQUAL source_file)
                    set(is_excluded ON)
                    break()
                endif()
            endforeach()
            if(NOT is_excluded)
                list(APPEND out ${source_file})
            endif()
        endif()
    endforeach()
    set(${_out} ${out} PARENT_SCOPE)
endfunction()

function(target_precompile_header _target)
    set(enabled_option_name ${PROJECT_NAME}_USE_PRECOMPILED_HEADER)
    string(TOUPPER ${enabled_option_name} enabled_option_name)

    # Allow to disable the precompiled headers per-project
    if(DEFINED ${enabled_option_name} AND NOT ${enabled_option_name})
        message(STATUS "Precompiled header disabled for target ${_target}")
        return()
    endif()

    set(options "")
    set(one_value_args PREFIX_FILE)
    set(multi_value_args EXCLUDE)
    cmake_parse_arguments(arg "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT arg_PREFIX_FILE)
        message(FATAL_ERROR "No PREFIX_FILE specified in target_precompile_header(${_target})")
    endif()

    if(MSVC)
        set(host_file ${CMAKE_CURRENT_BINARY_DIR}/${_target}_pch.cpp)
        set(pch_file ${CMAKE_CURRENT_BINARY_DIR}/${_target}_pch.pch)
        get_filename_component(prefix_file ${arg_PREFIX_FILE} REALPATH)

        file(TO_NATIVE_PATH ${host_file} host_file_native)
        file(TO_NATIVE_PATH ${pch_file} pch_file_native)
        file(TO_NATIVE_PATH ${prefix_file} prefix_file_native)

        set(host_file_flags
            "/Yc${prefix_file_native}"
            "/Fp${pch_file_native}"
            /Zm170
        )
        set(source_file_flags
            "/Yu${prefix_file_native}"
            "/Fp${pch_file_native}"
            "/FI${prefix_file_native}"
            /Zm170
        )

        __filter_source_files_pch(${_target} "${arg_EXCLUDE}" source_files)
        foreach(source_file IN ITEMS ${source_files})
            set_property(SOURCE ${source_file}
                APPEND_STRING PROPERTY COMPILE_OPTIONS ${source_file_flags}
            )
            # OBJECT_DEPENDS is needed for Ninja generator, MSBuild simply ignores it
            set_property(SOURCE ${source_file}
                APPEND PROPERTY OBJECT_DEPENDS ${pch_file}
            )
        endforeach()

        # Generate a host file (e.g stdafx.cpp), add it as a source to the given target
        # and set proper compile flags
        file(GENERATE OUTPUT ${host_file} CONTENT "#include \"${prefix_file}\"\n")
        target_sources(${_target} PRIVATE ${host_file})
        set_source_files_properties(${host_file} PROPERTIES
            COMPILE_OPTIONS "${host_file_flags}"
            OBJECT_OUTPUTS ${pch_file}
            OBJECT_DEPENDS ${prefix_file}
        )
    elseif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(host_file ${CMAKE_CURRENT_BINARY_DIR}/${_target}_pch.hpp)
        set(gch_file ${CMAKE_CURRENT_BINARY_DIR}/${_target}_pch.hpp.gch)
        get_filename_component(prefix_file ${arg_PREFIX_FILE} REALPATH)

        file(TO_NATIVE_PATH ${host_file} host_file_native)

        # We dont need a host file in case of GCC/Clang but if the prefix file
        # contains #pragma once then we avoid getting a warning of having this in 'main file'
        file(GENERATE OUTPUT ${host_file} CONTENT "#include \"${prefix_file}\"\n")

        # Add target to compile the prefix header with exactly the same properties as the source target
        add_library(${_target}.pch OBJECT ${host_file})
        set_source_files_properties(${host_file} PROPERTIES
            COMPILE_OPTIONS "-x;c++-header"
            LANGUAGE CXX
        )

        set(properties
            INTERFACE_COMPILE_DEFINITIONS
            INTERFACE_COMPILE_FEATURES
            INTERFACE_COMPILE_OPTIONS
            INTERFACE_INCLUDE_DIRECTORIES
            INTERFACE_LINK_LIBRARIES
            INTERFACE_POSITION_INDEPENDENT_CODE
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
            COMPILE_DEFINITIONS
            COMPILE_FEATURES
            COMPILE_OPTIONS
            INCLUDE_DIRECTORIES
            LINK_LIBRARIES
            POSITION_INDEPENDENT_CODE
            SYSTEM_INCLUDE_DIRECTORIES)
        foreach(property IN ITEMS ${properties})
            get_target_property(var ${_target} ${property})
            if(var)
                set_target_properties(${_target}.pch PROPERTIES ${property} "${var}")
            endif()
        endforeach()

        # # Generate a symlink from a ${host_file}.obj to gch_file
        add_custom_command(OUTPUT ${gch_file}
            COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_OBJECTS:${_target}.pch> ${gch_file}
        )
        add_custom_target(${_target}.pch-symlink DEPENDS ${gch_file})
        add_dependencies(${_target}.pch-symlink ${_target}.pch)

        # Finally, append proper compile flags for each not-excluded source file
        __filter_source_files_pch(${_target} "${arg_EXCLUDE}" source_files)
        foreach(source_file IN ITEMS ${source_files})
            set_property(SOURCE ${source_file}
                APPEND_STRING PROPERTY COMPILE_OPTIONS "-include;${host_file_native};-Winvalid-pch"
            )
            set_property(SOURCE ${source_file}
                APPEND PROPERTY OBJECT_DEPENDS ${prefix_file}
            )
        endforeach()
        add_dependencies(${_target} ${_target}.pch-symlink)
    endif()
endfunction()
