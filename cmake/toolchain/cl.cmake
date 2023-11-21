include_guard()

set(CMAKE_CXX_COMPILER "cl.exe" CACHE STRING "CXX Compiler")

set(compile_options
    /utf-8           # Set source and execution character sets to UTF-8
    /GR              # Enable Run-Time Type Information
    /EHsc            # Enables standard C++ stack unwinding, assumes that functions declared as extern "C" never throw a C++ exception
    /permissive-     # Specify standards-conforming compiler behavior
    /Zc:inline       # Remove unreferenced COMDAT
    /Zc:throwingNew  # Assume operator new throws
    /Zc:preprocessor # Enable preprocessor conformance mode
    /volatile:iso    # Selects strict volatile semantics as defined by the ISO-standard C++ language.
    /W4 
    /wd4127 # disable: `conditional expression is constant`
    /wd4251 # disable: `'type' : class 'type1' needs to have dll-interface to be used by clients of class 'type2'`
    /wd4275 # disable: `non - DLL-interface class 'class_1' used as base for DLL-interface class 'class_2'`
    /DWIN32
    /D_WINDOWS
    /D_SCL_SECURE_NO_WARNINGS
    /DWIN32_LEAN_AND_MEAN
    /D_WIN32_WINNT=0x0A00
)
set(compile_options_debug
    /JMC # Just My Code debugging
    /Od  # Turns off all optimizations in the program and speeds compilation.
    /Zi  # Produces a separate PDB file that contains all the symbolic debugging information for use with the debugger
)
set(compile_options_release
    /O2  # Maximize Speed
    /Zi  # Produces a separate PDB file that contains all the symbolic debugging information for use with the debugger
    /Oi  # Generate Intrinsic Functions
    /Ob3 # Specifies more aggressive inlining than /Ob2
    /Gw  # Package global data in COMDAT sections for optimization.
    /Gy  # Allows the compiler to package individual functions in the form of packaged functions (COMDATs).
    /DNDEBUG
)
set(link_options)
set(link_options_debug
    /DEBUG          # Create a debugging information file for the executable.
    /INCREMENTAL    # Link incrementally 
)
set(link_options_release
    /DEBUG          # Create a debugging information file for the executable.
    /INCREMENTAL:NO # always perform a full link
    /OPT:REF        # Remove unreferenced packaged functions and data
    /OPT:ICF        # Perform identical COMDAT folding.
)

include(${CMAKE_CURRENT_LIST_DIR}/../SetFlags.cmake)
