#!/bin/bash

set -e

if ! command -v cmake-format &>/dev/null; then
    echo "pip install cmake-format to continue"
    exit 1
fi
REPO_ROOT=$(git rev-parse --show-toplevel)
cd "${REPO_ROOT}"
find "${REPO_ROOT}" -iwholename "${REPO_ROOT}/nano/*/CMakeLists.txt" -o -iwholename "${REPO_ROOT}/CMakeLists.txt" -o -iwholename "${REPO_ROOT}/coverage/CMakeLists.txt" | xargs cmake-format -i
