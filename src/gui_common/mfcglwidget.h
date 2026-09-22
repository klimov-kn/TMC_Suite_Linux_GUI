// mfcglwidget.h — виджет-хозяин для вида, рисующего через OpenGL.
//
// FieldView рисует поле средствами OpenGL: сам создаёт контекст, ставит формат
// пикселей и меняет буферы. В Qt всем этим владеет `QOpenGLWidget`, поэтому
// вызовы `wgl*` в коде Windows-версии стали пустышками (см. mfc_wgl.h), а этот
// виджет даёт виду то, что ему действительно нужно: контекст сделан текущим,
// размер известен, кадр показывается сам.
//
// Важно: запрашивается СОВМЕСТИМЫЙ профиль OpenGL. Отрисовка в TMC написана в
// старом стиле (`glBegin`/`glEnd`, матрицы, без шейдеров), и переписывать её на
// шейдеры значит переписывать саму картинку. Совместимый профиль позволяет
// оставить код Windows-версии как есть.

#pragma once

#include <QMap>
#include <QOpenGLWidget>

#include "mfc_view.h"

class MfcGLWidget : public QOpenGLWidget, public MfcQtHost
{
    Q_OBJECT

public:
    explicit MfcGLWidget(QWidget *parent = nullptr);
    ~MfcGLWidget() override;

    /// Привязать вид. Владение остаётся у вызывающего.
    void setView(CScrollView *view);
    CScrollView *view() const { return m_view; }

    /// Формат поверхности с совместимым профилем — задать ДО создания виджетов.
    static void setupSurfaceFormat();

signals:
    void statusTextChanged(int pane, const QString &text);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

    // --- MfcQtHost -----------------------------------------------------------
    void hostInvalidate(const RECT *logicalRect, BOOL erase) override;
    void hostUpdateWindow() override;
    void hostGetClientRect(RECT *deviceRect) const override;
    void hostSetCapture() override;
    void hostReleaseCapture() override;
    UINT hostSetTimer(UINT id, UINT elapseMs) override;
    BOOL hostKillTimer(UINT id) override;
    int  hostMessageBox(const char *text, const char *caption, UINT type) override;
    void hostSetScrollSizes(const SIZE &total, const SIZE &page, const SIZE &line) override;
    CPoint hostScrollPosition() const override;
    void hostScrollToPosition(const POINT &devicePos) override;
    void hostSetStatusText(int pane, const char *text) override;
    int  hostLogicalDpiX() const override;
    int  hostLogicalDpiY() const override;

    /// Пришёл ли вызов из главного потока (в котором живёт виджет).
    /// Код Windows-версии зовёт перерисовку и окна сообщений в том числе из
    /// расчётного потока; в Qt это разрешено только главному, поэтому такие
    /// вызовы откладываются в его очередь событий.
    bool inGuiThread() const;

private:
    /// Вывести надписи, собранные видом во время отрисовки сцены.
    void drawDeferredText(const CDC &dc);

    UINT mouseFlags(QMouseEvent *event) const;
    CPoint devicePoint(QMouseEvent *event) const;

    CScrollView    *m_view = nullptr;
    QMap<int, UINT> m_timers;
    bool            m_initialDone = false;
    bool            m_painting = false;
};
