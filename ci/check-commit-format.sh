#!/usr/bin/env bash

REPO_ROOT=$(git rev-parse --show-toplevel)

"${REPO_ROOT}/ci/update-clang-format"

RESULT=$(python $REPO_ROOT/ci/git-clang-format.py --diff -f --commit e387c89 --extensions "hpp,cpp")
if [ "$RESULT" != "no modified files to format" ] && [ "$RESULT" != "clang-format did not modify any files" ]; then
    python $REPO_ROOT/ci/git-clang-format.py --diff -f --commit e387c89 --extensions "hpp,cpp"
    echo
    echo "Code formatting differs from expected - please run ci/clang-format-all.sh"
    exit 1
else
    echo "clang-format passed"
    exit 0
fi
