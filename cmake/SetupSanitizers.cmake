# This only works for GCC or Clang
if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang") AND
    NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"))
    message(STATUS "No sanitize targets for compiler: ${CMAKE_CXX_COMPILER_ID}")
    return()
endif()

function(setup_sanitize_target _name _c_flags _linker_flags)
    string(TOUPPER ${_name} _name_uppercase)

    if(CMAKE_CXX_FLAGS_DEBUG)
        # No point to set it if it's C-only project
        set(CMAKE_CXX_FLAGS_${_name_uppercase} ${_c_flags}
            CACHE STRING "Flags used by the C++ compiler during ${_name} builds." FORCE)
    endif()
    if(CMAKE_C_FLAGS_DEBUG)
        # No point to set it if it's C++-only project
        set(CMAKE_C_FLAGS_${_name_uppercase} ${_c_flags}
            CACHE STRING "Flags used by the C compiler during ${_name} builds." FORCE)
    endif()

    set(CMAKE_SHARED_LINKER_FLAGS_${_name_uppercase} ${_linker_flags}
        CACHE STRING "Flags used by the shared libraries linker during ${_name} builds." FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_${_name_uppercase} ${_linker_flags}
        CACHE STRING "Flags used for linking binaries during ${_name} builds." FORCE)

    mark_as_advanced(CMAKE_CXX_FLAGS_${_name_uppercase}
                     CMAKE_C_FLAGS_${_name_uppercase},
                     CMAKE_SHARED_LINKER_FLAGS_${_name_uppercase},
                     CMAKE_EXE_LINKER_FLAGS_${_name_uppercase})

    if(CMAKE_CONFIGURATION_TYPES)
        # Add '${_name}' build type for multi-config generators
        list(APPEND CMAKE_CONFIGURATION_TYPES ${_name})
        list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
        set(CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" 
            CACHE STRING "List of supported configuration types" FORCE)
    else()
        # Modify CMAKE_BUILD_TYPE enum values for cmake-gui
        get_property(_build_type_strings CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS)
        list(LENGTH _build_type_strings _build_type_strings_length)
        if(NOT _build_type_strings_length EQUAL 0)
            list(APPEND _build_type_strings ${_name})
            list(REMOVE_DUPLICATES _build_type_strings)
            set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${_build_type_strings})
        endif()
    endif()
endfunction()

setup_sanitize_target("ASan"  
                      "-O1 -g -fsanitize=address -fno-omit-frame-pointer -DNDEBUG"
                      "-fsanitize=address")
setup_sanitize_target("TSan"  
                      "-O1 -g -fsanitize=thread -DNDEBUG" 
                      "-fsanitize=thread")
setup_sanitize_target("UBSan" 
                      "-O1 -g -fsanitize=undefined -fno-omit-frame-pointer -DNDEBUG" 
                      "-fsanitize=undefined")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    setup_sanitize_target("MSan"  
                          "-O2 -g -fsanitize=memory -fno-omit-frame-pointer -DNDEBUG"
                          "-fsanitize=memory")
endif()
