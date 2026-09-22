#!/bin/bash
# Сверка одного выходного файла с расчётом Windows-версии.
# Строка «!File name» содержит полный путь и всегда различается — исключаем.
WIN="$1"; OUR="$2"
grep -v "^!File name" "$WIN" > /tmp/cmp_win.txt
grep -v "^!File name" "$OUR" > /tmp/cmp_our.txt
if cmp -s /tmp/cmp_win.txt /tmp/cmp_our.txt; then
    echo "СОВПАДАЕТ байт в байт: $(basename "$OUR") ($(stat -c %s "$OUR") байт)"
    exit 0
fi
TOTAL=$(wc -l < /tmp/cmp_win.txt)
NDIFF=$(diff /tmp/cmp_win.txt /tmp/cmp_our.txt | grep -c '^<')
echo "ОТЛИЧИЯ: $NDIFF строк из $TOTAL"
diff /tmp/cmp_win.txt /tmp/cmp_our.txt | head -10
