add_executable(kl-test
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
    json_convert_test.cpp
    json_print_test.cpp
    main_test.cpp
    match_test.cpp
    meta_test.cpp
    signal_test.cpp
    split_test.cpp
    stream_join_test.cpp
    string_switch_test.cpp
    tuple_test.cpp
    type_traits_test.cpp
    type_class_test.cpp
    zip_test.cpp
)
target_link_libraries(kl-test PRIVATE
                      kl::kl Catch::catch)
kl_compile_options(kl-test)

get_target_property(sources_list kl-test SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
             PREFIX kl
             FILES ${sources_list})

# Add kl-coverage target for calculating test coverage
include(SetupCoverage)
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    setup_coverage_lcov(kl-coverage kl-test coverage
                        FILTERS 
                            '${kl_SOURCE_DIR}/src/*'
                            '${kl_SOURCE_DIR}/include/*')
endif()
