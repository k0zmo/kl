cmake_minimum_required(VERSION 3.11)
project(kl LANGUAGES CXX VERSION 0.1)

# Set CMake modules path
list(APPEND CMAKE_MODULE_PATH "${kl_SOURCE_DIR}/cmake")

include(CMakeDependentOption)
include(GNUInstallDirs)

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

    include(kl/Sanitizers)
endif()

find_package(Boost 1.61.0 REQUIRED COMPONENTS)
find_package(yaml-cpp 0.6 CONFIG REQUIRED)
find_package(RapidJSON 1.1.0 CONFIG REQUIRED)
find_package(GSL 2.0.0 CONFIG REQUIRED)

add_subdirectory(external)
add_subdirectory(src)
if(KL_TEST)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation stuff
set(config_file ${CMAKE_CURRENT_BINARY_DIR}/klConfig.cmake)
set(config_version_file ${CMAKE_CURRENT_BINARY_DIR}/klConfigVersion.cmake)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(${config_version_file}
    VERSION ${kl_VERSION}
    COMPATIBILITY SameMinorVersion
)

file(WRITE ${config_file}
"include(CMakeFindDependencyMacro)
find_dependency(Boost 1.61.0 COMPONENTS)
find_dependency(yaml-cpp 0.6 CONFIG)
find_dependency(RapidJSON 1.1.0 CONFIG)
find_dependency(GSL 2.0.0 CONFIG)
if(NOT TARGET kl::kl)
  include(\"\${CMAKE_CURRENT_LIST_DIR}/klTarget.cmake\")
endif()
")

install(FILES ${config_file} ${config_version_file}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kl
)

install(EXPORT klTarget
    NAMESPACE   kl::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kl
    FILE        klTarget.cmake
)
