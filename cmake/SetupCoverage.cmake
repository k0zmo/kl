# SetupCoverage
#
#   setup_coverage_lcov(<target_name>
#                       <runner>
#                       <output_name> 
#                       [BRANCHES]
#                       [FILTERS <filter>...]
#                       [ARGS <arg>...] 
#                       )
#
#
#   setup_coverage_gcovr(<target_name>
#                        <runner>
#                        <output_name> 
#                        [XML]
#                        [FILTERS <filter>...]
#                        [ARGS <arg>...] 
#                        )
# Example:
#   include(SetupCoverage)
#   if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
#       setup_coverage_lcov(${CMAKE_PROJECT_NAME}-coverage
#                           ${CMAKE_PROJECT_NAME}-tests
#                           coverage
#                           BRANCHES
#                           FILTERS '${CMAKE_SOURCE_DIR}/lib/*' '${CMAKE_SOURCE_DIR}/src/*')
#   endif()

# This only works for GCC or Clang
if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang") AND
    NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"))
    message(STATUS "No coverage target for compiler: ${CMAKE_CXX_COMPILER_ID}")
    return()
endif()

# Add 'Coverage' build type
set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES} Coverage)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
             "Debug" "Release" "MinSizeRel" "RelWithDebInfo" "Coverage")

# Set compiler/linker flags for coverage build type based on debug one
if (CMAKE_CXX_FLAGS_DEBUG)
    set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_DEBUG} --coverage"
        CACHE STRING "" FORCE)
    mark_as_advanced(CMAKE_CXX_FLAGS_COVERAGE)
endif()
if (CMAKE_C_FLAGS_DEBUG)
    set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_DEBUG} --coverage"
        CACHE STRING "" FORCE) 
    mark_as_advanced(CMAKE_C_FLAGS_COVERAGE)
endif()
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} --coverage"
    CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage"
    CACHE STRING "" FORCE)
mark_as_advanced(CMAKE_SHARED_LINKER_FLAGS_COVERAGE
                 CMAKE_EXE_LINKER_FLAGS_COVERAGE)

include(FindPackageHandleStandardArgs)

function(setup_coverage_lcov _target_name _test_runner _output_name)
    find_program(LCOV_EXECUTABLE lcov)
    find_program(GENHTML_EXECUTABLE genhtml)
    find_package_handle_standard_args(lcov DEFAULT_MSG LCOV_EXECUTABLE GENHTML_EXECUTABLE)
    mark_as_advanced(LCOV_EXECUTABLE GENHTML_EXECUTABLE)
    if (NOT LCOV_FOUND)
        message(FATAL_ERROR "lcov not found!")
    endif()

    set(_options BRANCHES)
    set(_one_value_args "")
    set(_multi_value_args FILTERS ARGS)
    cmake_parse_arguments(_options "${_options}" "${_one_value_args}" "${_multi_value_args}" ${ARGN})  
    if (_options_BRANCHES)
        set(_branch_coverage --rc lcov_branch_coverage=1)
    endif()
    if (NOT _options_FILTERS)
        set(_options_FILTERS '*')
    endif()

    set(_coverage_args)
    list(APPEND _coverage_args COMMAND ${LCOV_EXECUTABLE}
         -q --zerocounters --directory .)
    list(APPEND _coverage_args COMMAND ${_test_runner}
         ${_options_ARGS})
    list(APPEND _coverage_args COMMAND ${LCOV_EXECUTABLE}
         -q --capture --directory . --output-file ${_output_name}.all.info ${_branch_coverage})
    list(APPEND _coverage_args COMMAND ${LCOV_EXECUTABLE}
         -q --extract ${_output_name}.all.info ${_options_FILTERS} --output-file ${_output_name}.info  ${_branch_coverage})
    list(APPEND _coverage_args COMMAND ${GENHTML_EXECUTABLE}
         -q --output-directory ${_output_name} ${_output_name}.info  ${_branch_coverage})
    list(APPEND _coverage_args COMMAND ${CMAKE_COMMAND}
         -E remove ${_output_name}.info ${_output_name}.all.info)

    add_custom_target(${_target_name} ${_coverage_args}
                      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                      COMMENT "Gathering and processing code coverage counters to generate HTML report")
    add_custom_command(TARGET ${_target_name} POST_BUILD
                       COMMAND ;
                       COMMENT "Open ./${_output_name}/index.html to see coverage report")
endfunction()

function(setup_coverage_gcovr _target_name _test_runner _output_name)
    find_program(GCOVR_EXECUTABLE gcovr)
    find_package_handle_standard_args(gcovr DEFAULT_MSG GCOVR_EXECUTABLE)
    mark_as_advanced(GCOVR_EXECUTABLE)
    if (NOT GCOVR_FOUND)
        message(FATAL_ERROR "gcovr not found!")
    endif()
    
    set(_options XML)
    set(_one_value_args "")
    set(_multi_value_args FILTERS ARGS)
    cmake_parse_arguments(_options "${_options}" "${_one_value_args}" "${_multi_value_args}" ${ARGN})  
    if (_options_FILTERS)
        set(_tmp_list "")
        foreach(_loop ${_options_FILTERS})
            list(APPEND _tmp_list --filter=${_loop})
        endforeach()
        set(_options_FILTERS ${_tmp_list})
    endif()
    if (_options_XML)
        set(_output_mode --xml)
        set(_output_file ${_output_name}.xml)
    else()
        set(_output_mode --html --html-details)
        set(_output_file ${_output_name}.html)
    endif()

    set(_coverage_args)
    list(APPEND _coverage_args COMMAND ${_test_runner}
         ${_options_ARGS})
    list(APPEND _coverage_args COMMAND ${GCOVR_EXECUTABLE}
         --delete ${_output_mode} --root=${CMAKE_SOURCE_DIR} ${_options_FILTERS} --output=${_output_file})

    add_custom_target(${_target_name} ${_coverage_args}
                      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                      COMMENT "Running gcovr to produce HTML report")
    add_custom_command(TARGET ${_target_name} POST_BUILD
                       COMMAND ;
                       COMMENT "Open ./${_output_file} to see coverage report")
endfunction()
