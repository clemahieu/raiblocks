add_custom_target(
  clean_coverage
  COMMAND echo "CLEANING COUNTERS"
  COMMAND ${LCOV_PATH} --directory . --zerocounter -q
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  DEPENDS build_tests)

add_custom_target(
  coverage_tests
  COMMAND echo "RUN TESTS FOR COVERAGE"
  COMMAND ${PROJECT_SOURCE_DIR}/ci/test.sh ${CMAKE_BINARY_DIR} || true
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  DEPENDS clean_coverage)

add_custom_target(
  coveralls_upload
  COMMAND echo "UPLOADING COVERALLS COVERAGE REPORT"
  COMMAND coveralls -i nano -b build -r . --gcov-options '\\-lp'
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  DEPENDS coverage_tests)

add_custom_target(
  generate_coverage
  COMMAND echo "CAPTURING COVERAGE DATA"
  COMMAND ${LCOV_PATH} --directory . --capture --output-file lcov.info.raw -q
  COMMAND echo "REMOVING FLUFF FROM REPORT"
  COMMAND
    ${LCOV_PATH} --remove lcov.info.raw '/usr/*' '/tmp/*'
    '${PROJECT_SOURCE_DIR}/cpptoml/*' '${PROJECT_SOURCE_DIR}/crypto/*'
    '${PROJECT_SOURCE_DIR}/flatbuffers/*' '${PROJECT_SOURCE_DIR}/gtest/*'
    '${PROJECT_SOURCE_DIR}/rocksdb/*' '${PROJECT_SOURCE_DIR}/valgrind/*'
    '${PROJECT_SOURCE_DIR}/nano/core_test/*'
    '${PROJECT_SOURCE_DIR}/nano/load_test/*'
    '${PROJECT_SOURCE_DIR}/nano/ipc_flatbuffers_test/*'
    '${PROJECT_SOURCE_DIR}/nano/ipc_flatbuffers_lib/*'
    '${PROJECT_SOURCE_DIR}/nano/nano_node/*'
    '${PROJECT_SOURCE_DIR}/nano/nano_wallet/*'
    '${PROJECT_SOURCE_DIR}/nano/nano_rpc/*'
    '${PROJECT_SOURCE_DIR}/nano/rpc_test/*'
    '${PROJECT_SOURCE_DIR}/nano/qt_test/*' '${PROJECT_SOURCE_DIR}/nano/qt/*'
    '${PROJECT_SOURCE_DIR}/nano/test_common/*'
    '${PROJECT_SOURCE_DIR}/nano/qt_system/*' --output-file
    ${PROJECT_SOURCE_DIR}/coverage/lcov.info -q
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  DEPENDS coverage_tests)

add_custom_target(
  generate_html
  COMMAND echo "GENERATING HTML COVERAGE REPORT"
  COMMAND ${GENHTML_PATH} -o coverage ${PROJECT_SOURCE_DIR}/coverage/lcov.info
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  DEPENDS generate_coverage)
