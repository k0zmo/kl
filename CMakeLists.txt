cmake_minimum_required(VERSION 3.1.2)
project(kl CXX)

if(POLICY CMP0054)
    cmake_policy(SET CMP0054 NEW)
endif()

include(CheckCXXCompilerFlag)

# Default to Release build if non provided
if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
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

elseif(("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang") OR 
       ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"))
    # Generate JSON Compilation Database
    set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE) 
    # Turn on standard set of warnings
    add_compile_options("-Wall")
    add_compile_options("-pedantic")
    add_compile_options("-Wextra")
    # That's a bit too aggressive
    add_compile_options("-Wno-unused-parameter")

    # Activate C++14 or bail-out
    CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
    CHECK_CXX_COMPILER_FLAG("-std=c++1y" COMPILER_SUPPORTS_CXX1Y)
    if(COMPILER_SUPPORTS_CXX14)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    elseif(COMPILER_SUPPORTS_CXX1Y)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
    else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support.\
                        Please use a different C++ compiler.")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options("-fcxx-exceptions")
        add_compile_options("-Wno-missing-braces")
        add_compile_options("-Wno-unused-const-variable")
    endif()
endif()

set(KL_3RDPARTY_GSL_FILES
    ${CMAKE_SOURCE_DIR}/3rdparty/gsl/gsl.h
    ${CMAKE_SOURCE_DIR}/3rdparty/gsl/gsl_assert.h
    ${CMAKE_SOURCE_DIR}/3rdparty/gsl/gsl_util.h
    ${CMAKE_SOURCE_DIR}/3rdparty/gsl/span.h
    ${CMAKE_SOURCE_DIR}/3rdparty/gsl/string_span.h
)
set(KL_3RDPARTY_FILES
    ${KL_3RDPARTY_GSL_FILES}
)

set(KL_FILES
    src/kl/base_range.hpp
    src/kl/defer.hpp
    src/kl/enum_range.hpp
    src/kl/enum_reflector.hpp
    src/kl/enum_traits.hpp
    src/kl/index_sequence.hpp
    src/kl/match.hpp
    src/kl/split.hpp
    src/kl/stream_join.hpp
    src/kl/string_switch.hpp
    src/kl/tuple.hpp
    src/kl/type_traits.hpp
    src/kl/zip.hpp
    src/kl/compile_all.cpp
)
source_group("kl" FILES ${KL_FILES})
source_group("3rdparty\\gsl" FILES ${KL_3RDPARTY_GSL_FILES})

add_library(kl STATIC
    ${KL_3RDPARTY_FILES}
    ${KL_FILES}
)
target_include_directories(kl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(KL_3RDPARTY_CATCH_FILES
    ${CMAKE_SOURCE_DIR}/3rdparty/catch/catch.hpp
)
set(KL_TEST_FILES
    tests/enum_test.cpp
    tests/split_test.cpp
    tests/stream_join_test.cpp
    tests/string_switch_test.cpp
    tests/main_test.cpp
    tests/match_test.cpp
    tests/zip_test.cpp
)

source_group("3rdparty\\catch" FILES ${KL_3RDPARTY_CATCH_FILES})
source_group("kl" FILES ${KL_TEST_FILES})

add_executable(kl_tests
    ${KL_3RDPARTY_CATCH_FILES}
    ${KL_TEST_FILES}
)
target_link_libraries(kl_tests kl)
target_include_directories(kl_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty")
