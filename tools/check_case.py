#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Проверка регистра имён в #include.

Зачем. Разработка идёт на дисках Windows (NTFS, регистр не важен) и в WSL через
/mnt/..., где регистр тоже не важен. У заказчика — ext4, где `#include <Error1.h>`
не найдёт файл `error1.h`. Ошибка вылезет только на его машине, поэтому проверяем
заранее и на каждом шаге.

Что делает: собирает все #include из наших исходников и из снимка win_src,
резолвит их так же, как компилятор (сначала каталог файла, потом -I), и печатает
те, где имя на диске отличается регистром или где включён путь с обратным слэшем
(MSVC его принимает, g++ — нет).

Запуск:  python tools/check_case.py
Код 0 — всё чисто, 1 — есть расхождения.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WIN_SRC = os.path.join(ROOT, "src", "win_src")

# Каталоги, которые CMake отдаёт компилятору через -I.
INCLUDE_DIRS = [
    os.path.join(WIN_SRC, "include"),
    os.path.join(ROOT, "src", "compat", "include"),
    os.path.join(ROOT, "src", "compat", "case_include"),
]

SRC_EXT = (".c", ".cpp", ".cxx", ".h", ".hpp", ".inl")
INCLUDE_RE = re.compile(rb'^\s*#\s*include\s*[<"]([^">]+)[">]', re.M)

# Заголовки стандартной библиотеки и системные — не наши, регистр не проверяем.
SYSTEM = {
    "stdio.h", "stdlib.h", "string.h", "math.h", "ctype.h", "time.h", "float.h",
    "limits.h", "stdarg.h", "assert.h", "memory.h", "malloc.h", "errno.h",
    "setjmp.h", "signal.h", "locale.h", "process.h", "direct.h", "sys/types.h",
    "sys/stat.h", "fcntl.h", "share.h", "new.h", "iostream", "fstream",
    "sstream", "string", "vector", "algorithm", "cstdio", "cstdlib", "cstring",
    "cmath", "cctype", "ctime", "cfloat", "climits", "cstdarg", "cassert",
}


def index_dir(path):
    """{имя в нижнем регистре: реальное имя} для одного каталога."""
    out = {}
    if os.path.isdir(path):
        for n in os.listdir(path):
            out.setdefault(n.lower(), n)
    return out


def collect_sources():
    roots = [os.path.join(ROOT, "src")]
    for r in roots:
        for dirpath, _dirs, files in os.walk(r):
            for f in files:
                if f.lower().endswith(SRC_EXT):
                    yield os.path.join(dirpath, f)


def resolve(inc, own_dir):
    """Как компилятор: каталог файла, затем -I. Возвращает (статус, подсказка)."""
    inc_dir, inc_name = os.path.split(inc)
    for base in [own_dir] + INCLUDE_DIRS:
        d = os.path.join(base, inc_dir) if inc_dir else base
        idx = index_dir(d)
        real = idx.get(inc_name.lower())
        if real is None:
            continue
        if real == inc_name:
            return ("ok", "")
        return ("case", os.path.join(base, inc_dir, real))
    return ("missing", "")


def main():
    problems = []
    for path in collect_sources():
        with open(path, "rb") as fh:
            data = fh.read()
        own_dir = os.path.dirname(path)
        for m in INCLUDE_RE.finditer(data):
            inc = m.group(1).decode("latin1")
            if "\\" in inc:
                problems.append(("слэш", path, inc, "обратный слэш: g++ не примет"))
                inc = inc.replace("\\", "/")
            # Заголовки Qt и стандартной библиотеки к нашему снимку отношения
            # не имеют: у них нет расширения (<QPainter>, <vector>) либо они
            # перечислены в SYSTEM.
            if inc.lower() in SYSTEM or "." not in inc:
                continue
            # Заголовки, которые порождает uic из файлов .ui, на диске появляются
            # только при сборке — проверять их нечего.
            if inc.startswith("ui_"):
                continue
            # Системные заголовки в подкаталогах (sys/..., gl/...).
            if inc.startswith(("sys/", "gl/", "GL/", "linux/", "bits/")):
                continue
            status, hint = resolve(inc, own_dir)
            if status == "case":
                problems.append(("регистр", path, inc, "на диске: " + os.path.basename(hint)))
            elif status == "missing":
                # Может быть системный заголовок, которого нет в списке SYSTEM.
                if "/" not in inc and inc.lower().endswith(".h") and inc.islower():
                    continue
                problems.append(("нет файла", path, inc, ""))

    if not problems:
        print("Регистр имён в #include: расхождений нет.")
        return 0

    kinds = {}
    for kind, path, inc, hint in problems:
        kinds.setdefault(kind, []).append((path, inc, hint))
    for kind, items in kinds.items():
        print("\n== %s (%d) ==" % (kind, len(items)))
        for path, inc, hint in items:
            rel = os.path.relpath(path, ROOT)
            print("  %s: %s %s" % (rel, inc, ("-> " + hint) if hint else ""))
    print("\nВсего расхождений: %d" % len(problems))
    return 1


if __name__ == "__main__":
    sys.exit(main())
