cmake_minimum_required(VERSION 3.9)
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

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    include(SetupSanitizers)
endif()

# Build tests in the master project by default
option(KL_TEST "Generate the test target." ${MASTER_PROJECT})

include(kl)

find_package(Boost 1.61.0 REQUIRED COMPONENTS)

add_subdirectory(3rdparty)
add_subdirectory(src)
if(KL_TEST)
  add_subdirectory(tests)
endif()
