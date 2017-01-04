cmake_minimum_required(VERSION 3.1.2)
project(KL CXX)

if(POLICY CMP0054)
    cmake_policy(SET CMP0054 NEW)
endif()

# Set CMake modules path
set(KL_CMAKE_DIR "${KL_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH ${KL_CMAKE_DIR} ${CMAKE_MODULE_PATH})

# Determine if kl is build as a subproject (using add_subdirectory)
# or if it is the master project.
set(MASTER_PROJECT OFF)
if(KL_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MASTER_PROJECT ON)
endif()

# Build tests in the master project by default
option(KL_TEST "Generate the test target." ${MASTER_PROJECT})

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 "Debug;Release;MinSizeRel;RelWithDebInfo")
endif()

# Configure Boost library
if(MSVC)
    set(Boost_USE_STATIC_LIBS ON)
endif()
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.58.0 REQUIRED)
include_directories(SYSTEM "${Boost_INCLUDE_DIRS}")

if(WIN32)
    add_definitions("-DNOMINMAX")
endif()

if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        add_compile_options("/W4")
    endif()

    add_compile_options("/wd4996") # Deprecated stuff
    add_compile_options("/wd4127") # conditional expression in constant
    add_compile_options("/wd4512") # assignment operator could not be generated
    add_compile_options("/wd4510") # default constructor could not be generated
    add_compile_options("/wd4610") # ... can never be instantiated - user defined constructor required
    add_compile_options("/wd4100") # unreferenced formal parameter

    if(CMAKE_CL_64)
        # Visual Studio bails out on debug builds in 64bit mode unless this flag is set...
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /bigobj")
    endif()

elseif((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR
       (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    # Generate JSON Compilation Database
    set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
    # Turn on standard set of warnings
    add_compile_options("-Wall")
    add_compile_options("-pedantic")
    add_compile_options("-Wextra")
    # That's a bit too aggressive
    add_compile_options("-Wno-unused-parameter")

    # Activate C++14 or bail-out
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-std=c++14" COMPILER_SUPPORTS_CXX14)
    check_cxx_compiler_flag("-std=c++1y" COMPILER_SUPPORTS_CXX1Y)
    if(COMPILER_SUPPORTS_CXX14)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    elseif(COMPILER_SUPPORTS_CXX1Y)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
    else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support.\
                        Please use a different C++ compiler.")
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_definitions("-D_GLIBCXX_DEBUG")
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options("-Wno-missing-braces")
        add_compile_options("-Wno-unused-const-variable")
        #add_compile_options("-stdlib=libc++")
        #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++ -lc++abi")
        #set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lc++ -lc++abi")
    endif()

    include(SetupSanitizers)
endif()

set(KL_3RDPARTY_JSON11_FILES
    3rdparty/json11/json11.hpp
    3rdparty/json11/json11.cpp
)
set(KL_3RDPARTY_GSL_FILES
    3rdparty/gsl/gsl.h
    3rdparty/gsl/gsl_assert.h
    3rdparty/gsl/gsl_util.h
    3rdparty/gsl/span.h
    3rdparty/gsl/string_span.h
)
set(KL_3RDPARTY_FILES
    ${KL_3RDPARTY_JSON11_FILES}
    ${KL_3RDPARTY_GSL_FILES}
)

set(KL_FILES
    src/kl/base_range.hpp
    src/kl/base64.hpp
    src/kl/base64.cpp
    src/kl/byte.hpp
    src/kl/ctti.hpp
    src/kl/defer.hpp
    src/kl/enum_range.hpp
    src/kl/enum_reflector.hpp
    src/kl/enum_traits.hpp
    src/kl/index_sequence.hpp
    src/kl/json_convert.hpp
    src/kl/json_print.hpp
    src/kl/match.hpp
    src/kl/signal.hpp
    src/kl/split.hpp
    src/kl/stream_join.hpp
    src/kl/string_switch.hpp
    src/kl/tuple.hpp
    src/kl/type_traits.hpp
    src/kl/type_class.hpp
    src/kl/zip.hpp
)

source_group("external\\gsl" FILES ${KL_3RDPARTY_GSL_FILES})
source_group("external\\json11" FILES ${KL_3RDPARTY_JSON11_FILES})
source_group("kl" FILES ${KL_FILES})

add_library(kl STATIC
    ${KL_3RDPARTY_FILES}
    ${KL_FILES}
)
target_include_directories(kl PUBLIC src)
target_include_directories(kl PUBLIC 3rdparty)

if(KL_TEST)
    set(KL_3RDPARTY_CATCH_FILES
        3rdparty/catch/catch.hpp
    )
    set(KL_TEST_FILES
        tests/base_range_test.cpp
        tests/base64_test.cpp
        tests/byte_test.cpp
        tests/ctti_test.cpp
        tests/defer_test.cpp
        tests/enum_test.cpp
        tests/index_sequence_test.cpp
        tests/json_convert_test.cpp
        tests/json_print_test.cpp
        tests/main_test.cpp
        tests/match_test.cpp
        tests/signal_test.cpp
        tests/split_test.cpp
        tests/stream_join_test.cpp
        tests/string_switch_test.cpp
        tests/tuple_test.cpp
        tests/type_traits_test.cpp
        tests/type_class_test.cpp
        tests/zip_test.cpp
    )

    source_group("3rdparty\\catch" FILES ${KL_3RDPARTY_CATCH_FILES})
    source_group("kl" FILES ${KL_TEST_FILES})

    add_executable(kl-tests
        ${KL_3RDPARTY_CATCH_FILES}
        ${KL_TEST_FILES}
    )
    target_link_libraries(kl-tests kl)
    target_include_directories(kl-tests PRIVATE 3rdparty)

    include(SetupCoverage)
    if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
        setup_coverage_lcov(kl-coverage kl-tests coverage
                            FILTERS '${KL_SOURCE_DIR}/src/*')
    endif()
endif()
