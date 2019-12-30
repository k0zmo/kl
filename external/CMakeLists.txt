include(FetchContent)

set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL ON)

FetchContent_Declare(rapidjson
    URL https://github.com/Tencent/rapidjson/archive/v1.1.0.zip
    URL_HASH SHA256=8e00c38829d6785a2dfb951bb87c6974fa07dfe488aa5b25deec4b8bc0f6a3ab
)
FetchContent_Declare(yamlcpp
    URL https://github.com/jbeder/yaml-cpp/archive/0f9a586ca1dc29c2ecb8dd715a315b93e3f40f79.zip
    URL_HASH SHA256=ff55e0cc373295b8503faf52a5e9569b950d8ec3e704508a62fe9159c37185bc
)
FetchContent_Declare(GSL
    URL https://github.com/Microsoft/GSL/archive/v2.0.0.zip
    URL_HASH SHA256=837f35a9f00b2543bada8ae680e5c56af84709d36d48cdc3ed8db69df043f5b2
)
if(KL_TEST)
    FetchContent_Declare(Catch2
        URL https://github.com/catchorg/Catch2/archive/v2.9.1.zip
        URL_HASH SHA256=9a79099873c8b3156a8f7937d7976ef85bf35f46e3f0a963117be44bc9bbc5f8
    )
endif()

FetchContent_GetProperties(rapidjson)
if(NOT rapidjson_POPULATED)
    FetchContent_Populate(rapidjson)
    # No point in add_subdirectory() - there's no target defined
    add_library(rapidjson INTERFACE)
    add_library(rapidjson::rapidjson ALIAS rapidjson)
    target_include_directories(rapidjson SYSTEM INTERFACE ${rapidjson_SOURCE_DIR}/include)
    target_compile_definitions(rapidjson INTERFACE RAPIDJSON_HAS_STDSTRING=1)
endif()

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
    set_target_properties(yaml-cpp PROPERTIES FOLDER external)
endif()

FetchContent_GetProperties(GSL)
if(NOT gsl_POPULATED)
    FetchContent_Populate(GSL)
    add_subdirectory(${gsl_SOURCE_DIR} ${gsl_BINARY_DIR})
    add_library(GSL::gsl ALIAS GSL)
endif()

if(KL_TEST)
    FetchContent_GetProperties(Catch2)
    if(NOT catch2_POPULATED) # Variable names are lowercased
        FetchContent_Populate(Catch2)
        add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
    endif()
endif()
