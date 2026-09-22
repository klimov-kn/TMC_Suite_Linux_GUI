#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Сверка выходных файлов расчёта: Windows, Linux-консоль, Linux-интерфейс.

    python3 tools/dev/compare_three.py <каталог с результатами> [ещё каталог ...]

Каждый каталог — это результаты одной задачи, посчитанной тремя способами:
    <каталог>/windows/<задача>/   — расчёт Windows-версией (tmc_rth.exe /Arb)
    <каталог>/linux-console/<задача>/
    <каталог>/linux-gui/<задача>/

Сравниваются одноимённые файлы. Из сравнения исключается строка с ПОЛНЫМ ИМЕНЕМ
файла: её ядро записывает в заголовок, и она законно разная на разных машинах
(так же поступает tools/dev/ab_all.sh). Всё остальное обязано совпадать байт в
байт — иначе выходные файлы, посчитанные на Linux, не откроются в Windows-версии
и наоборот.
"""
import os
import re
import sys

# Заголовок содержит путь к файлу — он разный на разных машинах. В двоичных
# файлах (.s) перед строкой пути стоит её длина в двух байтах, поэтому вырезать
# надо и её: иначе файлы «различаются» только потому, что путь другой длины.
PATH_RE = re.compile(
    rb"(?:[A-Za-z]:[\\/]|/(?:home|mnt|tmp|Users)/)[^\r\n\x00]{0,300}",
    re.IGNORECASE,
)


def strip_paths(data: bytes) -> bytes:
    """Убрать из файла имена путей вместе с их длиной."""
    out = bytearray()
    pos = 0
    for m in PATH_RE.finditer(data):
        start, end = m.start(), m.end()
        # Если перед путём стоит его длина в двух байтах — убираем и её.
        if start >= 2:
            length = data[start - 2] | (data[start - 1] << 8)
            if length == end - start:
                start -= 2
        out += data[pos:start]
        out += b"<path>"
        pos = end
    out += data[pos:]
    return bytes(out)


# Заголовок двоичного файла S-матрицы хранит смещение начала данных, а оно
# зависит от длины имени файла — то есть законно разное на разных машинах.
# Поэтому у .s сверяем сами данные: они лежат в конце файла.
HEADER_RESERVE = 300


def compare_dirs(name_a: str, dir_a: str, name_b: str, dir_b: str) -> tuple:
    """Сравнить два каталога. Возвращает (совпало, отличия, пропущено)."""
    same, diff, missing = [], [], []
    if not os.path.isdir(dir_a) or not os.path.isdir(dir_b):
        return same, diff, missing

    for fn in sorted(os.listdir(dir_a)):
        pa = os.path.join(dir_a, fn)
        pb = os.path.join(dir_b, fn)
        if not os.path.isfile(pa):
            continue
        if not os.path.isfile(pb):
            missing.append(fn)
            continue
        da = strip_paths(open(pa, "rb").read())
        db = strip_paths(open(pb, "rb").read())

        if fn.lower().endswith(".s"):
            # Только данные, без заголовка со смещением. У коротких файлов
            # (одна частотная точка) заголовок занимает заметную долю, поэтому
            # отступ не больше половины файла.
            shortest = min(len(da), len(db))
            reserve = min(HEADER_RESERVE, shortest // 2)
            n = shortest - reserve
            if n > 0:
                da, db = da[-n:], db[-n:]

        if da == db:
            same.append((fn, os.path.getsize(pa)))
        else:
            # Где именно разошлось — первое несовпадение и сколько байт всего.
            n = min(len(da), len(db))
            at = next((i for i in range(n) if da[i] != db[i]), n)
            diff.append((fn, at, len(da), len(db)))
    return same, diff, missing


def main(argv) -> int:
    root = argv[1] if len(argv) > 1 else "results"
    ways = ["windows", "linux-console", "linux-gui"]

    tasks = set()
    for way in ways:
        d = os.path.join(root, way)
        if os.path.isdir(d):
            tasks.update(x for x in os.listdir(d) if os.path.isdir(os.path.join(d, x)))

    if not tasks:
        print("нечего сверять: нет ни одного каталога с результатами")
        return 1

    total_diff = 0
    for task in sorted(tasks):
        print(f"\n=== задача: {task} ===")
        for i in range(len(ways)):
            for j in range(i + 1, len(ways)):
                a, b = ways[i], ways[j]
                da = os.path.join(root, a, task)
                db = os.path.join(root, b, task)
                if not (os.path.isdir(da) and os.path.isdir(db)):
                    continue
                same, diff, missing = compare_dirs(a, da, b, db)
                mark = "ОК" if not diff and not missing else "РАЗОШЛИСЬ"
                print(f"  {a} <-> {b}: {mark} "
                      f"(совпало {len(same)}, отличий {len(diff)}, нет пары {len(missing)})")
                for fn, at, la, lb in diff:
                    print(f"      ОТЛИЧИЕ  {fn}: с байта {at}, размеры {la} и {lb}")
                for fn in missing:
                    print(f"      НЕТ ПАРЫ {fn}")
                total_diff += len(diff) + len(missing)

    print("\n" + ("ИТОГ: все три версии дали одинаковые файлы"
                  if total_diff == 0 else
                  f"ИТОГ: расхождений {total_diff}"))
    return 0 if total_diff == 0 else 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
