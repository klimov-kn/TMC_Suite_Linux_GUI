// mfc_gdi.h — рисование MFC поверх Qt.
//
// Виды программ TMC (графики, поля, диаграммы) рисуют через CDC: MoveTo/LineTo,
// перья, кисти, шрифты, режим отображения MM_HIMETRIC. Переписывать эти 3000
// строк на QPainter значит переписывать саму картинку — расхождения полезут в
// мелочах: толщина линии, шаг сетки, положение подписи.
//
// Поэтому код вида компилируется из Windows-версии как есть, а здесь
// реализованы те классы GDI, которыми он пользуется. Внутри каждого — QPainter.
//
// Как устроено отображение. GDI считает так:
//   устройство = (логическая - НачалоОкна) * РазмерОбласти / РазмерОкна + НачалоОбласти
// Для MM_HIMETRIC логическая единица равна 0.01 мм, ось Y направлена ВВЕРХ.
// Мы повторяем эту формулу буквально, а не подменяем её преобразованием QPainter:
// так толщина пера и размер шрифта пересчитываются в пиксели теми же числами,
// что и на Windows.

#pragma once

#ifndef _WIN32

#include "tmc_mfc_doc.h"

#include <cmath>
#include <string>
#include <vector>

class QPainter;
class QPaintDevice;
// Палитра объявлена в слое OpenGL (mfc_wgl.h): 256-цветные режимы нужны были
// только ему. Здесь достаточно предварительного объявления — методы CDC ниже
// принимают и возвращают указатель.
class CPalette;

// --- Описатели GDI -----------------------------------------------------------
// В Windows это непрозрачные числа, которыми система обозначает контекст
// рисования, шрифт и растр. Здесь всеми тремя владеет Qt, поэтому описатель —
// только метка: код Windows-версии сверяет её с нулём и передаёт дальше.
typedef void* HDC;        // контекст рисования (нужен и слою OpenGL, mfc_wgl.h)
typedef void* HFONT;      // шрифт: под Linux шрифтом владеет Qt
typedef void* HBITMAP;    // растр

// --- Точка, размер, прямоугольник -------------------------------------------
struct POINT { long x; long y; };
struct SIZE  { long cx; long cy; };
struct RECT  { long left; long top; long right; long bottom; };

// SIZEL в Windows — та же пара координат, что и SIZE (буква L осталась от
// 16-битных времён). Встречается в объявлениях CTmcGLText (FieldView).
typedef SIZE   SIZEL;
typedef SIZEL* PSIZEL;

// Описание растра GDI. Нужно, чтобы компилировалось объявление
// CTmcGLText::getBitmap; сам растр строки под Linux строится средствами Qt
// (src/apps/fieldview/tmcgltext_qt.cpp).
struct BITMAP
{
    long  bmType;
    long  bmWidth;
    long  bmHeight;
    long  bmWidthBytes;
    WORD  bmPlanes;
    WORD  bmBitsPixel;
    void* bmBits;
};

class CPoint : public POINT
{
public:
    CPoint() { x = 0; y = 0; }
    CPoint(long ax, long ay) { x = ax; y = ay; }
    CPoint(const POINT& p) { x = p.x; y = p.y; }
    void Offset(long dx, long dy) { x += dx; y += dy; }
    bool operator==(const CPoint& o) const { return x == o.x && y == o.y; }
    bool operator!=(const CPoint& o) const { return !(*this == o); }
};

class CSize : public SIZE
{
public:
    CSize() { cx = 0; cy = 0; }
    CSize(long acx, long acy) { cx = acx; cy = acy; }
    CSize(const SIZE& s) { cx = s.cx; cy = s.cy; }
};

class CRect : public RECT
{
public:
    CRect() { left = top = right = bottom = 0; }
    CRect(long l, long t, long r, long b) { left = l; top = t; right = r; bottom = b; }
    CRect(const RECT& r) { left = r.left; top = r.top; right = r.right; bottom = r.bottom; }

    long Width()  const { return right - left; }
    long Height() const { return bottom - top; }
    CPoint TopLeft() const { return CPoint(left, top); }
    CPoint BottomRight() const { return CPoint(right, bottom); }
    void SetRect(long l, long t, long r, long b) { left = l; top = t; right = r; bottom = b; }
    void SetRectEmpty() { left = top = right = bottom = 0; }
    BOOL IsRectEmpty() const { return (right <= left || bottom <= top) ? TRUE : FALSE; }
    BOOL PtInRect(const POINT& p) const
    {
        return (p.x >= left && p.x < right && p.y >= top && p.y < bottom) ? TRUE : FALSE;
    }
    void NormalizeRect()
    {
        if (left > right) { long t2 = left; left = right; right = t2; }
        if (top > bottom) { long t2 = top; top = bottom; bottom = t2; }
    }
};

