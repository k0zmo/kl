cmake_minimum_required(VERSION 3.11)

# Determine if kl is build as a subproject (using add_subdirectory)
# or if it is the master project.
set(master_project OFF)
if(NOT DEFINED PROJECT_NAME)
    set(master_project ON)
endif()

project(kl LANGUAGES CXX VERSION 0.1)

# Set CMake modules path
list(APPEND CMAKE_MODULE_PATH "${kl_SOURCE_DIR}/cmake")

include(CMakeDependentOption)

cmake_dependent_option(KL_TEST "Generate the test target." ON
    "${master_project}" OFF)
cmake_dependent_option(KL_DEV_BUILD "Enable compiler flags useful while developing kl." ON
    "${master_project}" OFF)
cmake_dependent_option(KL_USE_OPENCPPCOVERAGE "Use OpenCppCoverage to calculate code coverage." OFF
    "${master_project}" OFF)

if(master_project)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    # Don't use any extensions (GNU or MSVC)
    set(CMAKE_CXX_EXTENSIONS OFF)

    include(kl/Sanitizers)
endif()

find_package(Boost 1.61.0 REQUIRED COMPONENTS)

add_subdirectory(external)
add_subdirectory(src)
if(KL_TEST)
    enable_testing()
    add_subdirectory(tests)
endif()
