# dropbox/json11
add_library(json11
     json11/json11.hpp
     json11/json11.cpp
)
target_compile_features(json11 PUBLIC cxx_std_11)
target_include_directories(json11 PUBLIC
     ${CMAKE_CURRENT_SOURCE_DIR})
add_library(json11::json11 ALIAS json11)
set_target_properties(json11
     PROPERTIES FOLDER 3rdparty)

# Microsoft/GSL
add_library(gsl INTERFACE)
target_include_directories(gsl INTERFACE
     ${CMAKE_CURRENT_SOURCE_DIR})
add_library(GSL::gsl ALIAS gsl)

# philsquared/Catch
if(KL_TEST)
     add_library(catch INTERFACE)
     target_include_directories(catch INTERFACE
          ${CMAKE_CURRENT_SOURCE_DIR})
     add_library(Catch::catch ALIAS catch)
endif()
