// mfc_view.h — окна и виды MFC поверх Qt.
//
// Продолжение слоя совместимости: здесь то, от чего наследуются виды программ
// TMC (CScrollView) и что они зовут у окна — перерисовка, захват мыши, таймер,
// прокрутка, подготовка контекста рисования.
//
// Qt-виджет не наследуется от этих классов, а РЕАЛИЗУЕТ интерфейс MfcQtHost и
// отдаёт виду события. Благодаря этому в слое совместимости нет ни одного
// включения Qt: он собирается и без графики.

#pragma once

#ifndef _WIN32

#include "mfc_gdi.h"

// --- Отладочные макросы MFC --------------------------------------------------
// В Windows-сборке они разворачивались в проверки отладчика. Здесь пустые:
// падать на пользователе из-за проверки — хуже, чем нарисовать что-то не так.
#ifndef ASSERT
  #define ASSERT(expr)        ((void)0)
#endif
#ifndef ASSERT_VALID
  #define ASSERT_VALID(p)     ((void)0)
#endif
#ifndef VERIFY
  #define VERIFY(expr)        ((void)(expr))
#endif
#ifndef TRACE
  #define TRACE               (void)sizeof
#endif
#ifndef RUNTIME_CLASS
  #define RUNTIME_CLASS(cls)  ((void*)0)
#endif
#ifndef AFX_IDW_PANE_FIRST
  #define AFX_IDW_PANE_FIRST 0xE900
#endif

// Стили окна, встречающиеся в PreCreateWindow. Значения как в Windows.
#define WS_HSCROLL      0x00100000L
#define WS_VSCROLL      0x00200000L
#define WS_MAXIMIZE     0x01000000L
#define SB_HORZ 0
#define SB_VERT 1

#define GRAY_BRUSH   2
#define LTGRAY_BRUSH 1
#define DKGRAY_BRUSH 3
#define PS_COSMETIC  0x00000000
#define PS_GEOMETRIC 0x00010000

// Первое сообщение, которое программа может определить сама. От него отсчитаны
// собственные сообщения TMC: «применить страницу свойств» (WM_USERAPPLY_* в
// fldview.h) и «окно уровней цвета закрылось» (WM_COLOR_LEVEL_DIALOG_BYE в
// colorleveldlg.h). Значение как в Windows.
#ifndef WM_USER
  #define WM_USER 0x0400
#endif

// Кнопки и клавиши-модификаторы в обработчиках мыши.
#define MK_LBUTTON  0x0001
#define MK_RBUTTON  0x0002
#define MK_SHIFT    0x0004
#define MK_CONTROL  0x0008
#define MK_MBUTTON  0x0010

struct CREATESTRUCT
{
    void*        lpCreateParams;
    void*        hInstance;
    void*        hMenu;
    void*        hwndParent;
    int          cy, cx, y, x;
    long         style;
    const char*  lpszName;
    const char*  lpszClass;
    DWORD        dwExStyle;
};

class CPrintInfo
{
public:
    BOOL m_bPreview = FALSE;
    UINT m_nCurPage = 1;
};

// --- Мост к Qt ----------------------------------------------------------------
// Всё, что виду нужно от настоящего окна. Реализует Qt-виджет-хозяин.
class MfcQtHost
{
public:
    virtual ~MfcQtHost() {}

    virtual void hostInvalidate(const RECT* logicalRect, BOOL erase) = 0;
    virtual void hostUpdateWindow() = 0;
    virtual void hostGetClientRect(RECT* deviceRect) const = 0;
    virtual void hostSetCapture() = 0;
    virtual void hostReleaseCapture() = 0;
    virtual UINT hostSetTimer(UINT id, UINT elapseMs) = 0;
    virtual BOOL hostKillTimer(UINT id) = 0;
    virtual int  hostMessageBox(const char* text, const char* caption, UINT type) = 0;
    virtual void hostSetScrollSizes(const SIZE& total, const SIZE& page, const SIZE& line) = 0;
    virtual CPoint hostScrollPosition() const = 0;      // в устройстве (пикселях)
    virtual void hostScrollToPosition(const POINT& devicePos) = 0;
    virtual void hostSetStatusText(int pane, const char* text) = 0;
    virtual int  hostLogicalDpiX() const = 0;
    virtual int  hostLogicalDpiY() const = 0;
};

