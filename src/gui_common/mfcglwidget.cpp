#include "mfcglwidget.h"

#include <QMessageBox>
#include <QMetaObject>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QSurfaceFormat>
#include <QThread>
#include <QTimerEvent>

MfcGLWidget::MfcGLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

MfcGLWidget::~MfcGLWidget()
{
    if (m_view)
        m_view->SetHost(nullptr);
}

void MfcGLWidget::setupSurfaceFormat()
{
    // Совместимый профиль нужен потому, что отрисовка написана в старом стиле
    // (glBegin/glEnd, матрицы). Глубина буфера — как в Windows-версии, где
    // формат пикселей просил 24 бита глубины и двойную буферизацию.
    QSurfaceFormat fmt;
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    fmt.setVersion(2, 1);
    fmt.setDepthBufferSize(24);
    fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(fmt);
}

void MfcGLWidget::setView(CScrollView *view)
{
    m_view = view;
    if (m_view)
        m_view->SetHost(this);
}

void MfcGLWidget::initializeGL()
{
    if (m_view && !m_initialDone) {
        m_initialDone = true;
        // В Windows-версии отсюда шёл GL_Init(): создание контекста и настройка
        // проекции. Контекст уже создан Qt и сделан текущим, а всё остальное в
        // коде вида выполняется как есть.
        m_view->OnInitialUpdate();
    }
}

void MfcGLWidget::resizeGL(int w, int h)
{
    if (m_view)
        m_view->OnSize(0, w, h);
}

void MfcGLWidget::paintGL()
{
    if (!m_view)
        return;

    // Сама сцена идёт через OpenGL, контекст рисования нужен виду формально.
    // Но вид выводит через него сообщения (например, «нет файла топологии»),
    // поэтому надписи собираются во время отрисовки и выводятся поверх кадра.
    CDC dc;
    dc.Attach(nullptr, nullptr);
    dc.SetDeferText(true);

    m_painting = true;
    m_view->OnDraw(&dc);
    m_painting = false;

    drawDeferredText(dc);
}

void MfcGLWidget::drawDeferredText(const CDC &dc)
{
    if (dc.deferredText().empty())
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    for (const CDC::DeferredText &item : dc.deferredText()) {
        QFont font = painter.font();
        if (item.font.lfFaceName[0])
            font.setFamily(QString::fromLatin1(item.font.lfFaceName));
        if (item.font.lfHeight)
            font.setPixelSize(int(item.font.lfHeight < 0 ? -item.font.lfHeight
                                                         : item.font.lfHeight));
        painter.setFont(font);
        painter.setPen(QColor(int(GetRValue(item.color)),
                              int(GetGValue(item.color)),
                              int(GetBValue(item.color))));

        // Строки в исходниках Windows-версии записаны в CP1251.
        const QString text = QString::fromLocal8Bit(item.text.c_str());
        QFontMetrics fm(font);
        painter.drawText(int(item.x), int(item.y) + fm.ascent(), text);
    }
}

UINT MfcGLWidget::mouseFlags(QMouseEvent *event) const
{
    UINT flags = 0;
    if (event->buttons() & Qt::LeftButton)   flags |= MK_LBUTTON;
    if (event->buttons() & Qt::RightButton)  flags |= MK_RBUTTON;
    if (event->buttons() & Qt::MiddleButton) flags |= MK_MBUTTON;
    if (event->modifiers() & Qt::ShiftModifier)   flags |= MK_SHIFT;
    if (event->modifiers() & Qt::ControlModifier) flags |= MK_CONTROL;
    return flags;
}

CPoint MfcGLWidget::devicePoint(QMouseEvent *event) const
{
    const QPoint p = event->position().toPoint();
    return CPoint(p.x(), p.y());
}

void MfcGLWidget::mousePressEvent(QMouseEvent *event)
{
    if (!m_view) {
        QOpenGLWidget::mousePressEvent(event);
        return;
    }
    if (event->button() == Qt::LeftButton)
        m_view->OnLButtonDown(mouseFlags(event), devicePoint(event));
    else if (event->button() == Qt::RightButton)
        m_view->OnRButtonDown(mouseFlags(event), devicePoint(event));
    update();
}

void MfcGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    // Дочернее окно многодокументного режима могло захватить мышь для
    // перетаскивания и не получить отпускание. Снимаем такой захват сразу:
    // иначе все дальнейшие щелчки уйдут ему, и программа перестанет отвечать.
    if (QWidget *grabber = QWidget::mouseGrabber()) {
        if (event->buttons() == Qt::NoButton)
            grabber->releaseMouse();
    }

    if (m_view && event->button() == Qt::LeftButton)
        m_view->OnLButtonUp(mouseFlags(event), devicePoint(event));
    update();
}

void MfcGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_view && event->button() == Qt::LeftButton)
        m_view->OnLButtonDblClk(mouseFlags(event), devicePoint(event));
    update();
}

void MfcGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_view)
        m_view->OnMouseMove(mouseFlags(event), devicePoint(event));
}

void MfcGLWidget::timerEvent(QTimerEvent *event)
{
    QMap<int, UINT>::const_iterator it = m_timers.find(event->timerId());
    if (it != m_timers.end() && m_view)
        m_view->OnTimer(it.value());
}

// --- MfcQtHost ----------------------------------------------------------------

bool MfcGLWidget::inGuiThread() const
{
    // Виджет живёт в главном потоке; сравнение с ним и отвечает на вопрос,
    // можно ли трогать окно прямо сейчас.
    return QThread::currentThread() == thread();
}

void MfcGLWidget::hostInvalidate(const RECT *, BOOL)
{
    if (!inGuiThread()) {
        // Вызов из расчётного потока: в Qt трогать виджет оттуда нельзя,
        // поэтому просьба перерисоваться кладётся в очередь главного потока.
        QMetaObject::invokeMethod(this, [this]() { update(); }, Qt::QueuedConnection);
        return;
    }
    update();
}

void MfcGLWidget::hostUpdateWindow()
{
    if (!inGuiThread()) {
        // Немедленной перерисовки из чужого потока не бывает: контекст OpenGL
        // принадлежит главному потоку. Откладываем на него.
        QMetaObject::invokeMethod(this, [this]() { update(); }, Qt::QueuedConnection);
        return;
    }
    // Немедленная перерисовка внутри самой перерисовки в Qt запрещена.
    if (m_painting)
        update();
    else
        repaint();
}

void MfcGLWidget::hostGetClientRect(RECT *deviceRect) const
{
    if (!deviceRect)
        return;
    deviceRect->left = 0;
    deviceRect->top = 0;
    deviceRect->right = width();
    deviceRect->bottom = height();
}

void MfcGLWidget::hostSetCapture()
{
    // Не захватываем мышь: см. пояснение в MfcViewWidget. Qt и так доставляет
    // движения тому же виджету, пока кнопка зажата, а забытый захват лишает
    // программу отклика.
}

void MfcGLWidget::hostReleaseCapture()
{
    if (mouseGrabber() == this)
        releaseMouse();
}

UINT MfcGLWidget::hostSetTimer(UINT id, UINT elapseMs)
{
    const int qtId = startTimer(int(elapseMs));
    m_timers.insert(qtId, id);
    return id;
}

BOOL MfcGLWidget::hostKillTimer(UINT id)
{
    for (QMap<int, UINT>::iterator it = m_timers.begin(); it != m_timers.end(); ++it) {
        if (it.value() == id) {
            killTimer(it.key());
            m_timers.erase(it);
            return TRUE;
        }
    }
    return FALSE;
}

int MfcGLWidget::hostMessageBox(const char *text, const char *caption, UINT)
{
    const QString t = text ? QString::fromLocal8Bit(text) : QString();
    const QString c = caption ? QString::fromLocal8Bit(caption) : windowTitle();

    if (!inGuiThread()) {
        // Модальное окно из расчётного потока повесило бы программу. Показываем
        // его в главном потоке, а расчёт продолжается: в Windows-версии на
        // ответ здесь не смотрят.
        QMetaObject::invokeMethod(this, [this, t, c]() {
            QMessageBox::information(this, c, t);
        }, Qt::QueuedConnection);
        return 1;
    }

    QMessageBox::information(this, c, t);
    return 1;
}

void MfcGLWidget::hostSetScrollSizes(const SIZE &, const SIZE &, const SIZE &)
{
    // Сцена OpenGL занимает всё окно: прокрутки у неё нет, масштаб меняется
    // командами Zoom, как в Windows-версии.
}

CPoint MfcGLWidget::hostScrollPosition() const
{
    return CPoint(0, 0);
}

void MfcGLWidget::hostScrollToPosition(const POINT &)
{
}

void MfcGLWidget::hostSetStatusText(int pane, const char *text)
{
    emit statusTextChanged(pane, text ? QString::fromLocal8Bit(text) : QString());
}

int MfcGLWidget::hostLogicalDpiX() const
{
    return logicalDpiX();
}

int MfcGLWidget::hostLogicalDpiY() const
{
    return logicalDpiY();
}
