#!/usr/bin/env sh

set -e
set -x

for file in tests/*.c; do
  clang -g $file -Wno-nullability-completeness -fsanitize=address -o test
  ./test
done

rm test
