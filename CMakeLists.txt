cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    ndi-rist-server
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

# add_library(
#     ndi-rist-server_lib OBJECT
#     src/lib.cpp
# )

# target_include_directories(
#     ndi-rist-server_lib ${warning_guard}
#     PUBLIC
#     "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
# )

# target_compile_features(ndi-rist-server_lib PUBLIC cxx_std_20)

# ---- Declare executable ----

add_executable(ndi-rist-server_exe src/main.cpp)
add_executable(ndi-rist-server::exe ALIAS ndi-rist-server_exe)

set_property(TARGET ndi-rist-server_exe PROPERTY OUTPUT_NAME ndi-rist-server)

target_compile_features(ndi-rist-server_exe PRIVATE cxx_std_20)

target_link_libraries(ndi-rist-server_exe PRIVATE ndi-rist-server_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT ndi-rist-server_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of ndi-rist-server"
  )
endif()

include(cmake/dev-mode.cmake)

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
# Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive --remote
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()