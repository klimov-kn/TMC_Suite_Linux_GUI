// mfcviewwidget.h — виджет-хозяин для вида, перенесённого из Windows-версии.
//
// Вид (CScrollView и наследники) компилируется из исходников Windows-версии и
// умеет рисовать себя в CDC и отвечать на события мыши. Этот виджет даёт ему всё
// остальное: холст QPainter, прокрутку, захват мыши, таймер, окна сообщений.
//
// Разделение обязанностей:
//   вид      — ЧТО рисовать и как реагировать (код Windows-версии, не тронут);
//   виджет   — ГДЕ рисовать и откуда берутся события (Qt).

#pragma once

#include <QAbstractScrollArea>
#include <QColor>
#include <QElapsedTimer>
#include <QMap>

#include "mfc_view.h"

class MfcViewWidget : public QAbstractScrollArea, public MfcQtHost
{
    Q_OBJECT

public:
    explicit MfcViewWidget(QWidget *parent = nullptr);
    ~MfcViewWidget() override;

    /// Привязать вид. Владение остаётся у вызывающего.
    void setView(CScrollView *view);
    CScrollView *view() const { return m_view; }

    /// Вызвать OnInitialUpdate у вида (после того, как виджет получил размер).
    void initialUpdate();

    /// Цвет подложки области рисования. В Windows-версии фон давало само окно
    /// (белый), а документ хранит свой цвет — его и ставим, чтобы картинка
    /// совпадала с оригиналом.
    void setDocumentBackground(const QColor &color);

signals:
    /// Текст для строки состояния: вид шлёт его через hostSetStatusText.
    void statusTextChanged(int pane, const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
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
    /// Показать текст немедленно (без ограничения частоты).
    void hostSetStatusTextImmediate(int pane, const char *text);
    int  hostLogicalDpiX() const override;
    int  hostLogicalDpiY() const override;

    /// Пришёл ли вызов из главного потока (в котором живёт виджет).
    ///
    /// Расчёт в TMC идёт в отдельном потоке (AfxBeginThread), а код
    /// Windows-версии зовёт оттуда RedrawWindow/Invalidate и окна сообщений.
    /// В Windows это было можно, в Qt — нет: работать с виджетами разрешено
    /// только главному потоку. Поэтому такие вызовы откладываются в его
    /// очередь событий. Порядок вычислений от этого не меняется.
    bool inGuiThread() const;

private:
    UINT mouseFlags(QMouseEvent *event) const;
    CPoint devicePoint(QMouseEvent *event) const;

    CScrollView      *m_view = nullptr;
    bool              m_painting = false;   // защита от перерисовки внутри OnDraw
    QColor            m_background = Qt::white;

    // Ограничение частоты обновления строки состояния. Счётное ядро сообщает о
    // ходе работы на каждом такте — это тысячи сообщений в секунду. Человек
    // столько не читает, а очередь событий забивается, поэтому пропускаем не
    // чаще, чем раз в 50 мс, но последнее сообщение показываем обязательно.
    QMap<int, QString> m_pendingStatus;
    qint64             m_lastStatusMs = 0;
    QMap<int, UINT>   m_timers;      // идентификатор Qt -> идентификатор MFC
    bool              m_initialDone = false;
};
