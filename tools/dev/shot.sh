#!/bin/bash
# Снимок окна программы без участия человека (для проверки внешнего вида).
#
#   tools/dev/shot.sh <каталог образца> <куда.png> [ширина] [высота]
#
# Работает на копии образца в /tmp: программа может дописать файл задания.
set -e
cd "$(dirname "$0")/../.."
# Каталог сборки можно задать переменной BUILD (по умолчанию build).
BUILD="${BUILD:-build}"
DIR="$1"; OUT="$2"; W="${3:-1100}"; H="${4:-780}"

WORK=/tmp/tmc_shot
rm -rf "$WORK" && mkdir -p "$WORK"
cp -r "$DIR"/. "$WORK"/

DOC=""
for f in "$WORK"/*; do
    [ -f "$f" ] || continue
    if head -c 9 "$f" | grep -q '#TMCGROTS'; then DOC="$f"; break; fi
done

QT_QPA_PLATFORM=offscreen $BUILD/bin/tmcros_shot "$OUT" "$DOC" "$W" "$H" 2>&1 | grep -v propagateSizeHints || true
ls -la "$OUT"
