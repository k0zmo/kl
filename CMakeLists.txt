cmake_minimum_required(VERSION 3.16...3.24)
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

set(default_fetch_dependencies ON)
if(DEFINED VCPKG_TOOLCHAIN OR DEFINED CONAN_HOST_PROFILE)
    set(default_fetch_dependencies OFF)
endif()

cmake_dependent_option(KL_TEST "Generate the test target." ON
    "${kl_master_project}" OFF)
cmake_dependent_option(KL_BENCH "Generate the benchmark target." OFF
    "${kl_master_project};KL_TEST" OFF)
cmake_dependent_option(KL_DEV_BUILD "Enable compiler flags useful while developing kl." ON
    "${kl_master_project}" OFF)
cmake_dependent_option(KL_USE_OPENCPPCOVERAGE "Use OpenCppCoverage to calculate code coverage." OFF
    "${kl_master_project}" OFF)
cmake_dependent_option(KL_DISABLE_CCACHE "Disable auto-detection of ccache." OFF
    "${kl_master_project}" OFF)
option(KL_ENABLE_JSON "Enable kl-json target and fetch its dependencies (RapidJSON)." ON)
option(KL_ENABLE_YAML "Enable kl-yaml target and fetch its dependencies (yaml-cpp)." ON)
option(KL_FETCH_DEPENDENCIES "Fetch required dependencies (barring Boost) using CMake's FetchContent."
       ${default_fetch_dependencies})

if(kl_master_project)
    if(NOT KL_DISABLE_CCACHE)
        find_program(ccache_executable ccache)
        if(ccache_executable)
            message(STATUS "Using ccache from '${ccache_executable}'")
            set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)
            set(CMAKE_CXX_COMPILER_LAUNCHER ${ccache_executable})
        endif()
    endif()

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    # Don't use any extensions (GNU or MSVC)
    set(CMAKE_CXX_EXTENSIONS OFF)

    if(NOT KL_FETCH_DEPENDENCIES)
        set(kl_install_rules TRUE)
    endif()
endif()
include(SourceGroup)

find_package(Boost 1.61.0 REQUIRED)

if(NOT KL_FETCH_DEPENDENCIES)
    find_package(Microsoft.GSL REQUIRED)
    if(KL_ENABLE_JSON)
        find_package(RapidJSON REQUIRED)
    endif()
    if(KL_ENABLE_YAML)
        find_package(yaml-cpp 0.7 REQUIRED)
    endif()
    if(KL_TEST)
        find_package(Catch2 3.0 REQUIRED)
    endif()
else()
    add_subdirectory(external)
endif()

add_subdirectory(src)
if(KL_TEST)
    enable_testing()
    add_subdirectory(tests)
endif()
if(KL_BENCH)
    add_subdirectory(bench)
endif()

if(kl_install_rules)
    include(CMakePackageConfigHelpers)
    configure_package_config_file(klConfig.cmake.in
        klConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kl
        NO_SET_AND_CHECK_MACRO
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/klConfigVersion.cmake
        VERSION ${kl_VERSION}
        COMPATIBILITY SameMinorVersion
    )

    include(GNUInstallDirs)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/klConfig.cmake
                  ${CMAKE_CURRENT_BINARY_DIR}/klConfigVersion.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kl
    )
    install(EXPORT klTarget
        NAMESPACE   kl::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kl
        FILE        klTarget.cmake
    )
endif()
