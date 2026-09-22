#include "theme.h"

#include "aboutbox.h"

#include <QApplication>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QPalette>
#include <QScreen>
#include <QWidget>

#include <algorithm>
#include <clocale>

namespace theme {

QString styleSheet()
{
    return QStringLiteral(R"(
QWidget {
    background: #0a0e16;
    color: #e7eef7;
    font-family: 'Segoe UI', 'Inter', 'DejaVu Sans', sans-serif;
    font-size: 10pt;
}

QMainWindow::separator { background: #243049; width: 1px; height: 1px; }

QMenuBar { background: #131a28; border-bottom: 1px solid #243049; }
QMenuBar::item { padding: 5px 10px; background: transparent; }
QMenuBar::item:selected { background: #1b2436; color: #34e3c4; }

QMenu { background: #131a28; border: 1px solid #243049; padding: 4px; }
QMenu::item { padding: 5px 26px 5px 22px; }
QMenu::item:selected { background: #1b2436; color: #34e3c4; }
QMenu::item:disabled { color: #6c7d97; }
QMenu::separator { height: 1px; background: #243049; margin: 4px 8px; }

QToolBar {
    background: #131a28;
    border-bottom: 1px solid #243049;
    padding: 3px;
    spacing: 2px;
}
QToolBar::separator { background: #243049; width: 1px; margin: 3px 5px; }
QToolButton { background: transparent; border: 1px solid transparent; border-radius: 5px; padding: 3px; }
QToolButton:hover { background: #1b2436; border: 1px solid #243049; }
QToolButton:pressed, QToolButton:checked { background: #243049; border: 1px solid #34e3c4; }

QStatusBar { background: #131a28; border-top: 1px solid #243049; color: #9fb0c8; }
QStatusBar::item { border: none; }

QMdiArea { background: #070a10; }
QMdiSubWindow { background: #0a0e16; border: 1px solid #243049; }
QMdiSubWindow:!active { border: 1px solid #1b2436; }

QGroupBox {
    /* Прозрачный фон обязателен: в ресурсах Windows рамка группы записана ПОСЛЕ
       своих элементов, и с непрозрачным фоном она их перекрывала бы. В Windows
       рамка тоже прозрачная — рисуется только контур с подписью. */
    background: transparent;
    border: 1px solid #243049;
    border-radius: 12px;
    margin-top: 18px;
    padding: 12px 10px 10px 10px;
    font-weight: 600;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 12px;
    top: 2px;
    padding: 0 6px;
    color: #34e3c4;
}

QLabel { background: transparent; }
QLabel[dim="true"] { color: #9fb0c8; }
QLabel[mute="true"] { color: #6c7d97; }

QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QPlainTextEdit, QTextEdit, QListWidget, QTreeWidget, QTableWidget {
    background: #0a0e16;
    border: 1px solid #243049;
    border-radius: 8px;
    padding: 4px 6px;
    selection-background-color: #34e3c4;
    selection-color: #0a0e16;
}
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus, QPlainTextEdit:focus {
    border: 1px solid #34e3c4;
}
QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled {
    color: #6c7d97;
    background: #10151f;
}

QComboBox::drop-down { border: none; width: 18px; }
QComboBox QAbstractItemView {
    background: #131a28;
    border: 1px solid #243049;
    selection-background-color: #1b2436;
    selection-color: #34e3c4;
}

QHeaderView::section {
    background: #131a28;
    color: #9fb0c8;
    border: none;
    border-right: 1px solid #243049;
    border-bottom: 1px solid #243049;
    padding: 4px 6px;
}

QPushButton {
    background: #1b2436;
    border: 1px solid #243049;
    border-radius: 8px;
    padding: 6px 14px;
    min-width: 76px;
}
QPushButton:hover { border: 1px solid #34e3c4; }
QPushButton:pressed { background: #243049; }
QPushButton:default { border: 1px solid #34e3c4; }
QPushButton:disabled { color: #6c7d97; background: #10151f; }

QCheckBox, QRadioButton { background: transparent; spacing: 7px; }
QCheckBox::indicator, QRadioButton::indicator { width: 15px; height: 15px; }
QCheckBox::indicator {
    background: #0a0e16; border: 1px solid #243049; border-radius: 4px;
}
QCheckBox::indicator:checked { background: #34e3c4; border: 1px solid #34e3c4; }
QRadioButton::indicator {
    background: #0a0e16; border: 1px solid #243049; border-radius: 8px;
}
QRadioButton::indicator:checked { background: #34e3c4; border: 3px solid #0a0e16; }

QScrollBar:vertical { background: #0a0e16; width: 12px; margin: 0; }
QScrollBar:horizontal { background: #0a0e16; height: 12px; margin: 0; }
QScrollBar::handle { background: #243049; border-radius: 6px; min-height: 26px; min-width: 26px; }
QScrollBar::handle:hover { background: #34506b; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

QTabBar::tab {
    background: #131a28;
    border: 1px solid #243049;
    padding: 5px 12px;
    margin-right: 2px;
    border-top-left-radius: 8px;
    border-top-right-radius: 8px;
}
QTabBar::tab:selected { color: #34e3c4; border-bottom: 1px solid #0a0e16; }

/* Диалоги перенесены из ресурсов Windows: координаты элементов рассчитаны на
   шрифт «MS Sans Serif 8». Поэтому внутри диалогов держим тот же размер шрифта
   и минимальные отступы — иначе подписи не помещаются в свои поля и обрезаются.
   Кнопкам снимаем минимальную ширину: в ресурсах она задана явно. */
/* Размер и семейство задаём КАЖДОМУ элементу внутри диалога: правило для
   QWidget с общим размером 10pt иначе перекрывает шрифт окна, и подписи
   перестают помещаться в поля с координатами из ресурсов Windows. */
QDialog, QDialog QWidget, QDialog QLabel, QDialog QPushButton, QDialog QCheckBox,
QDialog QRadioButton, QDialog QGroupBox, QDialog QLineEdit, QDialog QComboBox,
QDialog QListWidget, QDialog QSpinBox, QDialog QDoubleSpinBox, QDialog QTextEdit,
QDialog QPlainTextEdit, QDialog QProgressBar, QDialog QTreeWidget, QDialog QTabWidget {
    font-family: 'Liberation Sans', 'Nimbus Sans', 'DejaVu Sans', sans-serif;
    font-size: 8pt;
}
QDialog QPushButton { min-width: 0; padding: 2px 6px; }
QDialog QLineEdit, QDialog QComboBox, QDialog QSpinBox, QDialog QDoubleSpinBox {
    padding: 1px 4px;
    border-radius: 5px;
}
/* Многострочные поля в ресурсах Windows часто высотой в одну строку (13 единиц
   диалога). С обычными отступами текст в такое поле не помещается вовсе. */
QDialog QPlainTextEdit, QDialog QTextEdit {
    padding: 0px 2px;
    border-radius: 5px;
}
QDialog QGroupBox { margin-top: 14px; padding: 8px 6px 6px 6px; border-radius: 8px; }
/* Значок флажка и переключателя в Windows заметно меньше нашего, а координаты
   подписей взяты из ресурсов Windows. Поэтому внутри диалогов уменьшаем значок
   и зазор — иначе подпись не помещается и обрезается. */
QDialog QCheckBox, QDialog QRadioButton { spacing: 3px; }
QDialog QCheckBox::indicator, QDialog QRadioButton::indicator {
    width: 12px;
    height: 12px;
}
QDialog QGroupBox::title { left: 6px; padding: 0 3px; }
QDialog QLabel { padding: 0; }

QToolTip {
    background: #131a28;
    color: #e7eef7;
    border: 1px solid #34e3c4;
    padding: 4px 6px;
}
)");
}

void apply()
{
    // Числа в файлах TMC пишутся и читаются только с ТОЧКОЙ — так их создаёт
    // Windows-версия, и так их ждут все sscanf("%lg") в перенесённом коде.
    //
    // Конструктор QApplication вызывает setlocale(LC_ALL, "") и берёт числовую
    // локаль из окружения. Из ярлыка рабочего стола переменной LANG может не
    // быть и локаль остаётся «C», а вот из терминала приходит язык системы: в
    // русской локали разделитель — запятая. Тогда sscanf спотыкается на первой
    // же точке (файл топологии .tt читается как «Bad topology file»), а
    // printf("%g") записал бы запятые в выходные файлы — и они перестали бы
    // совпадать с Windows байт в байт.
    //
    // Локаль Qt (QLocale) этим не затрагивается: подписи интерфейса и разбор
    // введённых человеком чисел живут отдельно от локали языка C.
    std::setlocale(LC_NUMERIC, "C");

    // Окно «О программе»: адрес сайта и руководство делаем ссылками,
    // к версии приписываем систему — как в Windows-версии.
    tmcabout::install();

    QPalette p;
    p.setColor(QPalette::Window, bg());
    p.setColor(QPalette::WindowText, text());
    p.setColor(QPalette::Base, bg());
    p.setColor(QPalette::AlternateBase, surface());
    p.setColor(QPalette::Text, text());
    p.setColor(QPalette::Button, surface());
    p.setColor(QPalette::ButtonText, text());
    p.setColor(QPalette::Highlight, accent());
    p.setColor(QPalette::HighlightedText, bg());
    p.setColor(QPalette::ToolTipBase, surface());
    p.setColor(QPalette::ToolTipText, text());
    p.setColor(QPalette::Disabled, QPalette::Text, textMute());
    p.setColor(QPalette::Disabled, QPalette::ButtonText, textMute());
    qApp->setPalette(p);
    qApp->setStyleSheet(styleSheet());
}

QIcon commandIcon(const QString &commandId)
{
    // Пиктограммы вырезаны из ресурсов Windows-версии (tools/extract_icons.py).
    // Для тёмной темы у каждой есть вариант со светлым штрихом: форма та же,
    // читаемость на тёмном фоне появляется.
    const QString dark = QStringLiteral(":/icons/") + commandId + QStringLiteral("_dark.png");
    if (QFile::exists(dark))
        return QIcon(dark);
    return QIcon(QStringLiteral(":/icons/") + commandId + QStringLiteral(".png"));
}

void fitToScreen(QWidget *w)
{
    if (!w)
        return;
    const QScreen *screen = w->screen() ? w->screen() : QGuiApplication::primaryScreen();
    if (!screen)
        return;
    const QRect avail = screen->availableGeometry();
    const QSize hint = w->sizeHint();

    w->setMinimumHeight(0);
    w->setMaximumHeight(avail.height());
    w->resize(std::min(hint.width(), int(avail.width() * 0.95)),
              std::min(hint.height(), int(avail.height() * 0.88)));

    const QPoint c = avail.center();
    w->move(std::max(avail.left(), c.x() - w->width() / 2),
            std::max(avail.top(), c.y() - w->height() / 2));
}

} // namespace theme
