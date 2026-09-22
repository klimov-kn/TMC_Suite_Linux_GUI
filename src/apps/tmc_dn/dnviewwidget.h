// dnviewwidget.h — виджет-хозяин вида TMC_DN, безопасный для фонового потока.
//
// Зачем нужен. TMC_DN считает диаграмму направленности в отдельном потоке
// (`AfxBeginThread(ReadDocFileW_Thread1, ...)` в tmcgroutview.cpp). В конце
// расчёта код Windows-версии зовёт `RedrawWindow()`, а по дороге — `PutTrace` и
// `PutStatistics`. В Windows рисовать из другого потока было можно; в Qt
// обращаться к виджетам разрешено только из главного потока, и обычный
// `MfcViewWidget` в этом месте падает (SIGSEGV в QPainter::begin, стек уходит
// в QWidget::repaint прямо из потока расчёта).
//
// Здесь те же вызовы, придя из фонового потока, кладутся в очередь событий
// главного потока. Порядок вычислений и результат от этого не меняются: расчёт
// как шёл в своём потоке, так и идёт, откладывается только перерисовка.
//
// Правильное место для такой защиты — общий MfcViewWidget (src/gui_common):
// то же самое понадобится счётным оболочкам PlanarRT_H/PlanarRT_X. Пока он
// занят другим исполнителем, защита живёт здесь. См. src/apps/tmc_dn/NEEDS.md.

#pragma once

#include "mfcviewwidget.h"

class DnViewWidget : public MfcViewWidget
{
    Q_OBJECT

public:
    explicit DnViewWidget(QWidget *parent = nullptr);

protected:
    // --- MfcQtHost: то, до чего дотягивается поток расчёта -------------------
    void hostInvalidate(const RECT *logicalRect, BOOL erase) override;
    void hostUpdateWindow() override;
    int  hostMessageBox(const char *text, const char *caption, UINT type) override;

private:
    /// Пришёл ли вызов из главного потока (где живёт виджет).
    bool inGuiThread() const;
};
