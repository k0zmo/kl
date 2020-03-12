add_library(kl_cxx_flags INTERFACE)
target_compile_features(kl_cxx_flags INTERFACE cxx_std_14)

if(KL_DEV_BUILD)
    include(DevFlags)
    target_link_libraries(kl_cxx_flags INTERFACE
        $<BUILD_INTERFACE:kl::dev_flags>
    )
endif()

add_library(kl
    ${kl_SOURCE_DIR}/include/kl/base64.hpp
    ${kl_SOURCE_DIR}/include/kl/binary_rw.hpp
    ${kl_SOURCE_DIR}/include/kl/byte.hpp
    ${kl_SOURCE_DIR}/include/kl/ctti.hpp
    ${kl_SOURCE_DIR}/include/kl/defer.hpp
    ${kl_SOURCE_DIR}/include/kl/describe_enum.hpp
    ${kl_SOURCE_DIR}/include/kl/describe_record.hpp
    ${kl_SOURCE_DIR}/include/kl/enum_flags.hpp
    ${kl_SOURCE_DIR}/include/kl/enum_range.hpp
    ${kl_SOURCE_DIR}/include/kl/enum_reflector.hpp
    ${kl_SOURCE_DIR}/include/kl/enum_traits.hpp
    ${kl_SOURCE_DIR}/include/kl/file_view.hpp
    ${kl_SOURCE_DIR}/include/kl/hash.hpp
    ${kl_SOURCE_DIR}/include/kl/match.hpp
    ${kl_SOURCE_DIR}/include/kl/meta.hpp
    ${kl_SOURCE_DIR}/include/kl/range.hpp
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
        GSL::gsl
        kl_cxx_flags
    PRIVATE
        Boost::disable_autolinking
)
kl_source_group(kl TREE ${kl_SOURCE_DIR})
set_target_properties(kl PROPERTIES FOLDER kl)
add_library(kl::kl ALIAS kl)

if(KL_ENABLE_JSON)
    add_library(kl-json
        ${kl_SOURCE_DIR}/include/kl/json.hpp
        json.cpp
    )
    target_link_libraries(kl-json PUBLIC
        kl
        rapidjson::rapidjson
    )
    kl_source_group(kl-json TREE ${kl_SOURCE_DIR})
    set_target_properties(kl-json PROPERTIES FOLDER kl)
    add_library(kl::json ALIAS kl-json)
endif()

if(KL_ENABLE_YAML)
    add_library(kl-yaml
        ${kl_SOURCE_DIR}/include/kl/yaml.hpp
        yaml.cpp
    )
    target_link_libraries(kl-yaml PUBLIC
        kl
        yaml::yaml-cpp
    )
    if(MSVC)
        target_compile_options(kl-yaml PRIVATE /wd4100 /wd4127 /wd4244 /wd4456 /wd4702)
    endif()
    kl_source_group(kl-yaml TREE ${kl_SOURCE_DIR})
    set_target_properties(kl-yaml PROPERTIES FOLDER kl)
    add_library(kl::yaml ALIAS kl-yaml)
endif()
