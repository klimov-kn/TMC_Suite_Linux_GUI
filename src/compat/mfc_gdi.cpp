// Реализация рисования MFC поверх QPainter (см. mfc_gdi.h).
//
// Каждая координата переводится по формуле GDI, а не преобразованием QPainter:
// так толщина пера и размер шрифта считаются теми же числами, что на Windows,
// и картинка совпадает вплоть до пикселя сетки.

#ifndef _WIN32

#include "mfc_gdi.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QPaintDevice>
#include <QScreen>
#include <QPainter>
#include <QPen>
#include <QRect>
#include <QStringList>
#include <QString>

namespace {

QColor toQColor(COLORREF c)
{
    // COLORREF в Windows это 0x00BBGGRR.
    return QColor(int(GetRValue(c)), int(GetGValue(c)), int(GetBValue(c)));
}

// Шрифты Windows и их метрически совместимые замены. Ширина символов у замен
// совпадает с оригиналом, поэтому подписи на графике занимают столько же места,
// сколько на Windows, и не наезжают друг на друга. Первым в списке идёт точное
// имя (вдруг шрифт установлен), дальше — замены по убыванию точности.
QStringList substituteFamilies(const QString& winFamily)
{
    QStringList out;
    out << winFamily;
    const QString f = winFamily.toLower();
    if (f.startsWith(QLatin1String("times")))
        out << QStringLiteral("Liberation Serif") << QStringLiteral("Nimbus Roman")
            << QStringLiteral("DejaVu Serif");
    else if (f.startsWith(QLatin1String("arial")) || f.startsWith(QLatin1String("helvetica")))
        out << QStringLiteral("Liberation Sans") << QStringLiteral("Nimbus Sans")
            << QStringLiteral("DejaVu Sans");
    else if (f.startsWith(QLatin1String("courier")))
        out << QStringLiteral("Liberation Mono") << QStringLiteral("Nimbus Mono PS")
            << QStringLiteral("DejaVu Sans Mono");
    else if (f.startsWith(QLatin1String("system")) || f.startsWith(QLatin1String("ms sans")))
        out << QStringLiteral("Liberation Sans") << QStringLiteral("DejaVu Sans");
    return out;
}

Qt::PenStyle toPenStyle(int style)
{
    switch (style) {
    case PS_DASH:        return Qt::DashLine;
    case PS_DOT:         return Qt::DotLine;
    case PS_DASHDOT:     return Qt::DashDotLine;
    case PS_DASHDOTDOT:  return Qt::DashDotDotLine;
    case PS_NULL:        return Qt::NoPen;
    default:             return Qt::SolidLine;
    }
}

// Кодировка строк в исходниках — CP1251 (комментарии и подписи Windows-версии).
// Подписи графиков приходят из файлов задания, созданных на Windows, поэтому
// перекодируем их так же, иначе русские названия превратятся в мусор.
QString fromCp1251(const char* s, int len)
{
    static const ushort table[128] = {
        0x0402,0x0403,0x201A,0x0453,0x201E,0x2026,0x2020,0x2021,
        0x20AC,0x2030,0x0409,0x2039,0x040A,0x040C,0x040B,0x040F,
        0x0452,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
        0x0098,0x2122,0x0459,0x203A,0x045A,0x045C,0x045B,0x045F,
        0x00A0,0x040E,0x045E,0x0408,0x00A4,0x0490,0x00A6,0x00A7,
        0x0401,0x00A9,0x0404,0x00AB,0x00AC,0x00AD,0x00AE,0x0407,
        0x00B0,0x00B1,0x0406,0x0456,0x0491,0x00B5,0x00B6,0x00B7,
        0x0451,0x2116,0x0454,0x00BB,0x0458,0x0405,0x0455,0x0457,
        0x0410,0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,
        0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,0x041F,
        0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,
        0x0428,0x0429,0x042A,0x042B,0x042C,0x042D,0x042E,0x042F,
        0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,
        0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x043E,0x043F,
        0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,0x0446,0x0447,
        0x0448,0x0449,0x044A,0x044B,0x044C,0x044D,0x044E,0x044F,
    };
    QString out;
    if (!s)
        return out;
    if (len < 0)
        len = int(std::strlen(s));
    out.reserve(len);
    for (int i = 0; i < len; ++i) {
        unsigned char ch = static_cast<unsigned char>(s[i]);
        out.append(ch < 0x80 ? QChar(ch) : QChar(table[ch - 0x80]));
    }
    return out;
}

} // namespace

