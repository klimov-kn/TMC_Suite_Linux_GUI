#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Перевод диалогов MFC (.rc) в файлы Qt (.ui) и код регистрации.

Зачем это нужно
---------------
Поведение диалогов (обмен данными DDX, обработчики кнопок) компилируется из
Windows-версии как есть — поверх слоя совместимости src/compat/include/mfc_dialog.h.
Не хватает только разметки: в Windows её брал из ресурса `.rc` сам диалоговый
движок. Здесь ту же разметку берёт Qt из `.ui`, полученного этим скриптом.

Что порождается
---------------
1. По одному `.ui` на каждый блок DIALOG/DIALOGEX. Имя файла — символьное имя
   диалога (IDD_DIALOG2.ui), значит uic сделает заголовок ui_IDD_DIALOG2.h с
   классом Ui_IDD_DIALOG2. Компоновщиков нет: каждый элемент стоит по
   абсолютным координатам, как в Windows.
2. `dialog_registry.cpp` — включает сгенерированные ui_*.h и заполняет
   MfcDialogRegistry: «идентификатор диалога -> функция разметки» и
   «числовой идентификатор элемента -> символьное имя» для всех определений
   из resource.h.

Договор со слоем совместимости
------------------------------
CDialog::DoModal() создаёт пустой QDialog, зовёт функцию разметки, а потом ищет
элементы через findChild<QWidget*>(имя), где имя берётся из таблицы по числовому
идентификатору. Поэтому objectName каждого виджета в `.ui` ОБЯЗАН совпадать с
символьным именем из resource.h. Если один и тот же номер описан в resource.h
под несколькими именами (IDOK2 и ID_GRAPPARCORDEFAULT оба равны 3), выбирается
первое по файлу — «каноническое» — и именно оно ставится в objectName, иначе
поиск по номеру не найдёт элемент.

Запуск
------
    python tools/rc2ui.py \
        --rc        ../TMC_Suite/src/viewers/Tmcrtout/TMCGROUT.rc \
        --resource  src/win_src/viewers/tmcrtout/resource.h \
        --out       src/apps/tmcros/dialogs \
        --install-fn TmcInstallDialogs_tmcros
