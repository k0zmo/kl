add_library(kl
    ${kl_SOURCE_DIR}/include/kl/detail/concepts.hpp
    ${kl_SOURCE_DIR}/include/kl/detail/macros.hpp
    ${kl_SOURCE_DIR}/include/kl/base64.hpp
    ${kl_SOURCE_DIR}/include/kl/binary_rw.hpp
    ${kl_SOURCE_DIR}/include/kl/ctti.hpp
    ${kl_SOURCE_DIR}/include/kl/defer.hpp
    ${kl_SOURCE_DIR}/include/kl/enum_set.hpp
    ${kl_SOURCE_DIR}/include/kl/enum_range.hpp
    ${kl_SOURCE_DIR}/include/kl/enum_reflector.hpp
    ${kl_SOURCE_DIR}/include/kl/enum_traits.hpp
    ${kl_SOURCE_DIR}/include/kl/exception_info.hpp
    ${kl_SOURCE_DIR}/include/kl/file_view.hpp
    ${kl_SOURCE_DIR}/include/kl/hash.hpp
    ${kl_SOURCE_DIR}/include/kl/iterator_facade.hpp
    ${kl_SOURCE_DIR}/include/kl/match.hpp
    ${kl_SOURCE_DIR}/include/kl/meta.hpp
    ${kl_SOURCE_DIR}/include/kl/range.hpp
    ${kl_SOURCE_DIR}/include/kl/reflect_enum.hpp
    ${kl_SOURCE_DIR}/include/kl/reflect_struct.hpp
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

target_compile_features(kl PUBLIC cxx_std_20)
target_include_directories(kl
    PUBLIC $<BUILD_INTERFACE:${kl_SOURCE_DIR}/include>
)
target_link_libraries(kl
    PUBLIC
        Boost::boost
        Microsoft.GSL::GSL
    PRIVATE
        Boost::disable_autolinking
)
kl_source_group(kl TREE ${kl_SOURCE_DIR})
set_target_properties(kl PROPERTIES FOLDER kl DEBUG_POSTFIX d)
add_library(kl::kl ALIAS kl)

if(KL_ENABLE_JSON)
    add_library(kl-json
        ${kl_SOURCE_DIR}/include/kl/json.hpp
        ${kl_SOURCE_DIR}/include/kl/json_fwd.hpp
        json.cpp
    )
    target_link_libraries(kl-json PUBLIC
        kl
        rapidjson
    )
    target_compile_definitions(kl-json PUBLIC
        RAPIDJSON_HAS_STDSTRING=1
    )
    kl_source_group(kl-json TREE ${kl_SOURCE_DIR})
    set_target_properties(kl-json PROPERTIES FOLDER kl DEBUG_POSTFIX d)
    add_library(kl::json ALIAS kl-json)
endif()

if(KL_ENABLE_YAML)
    add_library(kl-yaml
        ${kl_SOURCE_DIR}/include/kl/yaml.hpp
        ${kl_SOURCE_DIR}/include/kl/yaml_fwd.hpp
        yaml.cpp
    )
    target_link_libraries(kl-yaml PUBLIC
        kl
        yaml-cpp
    )
    if(MSVC)
        target_compile_options(kl-yaml PRIVATE /wd4100 /wd4127 /wd4244 /wd4456 /wd4702)
    endif()
    kl_source_group(kl-yaml TREE ${kl_SOURCE_DIR})
    set_target_properties(kl-yaml PROPERTIES FOLDER kl DEBUG_POSTFIX d)
    add_library(kl::yaml ALIAS kl-yaml)
endif()

if(kl_install_rules)
    install(TARGETS kl
        EXPORT klTarget
        INCLUDES DESTINATION include
    )
    install(DIRECTORY ${kl_SOURCE_DIR}/include/kl DESTINATION include)
    if(KL_ENABLE_JSON)
        install(TARGETS kl-json EXPORT klTarget)
    endif()
    if(KL_ENABLE_YAML)
        install(TARGETS kl-yaml EXPORT klTarget)
    endif()
endif()