// --- Режимы отображения (wingdi.h) ------------------------------------------
#define MM_TEXT        1
#define MM_LOMETRIC    2
#define MM_HIMETRIC    3
#define MM_LOENGLISH   4
#define MM_HIENGLISH   5
#define MM_TWIPS       6
#define MM_ISOTROPIC   7
#define MM_ANISOTROPIC 8

#define TRANSPARENT 1
#define OPAQUE      2

#define BLACK_PEN    7
#define WHITE_PEN    6
#define NULL_PEN     8
#define BLACK_BRUSH  4
#define WHITE_BRUSH  0
#define NULL_BRUSH   5
#define HOLLOW_BRUSH NULL_BRUSH

#define PATCOPY   0x00F00021
#define BLACKNESS 0x00000042
#define WHITENESS 0x00FF0062

struct TEXTMETRIC
{
    long tmHeight;
    long tmAscent;
    long tmDescent;
    long tmInternalLeading;
    long tmExternalLeading;
    long tmAveCharWidth;
    long tmMaxCharWidth;
    long tmWeight;
};

// --- Объекты рисования -------------------------------------------------------
class CGdiObject
{
public:
    virtual ~CGdiObject() {}
    enum Kind { kNone, kPen, kBrush, kFont };
    virtual Kind kind() const { return kNone; }
};

class CPen : public CGdiObject
{
public:
    CPen() : m_style(PS_SOLID), m_width(0), m_color(0) {}
    CPen(int style, int width, COLORREF color)
        : m_style(style), m_width(width), m_color(color) {}
    BOOL CreatePen(int style, int width, COLORREF color)
    {
        m_style = style; m_width = width; m_color = color;
        return TRUE;
    }
    BOOL DeleteObject() { return TRUE; }
    Kind kind() const { return kPen; }

    int      m_style;
    int      m_width;      // в логических единицах, как в GDI
    COLORREF m_color;
};

class CBrush : public CGdiObject
{
public:
    CBrush() : m_color(0), m_null(TRUE) {}
    explicit CBrush(COLORREF color) : m_color(color), m_null(FALSE) {}
    BOOL CreateSolidBrush(COLORREF color) { m_color = color; m_null = FALSE; return TRUE; }
    BOOL DeleteObject() { return TRUE; }
    Kind kind() const { return kBrush; }

    COLORREF m_color;
    BOOL     m_null;
};

class CFont : public CGdiObject
{
public:
    CFont() { std::memset(&m_lf, 0, sizeof(m_lf)); }
    BOOL CreateFontIndirect(const LOGFONT* lf)
    {
        if (lf) m_lf = *lf;
        return TRUE;
    }
    BOOL DeleteObject() { return TRUE; }
    Kind kind() const { return kFont; }

    LOGFONT m_lf;
};

// --- Контекст рисования ------------------------------------------------------
class CDC
{
public:
    CDC();
    virtual ~CDC();

    // Связь с Qt. Устанавливает виджет-хозяин перед вызовом OnDraw.
    void Attach(QPainter* painter, QPaintDevice* device);
    QPainter* painter() const { return m_painter; }

    // --- Режим отображения ---------------------------------------------------
    virtual int  SetMapMode(int mode);
    int  GetMapMode() const { return m_mapMode; }
    CSize SetWindowExt(int cx, int cy);
    CSize SetViewportExt(int cx, int cy);
    CPoint SetWindowOrg(int x, int y);
    CPoint SetViewportOrg(int x, int y);
    CPoint GetWindowOrg() const { return m_winOrg; }
    CPoint GetViewportOrg() const { return m_vpOrg; }

    void LPtoDP(POINT* points, int count = 1) const;
    void DPtoLP(POINT* points, int count = 1) const;
    void LPtoDP(RECT* rect) const;
    void DPtoLP(RECT* rect) const;
    // Код видов передаёт прямоугольник и по ссылке — как в MFC.
    void LPtoDP(RECT& rect) const { LPtoDP(&rect); }
    void DPtoLP(RECT& rect) const { DPtoLP(&rect); }
    void DPtoLP(POINT& p) const { DPtoLP(&p, 1); }
    void LPtoDP(POINT& p) const { LPtoDP(&p, 1); }

    // --- Объекты -------------------------------------------------------------
    CGdiObject* SelectObject(CGdiObject* obj);
    CPen*   SelectObject(CPen* pen);
    CBrush* SelectObject(CBrush* brush);
    CFont*  SelectObject(CFont* font);
    CGdiObject* SelectStockObject(int index);

