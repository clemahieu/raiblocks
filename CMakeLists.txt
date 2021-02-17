cmake_minimum_required(VERSION 3.4)

if(CMAKE_VERSION VERSION_GREATER 3.12 OR CMAKE_VERSION VERSION_EQUAL 3.12)
  # find_package uses <PACKAGENAME>_ROOT variables
  cmake_policy(SET CMP0074 NEW)
endif()
if(CMAKE_VERSION VERSION_GREATER 3.13 OR CMAKE_VERSION VERSION_EQUAL 3.13)
  # option honors normal variables
  cmake_policy(SET CMP0077 NEW)
endif()

if(CMAKE_VERSION VERSION_LESS 3.13)
  # compatibility for boost import targets use bundled 3.13 FindBoost.cmake
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/legacyModules")
endif()

# compatibility for osx sierra and on needs to be set before project
set(CMAKE_OSX_DEPLOYMENT_TARGET
    10.14
    CACHE STRING "")

project(nano-node)

# Get the latest abbreviated commit hash of the working branch
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE)

option(CI_BUILD false)

set(COVERAGE
    OFF
    CACHE BOOL "Code Coverage Targets")
if(COVERAGE)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
  include(CoverageTest)
endif()

set(CI_TEST
    0
    CACHE STRING "")

if(MSVC)
  add_definitions(/MP)
endif()

set(CPACK_PACKAGE_VERSION_MAJOR "22")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_VERSION_PRE_RELEASE "0")
set(CPACK_PACKAGE_VENDOR "Nano Currency")

if(CI_BUILD)
  set(TAG_VERSION_STRING "$ENV{TRAVIS_TAG}")
else()
  set(TAG_VERSION_STRING
      "V${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
endif()

if(APPLE)
  set(CMAKE_INSTALL_RPATH
      "@executable_path/../Frameworks;@executable_path/../boost/lib")
else()
  set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()

# Create all libraries and executables in the root binary dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(NANO_GUI
    OFF
    CACHE BOOL "")
set(NANO_TEST
    OFF
    CACHE BOOL "")
set(NANO_SECURE_RPC
    OFF
    CACHE BOOL "")
set(NANO_POW_SERVER
    OFF
    CACHE BOOL "")
set(NANO_WARN_TO_ERR
    OFF
    CACHE BOOL "")
set(NANO_TIMED_LOCKS
    0
    CACHE STRING "")
set(NANO_TIMED_LOCKS_IGNORE_BLOCKED
    OFF
    CACHE BOOL "")
set(NANO_TIMED_LOCKS_FILTER
    ""
    CACHE
      STRING
      "Selects which mutex should be the only one to have stacktraces generated, empty string means report on all"
)
set_property(
  CACHE NANO_TIMED_LOCKS_FILTER
  PROPERTY STRINGS
           active
           block_arrival
           block_processor
           block_uniquer
           confirmation_height_processor
           dropped_elections,
           election_winner_details
           gap_cache
           network_filter
           observer_set
           request_aggregator
           state_block_signature_verification
           telemetry
           vote_generator
           vote_processor
           vote_uniquer
           votes_cache
           work_pool)
set(NANO_FUZZER_TEST
    OFF
    CACHE BOOL "")
set(NANO_ASIO_HANDLER_TRACKING
    0
    CACHE STRING "")
set(NANO_ROCKSDB_TOOLS
    OFF
    CACHE BOOL "")

option(NANO_STACKTRACE_BACKTRACE
       "Use BOOST_STACKTRACE_USE_BACKTRACE in stacktraces, for POSIX" OFF)
if(NANO_STACKTRACE_BACKTRACE)
  add_definitions(-DNANO_STACKTRACE_BACKTRACE=1)
  if(BACKTRACE_INCLUDE)
    add_definitions(
      -DBOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE=${BACKTRACE_INCLUDE})
  endif()
endif()

if(${NANO_TIMED_LOCKS} GREATER 0)
  add_definitions(-DNANO_TIMED_LOCKS=${NANO_TIMED_LOCKS})
  add_definitions(-DNANO_TIMED_LOCKS_FILTER=${NANO_TIMED_LOCKS_FILTER})
  if(NANO_TIMED_LOCKS_IGNORE_BLOCKED)
    add_definitions(-DNANO_TIMED_LOCKS_IGNORE_BLOCKED)
  endif()
