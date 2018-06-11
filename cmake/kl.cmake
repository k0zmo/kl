function(kl_compile_options _target)
    target_compile_features(${_target} PUBLIC cxx_std_14)
    target_compile_definitions(${_target} PRIVATE $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>)

    if(MSVC)
        target_compile_options(${_target}
                               PRIVATE # Tweak optimizations for Non-debug builds
                                       $<$<NOT:$<CONFIG:DEBUG>>:
                                           /Gw   # Optimize global data.
                                           /Oi   # Enable intrinsic functions.
                                           /Ob2  # Allows expansion of functions marked as inline, __inline, or __forceinline, and any other function that the compiler chooses
                                       >
                                       # Have the compiler eliminate unreferenced COMDAT functions and data before emitting the object file.
                                       /Zc:inline
                                       # Don't allow conversion from a string literal to mutable characters.
                                       /Zc:strictStrings
                                       # Disallow temporaries from binding to non-const lvalue references.
                                       /Zc:referenceBinding
                                       # Assume operator new throws on failure.
                                       /Zc:throwingNew)

        target_compile_options(${_target} PRIVATE 
                               /W4
                               /wd4996  # Deprecated stuff
                               /wd4127  # conditional expression in constant)
                               /wd4100) # unreferenced formal parameter

    elseif(CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
        target_compile_options(${_target} PRIVATE
                               -Wall
                               -pedantic
                               -Wextra
                               -Wno-unused-parameter
                               $<$<CXX_COMPILER_ID:Clang>:-Wno-missing-braces>)
    endif()
endfunction()
