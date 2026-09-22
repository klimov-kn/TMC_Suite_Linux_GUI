#!/bin/bash
# Проверка TMC_DN на образце: открывает задание, снимает окно, сообщает код возврата.
# Работает на копии в /tmp — образцы не портятся.
cd "$(dirname "$0")/../.."
BUILD="${BUILD:-build}"
rm -rf /tmp/dn && mkdir /tmp/dn
cp samples/tmc_dn/* /tmp/dn/ 2>/dev/null
DOC=$(ls /tmp/dn/* | grep -E '\.(dop|\$op)$' | head -1)
echo "документ: $DOC"
QT_QPA_PLATFORM=offscreen timeout 90 "./$BUILD/bin/tmc_dn_shot" /tmp/dn/out.png "$DOC" 1130 700 > /tmp/dn.log 2>&1
echo "код возврата: $?"
grep -v -E "propagateSizeHints|libEGL|MESA|ZINK|dri2" /tmp/dn.log | head -6
ls -la /tmp/dn/out.png 2>/dev/null
