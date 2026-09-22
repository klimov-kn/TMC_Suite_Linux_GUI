#!/bin/bash
# Сверка снимка окна с эталоном — защита от незаметных изменений вида.
#
# Эталоны лежат в tests/reference/. Если картинка изменилась, скрипт печатает
# долю изменившихся пикселей и сохраняет новый снимок рядом с эталоном,
# чтобы можно было посмотреть глазами.
#
#   tools/dev/compare_shot.sh <имя эталона без .png> <файл задания>
#   tools/dev/compare_shot.sh --update <имя эталона без .png> <файл задания>
set -u
cd "$(dirname "$0")/../.."
# Каталог сборки можно задать переменной BUILD (по умолчанию build).
BUILD="${BUILD:-build}"

UPDATE=0
if [ "${1:-}" = "--update" ]; then UPDATE=1; shift; fi
NAME="$1"; DOC="$2"
REF="tests/reference/$NAME.png"
NEW="/tmp/$NAME.new.png"

QT_QPA_PLATFORM=offscreen $BUILD/bin/tmcros_shot "$NEW" "$DOC" 1130 700 >/dev/null 2>&1

if [ "$UPDATE" = "1" ] || [ ! -f "$REF" ]; then
    cp "$NEW" "$REF"
    echo "эталон записан: $REF"
    exit 0
fi

python3 - "$REF" "$NEW" <<'PY'
import sys, zlib, struct

def load(path):
    # Минимальное чтение PNG без сторонних библиотек: нужен только сырой растр.
    data = open(path, 'rb').read()
    pos, width, height, idat = 8, 0, 0, b''
    while pos < len(data):
        ln = struct.unpack('>I', data[pos:pos+4])[0]
        typ = data[pos+4:pos+8]
        body = data[pos+8:pos+8+ln]
        if typ == b'IHDR':
            width, height, depth, color = struct.unpack('>IIBB', body[:10])
        elif typ == b'IDAT':
            idat += body
        pos += 12 + ln
    raw = zlib.decompress(idat)
    return width, height, raw

w1, h1, r1 = load(sys.argv[1])
w2, h2, r2 = load(sys.argv[2])
if (w1, h1) != (w2, h2):
    print("размер снимка изменился: было %dx%d, стало %dx%d" % (w1, h1, w2, h2))
    sys.exit(1)
diff = sum(1 for a, b in zip(r1, r2) if a != b)
total = max(len(r1), 1)
pct = 100.0 * diff / total
if diff == 0:
    print("снимок совпадает с эталоном")
    sys.exit(0)
print("снимок отличается от эталона: %.2f%% байт растра" % pct)
print("новый снимок: %s" % sys.argv[2])
sys.exit(1)
PY
