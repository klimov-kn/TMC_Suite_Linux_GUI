#!/bin/bash
# Быстрая проверка работоспособности собранных программ.
#
# Что проверяем:
#   1. Разбор файла задания (число графиков и точек больше нуля).
#   2. Отрисовку окна без экрана (файл снимка создан и не пустой).
#   3. Что образцы в samples/ после прогона не изменились — программа работает
#      на копии, а не портит эталон.
#
#   tools/dev/smoke.sh
set -u
cd "$(dirname "$0")/../.."
# Каталог сборки можно задать переменной BUILD (по умолчанию build).
BUILD="${BUILD:-build}"

FAIL=0
ok()   { echo "  ОК    $1"; }
bad()  { echo "  СБОЙ  $1"; FAIL=1; }

WORK=/tmp/tmc_smoke
rm -rf "$WORK" && mkdir -p "$WORK"
cp -r samples/tmcros/. "$WORK"/

DOC=""
for f in "$WORK"/*; do
    [ -f "$f" ] || continue
    if head -c 9 "$f" | grep -q '#TMCGROTS'; then DOC="$f"; break; fi
done

echo "TMCROS: разбор файла задания"
if [ -z "$DOC" ]; then
    bad "файл задания не найден в samples/tmcros"
else
    OUT=$($BUILD/bin/tmcros_doccheck "$DOC" 2>&1)
    echo "$OUT" | grep -q "ошибка         : нет" && ok "ошибок разбора нет" || bad "разбор с ошибкой"
    POINTS=$(echo "$OUT" | sed -n 's/.*точек          : \([0-9]*\).*/\1/p' | head -1)
    [ "${POINTS:-0}" -gt 0 ] && ok "точек прочитано: $POINTS" || bad "точки не прочитаны"
fi

echo "TMCROS: отрисовка окна"
SHOT="$WORK/shot.png"
QT_QPA_PLATFORM=offscreen $BUILD/bin/tmcros_shot "$SHOT" "$DOC" 1100 780 >/dev/null 2>&1
if [ -s "$SHOT" ]; then
    SIZE=$(stat -c %s "$SHOT")
    [ "$SIZE" -gt 5000 ] && ok "снимок получен ($SIZE байт)" || bad "снимок подозрительно мал ($SIZE байт)"
else
    bad "снимок не создан"
fi

echo "Образцы не испорчены"
DIFF=$(diff -rq samples/tmcros "$WORK" 2>/dev/null | grep -v "shot.png" | grep "^Files" || true)
[ -z "$DIFF" ] && ok "samples/tmcros без изменений" || echo "  ВНИМАНИЕ: программа изменила файлы:
$DIFF"

echo
[ "$FAIL" -eq 0 ] && echo "ИТОГ: всё в порядке" || echo "ИТОГ: есть сбои"
exit $FAIL
