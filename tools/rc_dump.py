#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Чтение ресурсов MFC (.rc) Windows-версии в читаемый вид.

Ресурсы описывают интерфейс эталона: меню, панели инструментов, диалоги,
акселераторы, строковые таблицы. Файлы в CP1251, поэтому напрямую их читать
неудобно — скрипт перекодирует в UTF-8 и раскладывает по разделам.

Запуск:
    python tools/rc_dump.py <путь к .rc> [раздел]

Разделы: menu, toolbar, dialog, accel, string, all (по умолчанию — оглавление).
Пример:
    python tools/rc_dump.py ../TMC_Suite/src/viewers/Tmcrtout/TMCGROUT.rc menu
"""
import io
import os
import re
import sys

SECTION_RE = re.compile(
    r'^(?P<name>[A-Za-z_0-9]+)\s+(?P<kind>MENU|DIALOGEX|DIALOG|TOOLBAR|ACCELERATORS|BITMAP|ICON)\b(?P<rest>.*)$'
)


def read_rc(path):
    with open(path, "rb") as fh:
        data = fh.read()
    # Ресурсы Windows-версии — CP1251. Ошибки не глушим: если встретится
    # что-то другое, лучше увидеть это сразу.
    return data.decode("cp1251", errors="replace")


def split_sections(text):
    """[(имя, вид, строки)] — по верхнеуровневым блокам .rc."""
    lines = text.splitlines()
    out = []
    i = 0
    while i < len(lines):
        m = SECTION_RE.match(lines[i].strip())
        if not m:
            if lines[i].strip().startswith("STRINGTABLE"):
                block, i = read_block(lines, i)
                out.append(("STRINGTABLE", "STRINGTABLE", block))
                continue
            i += 1
            continue
        name, kind = m.group("name"), m.group("kind")
        if kind in ("BITMAP", "ICON"):
            out.append((name, kind, [lines[i].strip()]))
            i += 1
            continue
        block, i = read_block(lines, i)
        out.append((name, kind, block))
    return out


def read_block(lines, i):
    """Блок от заголовка до парной END (учитывая вложенные BEGIN/END)."""
    block = [lines[i]]
    depth = 0
    started = False
    i += 1
    while i < len(lines):
        s = lines[i].strip()
        block.append(lines[i])
        if s == "BEGIN" or s.endswith("BEGIN"):
            depth += 1
            started = True
        elif s == "END":
            depth -= 1
            if started and depth <= 0:
                i += 1
                break
        i += 1
    return block, i


KIND_ALIAS = {
    "menu": ("MENU",),
    "toolbar": ("TOOLBAR",),
    "dialog": ("DIALOG", "DIALOGEX"),
    "accel": ("ACCELERATORS",),
    "string": ("STRINGTABLE",),
    "icon": ("ICON", "BITMAP"),
}


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    path = sys.argv[1]
    what = (sys.argv[2] if len(sys.argv) > 2 else "").lower()

    if not os.path.isfile(path):
        print("Нет файла:", path)
        return 1

    sections = split_sections(read_rc(path))

    if not what:
        print("Оглавление %s\n" % os.path.basename(path))
        by_kind = {}
        for name, kind, block in sections:
            by_kind.setdefault(kind, []).append((name, len(block)))
        for kind in sorted(by_kind):
            items = by_kind[kind]
            print("== %s (%d)" % (kind, len(items)))
            for name, n in items:
                print("   %-40s %d строк" % (name, n))
        print("\nПодробности: python tools/rc_dump.py <файл> menu|toolbar|dialog|accel|string")
        return 0

    kinds = KIND_ALIAS.get(what)
    if not kinds:
        # Можно передать имя конкретного ресурса.
        for name, kind, block in sections:
            if name.lower() == what:
                print("\n".join(block))
                return 0
        print("Неизвестный раздел:", what)
        return 1

    for name, kind, block in sections:
        if kind in kinds:
            print("\n".join(block))
            print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
