find_package(PkgConfig)
pkg_check_modules(_rapidjson QUIET RapidJSON)

find_path(RapidJSON_INCLUDE_DIR
    NAMES         rapidjson/rapidjson.h
    HUNTS         ${_rapidjson_INCLUDE_DIRS}
)
mark_as_advanced(RapidJSON_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RapidJSON
    REQUIRED_VARS RapidJSON_INCLUDE_DIR
)

if(RapidJSON_FOUND)
	if(NOT TARGET RapidJSON::RapidJSON)
		add_library(RapidJSON::RapidJSON INTERFACE IMPORTED)
		set_target_properties(RapidJSON::RapidJSON PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${RapidJSON_INCLUDE_DIR}"
            INTERFACE_COMPILE_DEFINITIONS "RAPIDJSON_HAS_STDSTRING=1"
		)
	endif()
endif()
