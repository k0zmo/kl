include_guard()

set(CMAKE_CXX_COMPILER "g++" CACHE STRING "CXX Compiler")

set(compile_options
    -Wall
    -Wextra
    -Wpedantic
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
set(link_options
    -Wl,--exclude-libs=ALL
    -Wl,--as-needed
    -Wl,-z,defs
)
set(link_options_debug
    # Consider adding -Wl,--gdb-index (requires gold, lld or mold)
)
set(link_options_release)
set(link_options_coverage ${link_options_debug})
set(link_options_asan)
set(link_options_tsan)
set(link_options_ubsan)

include(${CMAKE_CURRENT_LIST_DIR}/../SetFlags.cmake)
