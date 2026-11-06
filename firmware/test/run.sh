#!/bin/sh
# Build and run the host tests with address and undefined-behaviour sanitizers.
set -e
cd "$(dirname "$0")/.."
mkdir -p build/test
cc -std=c11 -g -O1 -Wall -Wextra -Wno-unused-parameter -fsanitize=address,undefined \
    -DFF_USE_MKFS=1 -Iapp -Itest -Ithird_party/fatfs \
    app/csv.c app/logger.c app/policy.c app/ubx.c \
    test/ram_disk.c test/test_app.c third_party/fatfs/ff.c \
    -o build/test/test_app
build/test/test_app
