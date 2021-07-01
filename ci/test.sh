#!/usr/bin/env bash

build_dir=${1-${PWD}}
if [[ ${TEST_USE_ROCKSDB-0} == 1 ]]; then
    TIMEOUT_DEFAULT=1440
else
    TIMEOUT_DEFAULT=720
fi

BUSYBOX_BASH=${BUSYBOX_BASH-0}

if [[ ${FLAVOR-_} == "_" ]]; then
    FLAVOR=""
fi

if [[ "$OSTYPE" == "darwin"* ]]; then
    TIMEOUT_CMD=gtimeout
else
    TIMEOUT_CMD=timeout
fi

set -o nounset
set -o xtrace

# Alpine doesn't offer an xvfb
xvfb_run_() {
    INIT_DELAY_SEC=3

    Xvfb :2 -screen 0 1024x768x24 &
    xvfb_pid=$!
    sleep ${INIT_DELAY_SEC}
    DISPLAY=:2 ${TIMEOUT_CMD} ${TIMEOUT_TIME_ARG} ${TIMEOUT_DEFAULT} $@
    res=${?}
    kill ${xvfb_pid}

    return ${res}
}

run_tests() {
    local tries try

    # when busybox pretends to be bash it needs different args
    #   for the timeout builtin
    if [[ "${BUSYBOX_BASH}" -eq 1 ]]; then
        TIMEOUT_TIME_ARG="-t"
    else
        TIMEOUT_TIME_ARG=""
    fi

    tries=(_initial_)

    for try in "${tries[@]}"; do
        if [ "${try}" != '_initial_' ]; then
            echo "core_test failed: ${core_test_res}, retrying (try=${try})"

            # Wait a while for sockets to be all cleaned up by the kernel
            sleep $((30 + (RANDOM % 30)))
        fi

        ${TIMEOUT_CMD} ${TIMEOUT_TIME_ARG} ${TIMEOUT_DEFAULT} ./core_test
        core_test_res=${?}
        if [ "${core_test_res}" = '0' ]; then
            break
        fi
    done

    ${TIMEOUT_CMD} ${TIMEOUT_TIME_ARG} ${TIMEOUT_DEFAULT} ./rpc_test
    rpc_test_res=${?}

    xvfb_run_ ./qt_test
    qt_test_res=${?}

    ${TIMEOUT_CMD} ${TIMEOUT_TIME_ARG} ${TIMEOUT_DEFAULT} ./load_test -s 150 -n 5
    load_test_res=${?}

    echo "Core Test return code: ${core_test_res}"
    echo "RPC  Test return code: ${rpc_test_res}"
    echo "QT Test return code: ${qt_test_res}"
    echo "Load Test return code: ${load_test_res}"
    if [[ ${core_test_res} != 0 || ${rpc_test_res} != 0 || ${qt_test_res} != 0 || ${load_test_res} != 0 ]]; then
        return 1
    else
        return 0
    fi
}

cd ${build_dir}
run_tests
