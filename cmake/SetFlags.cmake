macro(__list_to_string _list)
    string(REPLACE ";" " " ${_list} "${${_list}}")
endmacro()

__list_to_string(compile_options)
__list_to_string(compile_options_debug)
__list_to_string(compile_options_release)
__list_to_string(compile_options_coverage)
__list_to_string(compile_options_asan)
__list_to_string(compile_options_tsan)
__list_to_string(compile_options_msan)
__list_to_string(compile_options_ubsan)

__list_to_string(link_options)
__list_to_string(link_options_debug)
__list_to_string(link_options_release)
__list_to_string(link_options_coverage)
__list_to_string(link_options_asan)
__list_to_string(link_options_tsan)
__list_to_string(link_options_msan)
__list_to_string(link_options_ubsan)

if(compile_options)
    set(CMAKE_CXX_FLAGS "${compile_options}"
        CACHE STRING "Flags used by the CXX compiler during all build types.")
endif()
if (compile_options_debug)
    set(CMAKE_CXX_FLAGS_DEBUG "${compile_options_debug}"
        CACHE STRING "Flags used by the CXX compiler during DEBUG builds.")
endif()
if(compile_options_release)
    set(CMAKE_CXX_FLAGS_RELEASE "${compile_options_release}"
        CACHE STRING "Flags used by the CXX compiler during RELEASE builds.")
endif()
if(compile_options_coverage)
    set(CMAKE_CXX_FLAGS_COVERAGE "${compile_options_coverage}"
        CACHE STRING "Flags used by the CXX compiler during COVERAGE builds.")
endif()
if(compile_options_asan)
    set(CMAKE_CXX_FLAGS_ASAN "${compile_options_asan}"
        CACHE STRING "Flags used by the CXX compiler during ASAN builds.")
endif()
if(compile_options_tsan)
    set(CMAKE_CXX_FLAGS_TSAN "${compile_options_tsan}"
        CACHE STRING "Flags used by the CXX compiler during TSAN builds.")
endif()
if(compile_options_msan)
    set(CMAKE_CXX_FLAGS_MSAN "${compile_options_msan}"
        CACHE STRING "Flags used by the CXX compiler during MSAN builds.")
endif()
if(compile_options_ubsan)
    set(CMAKE_CXX_FLAGS_UBSAN "${compile_options_ubsan}"
        CACHE STRING "Flags used by the CXX compiler during UBSAN builds.")
endif()

foreach(_type IN ITEMS EXE MODULE SHARED)
    if(link_options)
        set(CMAKE_${_type}_LINKER_FLAGS "${link_options}"
            CACHE STRING "Flags used by the linker during all build types.")
    endif()
    if(link_options_debug)
        set(CMAKE_${_type}_LINKER_FLAGS_DEBUG "${link_options_debug}"
            CACHE STRING "Flags used by the linker during DEBUG builds.")
    endif()
    if(link_options_release)
        set(CMAKE_${_type}_LINKER_FLAGS_RELEASE "${link_options_release}"
            CACHE STRING "Flags used by the linker during RELEASE builds.")
    endif()
    if(link_options_coverage)
        set(CMAKE_${_type}_LINKER_FLAGS_COVERAGE "${link_options_coverage}"
            CACHE STRING "Flags used by the linker during COVERAGE builds.")
    endif()
    if(link_options_asan)
        set(CMAKE_${_type}_LINKER_FLAGS_ASAN "${link_options_asan}"
            CACHE STRING "Flags used by the linker during ASAN builds.")
    endif()
    if(link_options_tsan)
        set(CMAKE_${_type}_LINKER_FLAGS_TSAN "${link_options_tsan}"
            CACHE STRING "Flags used by the linker during TSAN builds.")
    endif()
    if(link_options_msan)
        set(CMAKE_${_type}_LINKER_FLAGS_MSAN "${link_options_msan}"
            CACHE STRING "Flags used by the linker during MSAN builds.")
    endif()
    if(link_options_ubsan)
        set(CMAKE_${_type}_LINKER_FLAGS_UBSAN "${link_options_ubsan}"
            CACHE STRING "Flags used by the linker during UBSAN builds.")
    endif()
endforeach()
