macro(__list_to_string _list)
    string(REPLACE ";" " " ${_list} "${${_list}}")
endmacro()

__list_to_string(CXX_FLAGS)
__list_to_string(CXX_FLAGS_DEBUG)
__list_to_string(CXX_FLAGS_RELEASE)
__list_to_string(CXX_FLAGS_COVERAGE)
__list_to_string(CXX_FLAGS_ASAN)
__list_to_string(CXX_FLAGS_TSAN)
__list_to_string(CXX_FLAGS_MSAN)
__list_to_string(CXX_FLAGS_UBSAN)
__list_to_string(LINKER_FLAGS)
__list_to_string(LINKER_FLAGS_DEBUG)
__list_to_string(LINKER_FLAGS_RELEASE)
__list_to_string(LINKER_FLAGS_COVERAGE)
__list_to_string(LINKER_FLAGS_ASAN)
__list_to_string(LINKER_FLAGS_TSAN)
__list_to_string(LINKER_FLAGS_MSAN)
__list_to_string(LINKER_FLAGS_UBSAN)

if(CXX_FLAGS)
    set(CMAKE_CXX_FLAGS "${CXX_FLAGS}"
        CACHE STRING "Flags used by the CXX compiler during all build types.")
endif()
if (CXX_FLAGS_DEBUG)
    set(CMAKE_CXX_FLAGS_DEBUG "${CXX_FLAGS_DEBUG}"
        CACHE STRING "Flags used by the CXX compiler during DEBUG builds.")
endif()
if(CXX_FLAGS_RELEASE)
    set(CMAKE_CXX_FLAGS_RELEASE "${CXX_FLAGS_RELEASE}"
        CACHE STRING "Flags used by the CXX compiler during RELEASE builds.")
endif()
if(CXX_FLAGS_COVERAGE)
    set(CMAKE_CXX_FLAGS_COVERAGE "${CXX_FLAGS_COVERAGE}"
        CACHE STRING "Flags used by the CXX compiler during COVERAGE builds.")
endif()
if(CXX_FLAGS_ASAN)
    set(CMAKE_CXX_FLAGS_ASAN "${CXX_FLAGS_ASAN}"
        CACHE STRING "Flags used by the CXX compiler during ASAN builds.")
endif()
if(CXX_FLAGS_TSAN)
    set(CMAKE_CXX_FLAGS_TSAN "${CXX_FLAGS_TSAN}"
        CACHE STRING "Flags used by the CXX compiler during TSAN builds.")
endif()
if(CXX_FLAGS_MSAN)
    set(CMAKE_CXX_FLAGS_MSAN "${CXX_FLAGS_MSAN}"
        CACHE STRING "Flags used by the CXX compiler during MSAN builds.")
endif()
if(CXX_FLAGS_UBSAN)
    set(CMAKE_CXX_FLAGS_UBSAN "${CXX_FLAGS_UBSAN}"
        CACHE STRING "Flags used by the CXX compiler during UBSAN builds.")
endif()

foreach(_type IN ITEMS EXE MODULE SHARED)
    if(LINKER_FLAGS)
        set(CMAKE_${_type}_LINKER_FLAGS "${LINKER_FLAGS}"
            CACHE STRING "Flags used by the linker during all build types.")
    endif()
    if(LINKER_FLAGS_DEBUG)
        set(CMAKE_${_type}_LINKER_FLAGS_DEBUG "${LINKER_FLAGS_DEBUG}"
            CACHE STRING "Flags used by the linker during DEBUG builds.")
    endif()
    if(LINKER_FLAGS_RELEASE)
        set(CMAKE_${_type}_LINKER_FLAGS_RELEASE "${LINKER_FLAGS_RELEASE}"
            CACHE STRING "Flags used by the linker during RELEASE builds.")
    endif()
    if(LINKER_FLAGS_COVERAGE)
        set(CMAKE_${_type}_LINKER_FLAGS_COVERAGE "${LINKER_FLAGS_COVERAGE}"
            CACHE STRING "Flags used by the linker during COVERAGE builds.")
    endif()
    if(LINKER_FLAGS_ASAN)
        set(CMAKE_${_type}_LINKER_FLAGS_ASAN "${LINKER_FLAGS_ASAN}"
            CACHE STRING "Flags used by the linker during ASAN builds.")
    endif()
    if(LINKER_FLAGS_TSAN)
        set(CMAKE_${_type}_LINKER_FLAGS_TSAN "${LINKER_FLAGS_TSAN}"
            CACHE STRING "Flags used by the linker during TSAN builds.")
    endif()
    if(LINKER_FLAGS_MSAN)
        set(CMAKE_${_type}_LINKER_FLAGS_MSAN "${LINKER_FLAGS_MSAN}"
            CACHE STRING "Flags used by the linker during MSAN builds.")
    endif()
    if(LINKER_FLAGS_UBSAN)
        set(CMAKE_${_type}_LINKER_FLAGS_UBSAN "${LINKER_FLAGS_UBSAN}"
            CACHE STRING "Flags used by the linker during UBSAN builds.")
    endif()
endforeach()

unset(CXX_FLAGS)
unset(CXX_FLAGS_DEBUG)
unset(CXX_FLAGS_RELEASE)
unset(CXX_FLAGS_COVERAGE)
unset(CXX_FLAGS_ASAN)
unset(CXX_FLAGS_TSAN)
unset(CXX_FLAGS_MSAN)
unset(CXX_FLAGS_UBSAN)
unset(LINKER_FLAGS)
unset(LINKER_FLAGS_DEBUG)
unset(LINKER_FLAGS_RELEASE)
unset(LINKER_FLAGS_COVERAGE)
unset(LINKER_FLAGS_ASAN)
unset(LINKER_FLAGS_TSAN)
unset(LINKER_FLAGS_MSAN)
unset(LINKER_FLAGS_UBSAN)
