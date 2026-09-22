#!/bin/bash
# Сверка всех выходных файлов расчёта Windows и Linux.
#   tools/dev/ab_all.sh <каталог Windows> <каталог Linux>
set -u
cd "$(dirname "$0")/../.."
WIN="${1:-tests/ab/win}"
LIN="${2:-/tmp/gui}"
SAME=0; DIFF=0
for f in "$WIN"/*; do
    name=$(basename "$f")
    case "$name" in *.TPL|*.H|*.log) continue;; esac
    ours="$LIN/$name"
    if [ ! -f "$ours" ]; then echo "  нет у нас: $name"; DIFF=$((DIFF+1)); continue; fi
    # -a обязателен: без него grep на двоичном файле (.tt, .s) печатает
    # «Binary file ... matches» вместо содержимого, и сравниваются две
    # одинаковые заглушки — любое отличие выглядит совпадением.
    grep -a -v "^!File name" "$f"    > /tmp/ab_w.txt
    grep -a -v "^!File name" "$ours" > /tmp/ab_l.txt
    if cmp -s /tmp/ab_w.txt /tmp/ab_l.txt; then
        echo "  СОВПАДАЕТ  $name  ($(stat -c %s "$ours") байт)"
        SAME=$((SAME+1))
    else
        n=$(diff /tmp/ab_w.txt /tmp/ab_l.txt | grep -c '^<')
        echo "  ОТЛИЧИЯ    $name  — строк $n из $(wc -l < /tmp/ab_w.txt)"
        diff /tmp/ab_w.txt /tmp/ab_l.txt | head -4 | sed 's/^/      /'
        DIFF=$((DIFF+1))
    fi
done
echo "совпало: $SAME, отличий: $DIFF"
