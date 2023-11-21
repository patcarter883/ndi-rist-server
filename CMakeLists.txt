cmake_minimum_required(VERSION 3.14)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cmake/prelude.cmake)

project(
    ndi-rist-server
    VERSION 0.1.0
    DESCRIPTION "Server for NDI RIST Encoder"
    HOMEPAGE_URL "https://github.com/patcarter883/ndi-rist-server.git"
    LANGUAGES CXX
)

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

IF (WIN32)

    find_package(GStreamer REQUIRED)
    find_package (Threads)

    target_link_libraries(ndi-rist-server_exe PRIVATE ${GSTREAMER_LIBRARIES})
    target_link_libraries(ndi-rist-server_exe PRIVATE iphlpapi wsock32 ws2_32 Threads::Threads)

ELSE()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4)
    pkg_search_module(gstreamer-sdp REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0>=1.4)
    pkg_search_module(gstreamer-rtp REQUIRED IMPORTED_TARGET gstreamer-rtp-1.0>=1.4)
    pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0>=1.4)
    pkg_search_module(gstreamer-video REQUIRED IMPORTED_TARGET gstreamer-video-1.0>=1.4)

    target_link_libraries(ndi-rist-server_exe PRIVATE
    PkgConfig::gstreamer
    PkgConfig::gstreamer-sdp
    PkgConfig::gstreamer-rtp
    PkgConfig::gstreamer-app
    PkgConfig::gstreamer-video)
ENDIF()

add_subdirectory(extern/rpclib)
add_subdirectory(extern/rist-cpp)

target_include_directories(ndi-rist-server_exe PUBLIC ${GSTREAMER_INCLUDE_DIRS})
target_include_directories(ndi-rist-server_exe PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/extern/rist-cpp/)
target_include_directories(ndi-rist-server_exe PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/extern/rist-cpp/rist/inc/ ${CMAKE_CURRENT_SOURCE_DIR}/extern/rist-cpp/rist/include/ ${CMAKE_CURRENT_SOURCE_DIR}/extern/rist-cpp/rist/include/librist ${CMAKE_CURRENT_SOURCE_DIR}/extern/rist-cpp/rist/build/include/librist)


target_link_libraries(ndi-rist-server_exe PRIVATE rpc ristnet)
add_dependencies(ndi-rist-server_exe rpc ristnet)
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