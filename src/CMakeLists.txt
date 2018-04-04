add_library(kl
    kl/base_range.hpp 
    kl/base_range.hpp
    kl/base64.hpp
    kl/base64.cpp
    kl/binary_rw.hpp
    kl/byte.hpp
    kl/ctti.hpp
    kl/defer.hpp
    kl/enum_flags.hpp
    kl/enum_range.hpp
    kl/enum_reflector.hpp
    kl/enum_traits.hpp
    kl/file_view.hpp
    kl/file_view.cpp
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
# binary_rw (WIP)
target_sources(kl PRIVATE
    kl/binary_rw/endian.hpp
    kl/binary_rw/map.hpp
    kl/binary_rw/optional.hpp
    kl/binary_rw/pair.hpp
    kl/binary_rw/set.hpp
    kl/binary_rw/string.hpp
    kl/binary_rw/variant.hpp
    kl/binary_rw/vector.hpp
)

get_target_property(_sources_list kl SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
             FILES ${_sources_list})

target_include_directories(kl PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kl
                      PUBLIC 
                          Boost::boost json11::json11 GSL::gsl
                      PRIVATE Boost::disable_autolinking)
target_kl_compile_options(kl)
add_library(kl::kl ALIAS kl)