CDC::CDC()
    : m_painter(0), m_device(0),
      m_mapMode(MM_TEXT),
      m_winExt(1, 1), m_vpExt(1, 1),
      m_winOrg(0, 0), m_vpOrg(0, 0),
      m_cur(0, 0),
      m_pen(0), m_brush(0), m_font(0),
      m_textColor(RGB(0, 0, 0)), m_bkColor(RGB(255, 255, 255)),
      m_bkMode(OPAQUE), m_printing(FALSE)
{
}

CDC::~CDC()
{
}

void CDC::Attach(QPainter* painter, QPaintDevice* device)
{
    m_painter = painter;
    m_device = device;
}

// --- Отображение --------------------------------------------------------------

int CDC::SetMapMode(int mode)
{
    int old = m_mapMode;
    m_mapMode = mode;

    // Для метрических режимов размеры окна и области задаёт сам режим: логическая
    // единица MM_HIMETRIC — 0.01 мм. Переводим её в пиксели через разрешение
    // устройства, как это делает GDI (там используется LOGPIXELS).
    if (mode == MM_HIMETRIC || mode == MM_LOMETRIC) {
        const double unitsPerMm = (mode == MM_HIMETRIC) ? 100.0 : 10.0;
        const double dpiX = GetDeviceCaps(LOGPIXELSX);
        const double dpiY = GetDeviceCaps(LOGPIXELSY);
        // пикселей на одну логическую единицу
        const double px = dpiX / 25.4 / unitsPerMm;
        const double py = dpiY / 25.4 / unitsPerMm;
        // Держим целые расширения крупными, чтобы не терять точность на делении.
        m_winExt = CSize(100000, 100000);
        m_vpExt  = CSize(long(px * 100000.0), long(-py * 100000.0));  // Y вверх
    } else if (mode == MM_TEXT) {
        m_winExt = CSize(1, 1);
        m_vpExt  = CSize(1, 1);
    }
    return old;
}

CSize CDC::SetWindowExt(int cx, int cy)
{
    CSize old = m_winExt;
    m_winExt = CSize(cx, cy);
    return old;
}

CSize CDC::SetViewportExt(int cx, int cy)
{
    CSize old = m_vpExt;
    m_vpExt = CSize(cx, cy);
    return old;
}

CPoint CDC::SetWindowOrg(int x, int y)
{
    CPoint old = m_winOrg;
    m_winOrg = CPoint(x, y);
    return old;
}

CPoint CDC::SetViewportOrg(int x, int y)
{
    CPoint old = m_vpOrg;
    m_vpOrg = CPoint(x, y);
    return old;
}

double CDC::scaleX() const
{
    return m_winExt.cx ? double(m_vpExt.cx) / double(m_winExt.cx) : 1.0;
}

double CDC::scaleY() const
{
    return m_winExt.cy ? double(m_vpExt.cy) / double(m_winExt.cy) : 1.0;
}

long CDC::deviceX(long lx) const
{
    return long(std::lround((lx - m_winOrg.x) * scaleX())) + m_vpOrg.x;
}

long CDC::deviceY(long ly) const
{
    return long(std::lround((ly - m_winOrg.y) * scaleY())) + m_vpOrg.y;
}

long CDC::logicalX(long dx) const
{
    const double s = scaleX();
    return s != 0.0 ? long(std::lround((dx - m_vpOrg.x) / s)) + m_winOrg.x : 0;
}

long CDC::logicalY(long dy) const
{
    const double s = scaleY();
    return s != 0.0 ? long(std::lround((dy - m_vpOrg.y) / s)) + m_winOrg.y : 0;
}

void CDC::LPtoDP(POINT* points, int count) const
{
    for (int i = 0; i < count; ++i) {
        long x = deviceX(points[i].x);
        long y = deviceY(points[i].y);
        points[i].x = x;
        points[i].y = y;
    }
}

void CDC::DPtoLP(POINT* points, int count) const
{
    for (int i = 0; i < count; ++i) {
        long x = logicalX(points[i].x);
        long y = logicalY(points[i].y);
        points[i].x = x;
        points[i].y = y;
    }
}

void CDC::LPtoDP(RECT* rect) const
{
    POINT p[2] = { { rect->left, rect->top }, { rect->right, rect->bottom } };
    LPtoDP(p, 2);
    rect->left = p[0].x; rect->top = p[0].y;
    rect->right = p[1].x; rect->bottom = p[1].y;
}