endif()

if(${NANO_ASIO_HANDLER_TRACKING} GREATER 0)
  add_definitions(-DNANO_ASIO_HANDLER_TRACKING=${NANO_ASIO_HANDLER_TRACKING}
                  -DBOOST_ASIO_ENABLE_HANDLER_TRACKING)
endif()

option(NANO_ASAN_INT "Enable ASan+UBSan+Integer overflow" OFF)
option(NANO_ASAN "Enable ASan+UBSan" OFF)
option(NANO_TSAN "Enable TSan" OFF)
option(NANO_SIMD_OPTIMIZATIONS
       "Enable CPU-specific SIMD optimizations (SSE/AVX or NEON, e.g.)" OFF)
option(
  ENABLE_AES
  "Enable AES optimizations (enabled by default with NANO_SIMD_OPTIMIZATIONS, set OFF to disable"
  ON)
option(ENABLE_AVX2 "Enable AVX2 optimizations" OFF)

set(ACTIVE_NETWORK
    nano_live_network
    CACHE STRING "Selects which network parameters are used")
set_property(
  CACHE ACTIVE_NETWORK PROPERTY STRINGS nano_dev_network nano_beta_network
                                nano_live_network nano_test_network)
set(CPACK_PACKAGE_NAME
    "nano-node"
    CACHE STRING "" FORCE)
set(CPACK_NSIS_PACKAGE_NAME
    "Nano"
    CACHE STRING "" FORCE)
set(CPACK_PACKAGE_INSTALL_DIRECTORY
    "nanocurrency"
    CACHE STRING "" FORCE)
set(NANO_SERVICE "nanocurrency.service")
set(NANO_PREFIX "")
if("${ACTIVE_NETWORK}" MATCHES "nano_beta_network")
  project("nano-node-beta")
  set(CPACK_PACKAGE_NAME
      "nano-node-beta"
      CACHE STRING "" FORCE)
  set(CPACK_NSIS_PACKAGE_NAME
      "Nano-Beta"
      CACHE STRING "" FORCE)
  set(CPACK_PACKAGE_INSTALL_DIRECTORY
      "nanocurrency-beta"
      CACHE STRING "" FORCE)
  set(NANO_SERVICE "nanocurrency-beta.service")
  set(NANO_PREFIX "Beta")
elseif("${ACTIVE_NETWORK}" MATCHES "nano_test_network")
  project("nano-node-test")
  set(CPACK_PACKAGE_NAME
      "nano-node-test"
      CACHE STRING "" FORCE)
  set(CPACK_NSIS_PACKAGE_NAME
      "Nano-Test"
      CACHE STRING "" FORCE)
  set(CPACK_PACKAGE_INSTALL_DIRECTORY
      "nanocurrency-test"
      CACHE STRING "" FORCE)
  set(NANO_SERVICE "nanocurrency-test.service")
  set(NANO_PREFIX "Test")
endif()
set(NANO_OSX_PACKAGE_NAME
    ${CPACK_NSIS_PACKAGE_NAME}
    CACHE STRING "" FORCE)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE
        STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(USING_ASAN (NANO_ASAN OR RAIBLOCKS_ASAN))
set(USING_ASAN_INT (NANO_ASAN_INT OR RAIBLOCKS_ASAN_INT))
set(USING_TSAN (NANO_TSAN OR RAIBLOCKS_TSAN))

find_package(Threads)

if(WIN32)
  find_library(PSAPI Psapi)
  add_definitions(-D_WIN32_WINNT=0x0600 -DWINVER=0x0600 -DWIN32_LEAN_AND_MEAN
                  -DMINIUPNP_STATICLIB -D_CRT_SECURE_NO_WARNINGS /EHsc)

  if(${USING_TSAN}
     OR ${USING_ASAN}
     OR ${USING_ASAN_INT})
    message(WARNING "Cannot use TSAN or ASAN on Windows, sanitizers ignored")
  endif()

