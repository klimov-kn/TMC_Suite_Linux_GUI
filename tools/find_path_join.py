#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Поиск склейки путей обратным слэшем внутри строковых литералов.

Разделитель каталогов встречается в исходниках Windows-версии двумя способами:
  * символом  —  `if( ch[i] == '\\' )`  — это уже разбирает tools/import_win_src.py;
  * внутри строки —  `sprintf( inf, "%s\\%s", dir, name )`  — а вот это нет.

Второй случай важнее: по такой строке собирается ИМЯ ВЫХОДНОГО ФАЙЛА и путь к
подключаемому файлу задания. На Linux обратный слэш — обычный символ имени,
поэтому файл создаётся с именем вроде `каталог\\файл.t` в родительском каталоге,
а вложенный файл задания не находится вовсе.

Скрипт показывает все такие места: что именно, в каком файле и в какой строке.
Запуск:  python tools/find_path_join.py [каталог]
"""
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Два подряд идущих байта обратного слэша в исходнике — это ОДИН символ
# разделителя в строковом литерале.
SEP = b"\\\\"

# Последовательности, где двойной слэш означает не путь, а экранирование кавычки
# внутри сообщения: их не трогаем.
SKIP_CONTEXT = (b'\\\\"', )


def scan(path):
    data = open(path, "rb").read()
    out = []
    for num, line in enumerate(data.split(b"\n"), 1):
        if SEP not in line:
            continue
        # Интересуют строки, где рядом со слэшем стоит подстановка или имя файла.
        if b'"' not in line:
            continue
        out.append((num, line.strip()))
    return out


def main():
    base = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "src", "win_src")
    total = 0
    for dirpath, _dirs, files in os.walk(base):
        if os.sep + "old" in dirpath:
            continue                     # старые копии в сборку не входят
        for name in sorted(files):
            if not name.endswith((".cpp", ".c", ".h", ".hpp")):
                continue
            path = os.path.join(dirpath, name)
            for num, line in scan(path):
                rel = os.path.relpath(path, ROOT).replace(os.sep, "/")
                text = line.decode("latin1")
                print("%s:%d  %s" % (rel, num, text[:110]))
                total += 1
    print("\nвсего строк: %d" % total)


if __name__ == "__main__":
    main()
