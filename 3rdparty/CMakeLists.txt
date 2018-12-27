include(FetchContent)

set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL ON)

FetchContent_Declare(json11
    GIT_REPOSITORY https://github.com/k0zmo/json11.git
    GIT_TAG ac1a452c1f420b8e912a8291f867c3d71db657f0
    GIT_SHALLOW ON
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

FetchContent_GetProperties(json11)
if(NOT json11_POPULATED)
    FetchContent_Populate(json11)
    add_subdirectory(${json11_SOURCE_DIR} ${json11_BINARY_DIR})
    set_target_properties(json11 PROPERTIES FOLDER 3rdparty)
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