else()
  if(NANO_WARN_TO_ERR)
    add_compile_options(-Werror -Wno-deprecated-declarations)
  endif()

  if((${USING_TSAN} AND ${USING_ASAN}) OR (${USING_TSAN} AND ${USING_ASAN_INT}))
    message(WARNING "Cannot use TSAN/ASAN together, defaulting to ASAN")
  endif()

  if(${USING_ASAN} OR ${USING_ASAN_INT})
    if(${USING_ASAN_INT})
      add_compile_options(-fsanitize=address,undefined,integer)
    else()
      add_compile_options(-fsanitize=address,undefined)
    endif()
    add_definitions(-DED25519_NO_INLINE_ASM)
  elseif(${USING_TSAN})
    add_compile_options(-fsanitize=thread)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      add_compile_options(
        "-fsanitize-blacklist=${PROJECT_SOURCE_DIR}/tsan_clang_blacklist")
    endif()
    add_definitions(-DED25519_NO_INLINE_ASM)
  endif()

  if(NANO_FUZZER_TEST)
    add_compile_options(-fsanitize=fuzzer-no-link -fno-omit-frame-pointer)
    add_definitions(-DNANO_FUZZER_TEST)
  endif()

  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86(_64)?)$")
    if(NANO_SIMD_OPTIMIZATIONS
       OR RAIBLOCKS_SIMD_OPTIMIZATIONS
       OR ENABLE_AVX2)
      add_compile_options(-msse4)
      if(ENABLE_AES)
        add_compile_options(-maes)
      else()
        add_definitions(-DCRYPTOPP_DISABLE_AESNI)
      endif()
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
      add_compile_options(-msse2)
      add_definitions(-DCRYPTOPP_DISABLE_SSSE3 -DCRYPTOPP_DISABLE_AESNI)
    endif()

  elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    if(NANO_SIMD_OPTIMIZATIONS OR RAIBLOCKS_SIMD_OPTIMIZATIONS)
      add_compile_options(-march=armv8-a+crc+crypto)
    else()
      add_compile_options(-march=armv8-a)
      add_definitions(-DCRYPTOPP_DISABLE_ASM)
    endif()
  endif()
endif()

if(ENABLE_AVX2)
  if(WIN32)
    add_definitions(/arch:AVX2)
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86(_64)?)$")
    add_compile_options(-mavx2 -mbmi -mbmi2 -maes)
  endif()
  if(PERMUTE_WITH_GATHER)
    add_definitions(-DPERMUTE_WITH_GATHER)
  elseif(PERMUTE_WITH_SHUFFLES)
    add_definitions(-DPERMUTE_WITH_SHUFFLES)
  endif()
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
  set(PLATFORM_LINK_FLAGS "")
  add_definitions(/bigobj)
else()
  if(APPLE)
    set(PLATFORM_LINK_FLAGS "-framework Foundation -framework OpenCL")
  else()
    set(PLATFORM_LINK_FLAGS "-static-libgcc -static-libstdc++")
  endif()

  if(${USING_ASAN_INT})
    set(PLATFORM_LINK_FLAGS
        "${PLATFORM_LINK_FLAGS} -fsanitize=address,undefined,integer")
  elseif(${USING_ASAN})
    set(PLATFORM_LINK_FLAGS
        "${PLATFORM_LINK_FLAGS} -fsanitize=address,undefined")
  elseif(${USING_TSAN})
    set(PLATFORM_LINK_FLAGS "${PLATFORM_LINK_FLAGS} -fsanitize=thread")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      set(PLATFORM_LINK_FLAGS
          "${PLATFORM_LINK_FLAGS} -fsanitize-blacklist=${PROJECT_SOURCE_DIR}/tsan_clang_blacklist"
      )
    endif()
  endif()
  if(NANO_FUZZER_TEST)
    set(PLATFORM_LINK_FLAGS "${PLATFORM_LINK_FLAGS} -fsanitize=fuzzer-no-link")
  endif()
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PLATFORM_LINK_FLAGS}")

