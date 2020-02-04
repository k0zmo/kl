cmake_minimum_required(VERSION 3.11)
project(kl LANGUAGES CXX VERSION 0.1)

# Set CMake modules path
set(CMAKE_MODULE_PATH "${kl_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

include(CMakeDependentOption)

# Determine if kl is build as a subproject (using add_subdirectory)
# or if it is the master project.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(kl_master_project ON)
else()
    set(kl_master_project OFF)
endif()

cmake_dependent_option(KL_TEST "Generate the test target." ON
    "${kl_master_project}" OFF)
cmake_dependent_option(KL_DEV_BUILD "Enable compiler flags useful while developing kl." ON
    "${kl_master_project}" OFF)
cmake_dependent_option(KL_USE_OPENCPPCOVERAGE "Use OpenCppCoverage to calculate code coverage." OFF
    "${kl_master_project}" OFF)

if(kl_master_project)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    # Don't use any extensions (GNU or MSVC)
    set(CMAKE_CXX_EXTENSIONS OFF)

    include(Sanitizers)
endif()
include(SourceGroup)

find_package(Boost 1.61.0 REQUIRED COMPONENTS)

add_subdirectory(external)
add_subdirectory(src)
if(KL_TEST)
    enable_testing()
    add_subdirectory(tests)
endif()