// --- CWnd ---------------------------------------------------------------------
class CWnd : public CObject
{
public:
    CWnd() : m_host(0) {}
    virtual ~CWnd() {}

    void SetHost(MfcQtHost* host) { m_host = host; }
    MfcQtHost* Host() const { return m_host; }

    void Invalidate(BOOL erase = TRUE);
    void InvalidateRect(const RECT* rect, BOOL erase = TRUE);
    void InvalidateRect(const RECT& rect, BOOL erase = TRUE) { InvalidateRect(&rect, erase); }
    void UpdateWindow();
    // В MFC перерисовывает окно немедленно; здесь — то же самое через Qt.
    void RedrawWindow(const RECT* = 0, void* = 0, UINT = 0) { UpdateWindow(); }
    void GetClientRect(RECT* rect) const;
    void SetCapture();
    void ReleaseCapture();
    UINT SetTimer(UINT id, UINT elapse, void* proc = 0);
    BOOL KillTimer(UINT id);
    int  MessageBox(const char* text, const char* caption = 0, UINT type = 0);
    CWnd* GetParentFrame() { return this; }
    BOOL IsKindOf(const void*) const { return TRUE; }

    // --- Сообщения окну -------------------------------------------------------
    // В Windows окна обмениваются сообщениями через очередь: диалог уровней
    // цвета сообщает виду, что закрылся, а страница свойств — что нажали
    // «Применить». Обработчик такого сообщения объявлен картой ON_MESSAGE.
    // Здесь очереди Windows нет, поэтому сообщение ищется в той же карте и
    // обработчик вызывается напрямую:
    //   SendMessage — сразу, как в Windows;
    //   PostMessage — после возврата в цикл событий Qt (это важно: отправитель
    //   обычно просит закрыть себя же, и разрушать его посреди его метода
    //   нельзя).
    /// Возвращает результат обработчика (0, если обработчика нет).
    LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
    /// Возвращает TRUE, если сообщение поставлено в очередь.
    BOOL    PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);

    /// Описатель окна. Настоящих окон Windows нет: у вида это виджет-хозяин, у
    /// диалога — его окно Qt (см. CDialog). Код Windows-версии сверяет
    /// результат только с нулём («окна ещё нет — создать»).
    virtual HWND GetSafeHwnd() const { return static_cast<HWND>(m_host); }

    // Рисование прямо в элементе (образец линии в диалоге цвета). В Qt рисовать
    // по виджету вне перерисовки нельзя, поэтому обёртка элемента отдаёт холст
    // над картинкой и подставляет её элементу в ReleaseDC.
    virtual CDC* GetDC() { return 0; }
    virtual void ReleaseDC(CDC*) {}

    /// Закрытие окна. В Qt окном распоряжается виджет-хозяин, поэтому здесь
    /// только уведомление вида (OnDestroy) — как в MFC перед разрушением.
    virtual void OnDestroy() {}
    virtual BOOL DestroyWindow() { OnDestroy(); return TRUE; }

protected:
    MfcQtHost* m_host;
};

// --- Контексты рисования ------------------------------------------------------
// CClientDC в коде видов используется только для перевода координат мыши
// (DPtoLP): само рисование идёт в OnDraw. Поэтому холста внутри нет — есть
// разрешение экрана, которого достаточно для пересчёта.
class CClientDC : public CDC
{
public:
    explicit CClientDC(CWnd* wnd);
};

class CPaintDC : public CDC
{
public:
    explicit CPaintDC(CWnd* wnd) : m_wnd(wnd) {}
    CWnd* m_wnd;
};

