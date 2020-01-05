include(FetchContent)

set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL ON)

if(KL_TEST)
    FetchContent_Declare(Catch2
        URL https://github.com/catchorg/Catch2/archive/v2.9.1.zip
        URL_HASH SHA256=9a79099873c8b3156a8f7937d7976ef85bf35f46e3f0a963117be44bc9bbc5f8
    )
    FetchContent_GetProperties(Catch2)
    if(NOT catch2_POPULATED) # Variable names are lowercased
        FetchContent_Populate(Catch2)
        add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
    endif()
endif()