void CDC::DPtoLP(RECT* rect) const
{
    POINT p[2] = { { rect->left, rect->top }, { rect->right, rect->bottom } };
    DPtoLP(p, 2);
    rect->left = p[0].x; rect->top = p[0].y;
    rect->right = p[1].x; rect->bottom = p[1].y;
}

// --- Объекты ------------------------------------------------------------------

CPen* CDC::SelectObject(CPen* pen)
{
    CPen* old = m_pen;
    m_pen = pen;
    applyPen();
    return old;
}

CBrush* CDC::SelectObject(CBrush* brush)
{
    CBrush* old = m_brush;
    m_brush = brush;
    if (m_painter) {
        if (!brush || brush->m_null)
            m_painter->setBrush(Qt::NoBrush);
        else
            m_painter->setBrush(QBrush(toQColor(brush->m_color)));
    }
    return old;
}

CFont* CDC::SelectObject(CFont* font)
{
    CFont* old = m_font;
    m_font = font;
    applyFont();
    return old;
}

CGdiObject* CDC::SelectObject(CGdiObject* obj)
{
    if (!obj)
        return 0;
    switch (obj->kind()) {
    case CGdiObject::kPen:   return SelectObject(static_cast<CPen*>(obj));
    case CGdiObject::kBrush: return SelectObject(static_cast<CBrush*>(obj));
    case CGdiObject::kFont:  return SelectObject(static_cast<CFont*>(obj));
    default: return 0;
    }
}

CGdiObject* CDC::SelectStockObject(int index)
{
    static CPen   blackPen(PS_SOLID, 0, RGB(0, 0, 0));
    static CPen   whitePen(PS_SOLID, 0, RGB(255, 255, 255));
    static CPen   nullPen(PS_NULL, 0, RGB(0, 0, 0));
    static CBrush blackBrush(RGB(0, 0, 0));
    static CBrush whiteBrush(RGB(255, 255, 255));
    static CBrush nullBrush;

    switch (index) {
    case BLACK_PEN:   return SelectObject(&blackPen);
    case WHITE_PEN:   return SelectObject(&whitePen);
    case NULL_PEN:    return SelectObject(&nullPen);
    case BLACK_BRUSH: return SelectObject(&blackBrush);
    case WHITE_BRUSH: return SelectObject(&whiteBrush);
    case NULL_BRUSH:  return SelectObject(&nullBrush);
    default: return 0;
    }
}

void CDC::applyPen()
{
    if (!m_painter)
        return;
    if (!m_pen) {
        m_painter->setPen(QPen(toQColor(RGB(0, 0, 0))));
        return;
    }
    QPen pen(toQColor(m_pen->m_color));
    pen.setStyle(toPenStyle(m_pen->m_style));
    // Толщина пера в GDI задана в логических единицах; 0 означает «тонкая линия
    // в один пиксель». Переводим в пиксели тем же масштабом, что и координаты.
    double w = m_pen->m_width * std::fabs(scaleX());
    pen.setWidth(w < 1.0 ? 0 : int(std::lround(w)));
    m_painter->setPen(pen);
}

void CDC::applyFont()
{
    if (!m_painter || !m_font)
        return;
    const LOGFONT& lf = m_font->m_lf;
    QFont f;
    if (lf.lfFaceName[0])
        f.setFamilies(substituteFamilies(QString::fromLatin1(lf.lfFaceName)));
    // lfHeight в логических единицах: отрицательная величина — высота символа,
    // положительная — высота ячейки. Переводим в пиксели масштабом по Y.
    double h = std::fabs(double(lf.lfHeight)) * std::fabs(scaleY());
    if (h >= 1.0)
        f.setPixelSize(int(std::lround(h)));
    f.setBold(lf.lfWeight >= FW_BOLD);
    f.setItalic(lf.lfItalic != 0);
    f.setUnderline(lf.lfUnderline != 0);
    f.setStrikeOut(lf.lfStrikeOut != 0);
    m_painter->setFont(f);
}

// --- Рисование ----------------------------------------------------------------

CPoint CDC::MoveTo(int x, int y)
{
    CPoint old = m_cur;
    m_cur = CPoint(x, y);
    return old;
}

