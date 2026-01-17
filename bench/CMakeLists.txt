add_executable(kl-bench
    enum_reflector_bench.cpp
    signal_bench.cpp
)
target_link_libraries(kl-bench PRIVATE kl::kl Catch2::Catch2WithMain)

if(KL_ENABLE_JSON)
    target_sources(kl-bench PRIVATE
        input/typedefs.hpp
        json_bench.cpp
    )
    target_link_libraries(kl-bench PRIVATE kl::json)
endif()

if(KL_ENABLE_YAML)
    target_sources(kl-bench PRIVATE
        input/typedefs.hpp
        yaml_bench.cpp
    )
    target_link_libraries(kl-bench PRIVATE kl::yaml)
endif()

kl_source_group(kl-bench TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX kl)
set_target_properties(kl-bench PROPERTIES FOLDER kl.tests)

if(COMMAND target_precompile_headers)
    target_precompile_headers(kl-bench PRIVATE
        <catch2/catch_test_macros.hpp>
        <catch2/benchmark/catch_benchmark.hpp>
    )
endif()
