include_guard(GLOBAL)

set(CMAKE_CXX_COMPILER "clang++" CACHE STRING "CXX Compiler")

set(CXX_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wno-missing-braces
)
set(CXX_FLAGS_DEBUG
    -O0
    -g
    # Consider adding -gsplit-dwarf
)
set(CXX_FLAGS_RELEASE
    -O3
    -DNDEBUG
)
set(CXX_FLAGS_COVERAGE ${CXX_FLAGS_DEBUG}
    --coverage
)
set(CXX_FLAGS_ASAN
    -O1
    -g
    -fsanitize=address
    -fno-omit-frame-pointer
    -DNDEBUG
)
set(CXX_FLAGS_TSAN
    -O1
    -g
    -fsanitize=thread
    -DNDEBUG
)
set(CXX_FLAGS_UBSAN
    -O1
    -g
    -fsanitize=undefined
    -fno-omit-frame-pointer
    -DNDEBUG
)
set(CXX_FLAGS_MSAN
    -O2
    -g
    -fsanitize=memory
    -fno-omit-frame-pointer
    -DNDEBUG
)
set(linker_flags_no_sanitizer
    -Wl,--exclude-libs=ALL
    -Wl,--as-needed
    -Wl,-z,defs
)
set(LINKER_FLAGS)
set(LINKER_FLAGS_DEBUG ${linker_flags_no_sanitizer}
    # Consider adding -Wl,--gdb-index (requires gold, lld or mold)
)
set(LINKER_FLAGS_RELEASE ${linker_flags_no_sanitizer})
set(LINKER_FLAGS_COVERAGE ${LINKER_FLAGS_DEBUG})
set(LINKER_FLAGS_ASAN)
set(LINKER_FLAGS_TSAN)
set(LINKER_FLAGS_UBSAN)
set(LINKER_FLAGS_MSAN)

include(${CMAKE_CURRENT_LIST_DIR}/SetFlags.cmake)