"""

import argparse
import os
import re
import sys

# --- Диалоговые единицы -> пиксели --------------------------------------------
# В `.rc` координаты заданы не в пикселях, а в диалоговых единицах (DLU). Windows
# переводит их по размеру шрифта диалога:
#     x_px = x_du * baseX / 4      y_px = y_du * baseY / 8
# где baseX — средняя ширина знака шрифта, baseY — высота знакоместа
# (GetDialogBaseUnits / MapDialogRect). Для «MS Sans Serif» 8 пунктов при 96 dpi —
# том самом шрифте, что стоит во всех диалогах TMC (строка FONT 8, "MS Sans Serif"),
# — это 6 и 13 пикселей. Отсюда значения по умолчанию: по горизонтали масштаб 1.5,
# по вертикали 1.625. Коэффициенты вынесены в параметры: другой шрифт — другие числа.
DEFAULT_BASE_X = 6
DEFAULT_BASE_Y = 13

# Идентификаторы, которых нет в resource.h: они приходят из windows.h/afxres.h.
# Добавляются только если номер не занят определением из resource.h — определения
# программы важнее (в TMCROS номер 3 — это IDOK2, а не IDABORT).
BUILTIN_IDS = [
    ("IDC_STATIC", -1),
    ("IDOK", 1),
    ("IDCANCEL", 2),
    ("IDABORT", 3),
    ("IDRETRY", 4),
    ("IDIGNORE", 5),
    ("IDYES", 6),
    ("IDNO", 7),
    ("IDHELP", 9),
]

# Операторы описания элементов в блоке диалога.
TEXT_CONTROLS = {
    "LTEXT", "RTEXT", "CTEXT", "PUSHBUTTON", "DEFPUSHBUTTON", "PUSHBOX",
    "CHECKBOX", "AUTOCHECKBOX", "RADIOBUTTON", "AUTORADIOBUTTON",
    "STATE3", "AUTO3STATE", "GROUPBOX",
}
PLAIN_CONTROLS = {"EDITTEXT", "COMBOBOX", "LISTBOX", "SCROLLBAR"}
KEYWORDS = TEXT_CONTROLS | PLAIN_CONTROLS | {"CONTROL", "ICON"}

# Виджеты Qt, которые получают фокус с клавиатуры: только они попадают в
# порядок обхода (<tabstops>).
FOCUSABLE = {
    "QLineEdit", "QPlainTextEdit", "QTextEdit", "QPushButton", "QCheckBox",
    "QRadioButton", "QComboBox", "QListWidget", "QScrollBar", "QSlider",
}


# --- Чтение ресурсов ----------------------------------------------------------

def read_cp1251(path):
    """Файлы Windows-версии записаны в CP1251; наружу отдаём текст Unicode."""
    with open(path, "rb") as fh:
        data = fh.read()
    return data.decode("cp1251", errors="replace").replace("\r\n", "\n")


def load_resource_h(path):
    """resource.h -> (имя -> номер, номер -> каноническое имя, порядок имён).

    Каноническим считается первое имя, встреченное для номера: именно оно
    попадёт в objectName и в таблицу реестра.
    """
    text = read_cp1251(path)
    name_to_id = {}
    id_to_name = {}
    order = []
    in_apstudio = 0
    for line in text.splitlines():
        s = line.strip()
        # Хвост resource.h, который правит только редактор ресурсов Visual C++
        # (_APS_NEXT_*), к элементам отношения не имеет.
        if s.startswith("#ifdef APSTUDIO_INVOKED"):
            in_apstudio = 1
            continue
        if in_apstudio:
            if s.startswith("#endif"):
                in_apstudio = 0
            continue
        m = re.match(r"#define\s+([A-Za-z_]\w*)\s+(\S+)", s)
        if not m:
            continue
        name, raw = m.group(1), m.group(2)
        if name.startswith("_APS_"):
            continue
        try:
            value = int(raw, 0)
        except ValueError:
            continue
        name_to_id[name] = value
        order.append(name)
        if value not in id_to_name:
            id_to_name[value] = name

    for name, value in BUILTIN_IDS:
        if name in name_to_id:
            continue
        name_to_id[name] = value
        if value not in id_to_name:
            id_to_name[value] = name
            order.append(name)
    # IDOK и IDCANCEL слой совместимости ищет именно под этими именами.
    for name, value in (("IDOK", 1), ("IDCANCEL", 2)):
        id_to_name[value] = name
    return name_to_id, id_to_name, order


def split_top_level(text):
    """Разбор строки оператора на части по запятым вне кавычек."""
    parts = []
    buf = []
    in_str = False
    i = 0
    while i < len(text):
        c = text[i]
        if in_str:
            if c == '"':
                # Удвоенная кавычка внутри строки .rc — это сама кавычка.
                if i + 1 < len(text) and text[i + 1] == '"':
                    buf.append('""')
                    i += 2
                    continue
                in_str = False
            buf.append(c)
        elif c == '"':
            in_str = True
            buf.append(c)
        elif c == ',':
            parts.append("".join(buf).strip())
            buf = []
        else:
            buf.append(c)
        i += 1
    parts.append("".join(buf).strip())
    return parts


def unquote(text):
    """Строковый литерал .rc -> обычная строка."""
    t = text.strip()
    if len(t) >= 2 and t.startswith('"') and t.endswith('"'):
        t = t[1:-1]
    return t.replace('""', '"')


def join_statements(body):
    """Строки блока BEGIN..END -> список операторов.

    Редактор ресурсов переносит длинные операторы на следующую строку, причём
    без всякого знака продолжения. Новый оператор начинается там, где строка
    начинается с ключевого слова, а предыдущая не оборвана на запятой или «|».
    """
    out = []
    for raw in body.splitlines():
        s = raw.strip()
        if not s or s.startswith("//"):
            continue
        first = s.split()[0].strip(",")
        starts = first in KEYWORDS and not (
            out and (out[-1].endswith(",") or out[-1].endswith("|"))
        )
        if starts or not out:
            out.append(s)
        else:
            out[-1] = out[-1] + " " + s
    return out


class Control(object):
    """Один элемент диалога, как он записан в .rc."""

    def __init__(self):
        self.kind = ""        # оператор .rc (CONTROL, EDITTEXT, LTEXT, ...)
        self.cls = ""         # класс окна для CONTROL ("Button", "Static", ...)
        self.text = ""
        self.symbol = ""      # символьное имя из .rc
        self.rect = (0, 0, 0, 0)   # в диалоговых единицах
        self.flags = set()    # BS_/ES_/WS_/SS_ и прочие признаки стиля
        self.style_raw = ""


def parse_control(stmt):
    """Оператор .rc -> Control (или None, если разобрать не удалось)."""
    m = re.match(r"^([A-Z0-9_]+)\s+(.*)$", stmt, re.S)
    if not m:
        return None, "не распознан оператор: %s" % stmt
    kind, rest = m.group(1), m.group(2)
    if kind not in KEYWORDS:
        return None, "неизвестный оператор %s" % kind
    parts = split_top_level(rest)

    c = Control()
    c.kind = kind
    try:
        if kind == "CONTROL":
            c.text = unquote(parts[0])
            c.symbol = parts[1].strip()
            c.cls = unquote(parts[2])
            c.style_raw = parts[3]
            c.rect = tuple(int(parts[i], 0) for i in (4, 5, 6, 7))
        elif kind == "ICON":
            c.text = ""
            c.cls = "Static"
            c.symbol = parts[1].strip()
            x, y = int(parts[2], 0), int(parts[3], 0)
            w = int(parts[4], 0) if len(parts) > 4 else 20
            h = int(parts[5], 0) if len(parts) > 5 else 20
            c.rect = (x, y, w, h)
            c.style_raw = " ".join(parts[6:])
        elif kind in TEXT_CONTROLS:
            c.text = unquote(parts[0])
            c.symbol = parts[1].strip()
            c.rect = tuple(int(parts[i], 0) for i in (2, 3, 4, 5))
            c.style_raw = " ".join(parts[6:])
        else:  # EDITTEXT, COMBOBOX, LISTBOX, SCROLLBAR — без подписи
            c.symbol = parts[0].strip()
            c.rect = tuple(int(parts[i], 0) for i in (1, 2, 3, 4))
            c.style_raw = " ".join(parts[5:])
    except (IndexError, ValueError) as exc:
        return None, "не разобраны поля (%s): %s" % (exc, stmt)

    c.flags = set(re.findall(r"\b[A-Z][A-Z0-9_]*\b", c.style_raw))
    # Явное «выключить признак»: NOT WS_VISIBLE.
    for neg in re.findall(r"NOT\s+([A-Z][A-Z0-9_]*)", c.style_raw):
        c.flags.discard(neg)
        c.flags.add("NOT_" + neg)
    return c, None


class Dialog(object):
    def __init__(self):
        self.symbol = ""
        self.rect = (0, 0, 0, 0)
        self.caption = ""
        self.font_size = 8
        self.font_name = "MS Sans Serif"
        self.style_flags = set()
        self.controls = []


DIALOG_RE = re.compile(
    r"^(?P<name>\w+)\s+(?P<kind>DIALOGEX|DIALOG)\s+(?P<geom>[-\d,\s]+)$", re.M)


def parse_rc(path):
    """.rc -> (список диалогов, список замечаний)."""
    text = read_cp1251(path)
    lines = text.split("\n")
    dialogs = []
    notes = []
    i = 0
    while i < len(lines):
        m = DIALOG_RE.match(lines[i].strip())
        if not m:
            i += 1
            continue
        d = Dialog()
        d.symbol = m.group("name")
        nums = [int(v.strip(), 0) for v in m.group("geom").split(",") if v.strip()]
        if len(nums) == 4:
            d.rect = tuple(nums)
        # Заголовок блока: до BEGIN.
        head = []
        i += 1
        while i < len(lines) and lines[i].strip() != "BEGIN":
            head.append(lines[i].strip())
            i += 1
        head_text = " ".join(head)
        mc = re.search(r'CAPTION\s+"((?:[^"]|"")*)"', head_text)
        if mc:
            d.caption = unquote('"%s"' % mc.group(1))
        mf = re.search(r'FONT\s+(\d+)\s*,\s*"([^"]*)"', head_text)
        if mf:
            d.font_size = int(mf.group(1))
            d.font_name = mf.group(2)
        ms = re.search(r"STYLE\s+(.*?)(?:CAPTION|FONT|EXSTYLE|$)", head_text)
        if ms:
            d.style_flags = set(re.findall(r"\b[A-Z][A-Z0-9_]*\b", ms.group(1)))
        # Тело блока.
        body = []
        if i < len(lines) and lines[i].strip() == "BEGIN":
            i += 1
        depth = 1
        while i < len(lines):
            s = lines[i].strip()
            if s == "BEGIN":
                depth += 1
            elif s == "END":
                depth -= 1
                if depth == 0:
                    i += 1
                    break
            body.append(lines[i])
            i += 1
        for stmt in join_statements("\n".join(body)):
            c, err = parse_control(stmt)
            if c is None:
                notes.append("%s: %s" % (d.symbol, err))
            else:
                d.controls.append(c)
        dialogs.append(d)
    return dialogs, notes


# --- Соответствие элементов Qt ------------------------------------------------

def map_widget(c, notes, dialog_name):
    """Control -> (класс Qt, свойства).

    Свойства — список пар (имя, xml), которые вставляются внутрь <widget>.
    """
    props = []
    flags = c.flags
    cls = None

    def align(default_h):
        h = default_h
        if "SS_RIGHT" in flags or "ES_RIGHT" in flags:
            h = "Qt::AlignRight"
        elif "SS_CENTER" in flags or "ES_CENTER" in flags:
            h = "Qt::AlignHCenter"
        # По вертикали в Windows однострочная надпись стоит в своём
        # прямоугольнике вплотную к верху, но высота прямоугольника в .rc
        # практически равна высоте строки, поэтому центрирование по вертикали
        # даёт то же положение и не зависит от точной высоты шрифта.
        return "%s|Qt::AlignVCenter" % h

    if c.kind in ("LTEXT", "RTEXT", "CTEXT") or (
            c.kind == "CONTROL" and c.cls.lower() == "static") or c.kind == "ICON":
        cls = "QLabel"
        default = {"LTEXT": "Qt::AlignLeft", "RTEXT": "Qt::AlignRight",
                   "CTEXT": "Qt::AlignHCenter"}.get(c.kind, "Qt::AlignLeft")
        props.append(("alignment", align(default)))
    elif c.kind in ("PUSHBUTTON", "DEFPUSHBUTTON", "PUSHBOX"):
        cls = "QPushButton"
        if c.kind == "DEFPUSHBUTTON":
            props.append(("default", "bool:true"))
    elif c.kind in ("CHECKBOX", "AUTOCHECKBOX", "STATE3", "AUTO3STATE"):
        cls = "QCheckBox"
    elif c.kind in ("RADIOBUTTON", "AUTORADIOBUTTON"):
        cls = "QRadioButton"
    elif c.kind == "GROUPBOX":
        cls = "QGroupBox"
    elif c.kind == "EDITTEXT":
        cls = "QPlainTextEdit" if "ES_MULTILINE" in flags else "QLineEdit"
    elif c.kind == "COMBOBOX":
        cls = "QComboBox"
    elif c.kind == "LISTBOX":
        cls = "QListWidget"
    elif c.kind == "SCROLLBAR":
        cls = "QScrollBar"
        props.append(("orientation",
                      "Qt::Vertical" if "SBS_VERT" in flags else "Qt::Horizontal"))
    elif c.kind == "CONTROL":
        low = c.cls.lower()
        if low == "button":
            if "BS_GROUPBOX" in flags:
                cls = "QGroupBox"
            elif flags & {"BS_AUTOCHECKBOX", "BS_CHECKBOX",
                          "BS_AUTO3STATE", "BS_3STATE"}:
                cls = "QCheckBox"
            elif flags & {"BS_AUTORADIOBUTTON", "BS_RADIOBUTTON"}:
                cls = "QRadioButton"
            else:
                cls = "QPushButton"
                if "BS_DEFPUSHBUTTON" in flags:
                    props.append(("default", "bool:true"))
        elif low == "edit":
            cls = "QPlainTextEdit" if "ES_MULTILINE" in flags else "QLineEdit"
        elif low == "combobox":
            cls = "QComboBox"
        elif low == "listbox":
            cls = "QListWidget"
        elif low == "scrollbar":
            cls = "QScrollBar"
            props.append(("orientation",
                          "Qt::Vertical" if "SBS_VERT" in flags else "Qt::Horizontal"))
        elif low == "msctls_trackbar32":
            cls = "QSlider"
            props.append(("orientation",
                          "Qt::Vertical" if "TBS_VERT" in flags else "Qt::Horizontal"))
        elif low == "msctls_progress32":
            cls = "QProgressBar"
        else:
            cls = "QWidget"
            notes.append("%s: класс окна \"%s\" (элемент %s) без соответствия в Qt — "
                         "поставлен пустой QWidget" % (dialog_name, c.cls, c.symbol))

    if cls is None:
        cls = "QWidget"
        notes.append("%s: оператор %s (элемент %s) без соответствия в Qt"
                     % (dialog_name, c.kind, c.symbol))

    # Подпись. Мнемоника «&» в .rc и в Qt записывается одинаково (в том числе
    # «&&» как сама амперсанда), поэтому текст переносится дословно.
    if cls == "QGroupBox":
        props.append(("title", "str:" + c.text))
    elif cls in ("QLabel", "QPushButton", "QCheckBox", "QRadioButton"):
        props.append(("text", "str:" + c.text))

    if "WS_DISABLED" in flags:
        props.append(("enabled", "bool:false"))
    if "NOT_WS_VISIBLE" in flags:
        props.append(("visible", "bool:false"))
    if cls in ("QLineEdit", "QPlainTextEdit") and "ES_READONLY" in flags:
        props.append(("readOnly", "bool:true"))
    if cls == "QLineEdit" and "ES_PASSWORD" in flags:
        props.append(("echoMode", "QLineEdit::Password"))
    if cls == "QLineEdit" and ("ES_RIGHT" in flags or "ES_CENTER" in flags):
        props.append(("alignment", align("Qt::AlignLeft")))
    if cls == "QPlainTextEdit":
        # Однострочное поле .rc с ES_MULTILINE в Qt даёт полосы прокрутки,
        # которых в Windows не видно.
        props.append(("lineWrapMode", "QPlainTextEdit::NoWrap"))
    if c.kind == "ICON":
        notes.append("%s: значок (ICON) выведен пустым QLabel — картинка из "
                     "ресурсов Windows не переносится" % dialog_name)
    return cls, props


# --- Порождение .ui -----------------------------------------------------------

def xml_escape(text):
    return (text.replace("&", "&amp;").replace("<", "&lt;")
                .replace(">", "&gt;").replace('"', "&quot;"))


def prop_xml(name, value, indent):
    """Свойство виджета. Значение с префиксом задаёт тип: str:, bool:, иначе — <set>."""
    pad = " " * indent
    if value.startswith("str:"):
        inner = "<string notr=\"true\">%s</string>" % xml_escape(value[4:])
    elif value.startswith("bool:"):
        inner = "<bool>%s</bool>" % value[5:]
    elif "::" in value and "|" not in value:
        inner = "<enum>%s</enum>" % value
    else:
        inner = "<set>%s</set>" % value
    return "%s<property name=\"%s\">\n%s %s\n%s</property>" % (
        pad, name, pad, inner, pad)


def du_to_px(rect, base_x, base_y):
    """Прямоугольник в диалоговых единицах -> в пикселях.

    Округление повторяет MulDiv из Windows (MapDialogRect считает именно им):
    целочисленное деление с прибавлением половины делителя, то есть половина
    округляется вверх. Свой вариант округления дал бы расхождение в пиксель.
    """
    x, y, w, h = rect
    return ((x * base_x + 2) // 4, (y * base_y + 4) // 8,
            (w * base_x + 2) // 4, (h * base_y + 4) // 8)


def geometry_xml(rect, indent):
    pad = " " * indent
    x, y, w, h = rect
    return ("%s<property name=\"geometry\">\n"
            "%s <rect>\n"
            "%s  <x>%d</x>\n%s  <y>%d</y>\n%s  <width>%d</width>\n%s  <height>%d</height>\n"
            "%s </rect>\n"
            "%s</property>" % (pad, pad, pad, x, pad, y, pad, w, pad, h, pad, pad))


def object_names(dialog, name_to_id, id_to_name, notes):
    """Для каждого элемента диалога — objectName и номер (или None).

    Имя берётся каноническое (то, под которым номер лежит в таблице реестра):
    именно по нему слой совместимости найдёт виджет. Элементы-оформление
    (IDC_STATIC, номер -1) и повторы получают уникальный суффикс — иначе uic
    объявит две переменные с одним именем.
    """
    used = {}
    result = []
    for c in dialog.controls:
        num = name_to_id.get(c.symbol)
        if num is None and re.match(r"^-?\d+$", c.symbol):
            # Редактор ресурсов иногда пишет номер прямо: «-1» вместо IDC_STATIC.
            num = int(c.symbol, 0)
        if num is None:
            notes.append("%s: элемент %s не описан в resource.h — оставлено имя "
                         "из .rc, поиск по номеру работать не будет"
                         % (dialog.symbol, c.symbol))
            base = c.symbol
        else:
            base = id_to_name.get(num)
            if base is None:
                base = "IDC_STATIC" if num < 0 else "ID_%d" % num
        lookup = num if (num is not None and num >= 0) else None
        name = base
        n = used.get(base, 0)
        if n:
            name = "%s_%d" % (base, n + 1)
            if lookup is not None:
                notes.append("%s: номер %d (%s) встречается в диалоге дважды — "
                             "второй виджет назван %s и по номеру не находится"
                             % (dialog.symbol, num, base, name))
        used[base] = n + 1
        result.append((name, lookup))
    return result


def button_groups(dialog):
    """Разбивка переключателей на группы по признаку WS_GROUP.

    В Windows группа начинается с элемента, помеченного WS_GROUP, и тянется до
    следующего такого элемента; переключатели внутри группы взаимно исключают
    друг друга. В Qt все QRadioButton одного родителя исключают друг друга
    скопом, поэтому границы приходится восстанавливать явно — через QButtonGroup
    (uic создаёт их из раздела <buttongroups>).
    """
    groups = []
    index = 0        # 0 — элементы до первого WS_GROUP, они образуют свою группу
    for c in dialog.controls:
        if "WS_GROUP" in c.flags:
            index += 1
        groups.append(index)
    return groups


def build_ui(dialog, name_to_id, id_to_name, base_x, base_y, notes, set_font):
    names = object_names(dialog, name_to_id, id_to_name, notes)
    groups = button_groups(dialog)
    dx, dy, dw, dh = du_to_px(dialog.rect, base_x, base_y)

    widgets = []
    tab_order = []
    used_groups = set()
    for idx, c in enumerate(dialog.controls):
        cls, props = map_widget(c, notes, dialog.symbol)
        name = names[idx][0]
        body = [geometry_xml(du_to_px(c.rect, base_x, base_y), 4)]
        for pname, pvalue in props:
            body.append(prop_xml(pname, pvalue, 4))
        attrs = ""
        if cls == "QRadioButton":
            gname = "buttonGroup_%d" % groups[idx]
            used_groups.add(gname)
            attrs = ("   <attribute name=\"buttonGroup\">\n"
                     "    <string>%s</string>\n"
                     "   </attribute>\n" % gname)
        widgets.append("   <widget class=\"%s\" name=\"%s\">\n%s\n%s   </widget>"
                       % (cls, name, "\n".join(body), attrs))
        if cls in FOCUSABLE:
            tab_order.append(name)

    # Порядок наложения. В Windows элемент, записанный в .rc раньше, лежит выше
    # остальных; в Qt наоборот — выше тот, кто создан позже. Поэтому виджеты
    # выводятся в обратном порядке: иначе рамка GROUPBOX (в .rc она обычно
    # записана последней) легла бы поверх своих переключателей и перехватывала
    # бы щелчки. Порядок обхода клавишей Tab восстанавливается разделом
    # <tabstops> — там элементы идут как в .rc.
    widgets.reverse()

    out = []
    out.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")
    out.append("<!-- Файл создан tools/rc2ui.py из ресурсов Windows-версии. "
               "Не править вручную. -->")
    out.append("<ui version=\"4.0\">")
    out.append(" <class>%s</class>" % dialog.symbol)
    out.append(" <widget class=\"QDialog\" name=\"%s\">" % dialog.symbol)
    out.append(geometry_xml((dx, dy, dw, dh), 2))
    # Диалоги MFC размер не меняют (DS_MODALFRAME без WS_THICKFRAME), поэтому
    # размер закрепляется — иначе Qt позволил бы растянуть окно, и элементы с
    # абсолютными координатами оказались бы в углу.
    if "WS_THICKFRAME" not in dialog.style_flags:
        for pname in ("minimumSize", "maximumSize"):
            out.append("  <property name=\"%s\">\n   <size>\n"
                       "    <width>%d</width>\n    <height>%d</height>\n"
                       "   </size>\n  </property>" % (pname, dw, dh))
    out.append(prop_xml("windowTitle", "str:" + dialog.caption, 2))
    if set_font:
        # Размер шрифта — тот же, что в .rc: от него зависят пропорции надписей
        # относительно рассчитанных координат.
        out.append("  <property name=\"font\">\n   <font>\n"
                   "    <pointsize>%d</pointsize>\n   </font>\n  </property>"
                   % dialog.font_size)
    out.extend(widgets)
    out.append(" </widget>")
    out.append(" <resources/>")
    out.append(" <connections/>")
    if tab_order:
        out.append(" <tabstops>")
        for name in tab_order:
            out.append("  <tabstop>%s</tabstop>" % name)
        out.append(" </tabstops>")
    if used_groups:
        out.append(" <buttongroups>")
        for gname in sorted(used_groups):
            out.append("  <buttongroup name=\"%s\"/>" % gname)
        out.append(" </buttongroups>")
    out.append("</ui>")
    return "\n".join(out) + "\n", [n for n, _ in names]


# --- Порождение кода регистрации ---------------------------------------------

REGISTRY_HEAD = """// Файл создан tools/rc2ui.py из %(rc)s. Не править вручную.
//
// Что здесь. Слой совместимости (src/compat/include/mfc_dialog.h) при вызове
// CDialog::DoModal() ищет по числовому идентификатору диалога функцию разметки,
// а элементы внутри — по символьному имени. Обе таблицы заполняются здесь:
//   * функция разметки — вызов setupUi() из заголовка, который uic делает
//     из соответствующего .ui;
//   * имена элементов — все определения из resource.h.
// Если один номер описан несколькими именами, берётся первое по resource.h:
// именно оно стоит в objectName виджетов.
//
// Почему номера записаны числами, а resource.h не включён. objectName виджета
// обязан совпадать с именем элемента (IDC_GRPAR_XMAX), и uic делает из этого
// имени имя переменной в ui_*.h. Но в resource.h то же самое имя — макрос
// (#define IDC_GRPAR_XMAX 1001), и препроцессор превратил бы объявление
// переменной в «QLineEdit *1001». Поэтому resource.h сюда не включается, а
// макросы, приходящие из слоя совместимости (IDOK, IDCANCEL), снимаются ниже.
// Числовые значения взяты из resource.h на момент порождения файла; повторный
// запуск rc2ui.py обновит их вместе с разметкой.

