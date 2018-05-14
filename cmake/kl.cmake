function(kl_compile_options _target)
    if(MSVC)
        target_compile_options(${_target}
                               PRIVATE # Tweak optimizations for Non-debug builds
                                       $<$<NOT:$<CONFIG:DEBUG>>:
                                           /Gw   # Optimize global data.
                                           /Oi   # Enable intrinsic functions.
                                           /Ob2  # Allows expansion of functions marked as inline, __inline, or __forceinline, and any other function that the compiler chooses
                                       >
                               PUBLIC # Have the compiler eliminate unreferenced COMDAT functions and data before emitting the object file.
                                      /Zc:inline
                                      # Don't allow conversion from a string literal to mutable characters.
                                      /Zc:strictStrings
                                      # Disallow temporaries from binding to non-const lvalue references.
                                      /Zc:referenceBinding
                                      # Assume operator new throws on failure.
                                      /Zc:throwingNew
                                      # Build in C++ Latest mode.
                                      /std:c++latest
                                      # Build with /permissive- flag if supported by the compiler
                                      $<$<VERSION_GREATER_EQUAL:${MSVC_VERSION},1910>:/permissive->)

        target_compile_definitions(${_target}
                                   # Boost still have dependencies on unary_function and binary_function, so we have to make sure not to remove them.
                                   # Also, Catch uses auto_ptr if __cplusplus is 199711L which is the MSVC case
                                   PUBLIC _HAS_AUTO_PTR_ETC=1
                                   PRIVATE WIN32_LEAN_AND_MEAN)

        target_compile_options(${_target} PRIVATE 
                               /W4
                               /wd4996  # Deprecated stuff
                               /wd4127  # conditional expression in constant)
                               /wd4100) # unreferenced formal parameter

    elseif(CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
        target_compile_features(${_target} PUBLIC cxx_std_14)

        target_compile_options(${_target} PRIVATE
                               -Wall
                               -pedantic
                               -Wextra
                               -Wno-unused-parameter
                               $<$<CXX_COMPILER_ID:Clang>:-Wno-missing-braces>)
    endif()
endfunction()
