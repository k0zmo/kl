include(cotire)

function(target_precompile_header _target)
    set(enabled_option_name ${PROJECT_NAME}_USE_PRECOMPILED_HEADER)
    string(TOUPPER ${enabled_option_name} enabled_option_name)

    # Allow to disable the precompiled headers per-project
    if(DEFINED ${enabled_option_name} AND NOT ${enabled_option_name})
        message(STATUS "Precompiled header disabled for target ${_target}")
        return()
    endif()

    set(options GENERATE_HOST_FILE)
    set(one_value_args PREFIX_FILE)
    set(multi_value_args EXCLUDE)
    cmake_parse_arguments(arg "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(arg_PREFIX_FILE)
        if(arg_GENERATE_HOST_FILE AND MSVC)
            # Extra stuff for MSVC - host file for prefix header
            set(host_file ${CMAKE_CURRENT_BINARY_DIR}/${_target}_pch.cxx)
            get_filename_component(prefix_file_abs ${arg_PREFIX_FILE} REALPATH)
            file(GENERATE OUTPUT ${host_file} CONTENT "#include \"${prefix_file_abs}\"\n")
            target_sources(${_target} PRIVATE ${host_file})
            set_target_properties(${_target} PROPERTIES 
                COTIRE_CXX_HOST_FILE_INIT ${host_file}
            )
        endif()

        set_target_properties(${_target} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT ${arg_PREFIX_FILE})
    endif()

    foreach(source_file IN ITEMS ${arg_EXCLUDE})
        set_source_files_properties(${source_file} PROPERTIES COTIRE_EXCLUDED TRUE)
    endforeach()
    set_target_properties(${_target} PROPERTIES COTIRE_ADD_UNITY_BUILD OFF)
    cotire(${_target})    
endfunction()
