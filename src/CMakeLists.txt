add_library(kl_cxx_flags INTERFACE)
target_compile_features(kl_cxx_flags INTERFACE cxx_std_14)

if(KL_DEV_BUILD)
    include(kl/DevFlags)
    target_link_libraries(kl_cxx_flags INTERFACE
        $<BUILD_INTERFACE:kl::dev_flags>
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
    ${kl_SOURCE_DIR}/include/kl/json.hpp
    ${kl_SOURCE_DIR}/include/kl/match.hpp
    ${kl_SOURCE_DIR}/include/kl/meta.hpp
    ${kl_SOURCE_DIR}/include/kl/signal.hpp
    ${kl_SOURCE_DIR}/include/kl/split.hpp
    ${kl_SOURCE_DIR}/include/kl/stream_join.hpp
    ${kl_SOURCE_DIR}/include/kl/tuple.hpp
    ${kl_SOURCE_DIR}/include/kl/type_traits.hpp
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
)
if(WIN32)
    target_sources(kl PRIVATE file_view_win32.cpp)
elseif(UNIX)
    target_sources(kl PRIVATE file_view_unix.cpp)
endif()

target_include_directories(kl
    PUBLIC $<BUILD_INTERFACE:${kl_SOURCE_DIR}/include>
)
target_link_libraries(kl
    PUBLIC
        Boost::boost
        rapidjson::rapidjson
        GSL::gsl
        kl_cxx_flags
    PRIVATE
        Boost::disable_autolinking
)

get_target_property(sources_list kl SOURCES)
source_group(TREE ${kl_SOURCE_DIR} FILES ${sources_list})

add_library(kl::kl ALIAS kl)
