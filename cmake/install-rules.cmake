install(
    TARGETS ndi-rist-server_exe
    RUNTIME COMPONENT ndi-rist-server_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