BOOL CDC::LineTo(int x, int y)
{
    if (m_painter) {
        applyPen();
        m_painter->drawLine(int(deviceX(m_cur.x)), int(deviceY(m_cur.y)),
                            int(deviceX(x)), int(deviceY(y)));
    }
    m_cur = CPoint(x, y);
    return TRUE;
}

BOOL CDC::Polyline(const POINT* points, int count)
{
    if (!m_painter || count < 2)
        return FALSE;
    applyPen();
    std::vector<QPoint> pts;
    pts.reserve(count);
    for (int i = 0; i < count; ++i)
        pts.push_back(QPoint(int(deviceX(points[i].x)), int(deviceY(points[i].y))));
    m_painter->drawPolyline(&pts[0], count);
    return TRUE;
}

BOOL CDC::Rectangle(int l, int t, int r, int b)
{
    if (!m_painter)
        return FALSE;
    applyPen();
    QRect rc(QPoint(int(deviceX(l)), int(deviceY(t))),
             QPoint(int(deviceX(r)), int(deviceY(b))));
    m_painter->drawRect(rc.normalized());
    return TRUE;
}

BOOL CDC::Ellipse(int l, int t, int r, int b)
{
    if (!m_painter)
        return FALSE;
    applyPen();
    QRect rc(QPoint(int(deviceX(l)), int(deviceY(t))),
             QPoint(int(deviceX(r)), int(deviceY(b))));
    m_painter->drawEllipse(rc.normalized());
    return TRUE;
}

void CDC::FillSolidRect(const RECT* r, COLORREF color)
{
    if (!r)
        return;
    FillSolidRect(int(r->left), int(r->top),
                  int(r->right - r->left), int(r->bottom - r->top), color);
}

void CDC::FillSolidRect(int x, int y, int cx, int cy, COLORREF color)
{
    if (!m_painter)
        return;
    QRect rc(QPoint(int(deviceX(x)), int(deviceY(y))),
             QPoint(int(deviceX(x + cx)), int(deviceY(y + cy))));
    m_painter->fillRect(rc.normalized(), toQColor(color));
}

BOOL CDC::PatBlt(int x, int y, int w, int h, DWORD rop)
{
    if (!m_painter)
        return FALSE;
    QRect rc(QPoint(int(deviceX(x)), int(deviceY(y))),
             QPoint(int(deviceX(x + w)), int(deviceY(y + h))));
    QColor c = (rop == BLACKNESS) ? QColor(0, 0, 0)
             : (rop == WHITENESS) ? QColor(255, 255, 255)
             : toQColor(m_brush ? m_brush->m_color : m_bkColor);
    m_painter->fillRect(rc.normalized(), c);
    return TRUE;
}

BOOL CDC::TextOut(int x, int y, const char* text, int len)
{
    if (!text)
        return FALSE;

    // Режим OpenGL: холста нет, поэтому надпись откладываем — виджет выведет её
    // поверх готового кадра.
    if (m_deferText || !m_painter) {
        DeferredText item;
        item.x = deviceX(x);
        item.y = deviceY(y);
        item.text.assign(text, len < 0 ? std::strlen(text) : size_t(len));
        item.color = m_textColor;
        if (m_font)
            item.font = m_font->m_lf;
        else
            std::memset(&item.font, 0, sizeof(item.font));
        m_deferredText.push_back(item);
        return TRUE;
    }
    applyFont();
    const QString s = fromCp1251(text, len);
    const int dx = int(deviceX(x));
    const int dy = int(deviceY(y));
    QFontMetrics fm(m_painter->font());
    // В GDI точка отсчёта текста — левый ВЕРХНИЙ угол, в Qt — базовая линия.
    const int baseline = dy + fm.ascent();
    if (m_bkMode == OPAQUE) {
        QRect back(dx, dy, fm.horizontalAdvance(s), fm.height());
        m_painter->fillRect(back, toQColor(m_bkColor));
    }
    m_painter->setPen(QPen(toQColor(m_textColor)));
    m_painter->drawText(dx, baseline, s);
    applyPen();
    return TRUE;
}

BOOL CDC::TextOut(int x, int y, const CString& text)
{
    return TextOut(x, y, text.GetString(), int(std::strlen(text.GetString())));
}

