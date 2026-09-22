#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Извлечение пиктограмм Windows-версии в ресурсы Qt.

Пользователь, перешедший с Windows, должен видеть на панели те же картинки.
Поэтому кнопки не рисуются заново: полоса Toolbar.bmp из MFC-ресурсов режется
на отдельные значки, каждый получает имя команды из .rc, и складывается в
ресурсы нашей программы. Значок приложения берётся из .ico.

Фон MFC-полосы — стандартный серый RGB(192,192,192); он становится прозрачным,
иначе кнопки будут с серыми заплатами на тёмной теме.

Запуск:
    python tools/extract_icons.py <имя программы> <каталог Windows-проекта>
Пример:
    python tools/extract_icons.py tmcros ../TMC_Suite/src/viewers/Tmcrtout
"""
import os
import re
import sys

try:
    from PIL import Image
except ImportError:
    print("Нужен Pillow:  pip install Pillow")
    sys.exit(1)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

MFC_BG = (192, 192, 192)      # фон полосы кнопок в ресурсах MFC
# Пробелы только горизонтальные: \s поглотил бы перевод строки и
# «съедал» кнопку, стоящую сразу после SEPARATOR.
TOOLBAR_RE = re.compile(r'^[ \t]*(BUTTON|SEPARATOR)[ \t]*(\S*)', re.M)


def find_rc(win_dir):
    for name in os.listdir(win_dir):
        if name.lower().endswith(".rc"):
            return os.path.join(win_dir, name)
    return None


def toolbar_buttons(rc_path):
    """Список ID кнопок в порядке следования (разделители пропускаем)."""
    with open(rc_path, "rb") as fh:
        text = fh.read().decode("cp1251", errors="replace")
    m = re.search(r'^\w+\s+TOOLBAR[^\n]*\n\s*BEGIN(.*?)\n\s*END', text, re.S | re.M)
    if not m:
        return []
    out = []
    for kind, ident in TOOLBAR_RE.findall(m.group(1)):
        if kind == "BUTTON" and ident:
            out.append(ident)
    return out


def to_transparent(img):
    img = img.convert("RGBA")
    px = img.load()
    w, h = img.size
    for y in range(h):
        for x in range(w):
            if px[x, y][:3] == MFC_BG:
                px[x, y] = (0, 0, 0, 0)
    return img


def for_dark_theme(img):
    """Вариант значка для тёмной темы.

    В ресурсах MFC штрихи чёрные — на светло-серой панели Windows они читаются,
    на тёмной подложке сливаются. Меняем только очень тёмные пиксели на светлый
    цвет темы; форма пиктограммы, из-за которой её и узнают, не меняется.
    Цветные пиксели (жёлтый, синий, красный) остаются как были.
    """
    img = img.convert("RGBA")
    px = img.load()
    w, h = img.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a and max(r, g, b) < 64:          # почти чёрный штрих
                px[x, y] = (0xE7, 0xEE, 0xF7, a)
            elif a and max(r, g, b) < 128 and abs(r - g) < 24 and abs(g - b) < 24:
                px[x, y] = (0x9F, 0xB0, 0xC8, a)  # серый штрих -> приглушённый
    return img


def slice_toolbar(bmp_path, ids, out_dir):
    strip = Image.open(bmp_path)
    w, h = strip.size
    if not ids:
        print("В .rc нет секции TOOLBAR — полоса не резалась")
        return 0
    cw = w // len(ids)
    if cw * len(ids) != w:
        print("Внимание: ширина %d не делится на %d кнопок нацело (кадр %d)"
              % (w, len(ids), cw))
    os.makedirs(out_dir, exist_ok=True)
    for i, ident in enumerate(ids):
        cell = strip.crop((i * cw, 0, (i + 1) * cw, h))
        cell = to_transparent(cell)
        cell.save(os.path.join(out_dir, ident + ".png"))
        for_dark_theme(cell).save(os.path.join(out_dir, ident + "_dark.png"))
    print("Кнопок сохранено: %d (кадр %dx%d)" % (len(ids), cw, h))
    return len(ids)


def convert_icons(win_dir, out_dir):
    res_dir = os.path.join(win_dir, "res")
    if not os.path.isdir(res_dir):
        return 0
    n = 0
    os.makedirs(out_dir, exist_ok=True)
    for name in os.listdir(res_dir):
        if not name.lower().endswith(".ico"):
            continue
        img = Image.open(os.path.join(res_dir, name))
        base = os.path.splitext(name)[0].lower()
        # В .ico несколько размеров; сохраняем каждый отдельным файлом.
        sizes = getattr(img, "ico", None)
        saved = []
        for size in sorted({s for s in (img.info.get("sizes") or {img.size})}):
            try:
                img.size = size
                img.load()
            except Exception:
                continue
            out = os.path.join(out_dir, "%s_%d.png" % (base, size[0]))
            img.convert("RGBA").save(out)
            saved.append(size[0])
            n += 1
        print("%s -> размеры %s" % (name, saved))
    return n


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    app = sys.argv[1].lower()
    win_dir = sys.argv[2]
    if not os.path.isdir(win_dir):
        print("Нет каталога:", win_dir)
        return 1

    out_dir = os.path.join(ROOT, "src", "apps", app, "resources", "icons")
    rc_path = find_rc(win_dir)
    if not rc_path:
        print("В каталоге нет .rc:", win_dir)
        return 1

    ids = toolbar_buttons(rc_path)
    bmp = None
    for cand in ("Toolbar.bmp", "toolbar.bmp", "TOOLBAR.BMP"):
        p = os.path.join(win_dir, "res", cand)
        if os.path.isfile(p):
            bmp = p
            break
    if bmp:
        slice_toolbar(bmp, ids, out_dir)
    else:
        print("Полоса кнопок не найдена в", os.path.join(win_dir, "res"))

    convert_icons(win_dir, out_dir)
    print("Ресурсы:", os.path.relpath(out_dir, ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main())
