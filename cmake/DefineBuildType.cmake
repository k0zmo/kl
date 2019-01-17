function(__define_build_type_impl _name)
    if(CMAKE_CONFIGURATION_TYPES)
        list(APPEND CMAKE_CONFIGURATION_TYPES ${_name})
        list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" 
            CACHE STRING "List of supported configuration types" FORCE)
    else()
        get_property(build_type_strings CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS)
        list(LENGTH build_type_strings build_type_strings_length)
        if(NOT build_type_strings_length EQUAL 0)
            list(APPEND build_type_strings ${_name})
            list(REMOVE_DUPLICATES build_type_strings)
            set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${build_type_strings})
        endif()
    endif()
endfunction()

function(define_build_type _name)
    string(TOUPPER ${_name} uc_name)

    set(options "")
    set(one_value_args 
        BASE
        COMPILER_FLAGS # Both C and CXX flags
        LINKER_FLAGS # EXE, SHARED and MODULE flags
        C_FLAGS
        CXX_FLAGS
        STATIC_LINKER_FLAGS
        EXE_LINKER_FLAGS
        SHARED_LINKER_FLAGS
        MODULE_LINKER_FLAGS
        RC_FLAGS
    )
    set(multi_value_args "")
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(ARG_BASE)
        string(TOUPPER ${ARG_BASE} uc_base)
        set(ARG_BASE "_${uc_base}")
    endif()

    if(ARG_COMPILER_FLAGS)
        set(ARG_C_FLAGS ${ARG_COMPILER_FLAGS})
        set(ARG_CXX_FLAGS ${ARG_COMPILER_FLAGS})
    endif()

    if(ARG_LINKER_FLAGS)
        set(ARG_EXE_LINKER_FLAGS ${ARG_LINKER_FLAGS})
        set(ARG_SHARED_LINKER_FLAGS ${ARG_LINKER_FLAGS})
        set(ARG_MODULE_LINKER_FLAGS ${ARG_LINKER_FLAGS})
    endif()

    foreach(flags IN ITEMS C_FLAGS
                           CXX_FLAGS
                           RC_FLAGS
                           STATIC_LINKER_FLAGS
                           EXE_LINKER_FLAGS
                           SHARED_LINKER_FLAGS
                           MODULE_LINKER_FLAGS)
        string(TOLOWER ${flags} lc_flags)
        if(ARG_BASE)
            set(${lc_flags} "${CMAKE_${flags}${ARG_BASE}} ${ARG_${flags}}")
        else()
            set(${lc_flags} "${ARG_${flags}}")
        endif()
    endforeach()

    # Don't define C flags if it's C++-only project
    if(DEFINED CMAKE_C_FLAGS${ARG_BASE}) 
        mark_as_advanced(CMAKE_C_FLAGS_${uc_name})
        set(CMAKE_C_FLAGS_${uc_name} "${c_flags}"
            CACHE STRING "Flags used by the C compiler during ${uc_name} builds." FORCE)
    endif()

    # Don't define CXX flags if it's C-only project
    if(DEFINED CMAKE_CXX_FLAGS${ARG_BASE})
        mark_as_advanced(CMAKE_CXX_FLAGS_${uc_name})
        set(CMAKE_CXX_FLAGS_${uc_name} "${cxx_flags}"
            CACHE STRING "Flags used by the CXX compiler during ${uc_name} builds." FORCE)
    endif()

    # Don't define RC flags if we're building on anything other than WIN32
    if(DEFINED CMAKE_RC_FLAGS${ARG_BASE})
        mark_as_advanced(CMAKE_RC_FLAGS_${uc_name})
        set(CMAKE_RC_FLAGS_${uc_name} "${rc_flags}"
            CACHE STRING "Flags for Windows Resource Compiler during ${uc_name} builds." FORCE)
    endif()

    set(CMAKE_STATIC_LINKER_FLAGS_${uc_name} "${static_linker_flags}"
        CACHE STRING "Flags used by the linker during the creation of static libraries during ${uc_name} builds." FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_${uc_name} "${exe_linker_flags}"
        CACHE STRING "Flags used by the linker during ${uc_name} builds." FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_${uc_name} "${shared_linker_flags}"
        CACHE STRING "Flags used by the linker during the creation of shared libraries during ${uc_name} builds." FORCE)
    set(CMAKE_MODULE_LINKER_FLAGS_${uc_name} "${module_linker_flags}"
        CACHE STRING "Flags used by the linker during the creation of modules during ${uc_name} builds." FORCE)

    mark_as_advanced(
        CMAKE_STATIC_LINKER_FLAGS_${uc_name}
        CMAKE_EXE_LINKER_FLAGS_${uc_name}
        CMAKE_SHARED_LINKER_FLAGS_${uc_name}
        CMAKE_MODULE_LINKER_FLAGS_${uc_name}
    )

    __define_build_type_impl(${_name})
endfunction()
