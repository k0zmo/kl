include_guard()

set(CMAKE_CXX_COMPILER "clang++" CACHE STRING "CXX Compiler")

set(compile_options
    -Wall
    -Wextra
    -Wpedantic
    -Wno-missing-braces
)
set(compile_options_debug
    -O0
    -g
    # Consider adding -gsplit-dwarf
)
set(compile_options_release
    -O3
    -DNDEBUG
)
set(compile_options_coverage
    ${compile_options_debug}
    --coverage
)
set(compile_options_asan
    -O1
    -g
    -fsanitize=address
    -fno-omit-frame-pointer
    -DNDEBUG
)
set(compile_options_tsan
    -O1
    -g
    -fsanitize=thread
    -DNDEBUG
)
set(compile_options_ubsan
    -O1
    -g
    -fsanitize=undefined
    -fno-omit-frame-pointer
    -DNDEBUG
)
set(compile_options_msan
    -O2
    -g
    -fsanitize=memory
    -fno-omit-frame-pointer
    -DNDEBUG
)
set(link_options_no_sanitizer
    -Wl,--exclude-libs=ALL
    -Wl,--as-needed
    -Wl,-z,defs
)
set(link_options)
set(link_options_debug
    ${link_options_no_sanitizer}
    # Consider adding -Wl,--gdb-index (requires gold, lld or mold)
)
set(link_options_release ${link_options_no_sanitizer})
set(link_options_coverage ${link_options_debug})
set(link_options_asan)
set(link_options_tsan)
set(link_options_ubsan)
set(link_options_msan)

include(${CMAKE_CURRENT_LIST_DIR}/../SetFlags.cmake)
