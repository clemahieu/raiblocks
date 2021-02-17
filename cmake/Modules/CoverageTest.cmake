set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage -lgcov")
find_program(LCOV_PATH lcov)
message("lcov found: ${LCOV_PATH}")
find_program(GENHTML_PATH genhtml)
message("genhtml found: ${GENHTML_PATH}")
if(NOT CMAKE_COMPILER_IS_GNUCXX)
  # Clang version 3.0.0 and greater now supports gcov as well.
  message(
    WARNING
      "Compiler is not GNU gcc! Clang Version 3.0.0 and greater supports gcov as well, but older versions don't."
  )
  if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(FATAL_ERROR "Compiler is not GNU gcc! Aborting...")
  endif()
endif()
if(NOT (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL
                                              "Coverage"))
  message(
    WARNING
      "Code coverage results with an optimized (non-Debug) build may be misleading"
  )
endif()