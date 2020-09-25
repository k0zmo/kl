add_executable(kl-tests main_test.cpp)
set(test_files
    base64_test.cpp
    binary_rw_test.cpp
    ctti_test.cpp
    defer_test.cpp
    enum_set_test.cpp
    enum_range_test.cpp
    enum_reflector_test.cpp
    enum_traits_test.cpp
    exception_info_test.cpp
    file_view_test.cpp
    hash_test.cpp
    match_test.cpp
    meta_test.cpp
    range_test.cpp
    reflect_enum_test.cpp
    reflect_struct_test.cpp
    signal_test.cpp
    split_test.cpp
    stream_join_test.cpp
    tuple_test.cpp
    type_traits_test.cpp
    utility_test.cpp
    zip_test.cpp
)
target_sources(kl-tests PRIVATE ${test_files} input/typedefs.hpp)
target_link_libraries(kl-tests PRIVATE
    kl::kl
    Catch2::Catch2
)

if(KL_ENABLE_JSON)
    target_sources(kl-tests PRIVATE json_test.cpp)
    target_link_libraries(kl-tests PRIVATE kl::json)
endif()
if(KL_ENABLE_YAML)
    target_sources(kl-tests PRIVATE yaml_test.cpp)
    target_link_libraries(kl-tests PRIVATE kl::yaml)
endif()

kl_source_group(kl-tests
    TREE ${CMAKE_CURRENT_SOURCE_DIR}
    PREFIX kl
)
set_target_properties(kl-tests PROPERTIES FOLDER kl.tests)

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
    kl_add_coverage_target_lcov(kl-coverage
        COMMAND kl-tests
        FILTERS
            '${kl_SOURCE_DIR}/src/*'
            '${kl_SOURCE_DIR}/include/*'
    )
    # kl_add_coverage_target_gcovr(kl-coverage
    #     COMMAND kl-tests
    #     FILTERS
    #        ${kl_SOURCE_DIR}/src/
    #        ${kl_SOURCE_DIR}/include/
    # )
elseif(KL_USE_OPENCPPCOVERAGE)
    kl_add_coverage_target_occ(kl-coverage
        RUNNER kl-tests
        FILTERS
            ${kl_SOURCE_DIR}/src
            ${kl_SOURCE_DIR}/include
    )
endif()

if(COMMAND target_precompile_headers)
    target_precompile_headers(kl-tests PRIVATE
        <boost/optional.hpp>
        <boost/variant.hpp>
        <istream>
        <ostream>
        <ios>
        <catch2/catch.hpp>
    )
    set_source_files_properties(main_test.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
endif()
