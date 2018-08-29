add_library(kl_cxx_flags INTERFACE)
target_compile_features(kl_cxx_flags INTERFACE cxx_std_14)
if(KL_DEV_BUILD)
    target_link_libraries(kl_cxx_flags INTERFACE
        $<BUILD_INTERFACE:kl_dev_cxx_flags>
    )
endif()

add_library(kl
    ${kl_SOURCE_DIR}/include/kl/base_range.hpp
    ${kl_SOURCE_DIR}/include/kl/base64.hpp
    ${kl_SOURCE_DIR}/include/kl/binary_rw.hpp
    ${kl_SOURCE_DIR}/include/kl/byte.hpp
    ${kl_SOURCE_DIR}/include/kl/ctti.hpp
    ${kl_SOURCE_DIR}/include/kl/defer.hpp
    ${kl_SOURCE_DIR}/include/kl/enum_flags.hpp
    ${kl_SOURCE_DIR}/include/kl/enum_range.hpp
    ${kl_SOURCE_DIR}/include/kl/enum_reflector.hpp
    ${kl_SOURCE_DIR}/include/kl/enum_traits.hpp
    ${kl_SOURCE_DIR}/include/kl/file_view.hpp
    ${kl_SOURCE_DIR}/include/kl/hash.hpp
    ${kl_SOURCE_DIR}/include/kl/json_convert.hpp
    ${kl_SOURCE_DIR}/include/kl/json_print.hpp
    ${kl_SOURCE_DIR}/include/kl/match.hpp
    ${kl_SOURCE_DIR}/include/kl/meta.hpp
    ${kl_SOURCE_DIR}/include/kl/signal.hpp
    ${kl_SOURCE_DIR}/include/kl/split.hpp
    ${kl_SOURCE_DIR}/include/kl/stream_join.hpp
    ${kl_SOURCE_DIR}/include/kl/tuple.hpp
    ${kl_SOURCE_DIR}/include/kl/type_traits.hpp
    ${kl_SOURCE_DIR}/include/kl/type_class.hpp
    ${kl_SOURCE_DIR}/include/kl/utility.hpp
    ${kl_SOURCE_DIR}/include/kl/zip.hpp
    # binary_rw (WIP)
    ${kl_SOURCE_DIR}/include/kl/binary_rw/endian.hpp
    ${kl_SOURCE_DIR}/include/kl/binary_rw/map.hpp
    ${kl_SOURCE_DIR}/include/kl/binary_rw/optional.hpp
    ${kl_SOURCE_DIR}/include/kl/binary_rw/pair.hpp
    ${kl_SOURCE_DIR}/include/kl/binary_rw/set.hpp
    ${kl_SOURCE_DIR}/include/kl/binary_rw/string.hpp
    ${kl_SOURCE_DIR}/include/kl/binary_rw/variant.hpp
    ${kl_SOURCE_DIR}/include/kl/binary_rw/vector.hpp
    base64.cpp
    file_view.cpp
)
target_include_directories(kl
    PUBLIC $<BUILD_INTERFACE:${kl_SOURCE_DIR}/include>
)
target_link_libraries(kl
    PUBLIC
        Boost::boost
        json11::json11
        GSL::gsl
        kl_cxx_flags
    PRIVATE
        Boost::disable_autolinking
)

get_target_property(sources_list kl SOURCES)
source_group(TREE ${kl_SOURCE_DIR} FILES ${sources_list})

add_library(kl::kl ALIAS kl)

add_library(kl_dev_cxx_flags INTERFACE)
if(MSVC)
    target_compile_options(kl_dev_cxx_flags INTERFACE 
        /W4
        /wd4127  # conditional expression in constant)
        /wd4100  # unreferenced formal parameter
    )
    target_compile_definitions(kl_dev_cxx_flags INTERFACE
        _SCL_SECURE_NO_WARNINGS
    )
elseif(CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    target_compile_options(kl_dev_cxx_flags INTERFACE
        -Wall
        -pedantic
        -Wextra
        -Wno-unused-parameter
        $<$<CXX_COMPILER_ID:Clang>:-Wno-missing-braces>
    )
endif()

install(TARGETS kl kl_cxx_flags EXPORT klTarget
    INCLUDES DESTINATION include
    RUNTIME DESTINATION bin/$<CONFIG>
    LIBRARY DESTINATION lib/$<CONFIG>
    ARCHIVE DESTINATION lib/$<CONFIG>
)
install(DIRECTORY ${kl_SOURCE_DIR}/include/kl
    DESTINATION include
)
