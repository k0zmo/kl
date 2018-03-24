cmake_minimum_required(VERSION 3.6)
project(KL CXX)

# Set CMake modules path
list(APPEND CMAKE_MODULE_PATH "${KL_SOURCE_DIR}/cmake")

# Determine if kl is build as a subproject (using add_subdirectory)
# or if it is the master project.
set(MASTER_PROJECT OFF)
if(KL_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MASTER_PROJECT ON)

    # Set a default build type if none was specified
    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        message(STATUS "Setting build type to 'Release' as none was specified.")
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                     "Debug;Release;MinSizeRel;RelWithDebInfo")
    endif()

    # Generate JSON Compilation Database
    set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
    # Don't use any extensions (GNU or MSVC)
    set(CMAKE_CXX_EXTENSIONS OFF)

    include(SetupSanitizers)
endif()

include(kl)

# Build tests in the master project by default
option(KL_TEST "Generate the test target." ${MASTER_PROJECT})

find_package(Boost 1.61.0 REQUIRED COMPONENTS)

set(KL_3RDPARTY_JSON11_FILES
    3rdparty/json11/json11.hpp
    3rdparty/json11/json11.cpp)
set(KL_3RDPARTY_GSL_FILES
    3rdparty/gsl/gsl
    3rdparty/gsl/gsl_algorithm
    3rdparty/gsl/gsl_assert
    3rdparty/gsl/gsl_byte
    3rdparty/gsl/gsl_util
    3rdparty/gsl/multi_span
    3rdparty/gsl/span
    3rdparty/gsl/string_span)
set(KL_3RDPARTY_FILES
    ${KL_3RDPARTY_JSON11_FILES}
    ${KL_3RDPARTY_GSL_FILES})

set(KL_FILES
    src/kl/base_range.hpp
    src/kl/base64.hpp
    src/kl/base64.cpp
    src/kl/binary_rw.hpp
    src/kl/byte.hpp
    src/kl/ctti.hpp
    src/kl/defer.hpp
    src/kl/enum_range.hpp
    src/kl/enum_reflector.hpp
    src/kl/enum_traits.hpp
    src/kl/file_view.hpp
    src/kl/file_view.cpp
    src/kl/json_convert.hpp
    src/kl/json_print.hpp
    src/kl/match.hpp
    src/kl/meta.hpp
    src/kl/signal.hpp
    src/kl/split.hpp
    src/kl/stream_join.hpp
    src/kl/string_switch.hpp
    src/kl/tuple.hpp
    src/kl/type_traits.hpp
    src/kl/type_class.hpp
    src/kl/zip.hpp)

set(KL_BINARY_RW_FILES
    src/kl/binary_rw/endian.hpp
    src/kl/binary_rw/map.hpp
    src/kl/binary_rw/optional.hpp
    src/kl/binary_rw/pair.hpp
    src/kl/binary_rw/set.hpp
    src/kl/binary_rw/string.hpp
    src/kl/binary_rw/variant.hpp
    src/kl/binary_rw/vector.hpp)

source_group(3rdparty\\gsl FILES ${KL_3RDPARTY_GSL_FILES})
source_group(3rdparty\\json11 FILES ${KL_3RDPARTY_JSON11_FILES})
source_group(kl FILES ${KL_FILES})
source_group(kl\\binary_rw FILES ${KL_BINARY_RW_FILES})

add_library(kl
            ${KL_3RDPARTY_FILES}
            ${KL_FILES}
            ${KL_BINARY_RW_FILES})
target_include_directories(kl PUBLIC
                           src
                           3rdparty)
target_link_libraries(kl
                      PUBLIC Boost::boost
                      PRIVATE Boost::disable_autolinking)
target_kl_compile_options(kl)

if(KL_TEST)
    set(KL_3RDPARTY_CATCH_FILES
        3rdparty/catch/catch.hpp)
    set(KL_TEST_FILES
        tests/base_range_test.cpp
        tests/base64_test.cpp
        tests/binary_rw_test.cpp
        tests/byte_test.cpp
        tests/ctti_test.cpp
        tests/defer_test.cpp
        tests/enum_range_test.cpp
        tests/enum_reflector_test.cpp
        tests/enum_traits_test.cpp
        tests/file_view_test.cpp
        tests/json_convert_test.cpp
        tests/json_print_test.cpp
        tests/main_test.cpp
        tests/match_test.cpp
        tests/meta_test.cpp
        tests/signal_test.cpp
        tests/split_test.cpp
        tests/stream_join_test.cpp
        tests/string_switch_test.cpp
        tests/tuple_test.cpp
        tests/type_traits_test.cpp
        tests/type_class_test.cpp
        tests/zip_test.cpp)

    source_group(3rdparty\\catch FILES ${KL_3RDPARTY_CATCH_FILES})
    source_group(kl FILES ${KL_TEST_FILES})

    add_executable(kl-tests
                   ${KL_3RDPARTY_CATCH_FILES}
                   ${KL_TEST_FILES})
    target_link_libraries(kl-tests kl)
    target_kl_compile_options(kl-tests)

    # Add kl-coverage target for calculating test coverage
    include(SetupCoverage)
    if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
        setup_coverage_lcov(kl-coverage kl-tests coverage
                            FILTERS '${KL_SOURCE_DIR}/src/*')
    endif()
endif()
