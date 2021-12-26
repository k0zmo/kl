include(FetchContent)

set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL ON)

FetchContent_Declare(GSL
    URL https://github.com/microsoft/GSL/archive/v3.0.0.zip
    URL_HASH SHA256=ed2d506839fd917142d562fa720459ce58c38fc5711b715779093e895a87d8c7
)
FetchContent_Declare(rapidjson
    URL https://github.com/Tencent/rapidjson/archive/0d4517f15a8d7167ba9ae67f3f22a559ca841e3b.zip
    URL_HASH SHA256=4f44adc689e7c08e9eb197276600e58c501624b54102cba06267814e2a4f66eb
)
FetchContent_Declare(yamlcpp
    URL https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.7.0.zip
    URL_HASH SHA256=4d5e664a7fb2d7445fc548cc8c0e1aa7b1a496540eb382d137e2cc263e6d3ef5
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
        if(BUILD_SHARED_LIBS)
            target_compile_definitions(yaml-cpp INTERFACE YAML_CPP_DLL)
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
    endif()
endif()
