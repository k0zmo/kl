include(FetchContent)

set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL ON)

FetchContent_Declare(GSL
    URL https://github.com/microsoft/GSL/archive/v4.2.0.zip
    URL_HASH SHA256=f694e7b0f3debc0c55fe138efb5dd9f5c402989c0dc04e8a74afc9c155cb66b9
)
FetchContent_Declare(rapidjson
    URL https://github.com/Tencent/rapidjson/archive/0d4517f15a8d7167ba9ae67f3f22a559ca841e3b.zip
    URL_HASH SHA256=4f44adc689e7c08e9eb197276600e58c501624b54102cba06267814e2a4f66eb
)
FetchContent_Declare(yamlcpp
    URL https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.9.0.zip
    URL_HASH SHA256=1c22709eb1fcde200c87ef4e878ce2c9477cc05eae84ebf1f72ec5b356468fee
)
FetchContent_Declare(Catch2
    URL https://github.com/catchorg/Catch2/archive/v3.14.0.zip
    URL_HASH SHA256=33c9dfd4de6979fe127d5c43eb883818994f723938ce5aec0945410c8d6ebea2
)

FetchContent_GetProperties(GSL)
if(NOT gsl_POPULATED)
    FetchContent_Populate(GSL)
    add_subdirectory(${gsl_SOURCE_DIR} ${gsl_BINARY_DIR})
endif()

if(KL_ENABLE_JSON)
    FetchContent_GetProperties(rapidjson)
    if(NOT rapidjson_POPULATED)
        FetchContent_Populate(rapidjson)
        # No point in add_subdirectory() - there's no target defined
        add_library(rapidjson INTERFACE)
        add_library(rapidjson::rapidjson ALIAS rapidjson)
        target_include_directories(rapidjson SYSTEM INTERFACE ${rapidjson_SOURCE_DIR}/include)
    endif()
endif()

if(KL_ENABLE_YAML)
    FetchContent_GetProperties(yamlcpp)
    if(NOT yamlcpp_POPULATED)
        FetchContent_Populate(yamlcpp)

        add_library(yaml-cpp)
        add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
        file(GLOB_RECURSE sources
            "${yamlcpp_SOURCE_DIR}/src/*.cpp"
            "${yamlcpp_SOURCE_DIR}/src/*.h"
            "${yamlcpp_SOURCE_DIR}/include/yaml-cpp/*.h")
        target_sources(yaml-cpp PRIVATE ${sources})
        target_include_directories(yaml-cpp PUBLIC ${yamlcpp_SOURCE_DIR}/include)
        target_compile_features(yaml-cpp PUBLIC cxx_std_11)
        if(NOT BUILD_SHARED_LIBS)
            target_compile_definitions(yaml-cpp PUBLIC YAML_CPP_STATIC_DEFINE)
        endif()
        if(MSVC)
            target_compile_options(yaml-cpp PRIVATE
                /wd4100 /wd4127 /wd4244 /wd4456 /wd4702 /wd4996
            )
        endif()
        set_target_properties(yaml-cpp PROPERTIES FOLDER external)
        if(COMMAND target_precompile_headers)
            target_precompile_headers(yaml-cpp PRIVATE
                <string>
                <ostream>
                <istream>
                <ios>
                <map>
                <vector>
            )
        endif()
    endif()
endif()

if(KL_TEST)
    FetchContent_GetProperties(Catch2)
    if(NOT catch2_POPULATED) # Variable names are lowercased
        FetchContent_Populate(Catch2)
        add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
        set_target_properties(Catch2 Catch2WithMain PROPERTIES FOLDER external)
        # Needed so Catch2 is built with support for std::string_view and
        # std::byte to std::string converters
        target_compile_features(Catch2 PRIVATE cxx_std_17)
        if(COMMAND target_precompile_headers)
            target_precompile_headers(Catch2 PRIVATE <catch2/catch_all.hpp>)
        endif()
    endif()
endif()
