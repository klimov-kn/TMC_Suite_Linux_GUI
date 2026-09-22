#!/bin/bash
# Сборка пакета .deb с программами TMC Suite для Ubuntu 24.04.
#
# Что кладём:
#   /usr/bin/<программа>                     — исполняемые файлы
#   /usr/share/applications/*.desktop        — запуск из меню рабочего стола
#   /usr/share/icons/hicolor/32x32/apps/     — значки программ
#   /usr/share/tmc-suite/samples/            — образцы файлов
#   /usr/share/tmc-suite/TMC_Suite_User_Manual.pdf — руководство
#
# Зависимости объявляются по факту: список библиотек снимается с самих
# бинарников, плюс шрифты Liberation — без них подписи на графиках имеют другую
# ширину, чем в Windows-версии.
#
#   tools/package/make_deb.sh [версия]
set -e
cd "$(dirname "$0")/../.."

VERSION="${1:-1.0.0}"
ARCH=$(dpkg --print-architecture 2>/dev/null || echo amd64)
# Собираем в каталоге на диске Linux: на разделе Windows у файлов права 777,
# а dpkg-deb требует не более 0775 для служебного каталога.
STAGE="/tmp/tmc_deb_$$"
PKG="$STAGE/tmc-suite_${VERSION}_${ARCH}"
OUT="dist/tmc-suite_${VERSION}_${ARCH}.deb"
# Каталог сборки можно задать переменной BUILD.
BIN_DIR="${BUILD:-build}/bin"

if [ ! -d "$BIN_DIR" ]; then
    echo "Сначала соберите проект: cmake --build ${BUILD:-build} -j"
    exit 1
fi

rm -rf "$PKG"
mkdir -p "$PKG/DEBIAN" \
         "$PKG/usr/bin" \
         "$PKG/usr/share/applications" \
         "$PKG/usr/share/icons/hicolor/32x32/apps" \
         "$PKG/usr/share/tmc-suite/samples"

APPS="tmcros tmcgrout tmc_dn fieldview planarrt_h planarrt_x"

# --- Программы ---------------------------------------------------------------
COUNT=0
for app in $APPS; do
    if [ -x "$BIN_DIR/$app" ]; then
        install -m 0755 "$BIN_DIR/$app" "$PKG/usr/bin/$app"
        COUNT=$((COUNT + 1))
    else
        echo "  пропуск: $app не собран"
    fi
done
echo "программ в пакете: $COUNT"

# --- Файлы запуска и значки ---------------------------------------------------
for app in $APPS; do
    [ -x "$PKG/usr/bin/$app" ] || continue
    [ -f "dist/desktop/$app.desktop" ] && \
        install -m 0644 "dist/desktop/$app.desktop" "$PKG/usr/share/applications/"
    ICON=$(ls src/apps/$app/resources/icons/*_32.png 2>/dev/null | head -1)
    [ -n "$ICON" ] && \
        install -m 0644 "$ICON" "$PKG/usr/share/icons/hicolor/32x32/apps/$app.png"
done

# --- Образцы -------------------------------------------------------------------
[ -d samples ] && cp -r samples/. "$PKG/usr/share/tmc-suite/samples/" 2>/dev/null || true

# Руководство пользователя кладём рядом с образцами: окно «О программе»
# ищет его в /usr/share/tmc-suite и открывает по ссылке.
for manual in docs/TMC_Suite_User_Manual.pdf \n              ../website/files/TMC_Suite_User_Manual.pdf; do
    if [ -f "$manual" ]; then
        install -m 0644 "$manual" "$PKG/usr/share/tmc-suite/TMC_Suite_User_Manual.pdf"
        break
    fi
done

# --- Зависимости ---------------------------------------------------------------
# Считает отдельный скрипт: берёт библиотеки из самих бинарников и спрашивает у
# dpkg, каким пакетам они принадлежат. Отдельно — чтобы список можно было
# проверить руками: tools/package/deps.sh
DEPS=$(tools/package/deps.sh | tr '\n' ',' | sed 's/,$//; s/,/, /g')
DEPS="${DEPS:+$DEPS, }fonts-liberation"

cat > "$PKG/DEBIAN/control" <<CONTROL
Package: tmc-suite
Version: $VERSION
Section: science
Priority: optional
Architecture: $ARCH
Depends: $DEPS
Maintainer: K. N. Klimov
Description: TMC Suite - электродинамическое моделирование
 Набор программ для расчёта и просмотра результатов: вьюверы S-матриц,
 графиков, диаграмм направленности и полей, а также оболочки счётных ядер.
CONTROL

chmod -R u+rwX,go+rX,go-w "$PKG"
dpkg-deb --build --root-owner-group "$PKG" >/dev/null
mkdir -p dist
cp "$PKG.deb" "$OUT"
rm -rf "$STAGE"

echo "пакет собран: $OUT ($(stat -c %s "$OUT") байт)"
dpkg-deb --info "$OUT" | sed -n '1,14p'