if(NANO_SECURE_RPC OR RAIBLOCKS_SECURE_RPC)
  find_package(OpenSSL 1.0 REQUIRED)
  include_directories(${OPENSSL_INCLUDE_DIR})
  add_definitions(-DNANO_SECURE_RPC)
  message("OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
  message("OpenSSL libraries: ${OPENSSL_LIBRARIES}")
  message("OpenSSL lib: ${OPENSSL_SSL_LIBRARY}")
  message("Crypto lib: ${OPENSSL_CRYPTO_LIBRARY}")
else()
  set(OPENSSL_LIBRARIES "")
endif()

include_directories(${CMAKE_SOURCE_DIR})

if(WIN32
   AND NANO_TEST
   AND NANO_SHARED_BOOST)
  message(
    SEND_ERROR
      " Linking errors occur if NANO_SHARED_BOOST is used with tests on Windows"
      " Disable NANO_SHARED_BOOST or NANO_TEST on Windows")
  set(NANO_SHARED_BOOST)
endif()

set(NANO_SHARED_BOOST
    OFF
    CACHE BOOL "Build Nano with shared boost")

if(NANO_SHARED_BOOST)
  set(Boost_USE_STATIC_LIBS OFF)
  set(Boost_USE_STATIC_RUNTIME OFF)
  set(Boost_NO_BOOST_CMAKE ON)
  add_definitions(-DBOOST_ALL_DYN_LINK -DBoost_ALL_NO_LIB)
else()
  set(Boost_USE_STATIC_LIBS ON)
endif()
set(Boost_USE_MULTITHREADED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

find_package(Boost 1.70.0 REQUIRED COMPONENTS filesystem log log_setup thread
                                              program_options system)

# RocksDB
include_directories(rocksdb/include)
set(USE_RTTI
    ON
    CACHE BOOL "")
set(WITH_GFLAGS
    OFF
    CACHE BOOL "")
set(WITH_TESTS
    OFF
    CACHE BOOL "")
set(WITH_BENCHMARK_TOOLS
    OFF
    CACHE BOOL "")
set(ROCKSDB_BUILD_SHARED
    OFF
    CACHE BOOL "")
set(WITH_CORE_TOOLS
    ${NANO_ROCKSDB_TOOLS}
    CACHE BOOL "" FORCE)
set(WITH_TOOLS
    ${NANO_ROCKSDB_TOOLS}
    CACHE BOOL "" FORCE)
if(ENABLE_AVX2)
  set(PORTABLE
      OFF
      CACHE BOOL "" FORCE)
else()
  set(PORTABLE
      ON
      CACHE BOOL "" FORCE)
endif()
add_subdirectory(rocksdb EXCLUDE_FROM_ALL)

include_directories(cpptoml/include)
add_subdirectory(crypto/ed25519-donna)

add_subdirectory(nano/ipc_flatbuffers_lib)
add_subdirectory(nano/ipc_flatbuffers_test)

set(UPNPC_BUILD_SHARED
    OFF
    CACHE BOOL "")
add_subdirectory(miniupnp/miniupnpc EXCLUDE_FROM_ALL)

set(BUILD_SHARED
    OFF
    CACHE BOOL "")
set(BUILD_TESTING
    OFF
    CACHE BOOL "")
set(USE_INTERMEDIATE_OBJECTS_TARGET
    OFF
    CACHE BOOL "")
set(CRYPTOPP_EXTRA "")
if(WIN32)
  set(CRYPTOPP_EXTRA crypto/cryptopp/x64dll.asm crypto/cryptopp/x64masm.asm)
  enable_language(ASM)
  enable_language(ASM_MASM)
  # similar to SSE2 settings
  add_definitions(-DCRYPTOPP_DISABLE_SSSE3 -DCRYPTOPP_DISABLE_AESNI)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64"
       AND (NANO_SIMD_OPTIMIZATIONS OR RAIBLOCKS_SIMD_OPTIMIZATIONS))
  set(CRYPTOPP_EXTRA
      crypto/cryptopp/crc_simd.cpp crypto/cryptopp/gcm_simd.cpp
      crypto/cryptopp/gf2n_simd.cpp crypto/cryptopp/neon_simd.cpp)
  add_definitions(-DCRYPTOPP_NO_CPU_FEATURE_PROBES)
endif()
# Some Clang cannot handle mixed asm with positional arguments, where the body
# is Intel style with no prefix and the templates are AT&T style. See:
# crypto/cryptopp/config.h Also see https://bugs.llvm.org/show_bug.cgi?id=39895
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_definitions(-DCRYPTOPP_DISABLE_MIXED_ASM -DCRYPTOPP_DISABLE_ASM)
  message(
    "CryptoPP with disabled ASM for ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
  )
endif()
add_definitions(-DCRYPTOPP_DISABLE_SHANI)
# Fix failing builds after commit
# https://github.com/weidai11/cryptopp/commit/df9fa62205f2d341e2b1b26595a3a1b6377c60c5
add_definitions(-DCRYPTOPP_DISABLE_CLMUL)

set(CRYPTOPP_LIBRARY cryptopp)
add_library(
  cryptopp
  crypto/cryptopp/algparam.cpp
  crypto/cryptopp/allocate.cpp
  crypto/cryptopp/asn.cpp
  crypto/cryptopp/basecode.cpp
  crypto/cryptopp/cpu.cpp
  crypto/cryptopp/cryptlib.cpp
  crypto/cryptopp/default.cpp
  crypto/cryptopp/des.cpp
  crypto/cryptopp/dessp.cpp
  crypto/cryptopp/dll.cpp
  crypto/cryptopp/ec2n.cpp
  crypto/cryptopp/ecp.cpp
  crypto/cryptopp/filters.cpp
  crypto/cryptopp/fips140.cpp
  crypto/cryptopp/gcm.cpp
  crypto/cryptopp/gf2n.cpp
  crypto/cryptopp/gfpcrypt.cpp
  crypto/cryptopp/hex.cpp
  crypto/cryptopp/hmac.cpp
  crypto/cryptopp/hrtimer.cpp
  crypto/cryptopp/integer.cpp
  crypto/cryptopp/iterhash.cpp
  crypto/cryptopp/misc.cpp
  crypto/cryptopp/modes.cpp
  crypto/cryptopp/mqueue.cpp
  crypto/cryptopp/nbtheory.cpp
  crypto/cryptopp/oaep.cpp
  crypto/cryptopp/osrng.cpp
  crypto/cryptopp/pubkey.cpp
  crypto/cryptopp/queue.cpp
  crypto/cryptopp/randpool.cpp
  crypto/cryptopp/rdtables.cpp
  crypto/cryptopp/rijndael.cpp
  crypto/cryptopp/rijndael_simd.cpp
  crypto/cryptopp/rng.cpp
  crypto/cryptopp/sha.cpp
  crypto/cryptopp/sha_simd.cpp
  crypto/cryptopp/simple.cpp
  crypto/cryptopp/sse_simd.cpp
  ${CRYPTOPP_EXTRA})

if(WIN32 OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86(_64)?)$")
  set(ARGON_CORE crypto/phc-winner-argon2/src/opt.c)
else()
  set(ARGON_CORE crypto/phc-winner-argon2/src/ref.c)
endif()

add_library(
  argon2
  crypto/phc-winner-argon2/src/argon2.c
  crypto/phc-winner-argon2/include/argon2.h
  crypto/phc-winner-argon2/src/core.c
  crypto/phc-winner-argon2/src/thread.c
  crypto/phc-winner-argon2/src/encoding.c
  ${ARGON_CORE})

target_include_directories(argon2 PUBLIC crypto/phc-winner-argon2/include)
target_include_directories(argon2 PUBLIC crypto/phc-winner-argon2/src)
target_include_directories(argon2 PUBLIC crypto/blake2)

add_library(lmdb lmdb/libraries/liblmdb/lmdb.h lmdb/libraries/liblmdb/mdb.c
                 lmdb/libraries/liblmdb/midl.c)

if(WIN32)
  target_link_libraries(lmdb ntdll)
endif()

if(WIN32)
  set(BLAKE2_IMPLEMENTATION "crypto/blake2/blake2b.c")
else()
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86(_64)?)$")
    set(BLAKE2_IMPLEMENTATION "crypto/blake2/blake2b.c")
  else()
    set(BLAKE2_IMPLEMENTATION "crypto/blake2/blake2b-ref.c")
  endif()
