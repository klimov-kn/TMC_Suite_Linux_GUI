#!/bin/bash
# Проверка всех программ пакета: разбор задания и отрисовка окна.
#
# Для каждой программы берётся её образец из samples/<программа>/, копируется
# в /tmp и открывается. Образцы остаются нетронутыми.
#
#   [BUILD=build-orch] tools/dev/smoke_all.sh
set -u
cd "$(dirname "$0")/../.."
BUILD="${BUILD:-build}"

FAIL=0
ok()  { echo "    ОК    $1"; }
bad() { echo "    СБОЙ  $1"; FAIL=1; }

# программа : признак файла задания (первая строка) : расширения снимка
check_app() {
    local app="$1" marker="$2"
    echo
    echo "== $app"

    if [ ! -x "$BUILD/bin/$app" ]; then
        bad "программа не собрана"
        return
    fi

    local work="/tmp/tmc_all_$app"
    rm -rf "$work" && mkdir -p "$work"
    if [ -d "samples/$app" ]; then
        cp -r "samples/$app/." "$work"/ 2>/dev/null
    else
        echo "    (образцов нет — проверяется только запуск)"
    fi

    # Файл задания ищем по первой строке, а не по расширению: в именах
    # встречается символ $, неудобный в командной строке.
    local doc=""
    for f in "$work"/*; do
        [ -f "$f" ] || continue
        if head -c 40 "$f" 2>/dev/null | grep -q "$marker"; then doc="$f"; break; fi
    done

    if [ -n "$doc" ] && [ -x "$BUILD/bin/${app}_doccheck" ]; then
        local out
        out=$("$BUILD/bin/${app}_doccheck" "$doc" 2>&1)
        # Вид отчёта у программ разный: одни печатают «ошибка: нет», другие —
        # «Данные: прочитаны». Признаком успеха считаем любой из них.
        if echo "$out" | grep -q "ошибка         : нет\|прочитан"; then
            ok "разбор задания"
        else
            bad "разбор задания"
            echo "$out" | head -3 | sed "s/^/          /"
        fi
    fi

    if [ -x "$BUILD/bin/${app}_shot" ]; then
        local png="$work/shot.png"
        # FieldView рисует через OpenGL: в режиме без экрана буфера кадра нет,
        # поэтому ему нужен настоящий вывод (в WSL это даёт WSLg) и программный
        # растеризатор. Остальным достаточно режима без экрана.
        local platform="offscreen"
        local software=""
        if [ "$app" = "fieldview" ]; then
            platform="xcb"
            software="1"
        fi
        QT_QPA_PLATFORM="$platform" LIBGL_ALWAYS_SOFTWARE="$software" \
            timeout 120 "$BUILD/bin/${app}_shot" \
            "$png" "$doc" 1130 700 > "$work/log" 2>&1
        local code=$?
        [ "$code" = 0 ] && ok "запуск и выход (код 0)" || bad "код возврата $code"
        if [ -s "$png" ]; then
            ok "снимок окна ($(stat -c %s "$png") байт)"
        else
            bad "снимок не получен"
        fi
    fi

    if [ -d "samples/$app" ]; then
        local diff
        diff=$(diff -rq "samples/$app" "$work" 2>/dev/null | grep "^Files" | grep -v "shot.png" || true)
        [ -z "$diff" ] && ok "образцы не изменены" || echo "    ВНИМАНИЕ: изменены файлы: $diff"
    fi
}

check_app tmcros     '#TMCGROTS'
check_app tmcgrout   '#TMCGROUT'
check_app tmc_dn     '#TMCGROTS'
check_app fieldview  '#TMC_GraphicsOutputFieldFile'
check_app planarrt_h 'TPL\|#'
check_app planarrt_x 'TPL\|#'

# Связка программ: из счётного ядра открываются вьюверы по windows-именам.
echo
echo "== связка программ"
if [ -x "$BUILD/bin/tmc_launchcheck" ]; then
    if (cd "$BUILD/bin" && ./tmc_launchcheck > /tmp/launch.log 2>&1); then
        ok "все программы пакета находятся по windows-именам"
    else
        bad "перевод имён программ"
        sed 's/^/      /' /tmp/launch.log | head -8
    fi
else
    echo "    (проверка связки не собрана)"
fi

echo
[ "$FAIL" -eq 0 ] && echo "ИТОГ: все проверки пройдены" || echo "ИТОГ: есть сбои"
exit $FAIL
