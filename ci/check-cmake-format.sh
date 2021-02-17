#!/bin/bash

if ! command -v cmake-format &>/dev/null; then
    echo "pip install cmake-format and try again"
    exit 1
fi
REPO_ROOT=$(git rev-parse --show-toplevel)
cd "${REPO_ROOT}"
find "${REPO_ROOT}" -iwholename "${REPO_ROOT}/nano/*/CMakeLists.txt" -o -iwholename "${REPO_ROOT}/CMakeLists.txt" -o -iwholename "${REPO_ROOT}/coverage/CMakeLists.txt" | xargs cmake-format --check &>.cmake_format_check
if [[ $(wc -l .cmake_format_check | cut -f1 -d ' ') != 0 ]]; then
    echo
    echo "Code formatting differs from expected - please run \n\t'bash ci/cmake-format-all.sh'"
    RET=1
fi
rm -fr .cmake_format_check
exit ${RET-0}
