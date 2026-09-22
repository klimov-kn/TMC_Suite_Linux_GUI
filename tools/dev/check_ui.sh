#!/bin/bash
# Проверка работы окон: нажать все команды, изменить размер, щёлкнуть по графику.
# Показывает зависания (со стеком) и падения при выходе.
#
#   [BUILD=build-final] tools/dev/check_ui.sh
set -u
cd "$(dirname "$0")/../.."
BUILD="${BUILD:-build}"
FAIL=0

run_one() {
    local app="$1" marker="$2"
    local tool="$BUILD/bin/${app}_uitest"
    [ -x "$tool" ] || { echo "  $app: проверка не собрана"; return; }

    local work="/tmp/ui_$app"
    rm -rf "$work" && mkdir -p "$work"
    cp -r "samples/$app/." "$work"/ 2>/dev/null

    local doc=""
    for f in "$work"/*; do
        [ -f "$f" ] || continue
        if head -c 40 "$f" 2>/dev/null | grep -q "$marker"; then doc="$f"; break; fi
    done

    # Проверяем в НАСТОЯЩЕМ графическом режиме: захват мыши и перетаскивание
    # дочерних окон в режиме без экрана не воспроизводятся, а именно там
    # возникали зависания.
    local platform=xcb software=
    if [ "$app" = "fieldview" ]; then software=1; fi

    QT_QPA_PLATFORM=$platform LIBGL_ALWAYS_SOFTWARE=$software \
        TMC_NO_PLACEMENT=1 timeout 300 "$tool" "$doc" 20 > "$work/log" 2>&1
    local code=$?

    if [ "$code" -eq 0 ]; then
        echo "  ОК    $app: команд $(grep -c '  команда' "$work/log"), зависаний и падений нет"
    else
        echo "  СБОЙ  $app: код $code"
        grep -A6 "ЗАВИСАНИЕ" "$work/log" | head -8 | sed 's/^/        /'
        tail -3 "$work/log" | sed 's/^/        /'
        FAIL=1
    fi
}

echo "Проверка работы окон:"
run_one tmcros    '#TMCGROTS'
run_one tmcgrout  '#TMCGROUT'
run_one tmc_dn    '#TMCGROTS'
run_one fieldview '#TMC_GraphicsOutputFieldFile'

echo
[ "$FAIL" -eq 0 ] && echo "ИТОГ: окна работают" || echo "ИТОГ: есть сбои"
exit $FAIL
