cmake_minimum_required(VERSION 3.9)
project(kl LANGUAGES CXX VERSION 0.1)

# Set CMake modules path
list(APPEND CMAKE_MODULE_PATH "${kl_SOURCE_DIR}/cmake")
include(kl)

# Determine if kl is build as a subproject (using add_subdirectory)
# or if it is the master project.
set(master_project OFF)
if(kl_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(master_project ON)
endif()

# Build tests in the master project by default
option(KL_TEST "Generate the test target." ${master_project})

if(master_project)
    # Generate JSON Compilation Database
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    # Don't use any extensions (GNU or MSVC)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    include(SetupSanitizers)
endif()

find_package(Boost 1.61.0 REQUIRED COMPONENTS)

add_subdirectory(3rdparty)
add_subdirectory(src)
if(KL_TEST)
    enable_testing()
    add_subdirectory(test)
endif()

# Installation stuff
set(config_file ${CMAKE_CURRENT_BINARY_DIR}/klConfig.cmake)
set(config_version_file ${CMAKE_CURRENT_BINARY_DIR}/klConfigVersion.cmake)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(${config_version_file}
    VERSION ${kl_VERSION}
    COMPATIBILITY AnyNewerVersion
)

file(WRITE ${config_file}
"include(CMakeFindDependencyMacro)
find_dependency(Boost 1.61.0 REQUIRED COMPONENTS)
if(NOT TARGET kl::kl)
  include(\"\${CMAKE_CURRENT_LIST_DIR}/klTarget.cmake\")
endif()
")

install(FILES ${config_file} ${config_version_file}
    DESTINATION lib/cmake/kl
)

install(EXPORT klTarget
    NAMESPACE kl::
    DESTINATION lib/cmake/kl
    FILE klTarget.cmake
)
