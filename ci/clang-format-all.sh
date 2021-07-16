#!/bin/bash

set -e

REPO_ROOT=$(git rev-parse --show-toplevel)
cd "${REPO_ROOT}"
./ci/update-clang-format
python ci/git-clang-format.py -f --commit e387c89 --extensions "hpp,cpp"
