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
#   include(kl/CodeCoverage)
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
    cmake_parse_arguments(arg "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT arg_COMMAND)
        message(FATAL_ERROR "No COMMAND specified in add_coverage_target_lcov(${_target})")
    endif()

    set(output_name ${_target})
    if(arg_OUTPUT_NAME)
        set(output_name ${arg_OUTPUT_NAME})
    endif()

    if(arg_BRANCHES)
        set(branch_coverage --rc lcov_branch_coverage=1)
    endif()

    if(NOT arg_FILTERS)
        set(arg_FILTERS '*')
    endif()

    list(APPEND cmd COMMAND ${LCOV_EXECUTABLE} -q --zerocounters --directory .)
    list(APPEND cmd COMMAND ${arg_COMMAND} ${arg_ARGS})
    list(APPEND cmd COMMAND
        ${LCOV_EXECUTABLE}
        -q
        --capture
        --directory .
        --output-file ${output_name}.all.info
        ${branch_coverage}
    )
    list(APPEND cmd COMMAND
        ${LCOV_EXECUTABLE}
        -q
        --extract ${output_name}.all.info
        ${arg_FILTERS}
        --output-file ${output_name}.info
        ${branch_coverage}
    )
    list(APPEND cmd COMMAND
        ${GENHTML_EXECUTABLE}
        -q
        --output-directory ${output_name}
        ${output_name}.info
        ${branch_coverage}
    )
    list(APPEND cmd COMMAND ${CMAKE_COMMAND} -E remove ${output_name}.all.info)

    add_custom_target(${_target}
        ${cmd}
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

    set(options XML LLVM)
    set(one_value_args COMMAND OUTPUT_NAME)
    set(multi_value_args FILTERS EXCLUDE ARGS)
    cmake_parse_arguments(arg "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT arg_COMMAND)
        message(FATAL_ERROR "No COMMAND specified in add_coverage_target_gcovr(${_target})")
    endif()

    set(output_name ${_target})
    if(arg_OUTPUT_NAME)
        set(output_name ${arg_OUTPUT_NAME})
    endif()

    if(arg_FILTERS)
        set(tmp_list "")
        foreach(_loop ${arg_FILTERS})
            list(APPEND tmp_list --filter=${_loop})
        endforeach()
        set(arg_FILTERS ${tmp_list})
    endif()

    if(arg_EXCLUDE)
        set(tmp_list "")
        foreach(_loop ${arg_EXCLUDE})
            list(APPEND tmp_list --exclude=${_loop})
        endforeach()
        set(arg_EXCLUDE ${tmp_list})
    endif()

    if(arg_XML)
        set(output_mode --xml)
        set(output_file ${output_name}.xml)
        set(report_type XML)
    else()
        set(output_mode --html --html-details)
        set(output_file ${output_name}.html)
        set(report_type HTML)
    endif()

    if(arg_LLVM)
 	    set(gcov_executable "--gcov-executable=\"llvm-cov\" gcov")
 	endif()

    list(APPEND cmd COMMAND ${arg_COMMAND} ${arg_ARGS})
    list(APPEND cmd COMMAND
        ${GCOVR_EXECUTABLE}
        --delete ${output_mode}
        --root=${CMAKE_SOURCE_DIR}
        ${arg_FILTERS}
        ${arg_EXCLUDE}
        --output=${output_file}
        ${gcov_executable}
    )

    add_custom_target(${_target}
        ${cmd}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running gcovr to produce ${report_type} report"
    )
    add_custom_command(TARGET ${_target} POST_BUILD
        COMMAND ;
        COMMENT "Open ./${output_file} to see coverage report"
    )
endfunction()

function(add_coverage_target_occ _target)
    find_program(OPEN_CPP_COVERAGE_EXECUTABLE OpenCppCoverage)
    mark_as_advanced(OPEN_CPP_COVERAGE_EXECUTABLE)
    if(NOT OPEN_CPP_COVERAGE_EXECUTABLE)
        message(FATAL_ERROR "OpenCppCoverage not found!")
    endif()

    set(options COBERTURA)
    set(one_value_args RUNNER OUTPUT_NAME)
    set(multi_value_args FILTERS EXCLUDE)
    cmake_parse_arguments(arg "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT arg_RUNNER)
        message(FATAL_ERROR "No RUNNER specified in add_coverage_target_occ(${_target})")
    endif()

    if(arg_FILTERS)
        set(sources)
        foreach(_loop IN ITEMS ${arg_FILTERS})
            # OpenCppCoverage requires backslashes
            file(TO_NATIVE_PATH ${_loop} filter_path)
            list(APPEND sources --sources \"${filter_path}\")
        endforeach()
    endif()

    if(arg_EXCLUDE)
        set(excluded)
        foreach(_loop IN ITEMS ${arg_EXCLUDE})
            # OpenCppCoverage requires backslashes
            file(TO_NATIVE_PATH ${_loop} filter_path)
            list(APPEND excluded --excluded_sources \"${filter_path}\")
        endforeach()
    endif()

    set(output_name ${_target})
    if(arg_OUTPUT_NAME)
        set(output_name ${arg_OUTPUT_NAME})
    endif()

    if(arg_COBERTURA)
        set(export_type "--export_type=cobertura:${output_name}.xml")
    else()
        set(export_type "--export_type=html:${output_name}")
    endif()

    add_custom_target(${_target}
        COMMAND ${OPEN_CPP_COVERAGE_EXECUTABLE} ${sources} ${excluded} ${export_type} -- $<TARGET_FILE:${arg_RUNNER}>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endfunction()

# Define 'Coverage' build type for Clang and GCC
if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND
   NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    message(STATUS "No coverage build type for compiler: ${CMAKE_CXX_COMPILER_ID}")
else()
    include(kl/DefineBuildType)
    define_build_type(Coverage
        BASE Debug
        COMPILER_FLAGS "--coverage"
    )
endif()
