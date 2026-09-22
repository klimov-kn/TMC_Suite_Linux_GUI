#!/bin/bash
# Сверка «Windows против Linux» на одном задании.
#
# Задание считается Windows-версией (в каталоге tests/ab/win это уже сделано),
# затем те же входные файлы считаются нашей сборкой на диске Linux, и выходные
# файлы сравниваются побайтно.
#
# Строка «!File name ...» содержит полный путь и на другой машине отличается
# всегда — из сравнения исключается. Всё остальное, включая окончания строк
# CRLF, обязано совпадать.
#
#   [BUILD=build-compat] tools/dev/ab_compare.sh
set -u
cd "$(dirname "$0")/../.."
BUILD="${BUILD:-build}"

WIN=tests/ab/win
WORK=/tmp/tmc_ab
[ -d "$WIN" ] || { echo "нет каталога с расчётом Windows: $WIN"; exit 1; }

rm -rf "$WORK" && mkdir -p "$WORK"
# На Linux берём ТОЛЬКО входные файлы, результат считаем сами.
cp "$WIN"/*.TPL "$WIN"/*.H "$WORK"/ 2>/dev/null
cp "$BUILD/bin/planarrt_h" "$WORK"/ 2>/dev/null || { echo "нет $BUILD/bin/planarrt_h"; exit 1; }

cd "$WORK"
echo "считаем на Linux (пакетный режим, рабочий каталог на диске Linux)..."
START=$(date +%s)
QT_QPA_PLATFORM=offscreen timeout 600 ./planarrt_h -Arb VOLN.TPL > run.log 2>&1
echo "код: $? , время: $(( $(date +%s) - START )) c"

cd - > /dev/null
echo
echo "сравнение выходных файлов:"
SAME=0; DIFF=0
for f in "$WIN"/*; do
    name=$(basename "$f")
    case "$name" in *.TPL|*.H) continue;; esac
    ours="$WORK/$name"
    if [ ! -f "$ours" ]; then
        echo "  НЕТ у нас: $name"
        DIFF=$((DIFF+1)); continue
    fi
    grep -v "^!File name" "$f"    > /tmp/ab_win.clean
    grep -v "^!File name" "$ours" > /tmp/ab_lin.clean
    if cmp -s /tmp/ab_win.clean /tmp/ab_lin.clean; then
        echo "  СОВПАДАЕТ  $name ($(stat -c %s "$f") байт)"
        SAME=$((SAME+1))
    else
        n=$(diff /tmp/ab_win.clean /tmp/ab_lin.clean | grep -c "^<")
        echo "  ОТЛИЧИЯ    $name — строк: $n из $(wc -l < /tmp/ab_win.clean)"
        diff /tmp/ab_win.clean /tmp/ab_lin.clean | head -4 | sed "s/^/      /"
        DIFF=$((DIFF+1))
    fi
done
echo
echo "совпало файлов: $SAME, с отличиями: $DIFF"