    // --- Рисование -----------------------------------------------------------
    CPoint MoveTo(int x, int y);
    CPoint MoveTo(POINT p) { return MoveTo(p.x, p.y); }
    BOOL   LineTo(int x, int y);
    BOOL   LineTo(POINT p) { return LineTo(p.x, p.y); }
    CPoint GetCurrentPosition() const { return m_cur; }
    BOOL   Polyline(const POINT* points, int count);
    BOOL   Rectangle(int l, int t, int r, int b);
    BOOL   Rectangle(const RECT* r) { return Rectangle(r->left, r->top, r->right, r->bottom); }
    BOOL   Ellipse(int l, int t, int r, int b);
    void   FillSolidRect(const RECT* r, COLORREF color);
    void   FillSolidRect(int x, int y, int cx, int cy, COLORREF color);
    BOOL   PatBlt(int x, int y, int w, int h, DWORD rop);

    BOOL   TextOut(int x, int y, const char* text, int len);
    BOOL   TextOut(int x, int y, const CString& text);
    CSize  GetTextExtent(const char* text, int len) const;
    CSize  GetTextExtent(const CString& text) const;
    BOOL   GetTextMetrics(TEXTMETRIC* tm) const;

    COLORREF SetTextColor(COLORREF color);
    COLORREF GetTextColor() const { return m_textColor; }
    COLORREF SetBkColor(COLORREF color);
    COLORREF GetBkColor() const { return m_bkColor; }
    int      SetBkMode(int mode);

    BOOL GetClipBox(RECT* r) const;

    // Описатель контекста Windows. Нужен только как значение, которое код
    // Windows-версии передаёт в вызовы OpenGL: контекстом владеет Qt.
    void* GetSafeHdc() const { return 0; }

    // --- Текст поверх сцены OpenGL -------------------------------------------
    // Вид FieldView рисует сцену через OpenGL, но сообщения (например, «нет
    // файла топологии») выводит обычным TextOut. Рисовать их тем же контекстом
    // нельзя: холста в этот момент нет. Поэтому в режиме OpenGL текст
    // запоминается, а виджет-хозяин выводит его поверх готового кадра.
    struct DeferredText
    {
        long     x;
        long     y;
        std::string text;    // в кодировке CP1251, как в исходниках
        COLORREF color;
        LOGFONT  font;
    };

    void SetDeferText(bool defer) { m_deferText = defer; }
    const std::vector<DeferredText>& deferredText() const { return m_deferredText; }
    void ClearDeferredText() { m_deferredText.clear(); }
    BOOL IsPrinting() const { return m_printing; }
    void SetPrinting(BOOL p) { m_printing = p; }

    int GetDeviceCaps(int index) const;

    // --- Палитра -------------------------------------------------------------
    // В Windows 256-цветные режимы требовали выбрать палитру в контекст и
    // «воплотить» её (FieldView, CFldViewView::CreateRGBPalette). Qt всегда даёт
    // полноцветную поверхность, поэтому ветка кода, которая это делает, не
    // выполняется, а вызовы ничего не меняют. Возвращается прежняя палитра,
    // как в MFC (её нет — значит ноль).
    CPalette* SelectPalette(CPalette* palette, BOOL forceBackground);
    UINT      RealizePalette();

protected:
    void   applyPen();
    void   applyFont();
    long   deviceX(long lx) const;
    long   deviceY(long ly) const;
    long   logicalX(long dx) const;
    long   logicalY(long dy) const;
    double scaleX() const;
    double scaleY() const;

    QPainter*     m_painter;
    QPaintDevice* m_device;

    int    m_mapMode;
    CSize  m_winExt;
    CSize  m_vpExt;
    CPoint m_winOrg;
    CPoint m_vpOrg;

    CPoint   m_cur;
    CPen*    m_pen;
    CBrush*  m_brush;
    CFont*   m_font;
    COLORREF m_textColor;
    COLORREF m_bkColor;
    bool     m_deferText = false;
    std::vector<DeferredText> m_deferredText;
    int      m_bkMode;
    BOOL     m_printing;
};

// Константы GetDeviceCaps, которыми пользуются виды.
#define HORZSIZE     4
#define VERTSIZE     6
#define HORZRES      8
#define VERTRES     10
#define LOGPIXELSX  88
#define LOGPIXELSY  90

// Те же сведения об устройстве, но как свободная функция Windows API: код вида
// зовёт её с описателем контекста OpenGL —
// `GetDeviceCaps( wglGetCurrentDC(), LOGPIXELSY )` (fldviewview.cpp,
// tmcdialogplace.cpp). Отсюда берётся высота шрифта подписей осей (12 пунктов),
// поэтому значение должно быть настоящим разрешением экрана.
int GetDeviceCaps(HDC dc, int index);

#endif // !_WIN32
