include(FetchContent)

set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL ON)

FetchContent_Declare(rapidjson
    URL https://github.com/Tencent/rapidjson/archive/v1.1.0.zip
    URL_HASH SHA1=0fe7b4f7b83df4b3d517f4a202f3a383af7a0818
)
FetchContent_Declare(GSL
    URL https://github.com/Microsoft/GSL/archive/v1.0.0.zip
    URL_HASH SHA1=e9824bf3693bf34f17850915a744deec905c90be
)
if(KL_TEST)
    FetchContent_Declare(Catch2
        URL https://github.com/catchorg/Catch2/archive/v2.4.0.zip
        URL_HASH SHA1=0c24c01df55aa8f06f5dfb47db27179aa24441c2
    )
endif()

FetchContent_GetProperties(rapidjson)
if(NOT rapidjson_POPULATED)
    FetchContent_Populate(rapidjson)
    # No point in add_subdirectory() - there's no target defined
    add_library(rapidjson INTERFACE)
    add_library(rapidjson::rapidjson ALIAS rapidjson)
    target_include_directories(rapidjson INTERFACE ${rapidjson_SOURCE_DIR}/include)
    target_compile_definitions(rapidjson INTERFACE RAPIDJSON_HAS_STDSTRING=1)
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
