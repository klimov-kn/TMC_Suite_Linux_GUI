#!/bin/bash
# Снимки всех диалогов всех программ — для проверки читаемости и размеров.
# Результат: tests/dialogs/<программа>/dialog_<номер>.png
set -u
cd "$(dirname "$0")/../.."
BUILD="${BUILD:-build}"

for app in tmcros tmcgrout tmc_dn fieldview planarrt_h planarrt_x; do
    tool="$BUILD/bin/${app}_shot"
    [ -x "$tool" ] || { echo "$app: средство снимка не собрано"; continue; }

    out="tests/dialogs/$app"
    rm -rf "$out" && mkdir -p "$out"
    work="/tmp/dlg_$app"
    rm -rf "$work"

    # FieldView рисует через OpenGL: без экрана буфера кадра нет.
    if [ "$app" = "fieldview" ]; then
        QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 "$tool" --dialogs "$work" 2>/dev/null | tail -1
    else
        QT_QPA_PLATFORM=offscreen "$tool" --dialogs "$work" 2>/dev/null | tail -1
    fi

    cp "$work"/*.png "$out"/ 2>/dev/null
    echo "  $app: $(ls "$out" 2>/dev/null | wc -l) окон"
done

echo "всего снимков: $(find tests/dialogs -name '*.png' | wc -l)"
