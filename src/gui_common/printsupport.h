// printsupport.h — печать вида, перенесённого из Windows-версии.
//
// В MFC печать шла тем же кодом, что и рисование на экране: каркас давал виду
// контекст устройства принтера, и `OnDraw` рисовал в него. Здесь ровно то же:
// контекст рисования подключается к принтеру Qt, а вид рисует себя не зная,
// куда именно.
//
// Благодаря этому на бумаге получается та же картинка, что на экране, и
// отдельного «кода печати» не появляется.

#pragma once

#include <QString>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

class CScrollView;

namespace tmcprint {

/// Печать вида. Показывает обычный диалог печати; возвращает false, если
/// пользователь отказался или печать не удалась.
bool printView(QWidget *parent, CScrollView *view, const QString &documentName);

/// Предварительный просмотр печати того же вида.
bool previewView(QWidget *parent, CScrollView *view, const QString &documentName);

/// Печать вида в файл PDF — тем же кодом, что и на принтер. Нужна, чтобы
/// проверять печать без участия человека (в том числе печать окна FieldView,
/// которое рисуется через OpenGL).
bool printViewToPdf(CScrollView *view, const QString &path);

/// Настройка страницы (в Windows-версии — пункт «Print Setup»).
bool pageSetup(QWidget *parent);

} // namespace tmcprint
