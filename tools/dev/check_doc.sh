#!/bin/bash
# Проверка чтения файла задания на КОПИИ образца.
#
# Программа при открытии может дописать файл задания (так же ведёт себя
# Windows-версия), поэтому проверяем на копии в /tmp: образцы в samples/
# остаются нетронутыми.
#
#   tools/dev/check_doc.sh <каталог образца> [--dump]
#
# Файл задания определяется по первой строке (#TMCGROTS), а не по расширению:
# в имени встречается символ $, который неудобно набирать в командной строке.
set -e
cd "$(dirname "$0")/../.."
# Каталог сборки можно задать переменной BUILD (по умолчанию build).
BUILD="${BUILD:-build}"
DIR="$1"
shift || true

WORK=/tmp/tmc_check
rm -rf "$WORK" && mkdir -p "$WORK"
cp -r "$DIR"/. "$WORK"/

DOC=""
for f in "$WORK"/*; do
    [ -f "$f" ] || continue
    if head -c 9 "$f" | grep -q '#TMCGROTS'; then DOC="$f"; break; fi
done
if [ -z "$DOC" ]; then
    echo "в каталоге $DIR нет файла задания (первая строка #TMCGROTS)"
    exit 1
fi

echo "копия: $DOC"
$BUILD/bin/tmcros_doccheck "$DOC" "$@"
