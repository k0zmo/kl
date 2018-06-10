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
    ${kl_SOURCE_DIR}/include/kl/json_convert.hpp
    ${kl_SOURCE_DIR}/include/kl/json_print.hpp
    ${kl_SOURCE_DIR}/include/kl/match.hpp
    ${kl_SOURCE_DIR}/include/kl/meta.hpp
    ${kl_SOURCE_DIR}/include/kl/signal.hpp
    ${kl_SOURCE_DIR}/include/kl/split.hpp
    ${kl_SOURCE_DIR}/include/kl/stream_join.hpp
    ${kl_SOURCE_DIR}/include/kl/hash.hpp
    ${kl_SOURCE_DIR}/include/kl/tuple.hpp
    ${kl_SOURCE_DIR}/include/kl/type_traits.hpp
    ${kl_SOURCE_DIR}/include/kl/type_class.hpp
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
    PUBLIC
        $<BUILD_INTERFACE:${kl_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(kl
    PUBLIC 
        Boost::boost
        json11::json11
        GSL::gsl
    PRIVATE
        Boost::disable_autolinking
)
kl_compile_options(kl)

get_target_property(sources_list kl SOURCES)
source_group(TREE ${kl_SOURCE_DIR} FILES ${sources_list})

add_library(kl::kl ALIAS kl)

install(TARGETS kl EXPORT klTarget
    RUNTIME DESTINATION bin/$<CONFIG>
    LIBRARY DESTINATION lib/$<CONFIG>
    ARCHIVE DESTINATION lib/$<CONFIG>
)
install(DIRECTORY ${kl_SOURCE_DIR}/include/kl
    DESTINATION include
)
