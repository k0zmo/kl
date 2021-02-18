include(FetchContent)

set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL ON)

FetchContent_Declare(GSL
    URL https://github.com/microsoft/GSL/archive/v3.0.0.zip
    URL_HASH SHA256=ed2d506839fd917142d562fa720459ce58c38fc5711b715779093e895a87d8c7
)
FetchContent_Declare(rapidjson
    URL https://github.com/Tencent/rapidjson/archive/13dfc96c9c2b104be7b0b09a9f6e06871ed3e81d.zip
    URL_HASH SHA256=477ff9f53c437dabcb01f3f47d7f26f126a94c3cc6b5e012d1e01e034e030041
)
FetchContent_Declare(yamlcpp
    URL https://github.com/jbeder/yaml-cpp/archive/0f9a586ca1dc29c2ecb8dd715a315b93e3f40f79.zip
    URL_HASH SHA256=ff55e0cc373295b8503faf52a5e9569b950d8ec3e704508a62fe9159c37185bc
)
FetchContent_Declare(Catch2
    URL https://github.com/catchorg/Catch2/archive/v2.9.1.zip
    URL_HASH SHA256=9a79099873c8b3156a8f7937d7976ef85bf35f46e3f0a963117be44bc9bbc5f8
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
        target_compile_definitions(rapidjson INTERFACE RAPIDJSON_HAS_STDSTRING=1)
        if(MSVC)
            target_compile_definitions(rapidjson INTERFACE _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
        endif()
    endif()
endif()

if(KL_ENABLE_YAML)
    FetchContent_GetProperties(yamlcpp)
    if(NOT yamlcpp_POPULATED)
        FetchContent_Populate(yamlcpp)

        # YAML-CPP's CMake file is very old (2.6) and doesn't use INTERFACE_ properties
        # add_subdirectory(${yamlcpp_SOURCE_DIR} ${yamlcpp_BINARY_DIR})
        add_library(yaml-cpp)
        add_library(yaml::yaml-cpp ALIAS yaml-cpp)
        file(GLOB_RECURSE sources
            "${yamlcpp_SOURCE_DIR}/src/*.cpp"
            "${yamlcpp_SOURCE_DIR}/src/*.h"
            "${yamlcpp_SOURCE_DIR}/include/yaml-cpp/*.h")
        target_sources(yaml-cpp PRIVATE ${sources})
        target_include_directories(yaml-cpp PUBLIC ${yamlcpp_SOURCE_DIR}/include)
        target_compile_features(yaml-cpp PUBLIC cxx_std_11)
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
    endif()
endif()
