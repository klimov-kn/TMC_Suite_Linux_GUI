// aboutbox.h — доводка окна «О программе» до вида Windows-версии.
//
// В Windows это окно показывает адрес сайта и ссылку на локальное руководство
// пользователя (код TmcDocLink.h). Здесь то же самое средствами Qt, плюс
// строка о том, для какой системы собрана программа.
//
// Ставится один раз при запуске: theme::apply() зовёт install(), а дальше слой
// совместимости сам применяет доводку к каждому построенному окну.

#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

#include <QString>

namespace tmcabout {

/// Готовое содержимое окна «О программе»: название, описание, версия пакета,
/// система и разрядность, ссылки на сайт и на руководство пользователя.
/// Тем же пользуются все шесть программ, чтобы окно было одинаковым.
QString aboutText(const QString &title, const QString &description);

/// Показать окно «О программе»: название, описание, версия, система и
/// работающие ссылки на руководство и сайт.
void show(QWidget *parent, const QString &windowTitle, const QString &title,
          const QString &description);

/// Доработать одно окно. Ничего не делает, если это не окно «О программе».
void decorate(QWidget *dialog);

/// Включить доводку для всех окон программы.
void install();

} // namespace tmcabout