#include <QDialog>

#include "afxwin.h"     // MfcDialogRegistry

// Снять макросы-идентификаторы: дальше эти имена — имена переменных в ui_*.h.
%(undefs)s
%(includes)s

namespace {

%(setups)s
// Соответствие «номер -> символьное имя» для всех элементов из resource.h.
struct TmcControlName
{
    UINT        id;
    const char* name;
};

const TmcControlName kControlNames[] = {
%(controls)s};

bool tmcInstall()
{
%(dialogs)s
    const int count = int(sizeof(kControlNames) / sizeof(kControlNames[0]));
    for (int i = 0; i < count; ++i)
        MfcDialogRegistry::registerControlName(kControlNames[i].id,
                                               kControlNames[i].name);
    return true;
}

} // namespace

// Вызывается из кода программы один раз при запуске. Не полагаемся на
// инициализацию статических объектов: файл лежит в статической библиотеке, и
// без явной ссылки компоновщик мог бы его не подключить.
void %(fn)s()
{
    static const bool done = tmcInstall();
    (void)done;
}
"""


def build_registry(rc_path, dialogs, name_to_id, id_to_name, order, fn_name,
                   used_names):
    includes = "\n".join('#include "ui_%s.h"' % d.symbol for d in dialogs)
    # Снимаются только те имена, которые действительно описаны как макросы
    # (в resource.h или в слое совместимости): именно они мешают uic.
    undefs = "\n".join("#undef %s" % n
                       for n in sorted(set(used_names) & set(name_to_id)))
    setups = []
    for d in dialogs:
        setups.append(
            "void tmcSetup_%s(void* qDialog)\n"
            "{\n"
            "    // Ui_-объект хранит только указатели: после setupUi() виджеты\n"
            "    // принадлежат диалогу, и держать его дальше не нужно.\n"
            "    Ui_%s ui;\n"
            "    ui.setupUi(static_cast<QDialog*>(qDialog));\n"
            "}\n" % (d.symbol, d.symbol))
    controls = []
    seen = set()
    for name in order:
        value = name_to_id[name]
        if value in seen or value < 0:
            continue
        seen.add(value)
        # В таблицу пишем каноническое имя номера — то же, что в objectName.
        controls.append("    { %d, \"%s\" },\n" % (value, id_to_name[value]))
    regs = []
    for d in dialogs:
        idd = name_to_id.get(d.symbol)
        if idd is None:
            regs.append("    // %s: номера нет в resource.h — диалог не "
                        "зарегистрирован\n" % d.symbol)
            continue
        regs.append("    MfcDialogRegistry::registerDialog(%d /* %s */, "
                    "&tmcSetup_%s);\n" % (idd, d.symbol, d.symbol))
    return REGISTRY_HEAD % {
        "rc": os.path.basename(rc_path),
        "undefs": undefs,
        "includes": includes,
        "setups": "\n".join(setups),
        "controls": "".join(controls),
        "dialogs": "".join(regs),
        "fn": fn_name,
    }


# --- Точка входа --------------------------------------------------------------

def main(argv):
    ap = argparse.ArgumentParser(
        description="Перевод диалогов MFC (.rc) в файлы Qt (.ui) и код регистрации")
    ap.add_argument("--rc", required=True, help="файл ресурсов Windows-версии")
    ap.add_argument("--resource", required=True, help="resource.h к нему")
    ap.add_argument("--out", required=True, help="каталог для .ui и dialog_registry.cpp")
    ap.add_argument("--install-fn", default="TmcInstallDialogs",
                    help="имя функции регистрации в dialog_registry.cpp")
    ap.add_argument("--registry", default="dialog_registry.cpp",
                    help="имя файла регистрации")
    ap.add_argument("--base-x", type=int, default=DEFAULT_BASE_X,
                    help="средняя ширина знака шрифта диалога, пикселей (по умолчанию 6)")
    ap.add_argument("--base-y", type=int, default=DEFAULT_BASE_Y,
                    help="высота знакоместа шрифта диалога, пикселей (по умолчанию 13)")
    ap.add_argument("--no-font", action="store_true",
                    help="не задавать размер шрифта диалога")
    ap.add_argument("--only", nargs="*", default=None,
                    help="перенести только указанные диалоги")
    args = ap.parse_args(argv)

    name_to_id, id_to_name, order = load_resource_h(args.resource)
    dialogs, notes = parse_rc(args.rc)
    if args.only:
        dialogs = [d for d in dialogs if d.symbol in args.only]
    if not dialogs:
        sys.stderr.write("В %s не найдено ни одного диалога\n" % args.rc)
        return 1

    if not os.path.isdir(args.out):
        os.makedirs(args.out)

    total_controls = 0
    used_names = []
    for d in dialogs:
        text, names = build_ui(d, name_to_id, id_to_name, args.base_x, args.base_y,
                               notes, not args.no_font)
        used_names.extend(names)
        used_names.append(d.symbol)
        path = os.path.join(args.out, d.symbol + ".ui")
        with open(path, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)
        total_controls += len(d.controls)
        print("%-24s %3d элементов -> %s" % (d.symbol, len(d.controls), path))

    reg = build_registry(args.rc, dialogs, name_to_id, id_to_name, order,
                         args.install_fn, used_names)
    reg_path = os.path.join(args.out, args.registry)
    with open(reg_path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(reg)
    print("Регистрация -> %s (функция %s)" % (reg_path, args.install_fn))
    print("Итого: диалогов %d, элементов %d, имён в таблице %d"
          % (len(dialogs), total_controls, len(set(name_to_id.values()))))

    if notes:
        print("\nЗамечания (%d):" % len(notes))
        seen = set()
        for n in notes:
            if n in seen:
                continue
            seen.add(n)
            print("  * " + n)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
