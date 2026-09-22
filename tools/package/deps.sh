#!/bin/bash
# Список пакетов, от которых зависят собранные программы.
# Используется при сборке .deb; выведен отдельно, чтобы его можно было проверить.
set -u
cd "$(dirname "$0")/../.."
BUILD="${BUILD:-build}"
APPS="tmcros tmcgrout tmc_dn fieldview planarrt_h planarrt_x"

for app in $APPS; do
    bin="$BUILD/bin/$app"
    [ -x "$bin" ] || continue
    ldd "$bin" 2>/dev/null | awk '{print $3}' | grep '^/'
done | sort -u | while read -r lib; do readlink -f "$lib"; done | sort -u \
  | xargs -r dpkg -S 2>/dev/null | cut -d: -f1 | tr ',' '\n' \
  | sed 's/^ *//; s/ *$//' | sort -u
