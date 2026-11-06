#!/usr/bin/env bash
# Builds liblogicforge.so for THIS computer (not Android), so JVM tests
# (Robolectric screenshots) can call the real C++ core through JNI.
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S app/src/main/cpp -B build-host-jni -DCMAKE_BUILD_TYPE=Release > /dev/null
cmake --build build-host-jni -j
echo "built $(pwd)/build-host-jni/liblogicforge.so"