endif()

add_library(blake2 crypto/blake2/blake2-config.h crypto/blake2/blake2-impl.h
                   crypto/blake2/blake2.h ${BLAKE2_IMPLEMENTATION})

target_compile_definitions(blake2 PRIVATE -D__SSE2__)

if(NANO_POW_SERVER)
  add_subdirectory(nano-pow-server)
endif()
add_subdirectory(nano/crypto_lib)
add_subdirectory(nano/secure)
add_subdirectory(nano/lib)
add_subdirectory(nano/node)
add_subdirectory(nano/nano_node)
add_subdirectory(nano/rpc)
add_subdirectory(nano/nano_rpc)

if(NANO_FUZZER_TEST)
  if(NOT WIN32)
    add_subdirectory(nano/fuzzer_test)
  else()
    message(WARNING "Fuzzing is not supported on Windows")
  endif()
endif()

if(NANO_TEST OR RAIBLOCKS_TEST)
  find_package(Boost 1.70.0 REQUIRED COMPONENTS coroutine context)
  if(WIN32)
    if(MSVC_VERSION)
      if(MSVC_VERSION GREATER_EQUAL 1910)
        add_definitions(-DGTEST_LANG_CXX11=1)
        add_definitions(-DGTEST_HAS_TR1_TUPLE=0)
      endif()
    endif()
    set(gtest_force_shared_crt ON)
  else()
    set(gtest_force_shared_crt OFF)
  endif()

  add_subdirectory(nano/load_test)

  # FIXME: This fixes googletest GOOGLETEST_VERSION requirement
  set(GOOGLETEST_VERSION 1.11.0)
  add_subdirectory(gtest/googletest)
  # FIXME: This fixes gtest include directories without modifying gtest's
  # CMakeLists.txt. Ideally we should use GTest::GTest and GTest::Main as
  # dependencies but it requires building gtest differently
  set_target_properties(
    gtest PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                     "${CMAKE_SOURCE_DIR}/gtest/googletest/include")

  add_subdirectory(nano/test_common)
  add_subdirectory(nano/core_test)
  add_subdirectory(nano/rpc_test)
  add_subdirectory(nano/slow_test)
  add_custom_target(
    build_tests
    COMMAND echo "BATCH BUILDING TESTS"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS core_test load_test rpc_test nano_node nano_rpc)

  add_custom_target(
    run_tests
    COMMAND ${PROJECT_SOURCE_DIR}/ci/test.sh ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    DEPENDS build_tests)
