add_library(kl_dev_flags INTERFACE)
add_library(kl::dev_flags ALIAS kl_dev_flags)

if(MSVC)
    target_compile_options(kl_dev_flags INTERFACE
        /W4
        /wd4127 # conditional expression in constant
        /wd4251 # needs to have dll-interface to be used by clients
        /wd4275 # non dll-interface class X used as base for dll-interface class
    )
    target_compile_definitions(kl_dev_flags INTERFACE
        _SCL_SECURE_NO_WARNINGS
    )
elseif(CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    target_compile_options(kl_dev_flags INTERFACE
        -Wall
        -Wpedantic
        -Wextra
        $<$<CXX_COMPILER_ID:Clang>:-Wno-missing-braces>
    )
endif()
