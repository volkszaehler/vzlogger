#!/bin/bash
set -xe
SRCDIRS=(include src tests)
clang-format-20 --version
find "${SRCDIRS[@]}"  -type f -name '*.h' -o -name '*.cpp' -o -name '*.hpp' | xargs -I{} -P1 clang-format-20 -i -style=file {}
[[ -z "$(git status --porcelain "${SRCDIRS[@]}")" ]] || (git status; git diff; false)
