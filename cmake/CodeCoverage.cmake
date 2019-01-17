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
#                        [EXCLUDE <filter>...]
#                        [ARGS <arg>...]
#                        )
# Example:
#   include(SetupCoverage)
#   if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
#       setup_coverage_lcov(${CMAKE_PROJECT_NAME}-coverage
#                           ${CMAKE_PROJECT_NAME}-tests
#                           coverage
#                           BRANCHES
#                           FILTERS '${CMAKE_SOURCE_DIR}/lib/*' '${CMAKE_SOURCE_DIR}/src/*')
#   endif()

# This only works for GCC or Clang
if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND
   NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    message(STATUS "No coverage target for compiler: ${CMAKE_CXX_COMPILER_ID}")
    return()
endif()

include(DefineBuildType)
define_build_type(Coverage
    BASE Debug
    COMPILER_FLAGS "--coverage"
)

include(FindPackageHandleStandardArgs)

function(setup_coverage_lcov _target_name _test_runner _output_name)
    find_program(LCOV_EXECUTABLE lcov)
    find_program(GENHTML_EXECUTABLE genhtml)
    find_package_handle_standard_args(lcov DEFAULT_MSG LCOV_EXECUTABLE GENHTML_EXECUTABLE)
    mark_as_advanced(LCOV_EXECUTABLE GENHTML_EXECUTABLE)
    if(NOT LCOV_FOUND)
        message(FATAL_ERROR "lcov not found!")
    endif()

    set(options BRANCHES)
    set(one_value_args "")
    set(multi_value_args FILTERS ARGS)
    cmake_parse_arguments(options "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
    if(options_BRANCHES)
        set(branch_coverage --rc lcov_branch_coverage=1)
    endif()
    if(NOT options_FILTERS)
        set(options_FILTERS '*')
    endif()

    set(coverage_args)
    list(APPEND coverage_args COMMAND ${LCOV_EXECUTABLE}
         -q --zerocounters --directory .)
    list(APPEND coverage_args COMMAND ${_test_runner}
         ${options_ARGS})
    list(APPEND coverage_args COMMAND ${LCOV_EXECUTABLE}
         -q --capture --directory . --output-file ${_output_name}.all.info ${branch_coverage})
    list(APPEND coverage_args COMMAND ${LCOV_EXECUTABLE}
         -q --extract ${_output_name}.all.info ${options_FILTERS} --output-file ${_output_name}.info  ${branch_coverage})
    list(APPEND coverage_args COMMAND ${GENHTML_EXECUTABLE}
         -q --output-directory ${_output_name} ${_output_name}.info  ${branch_coverage})
    list(APPEND coverage_args COMMAND ${CMAKE_COMMAND}
         -E remove ${_output_name}.all.info)

    add_custom_target(${_target_name} ${coverage_args}
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
    if(NOT GCOVR_FOUND)
        message(FATAL_ERROR "gcovr not found!")
    endif()

    set(options XML)
    set(one_value_args "")
    set(multi_value_args FILTERS EXCLUDE ARGS)
    cmake_parse_arguments(options "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
    if(options_FILTERS)
        set(tmp_list "")
        foreach(_loop ${options_FILTERS})
            list(APPEND tmp_list --filter=${_loop})
        endforeach()
        set(options_FILTERS ${tmp_list})
    endif()
    if(options_EXCLUDE)
        set(tmp_list "")
        foreach(_loop ${options_EXCLUDE})
            list(APPEND tmp_list --exclude=${_loop})
        endforeach()
        set(options_EXCLUDE ${tmp_list})
    endif()
    if(options_XML)
        set(output_mode --xml)
        set(output_file ${_output_name}.xml)
    else()
        set(output_mode --html --html-details)
        set(output_file ${_output_name}.html)
    endif()

    set(coverage_args)
    list(APPEND coverage_args COMMAND ${_test_runner}
         ${options_ARGS})
    list(APPEND coverage_args COMMAND ${GCOVR_EXECUTABLE}
         --delete ${output_mode} --root=${CMAKE_SOURCE_DIR}
         ${options_FILTERS} ${options_EXCLUDE} --output=${output_file})

    add_custom_target(${_target_name} ${coverage_args}
                      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                      COMMENT "Running gcovr to produce HTML report")
    add_custom_command(TARGET ${_target_name} POST_BUILD
                       COMMAND ;
                       COMMENT "Open ./${output_file} to see coverage report")
endfunction()
