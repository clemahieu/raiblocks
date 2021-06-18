#!/bin/bash

qt_dir=${1}

set -o errexit
set -o nounset
set -o xtrace
OS=$(uname)

# This is to prevent out of scope access in async_write from asio which is not picked up by static analysers
if [[ $(grep -rl --exclude="*asio.hpp" "asio::async_write" ./nano) ]]; then
    echo "Using boost::asio::async_write directly is not permitted (except in nano/lib/asio.hpp). Use nano::async_write instead"
    exit 1
fi

# prevent unsolicited use of std::lock_guard, std::unique_lock, std::condition_variable & std::mutex outside of allowed areas
if [[ $(grep -rl --exclude={"*random_pool.cpp","*random_pool.hpp","*random_pool_shuffle.hpp","*locks.hpp","*locks.cpp"} "std::unique_lock\|std::lock_guard\|std::condition_variable\|std::mutex" ./nano) ]]; then
    echo "Using std::unique_lock, std::lock_guard, std::condition_variable or std::mutex is not permitted (except in nano/lib/locks.hpp and non-nano dependent libraries). Use the nano::* versions instead"
    exit 1
fi

if [[ $(grep -rlP "^\s*assert \(" ./nano) ]]; then
    echo "Using assert is not permitted. Use debug_assert instead."
    exit 1
fi

# prevent unsolicited use of std::lock_guard & std::unique_lock outside of allowed areas
mkdir build
pushd build

if [[ "${RELEASE:-false}" == "true" ]]; then
    BUILD_TYPE="RelWithDebInfo"
fi

if [[ ${ASAN_INT:-0} -eq 1 ]]; then
    SANITIZERS="-DNANO_ASAN_INT=ON"
elif [[ ${ASAN:-0} -eq 1 ]]; then
    SANITIZERS="-DNANO_ASAN=ON"
elif [[ ${TSAN:-0} -eq 1 ]]; then
    SANITIZERS="-DNANO_TSAN=ON"
elif [[ ${LCOV:-0} -eq 1 ]]; then
    SANITIZERS="-DCOVERAGE=ON"
fi

ulimit -S -n 8192

if [[ "$OS" == 'Linux' ]]; then
    if clang --version && [ ${LCOV:-0} == 0 ]; then
        BACKTRACE="-DNANO_STACKTRACE_BACKTRACE=ON \
        -DBACKTRACE_INCLUDE=</tmp/backtrace.h>"
    else
        BACKTRACE="-DNANO_STACKTRACE_BACKTRACE=ON"
    fi
else
    BACKTRACE=""
fi

cmake \
-G'Unix Makefiles' \
-DACTIVE_NETWORK=nano_dev_network \
-DNANO_TEST=ON \
-DNANO_GUI=ON \
-DPORTABLE=1 \
-DNANO_WARN_TO_ERR=ON \
-DCMAKE_BUILD_TYPE=${BUILD_TYPE:-Debug} \
-DCMAKE_VERBOSE_MAKEFILE=ON \
-DBOOST_ROOT=${BOOST_ROOT:-/tmp/boost/} \
-DNANO_SHARED_BOOST=ON \
-DQt5_DIR=${qt_dir} \
-DCI_TEST="1" \
${BACKTRACE:-} \
${SANITIZERS:-} \
..

if [[ "$OS" == 'Linux' ]]; then
    if [[ ${LCOV:-0} == 1 ]]; then
        cmake --build ${PWD} --target generate_coverage -- -j2
    else
        cmake --build ${PWD} --target build_tests -k -- -j2
    fi
else
    sudo cmake --build ${PWD} --target build_tests -- -j2
fi

popd
