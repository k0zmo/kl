add_executable(kl-tests main_test.cpp)
set(test_files
    base_range_test.cpp
    base64_test.cpp
    binary_rw_test.cpp
    byte_test.cpp
    ctti_test.cpp
    defer_test.cpp
    enum_flags_test.cpp
    enum_range_test.cpp
    enum_reflector_test.cpp
    enum_traits_test.cpp
    file_view_test.cpp
    hash_test.cpp
    json_convert_test.cpp
    json_print_test.cpp
    match_test.cpp
    meta_test.cpp
    signal_test.cpp
    split_test.cpp
    stream_join_test.cpp
    tuple_test.cpp
    type_traits_test.cpp
    type_class_test.cpp
    utility_test.cpp
    zip_test.cpp
)
target_sources(kl-tests PRIVATE ${test_files})
target_link_libraries(kl-tests PRIVATE
                      kl::kl Catch2::Catch2)

get_target_property(sources_list kl-tests SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
             PREFIX kl
             FILES ${sources_list})

foreach(test_file ${test_files})
    get_filename_component(no_ext ${test_file} NAME_WE)
    if(no_ext MATCHES "_test")
        add_test(NAME ${no_ext} COMMAND kl-tests -r compact -\# ~[.] [\#${no_ext}])
        set_tests_properties(${no_ext} PROPERTIES FAIL_REGULAR_EXPRESSION "No tests ran.")
    endif()
endforeach()

# Add kl-coverage target for calculating test coverage
include(CodeCoverage)
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    add_coverage_target_lcov(kl-coverage
        COMMAND kl-tests
        FILTERS
            '${kl_SOURCE_DIR}/src/*'
            '${kl_SOURCE_DIR}/include/*'
    )
    # add_coverage_target_gcovr(kl-coverage
    #     COMMAND kl-tests
    #     FILTERS
    #        ${kl_SOURCE_DIR}/src/
    #        ${kl_SOURCE_DIR}/include/
    # )
endif()

# if(MSVC)
#     add_coverage_target_occ(kl-coverage
#         RUNNER kl-tests
#         FILTERS
#             ${kl_SOURCE_DIR}/src
#             ${kl_SOURCE_DIR}/include
#     )
# endif()

include(PrecompileHeader)
target_precompile_header(kl-tests
    PREFIX_FILE pch.hpp
    EXCLUDE main_test.cpp
)
