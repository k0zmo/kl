find_path(GSL_INCLUDE_DIR
    NAMES         gsl/gsl
)
mark_as_advanced(GSL_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GSL
    REQUIRED_VARS GSL_INCLUDE_DIR
)

if(GSL_FOUND)
	if(NOT TARGET GSL::GSL)
		add_library(GSL::GSL INTERFACE IMPORTED)
		set_target_properties(GSL::GSL PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${GSL_INCLUDE_DIR}"
		)
	endif()
endif()
