// theme.h — оформление программ TMC Suite под Linux.
//
// Палитра и приёмы те же, что в TPL Designer: тёмная подложка, приглушённые
// рамки, бирюзовый акцент. Компоновка окон при этом остаётся такой же, как в
// Windows-версии — меняется только оформление, не расположение элементов.

#pragma once

#include <QColor>
#include <QString>

QT_BEGIN_NAMESPACE
class QWidget;
class QIcon;
QT_END_NAMESPACE

namespace theme {

inline QColor bg()       { return QColor(0x0a, 0x0e, 0x16); }
inline QColor surface()  { return QColor(0x13, 0x1a, 0x28); }
inline QColor line()     { return QColor(0x24, 0x30, 0x49); }
inline QColor text()     { return QColor(0xe7, 0xee, 0xf7); }
inline QColor textDim()  { return QColor(0x9f, 0xb0, 0xc8); }
inline QColor textMute() { return QColor(0x6c, 0x7d, 0x97); }
inline QColor accent()   { return QColor(0x34, 0xe3, 0xc4); }
inline QColor danger()   { return QColor(0xff, 0x6b, 0x6b); }
inline QColor ok()       { return QColor(0x5c, 0xe0, 0x8a); }
inline QColor warn()     { return QColor(0xe3, 0xb3, 0x41); }

/// Таблица стилей приложения.
QString styleSheet();

/// Применить оформление ко всему приложению (палитра + таблица стилей).
void apply();

/// Пиктограмма команды из ресурсов: сначала ищется вариант для тёмной темы
/// (`<ID>_dark.png`), затем исходный из Windows-ресурсов.
QIcon commandIcon(const QString &commandId);

/// Ужать окно до экрана: диалог, у которого не видно кнопок, — неработающий диалог.
void fitToScreen(QWidget *w);

} // namespace theme
