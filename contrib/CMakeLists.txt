add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson)

find_package(fmt 8.1.1 GLOBAL)
if(NOT fmt_FOUND)
  add_subdirectory(fmt-8.1.1)
  set_target_properties(fmt PROPERTIES
      CXX_STANDARD 17
      CXX_STANDARD_REQUIRED YES
      CXX_EXTENSIONS NO
  )
endif()

add_subdirectory(crc32)
add_subdirectory(stacktrace)
add_subdirectory(folly_memcpy)
add_subdirectory(rapidxml)
add_subdirectory(sqlite)
add_subdirectory(SimpleOpt)
add_subdirectory(md5)
add_subdirectory(libb64)
if(NOT WIN32)
  add_subdirectory(linenoise)
  add_subdirectory(debug_determinism)
  add_subdirectory(monitoring)
  add_subdirectory(TraceLogHelper)
  add_subdirectory(TestHarness)
endif()
