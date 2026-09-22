#!/bin/bash
# Сверка результата счёта с эталоном Windows-версии.
#
# Берём задание и рядом лежащий эталонный выходной файл из образцов
# Windows-проекта, считаем нашей сборкой и сравниваем.
#
# Строка «!File name ...» в заголовке содержит полный путь и на другой машине
# отличается всегда — она из сравнения исключается. Всё остальное, включая
# окончания строк CRLF, должно совпадать байт в байт.
#
#   [BUILD=build-compat] tools/dev/compare_kernel.sh <программа> <имя задания>
#   пример: tools/dev/compare_kernel.sh planarrt_h horn
set -u
cd "$(dirname "$0")/../.."
BUILD="${BUILD:-build}"
APP="${1:-planarrt_h}"
NAME="${2:-horn}"

case "$APP" in
    planarrt_h) SRC="../TMC_Suite/samples/DEMO/PlanarRT_H" ;;
    planarrt_x) SRC="../TMC_Suite/samples/DEMO/PlanarRT_X" ;;
    *) echo "не знаю, где образцы для $APP"; exit 1 ;;
esac

WORK=/tmp/tmc_cmp_$APP
rm -rf "$WORK" && mkdir -p "$WORK"
cp "$SRC"/* "$WORK"/ 2>/dev/null

REF="$WORK/$NAME.t.reference"
[ -f "$WORK/$NAME.t" ] || { echo "нет эталона $NAME.t в $SRC"; exit 1; }
mv "$WORK/$NAME.t" "$REF"
echo "эталон: $(stat -c %s "$REF") байт"

# Ядро кладём рядом с заданием: обмен идёт по диску Linux.
cp "$BUILD/bin/${APP}_runcheck" "$WORK"/ || exit 1
cd "$WORK"
echo "считаем $NAME.tpl ..."
START=$(date +%s)
stdbuf -o0 timeout 1800 "./${APP}_runcheck" "$NAME.tpl" 2>&1 | tail -6
echo "время: $(( $(date +%s) - START )) c"

[ -f "$NAME.t" ] || { echo "результат $NAME.t не создан"; exit 1; }
echo "результат: $(stat -c %s "$NAME.t") байт"

# Сравниваем без строки с полным путём к файлу.
grep -v "^!File name" "$REF"     > /tmp/ref.clean
grep -v "^!File name" "$NAME.t"  > /tmp/our.clean

if cmp -s /tmp/ref.clean /tmp/our.clean; then
    echo "СОВПАДАЕТ с эталоном байт в байт (кроме строки с путём)"
    exit 0
fi

TOTAL=$(wc -l < /tmp/ref.clean)
DIFF=$(diff /tmp/ref.clean /tmp/our.clean | grep -c "^<")
echo "РАСХОЖДЕНИЕ: строк с отличиями $DIFF из $TOTAL"
echo "первые отличия:"
diff /tmp/ref.clean /tmp/our.clean | head -8