// --- CView --------------------------------------------------------------------
class CView : public CWnd
{
public:
    CView() : m_pDocument(0) {}

    virtual void OnDraw(CDC* pDC) = 0;
    virtual void OnInitialUpdate() {}
    virtual void OnPrepareDC(CDC* pDC, CPrintInfo* info = 0);
    virtual BOOL PreCreateWindow(CREATESTRUCT&) { return TRUE; }
    virtual BOOL OnPreparePrinting(CPrintInfo*) { return TRUE; }
    virtual void OnBeginPrinting(CDC*, CPrintInfo*) {}
    virtual void OnEndPrinting(CDC*, CPrintInfo*) {}
    BOOL DoPreparePrinting(CPrintInfo*) { return TRUE; }

    // Обработчики, которые вызывает Qt-хозяин при событиях мыши, клавиш и
    // таймера. Виды переопределяют те, что им нужны.
    virtual void OnLButtonDown(UINT, CPoint) {}
    virtual void OnLButtonUp(UINT, CPoint) {}
    virtual void OnLButtonDblClk(UINT, CPoint) {}
    virtual void OnRButtonDown(UINT, CPoint) {}
    virtual void OnMouseMove(UINT, CPoint) {}
    /// Таймер. Тип UINT_PTR — как в подписях Windows-версии: иначе метод вида
    /// не переопределяет этот, и обработчик никогда не вызывается.
    virtual void OnTimer(UINT_PTR) {}
    /// Изменение размера окна: в MFC приходило сообщением, здесь зовёт хозяин.
    virtual void OnSize(UINT /*type*/, int /*cx*/, int /*cy*/) {}
    /// Закрытие окна вида. В MFC приходило сообщением WM_CLOSE; здесь зовёт
    /// Qt-слой, когда пользователь закрывает окно документа.
    virtual void OnClose() {}

    CDocument* m_pDocument;
};

// --- CScrollView --------------------------------------------------------------
// Прокрутка в логических единицах выбранного режима отображения, как в MFC.
class CScrollView : public CView
{
public:
    CScrollView();

    void SetScrollSizes(int mapMode, SIZE sizeTotal,
                        const SIZE& sizePage = SIZE(),
                        const SIZE& sizeLine = SIZE());
    CPoint GetScrollPosition() const;          // в логических единицах
    CPoint GetDeviceScrollPosition() const;    // в пикселях
    void   ScrollToPosition(POINT pt);         // в логических единицах
    CSize  GetTotalSize() const { return m_totalLog; }

    virtual void OnPrepareDC(CDC* pDC, CPrintInfo* info = 0);

    // Базовые обработчики MFC: код видов зовёт их как «поведение по умолчанию».
    virtual void OnLButtonDown(UINT, CPoint) {}
    virtual void OnLButtonUp(UINT, CPoint) {}
    virtual void OnLButtonDblClk(UINT, CPoint) {}
    virtual void OnRButtonDown(UINT, CPoint) {}
    virtual void OnMouseMove(UINT, CPoint) {}
    virtual void OnTimer(UINT_PTR) {}
    virtual void OnSize(UINT, int, int) {}

    // Команды печати каркаса MFC. Печать в Qt-версии делает сам слой интерфейса,
    // поэтому здесь только точки, на которые ссылается карта сообщений.
    virtual void OnFilePrint() {}
    virtual void OnFilePrintPreview() {}

    int GetMapModeCompat() const { return m_mapMode; }

protected:
    int   m_mapMode;
    CSize m_totalLog;
    CSize m_pageLog;
    CSize m_lineLog;
};

// Оконный слой OpenGL (wglCreateContext, формат пикселей, палитра). Подключён
// здесь по той же причине, по какой на Windows его давал windows.h вместе с
// обычными оконными заголовками: код вида FieldView рассчитывает, что эти
// объявления приходят сами.
#include "mfc_wgl.h"

#endif // !_WIN32
