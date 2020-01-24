add_library(kl_dev_flags INTERFACE)
add_library(kl::dev_flags ALIAS kl_dev_flags)

if(MSVC)
    target_compile_options(kl_dev_flags INTERFACE
        /W4
        /wd4127  # conditional expression in constant)
        /wd4100  # unreferenced formal parameter
    )
    target_compile_definitions(kl_dev_flags INTERFACE
        _SCL_SECURE_NO_WARNINGS
    )
elseif(CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    target_compile_options(kl_dev_flags INTERFACE
        -Wall
        -pedantic
        -Wextra
        -Wno-unused-parameter
        $<$<CXX_COMPILER_ID:Clang>:-Wno-missing-braces>
    )
endif()