endif()
if(NANO_GUI OR RAIBLOCKS_GUI)
  install(FILES ${PROJECT_BINARY_DIR}/config-node.toml.sample DESTINATION .)
  install(FILES ${PROJECT_BINARY_DIR}/config-rpc.toml.sample DESTINATION .)
  if(WIN32)
    set(PLATFORM_QT_PACKAGES WinExtras)
  else()
    set(PLATFORM_QT_PACKAGES)
  endif()

  find_package(Qt5 COMPONENTS Core Gui Widgets Test ${PLATFORM_QT_PACKAGES})

  add_library(qt nano/qt/qt.cpp nano/qt/qt.hpp)

  target_link_libraries(
    qt
    node
    secure
    nano_lib
    test_common
    libminiupnpc-static
    Qt5::Gui
    Qt5::Widgets)

  target_compile_definitions(
    qt PRIVATE -DTAG_VERSION_STRING=${TAG_VERSION_STRING}
               -DGIT_COMMIT_HASH=${GIT_COMMIT_HASH})

  if(WIN32)
    set(PLATFORM_GUI_TARGET_PROPERTIES WIN32)
  else()
    set(PLATFORM_GUI_TARGET_PROPERTIES "")
  endif()

  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(PLATFORM_WALLET_SOURCE nano/nano_wallet/plat/default/icon.cpp)
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(PLATFORM_WALLET_SOURCE nano/nano_wallet/plat/windows/icon.cpp Nano.rc)
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(PLATFORM_WALLET_SOURCE nano/nano_wallet/plat/default/icon.cpp)
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(PLATFORM_WALLET_SOURCE nano/nano_wallet/plat/default/icon.cpp)
  else()
    error("Unknown platform: ${CMAKE_SYSTEM_NAME}")
  endif()

  qt5_add_resources(RES resources.qrc)

  add_executable(
    nano_wallet ${PLATFORM_GUI_TARGET_PROPERTIES} ${PLATFORM_WALLET_SOURCE}
                nano/nano_wallet/entry.cpp nano/nano_wallet/icon.hpp ${RES})

  target_link_libraries(nano_wallet rpc node qt)

  if(WIN32)
    target_link_libraries(nano_wallet Qt5::WinExtras)
    # nano_wallet.com executable for Windows console
    add_executable(nano_wallet_com nano/nano_wallet/entry_com.cpp)
    target_link_libraries(nano_wallet_com node)
    set_target_properties(
      nano_wallet_com
      PROPERTIES COMPILE_FLAGS "-DBOOST_ASIO_HAS_STD_ARRAY=1"
                 OUTPUT_NAME "nano_wallet"
                 SUFFIX ".com")
  endif()

  add_executable(qt_system nano/qt_system/entry.cpp)

  target_link_libraries(qt_system qt node Qt5::Gui Qt5::Widgets)

  set_target_properties(
    qt nano_wallet qt_system
    PROPERTIES COMPILE_FLAGS "-DQT_NO_KEYWORDS -DBOOST_ASIO_HAS_STD_ARRAY=1")

  if(NANO_TEST OR RAIBLOCKS_TEST)
    add_executable(qt_test nano/qt_test/entry.cpp nano/qt_test/qt.cpp)

    target_link_libraries(
      qt_test
      node
      secure
      test_common
      gtest
      gtest_main
      qt
      Qt5::Test)

    set_target_properties(
      qt_test PROPERTIES COMPILE_FLAGS
                         "-DQT_NO_KEYWORDS -DBOOST_ASIO_HAS_STD_ARRAY=1")
    add_dependencies(build_tests qt_test)
  endif()

  if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    configure_file(${CMAKE_SOURCE_DIR}/Info.plist.in
                   ${CMAKE_SOURCE_DIR}/Info.plist @ONLY)
    install(TARGETS nano_wallet
            DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents/MacOS)
    install(TARGETS nano_node
            DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents/MacOS)
    install(TARGETS nano_rpc
            DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents/MacOS)
    install(FILES Info.plist DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents)
    install(FILES qt.conf
            DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents/Resources)
    install(DIRECTORY ${Qt5_DIR}/../../QtCore.framework
            DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents/Frameworks)
    install(DIRECTORY ${Qt5_DIR}/../../QtDBus.framework
            DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents/Frameworks)
    install(DIRECTORY ${Qt5_DIR}/../../QtGui.framework
            DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents/Frameworks)
    install(DIRECTORY ${Qt5_DIR}/../../QtPrintSupport.framework
            DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents/Frameworks)
    install(DIRECTORY ${Qt5_DIR}/../../QtTest.framework
            DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents/Frameworks)
    install(DIRECTORY ${Qt5_DIR}/../../QtWidgets.framework
            DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents/Frameworks)
    install(FILES "${Qt5_DIR}/../../../plugins/platforms/libqcocoa.dylib"
            DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents/PlugIns/platforms)
    if(NANO_SHARED_BOOST)
      foreach(boost_lib IN LISTS Boost_LIBRARIES)
        string(REGEX MATCH "(.+/.*boost_[^-]+)" boost_lib_name ${boost_lib})
        set(boost_dll "${CMAKE_MATCH_1}")
        if(${boost_dll} MATCHES "boost")
          install(FILES ${boost_dll}
                  DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents/boost/lib)
        endif()
      endforeach(boost_lib)
    endif()
    if(NANO_POW_SERVER)
      install(TARGETS nano_pow_server
              DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents/MacOS)
      install(DIRECTORY ${PROJECT_SOURCE_DIR}/nano-pow-server/public
              DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents/MacOS)
    endif()
    install(FILES Nano.icns
            DESTINATION ${NANO_OSX_PACKAGE_NAME}.app/Contents/Resources)
  elseif(WIN32)
    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
      set(WIN_REDIST vc_redist.x64.exe)
    else()
      set(WIN_REDIST vc_redist.x86.exe)
    endif()
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
        "ExecWait '\\\"$INSTDIR\\\\${WIN_REDIST}\\\" /quiet /norestart'
												WriteRegDWORD HKLM 'SYSTEM\\\\CurrentControlSet\\\\Services\\\\EventLog\\\\Nano\\\\Nano${NANO_PREFIX}' 'TypesSupported' '0x7'
												WriteRegStr HKLM 'SYSTEM\\\\CurrentControlSet\\\\Services\\\\EventLog\\\\Nano\\\\Nano${NANO_PREFIX}' 'EventMessageFile' '$INSTDIR\\\\nano_wallet.exe'"
    )
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
        "DeleteRegKey HKLM 'SYSTEM\\\\CurrentControlSet\\\\Services\\\\EventLog\\\\Nano${NANO_PREFIX}'"
    )
    set(CPACK_NSIS_MUI_ICON ${CMAKE_SOURCE_DIR}/Nano.ico)
    set(CPACK_NSIS_DISPLAY_NAME "Nano${NANO_PREFIX}")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://nano.org")
    set(CPACK_NSIS_CONTACT "info@nano.org")
    set(CPACK_NSIS_MENU_LINKS "nano_wallet.exe" "Nano Wallet"
                              "https://nano.org" "Nano website")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    get_target_property(Qt5WindowsPlugin Qt5::QWindowsIntegrationPlugin
                        LOCATION)
    get_filename_component(Qt5_bin_DIR ${Qt5_DIR}/../../../bin ABSOLUTE)
    install(TARGETS nano_wallet DESTINATION .)
    install(TARGETS nano_wallet_com DESTINATION .)
    if(NANO_SHARED_BOOST)
      foreach(boost_lib IN LISTS Boost_LIBRARIES)
        if(${CMAKE_BUILD_TYPE} MATCHES "Rel")
          string(REGEX MATCH "(.+/.*boost_[^-]+-.+-mt-x64.+\)(.lib|a)"
                       boost_lib_name ${boost_lib})
          set(boost_dll "${CMAKE_MATCH_1}.dll")
          if(${boost_dll} MATCHES "boost")
            install(FILES ${boost_dll} DESTINATION .)
          endif()
        else()
          string(REGEX MATCH "(.+/.*boost_[^-]+-.+-mt-.+-x64.+\)(.lib|a)"
                       boost_lib_name ${boost_lib})
          set(boost_dll "${CMAKE_MATCH_1}.dll")
          if(${boost_dll} MATCHES "boost")
            install(FILES ${boost_dll} DESTINATION .)
          endif()
        endif()
      endforeach(boost_lib)
    endif()
    if(NANO_POW_SERVER)
      install(TARGETS nano_pow_server DESTINATION .)
      install(DIRECTORY ${PROJECT_SOURCE_DIR}/nano-pow-server/public
              DESTINATION .)
    endif()
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${WIN_REDIST} DESTINATION .)
    install(FILES ${Qt5_bin_DIR}/libGLESv2.dll DESTINATION .)
    install(FILES ${Qt5_bin_DIR}/Qt5Core.dll DESTINATION .)
    install(FILES ${Qt5_bin_DIR}/Qt5Gui.dll DESTINATION .)
    install(FILES ${Qt5_bin_DIR}/Qt5Widgets.dll DESTINATION .)
    install(FILES ${Qt5_bin_DIR}/Qt5WinExtras.dll DESTINATION .)
    install(FILES ${Qt5WindowsPlugin} DESTINATION platforms)
  else()
    set(CPACK_GENERATOR "TBZ2;DEB")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS qt5-default)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "russel@nano.org")
    install(TARGETS nano_wallet RUNTIME DESTINATION ./bin)
    if(NANO_SHARED_BOOST)
      foreach(boost_lib IN LISTS Boost_LIBRARIES)
        string(REGEX MATCH "(.+/.*boost_[^-]+)" boost_lib_name ${boost_lib})
        set(boost_dll "${CMAKE_MATCH_1}.${Boost_VERSION_STRING}")
        if(${boost_dll} MATCHES "boost")
          install(FILES ${boost_dll} DESTINATION ./lib)
        endif()
      endforeach(boost_lib)
    endif()
    if(NANO_POW_SERVER)
      install(TARGETS nano_pow_server DESTINATION ./bin)
      install(DIRECTORY ${PROJECT_SOURCE_DIR}/nano-pow-server/public
              DESTINATION ./bin)
    endif()
    set(DEBIAN_POSTINST postinst.in)
    set(DEBIAN_POSTRM postrm.in)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/debian-control/${DEBIAN_POSTINST}
                   ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/postinst)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/debian-control/${DEBIAN_POSTRM}
                   ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/postrm)
    file(
      COPY ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/postinst
           ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/postrm
      DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/debian-control/.
      FILE_PERMISSIONS
        OWNER_READ
        OWNER_WRITE
        OWNER_EXECUTE
        GROUP_READ
        GROUP_EXECUTE
        WORLD_READ
        WORLD_EXECUTE)
    install(FILES etc/systemd/${NANO_SERVICE} DESTINATION ./extras/systemd/.)
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_CURRENT_BINARY_DIR}/debian-control/postinst;${CMAKE_CURRENT_BINARY_DIR}/debian-control/postrm"
    )
  endif()
endif()

if(COVERAGE)
  add_subdirectory(coverage)
endif()

set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
include(CPack)