CSize CDC::GetTextExtent(const char* text, int len) const
{
    if (!m_painter || !text)
        return CSize(0, 0);
    QFontMetrics fm(m_painter->font());
    const QString s = fromCp1251(text, len);
    // Возвращаем размер в логических единицах: код вида считает в них.
    const double sx = scaleX() != 0.0 ? std::fabs(scaleX()) : 1.0;
    const double sy = scaleY() != 0.0 ? std::fabs(scaleY()) : 1.0;
    return CSize(long(fm.horizontalAdvance(s) / sx), long(fm.height() / sy));
}

CSize CDC::GetTextExtent(const CString& text) const
{
    return GetTextExtent(text.GetString(), int(std::strlen(text.GetString())));
}

BOOL CDC::GetTextMetrics(TEXTMETRIC* tm) const
{
    if (!tm || !m_painter)
        return FALSE;
    QFontMetrics fm(m_painter->font());
    const double sy = scaleY() != 0.0 ? std::fabs(scaleY()) : 1.0;
    const double sx = scaleX() != 0.0 ? std::fabs(scaleX()) : 1.0;
    tm->tmHeight          = long(fm.height() / sy);
    tm->tmAscent          = long(fm.ascent() / sy);
    tm->tmDescent         = long(fm.descent() / sy);
    tm->tmInternalLeading = 0;
    tm->tmExternalLeading = long(fm.leading() / sy);
    tm->tmAveCharWidth    = long(fm.averageCharWidth() / sx);
    tm->tmMaxCharWidth    = long(fm.maxWidth() / sx);
    tm->tmWeight          = m_font && m_font->m_lf.lfWeight ? m_font->m_lf.lfWeight : FW_NORMAL;
    return TRUE;
}

COLORREF CDC::SetTextColor(COLORREF color)
{
    COLORREF old = m_textColor;
    m_textColor = color;
    return old;
}

COLORREF CDC::SetBkColor(COLORREF color)
{
    COLORREF old = m_bkColor;
    m_bkColor = color;
    return old;
}

int CDC::SetBkMode(int mode)
{
    int old = m_bkMode;
    m_bkMode = mode;
    return old;
}

BOOL CDC::GetClipBox(RECT* r) const
{
    if (!r || !m_painter)
        return FALSE;
    QRect c = m_painter->clipBoundingRect().toAlignedRect();
    if (c.isNull() && m_device)
        c = QRect(0, 0, m_device->width(), m_device->height());
    r->left = c.left(); r->top = c.top();
    r->right = c.right(); r->bottom = c.bottom();
    DPtoLP(r);
    return TRUE;
}

CPalette* CDC::SelectPalette(CPalette* /*palette*/, BOOL /*forceBackground*/)
{
    // Палитр в полноцветном режиме нет: выбирать нечего, прежней тоже не было.
    return 0;
}

UINT CDC::RealizePalette()
{
    // Ни один цвет не поменялся — столько записей и «воплощено».
    return 0;
}

int GetDeviceCaps(HDC /*dc*/, int index)
{
    // Свободный вариант Windows API (см. mfc_gdi.h): описатель контекста здесь
    // только метка, поэтому сведения берём у основного экрана. Из разрешения
    // код Windows-версии считает высоту шрифта подписей, значит оно должно быть
    // настоящим.
    const QScreen* screen = QGuiApplication::primaryScreen();
    switch (index) {
    case LOGPIXELSX: return screen ? int(screen->logicalDotsPerInchX()) : 96;
    case LOGPIXELSY: return screen ? int(screen->logicalDotsPerInchY()) : 96;
    case HORZRES:    return screen ? screen->geometry().width() : 1024;
    case VERTRES:    return screen ? screen->geometry().height() : 768;
    case HORZSIZE:   return screen ? int(screen->physicalSize().width()) : 270;
    case VERTSIZE:   return screen ? int(screen->physicalSize().height()) : 200;
    default:         return 0;
    }
}

int CDC::GetDeviceCaps(int index) const
{
    // Значения берём у устройства Qt; при его отсутствии — типовые для экрана,
    // чтобы пересчёт координат не делился на ноль.
    switch (index) {
    case LOGPIXELSX: return m_device ? m_device->logicalDpiX() : 96;
    case LOGPIXELSY: return m_device ? m_device->logicalDpiY() : 96;
    case HORZRES:    return m_device ? m_device->width() : 1024;
    case VERTRES:    return m_device ? m_device->height() : 768;
    case HORZSIZE:   return m_device ? int(m_device->widthMM()) : 270;
    case VERTSIZE:   return m_device ? int(m_device->heightMM()) : 200;
    default:         return 0;
    }
}

#endif // !_WIN32
