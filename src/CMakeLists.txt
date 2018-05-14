set(headers
    kl/base_range.hpp 
    kl/base64.hpp
    kl/binary_rw.hpp
    kl/byte.hpp
    kl/ctti.hpp
    kl/defer.hpp
    kl/enum_flags.hpp
    kl/enum_range.hpp
    kl/enum_reflector.hpp
    kl/enum_traits.hpp
    kl/file_view.hpp
    kl/json_convert.hpp
    kl/json_print.hpp
    kl/match.hpp
    kl/meta.hpp
    kl/signal.hpp
    kl/split.hpp
    kl/stream_join.hpp
    kl/string_switch.hpp
    kl/tuple.hpp
    kl/type_traits.hpp
    kl/type_class.hpp
    kl/zip.hpp
)
set(binary_rw_headers
    # binary_rw (WIP)
    kl/binary_rw/endian.hpp
    kl/binary_rw/map.hpp
    kl/binary_rw/optional.hpp
    kl/binary_rw/pair.hpp
    kl/binary_rw/set.hpp
    kl/binary_rw/string.hpp
    kl/binary_rw/variant.hpp
    kl/binary_rw/vector.hpp
)
add_library(kl
    ${headers}
    ${binary_rw_headers}
    kl/base64.cpp
    kl/file_view.cpp
)

target_include_directories(kl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
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
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
             FILES ${sources_list})

add_library(kl::kl ALIAS kl)

install(TARGETS kl EXPORT klTarget
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(FILES ${headers}
    DESTINATION include/kl
)
install(FILES ${binary_rw_headers}
    DESTINATION include/kl/binary_rw
)
