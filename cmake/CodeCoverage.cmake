# CodeCoverage
#
#   add_coverage_target_lcov(<target_name>
#       COMMAND <runner>
#       [OUTPUT_NAME <output_name>]
#       [BRANCHES]
#       [FILTERS <filter>...]
#       [ARGS <arg>...]
#   )
#
#   add_coverage_target_gcovr(<target_name>
#       COMMAND <runner>
#       [OUTPUT_NAME <output_name>]
#       [XML]
#       [FILTERS <filter>...]
#       [EXCLUDE <filter>...]
#       [ARGS <arg>...]
#   )
#
# Example:
#   # Executable under the coverage test
#   add_executable(sample-tests all-tests.cpp)
#   # ...
#   include(CodeCoverage)
#   if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
#       add_coverage_target_lcov(sample-coverage
#           COMMAND sample-tests # Or ${CMAKE_CTEST_EXECUTABLE}
#           OUTPUT_NAME coverage
#           BRANCHES
#           FILTERS
#               '${CMAKE_SOURCE_DIR}/lib/*'
#               '${CMAKE_SOURCE_DIR}/src/*'
#       )
#   endif()

# This only works for GCC or Clang
if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND
   NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    message(STATUS "No coverage build type for compiler: ${CMAKE_CXX_COMPILER_ID}")
    return()
endif()

include(DefineBuildType)
define_build_type(Coverage
    BASE Debug
    COMPILER_FLAGS "--coverage"
)

function(add_coverage_target_lcov _target)
    find_program(LCOV_EXECUTABLE lcov)
    mark_as_advanced(LCOV_EXECUTABLE)
    if(NOT LCOV_EXECUTABLE)
        message(FATAL_ERROR "lcov not found!")
    endif()

    find_program(GENHTML_EXECUTABLE genhtml)
    mark_as_advanced(GENHTML_EXECUTABLE)
    if(NOT GENHTML_EXECUTABLE)
        message(FATAL_ERROR "genhtml not found!")
    endif()

    set(options BRANCHES)
    set(one_value_args COMMAND OUTPUT_NAME)
    set(multi_value_args FILTERS ARGS)
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT ARG_COMMAND)
        message(FATAL_ERROR "No COMMAND specified in add_coverage_target_lcov(${_target})")
    endif()

    set(output_name ${_target})
    if(ARG_OUTPUT_NAME)
        set(output_name ${ARG_OUTPUT_NAME})
    endif()

    if(ARG_BRANCHES)
        set(branch_coverage --rc lcov_branch_coverage=1)
    endif()

    if(NOT ARG_FILTERS)
        set(ARG_FILTERS '*')
    endif()

    list(APPEND args COMMAND ${LCOV_EXECUTABLE} -q --zerocounters --directory .)
    list(APPEND args COMMAND ${ARG_COMMAND} ${ARG_ARGS})
    list(APPEND args COMMAND
        ${LCOV_EXECUTABLE} 
        -q
        --capture
        --directory .
        --output-file ${output_name}.all.info
        ${branch_coverage}
    )
    list(APPEND args COMMAND
        ${LCOV_EXECUTABLE}
        -q
        --extract ${output_name}.all.info
        ${ARG_FILTERS}
        --output-file ${output_name}.info
        ${branch_coverage}
    )
    list(APPEND args COMMAND
        ${GENHTML_EXECUTABLE}
        -q
        --output-directory ${output_name}
        ${output_name}.info
        ${branch_coverage}
    )
    list(APPEND args COMMAND ${CMAKE_COMMAND} -E remove ${output_name}.all.info)

    add_custom_target(${_target}
        ${args}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Gathering and processing code coverage counters to generate HTML report"
    )
    add_custom_command(TARGET ${_target}
        POST_BUILD
        COMMAND ;
        COMMENT "Open ./${output_name}/index.html to see coverage report"
    )
endfunction()

function(add_coverage_target_gcovr _target)
    find_program(GCOVR_EXECUTABLE gcovr)
    mark_as_advanced(GCOVR_EXECUTABLE)
    if(NOT GCOVR_EXECUTABLE)
        message(FATAL_ERROR "gcovr not found!")
    endif()

    set(options XML)
    set(one_value_args COMMAND OUTPUT_NAME)
    set(multi_value_args FILTERS EXCLUDE ARGS)
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT ARG_COMMAND)
        message(FATAL_ERROR "No COMMAND specified in add_coverage_target_gcovr(${_target})")
    endif()

    set(output_name ${_target})
    if(ARG_OUTPUT_NAME)
        set(output_name ${ARG_OUTPUT_NAME})
    endif()

    if(ARG_FILTERS)
        set(tmp_list "")
        foreach(_loop ${ARG_FILTERS})
            list(APPEND tmp_list --filter=${_loop})
        endforeach()
        set(ARG_FILTERS ${tmp_list})
    endif()

    if(ARG_EXCLUDE)
        set(tmp_list "")
        foreach(_loop ${ARG_EXCLUDE})
            list(APPEND tmp_list --exclude=${_loop})
        endforeach()
        set(ARG_EXCLUDE ${tmp_list})
    endif()

    if(ARG_XML)
        set(output_mode --xml)
        set(output_file ${output_name}.xml)
    else()
        set(output_mode --html --html-details)
        set(output_file ${output_name}.html)
    endif()

    list(APPEND args COMMAND ${ARG_COMMAND} ${ARG_ARGS})
    list(APPEND args COMMAND 
        ${GCOVR_EXECUTABLE} 
        --delete ${output_mode}
        --root=${CMAKE_SOURCE_DIR}
        ${ARG_FILTERS}
        ${ARG_EXCLUDE}
        --output=${output_file}
    )

    add_custom_target(${_target}
        ${args}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running gcovr to produce HTML report"
    )
    add_custom_command(TARGET ${_target} POST_BUILD
        COMMAND ;
        COMMENT "Open ./${output_file} to see coverage report"
    )
endfunction()
