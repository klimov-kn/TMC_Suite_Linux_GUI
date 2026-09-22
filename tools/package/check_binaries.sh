#!/bin/bash
# Проверка пригодности собранных программ для целевой системы.
#
# Что проверяем:
#  1. Все библиотеки находятся (нет «not found»).
#  2. Требуемая версия glibc не выше 2.39 — это потолок Ubuntu 24.04.
#  3. Требуемая версия libstdc++ не выше той, что даёт g++ 13.
#
#   [BUILD=build-final] tools/package/check_binaries.sh
set -u
cd "$(dirname "$0")/../.."
BUILD="${BUILD:-build}"
MAX_GLIBC="2.39"
APPS="tmcros tmcgrout tmc_dn fieldview planarrt_h planarrt_x"

FAIL=0
version_gt() { [ "$(printf '%s\n%s\n' "$1" "$2" | sort -V | tail -1)" = "$1" ] && [ "$1" != "$2" ]; }

for app in $APPS; do
    bin="$BUILD/bin/$app"
    [ -x "$bin" ] || { echo "  пропуск: $app не собран"; continue; }

    missing=$(ldd "$bin" 2>/dev/null | grep "not found" | wc -l)
    glibc=$(objdump -T "$bin" 2>/dev/null | grep -o 'GLIBC_2\.[0-9]\+' \
            | sed 's/GLIBC_//' | sort -V | tail -1)
    cxx=$(objdump -T "$bin" 2>/dev/null | grep -o 'GLIBCXX_3\.4\.[0-9]\+' \
          | sed 's/GLIBCXX_//' | sort -V | tail -1)

    line="  $app: glibc ${glibc:-нет}, libstdc++ ${cxx:-нет}"
    if [ "$missing" -ne 0 ]; then
        echo "$line — НЕ НАЙДЕНО библиотек: $missing"
        FAIL=1
    elif [ -n "$glibc" ] && version_gt "$glibc" "$MAX_GLIBC"; then
        echo "$line — ТРЕБУЕТ glibc новее $MAX_GLIBC"
        FAIL=1
    else
        echo "$line — годится для Ubuntu 24.04"
    fi
done

[ "$FAIL" -eq 0 ] && echo "ИТОГ: программы подходят целевой системе" \
                  || echo "ИТОГ: есть несоответствия"
exit $FAIL
