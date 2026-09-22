// tmcgltext_qt.cpp — надписи внутри сцены OpenGL (класс CTmcGLText) под Linux.
//
// Windows-версия (win_src/fieldview/tmcgltext.cpp) рисует строку средствами GDI:
// CreateFontIndirect -> TextOut в монохромный растр -> GetDIBits -> glBitmap.
// Ни GDI, ни его растров под Linux нет, поэтому файл Windows-версии не
// компилируется, а те же методы реализованы здесь заново:
//
//   строка -> QImage (QPainter нужным шрифтом) -> glDrawPixels(GL_RGBA).
//
// Что сохранено один в один:
//   * положение надписи задаётся glRasterPos3f(x, y, z) — теми же координатами
//     сцены, что и раньше;
//   * начало растра смещено на 2 пикселя вниз — это аргумент yorig = 2.0
//     у прежнего вызова glBitmap;
//   * цвет берётся из текущего цвета OpenGL, как его брал glBitmap. Его
//     задают SetColor/SetColorSize/SetColorInput и сам вид перед вызовом.
//
// Отличие, которого не избежать: под Windows шрифт «TimesET» растрировался без
// сглаживания, здесь строка рисуется шрифтом с засечками из системы и со
// сглаживанием (см. NEEDS.md). Ни размеры, ни координаты, ни цвета от этого не
// меняются.
//
// Математика не затронута: файл только рисует подписи.

#include "stdafx.h"
#include "tmcgltext.h"

#include <QByteArray>
#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QScreen>
#include <QString>
#include <QStringList>

#include <cstdio>
#include <cstring>
#include <vector>

// Вызовы Windows, которых нет на Linux (wglGetCurrentDC, GetDeviceCaps),
// живут в общем слое совместимости: src/compat/mfc_wgl.cpp и mfc_gdi.cpp.

namespace {

/// Перевести описание шрифта Windows (LOGFONT) в шрифт Qt.
///
/// lfHeight в Windows-версии положительная и задана в пикселях устройства —
/// это высота знакоместа. lfWidth задаёт среднюю ширину знака: в FieldView она
/// равна трети высоты, отчего подписи получаются узкими. В Qt ширина задаётся
/// не напрямую, а растяжением в процентах от естественной ширины шрифта —
/// его и вычисляем.
QFont fontFromLogFont(const LOGFONT &lf)
{
    QFont font;

    // Замены шрифтов те же, что в слое рисования (решение D10): сначала точное
    // имя из Windows, затем метрически совместимые гарнитуры. «TimesET»
    // начинается на «times», поэтому попадает в семейство с засечками.
    const QString face = QString::fromLatin1(lf.lfFaceName);
    QStringList families;
    if (!face.isEmpty())
        families << face;
    const QString lower = face.toLower();
    if (lower.startsWith(QLatin1String("times")))
        families << QStringLiteral("Liberation Serif") << QStringLiteral("Nimbus Roman")
                 << QStringLiteral("DejaVu Serif");
    else if (lower.startsWith(QLatin1String("arial")) || lower.startsWith(QLatin1String("helvetica")))
        families << QStringLiteral("Liberation Sans") << QStringLiteral("Nimbus Sans")
                 << QStringLiteral("DejaVu Sans");
    else if (lower.startsWith(QLatin1String("courier")))
        families << QStringLiteral("Liberation Mono") << QStringLiteral("Nimbus Mono PS")
                 << QStringLiteral("DejaVu Sans Mono");
    else
        families << QStringLiteral("Liberation Sans") << QStringLiteral("DejaVu Sans");
    font.setFamilies(families);

    const int pixels = int(lf.lfHeight < 0 ? -lf.lfHeight : lf.lfHeight);
    font.setPixelSize(pixels > 0 ? pixels : 12);

    font.setItalic(lf.lfItalic != 0);
    font.setUnderline(lf.lfUnderline != 0);
    font.setStrikeOut(lf.lfStrikeOut != 0);
    font.setWeight(lf.lfWeight >= FW_BOLD ? QFont::Bold : QFont::Normal);

    if (lf.lfWidth > 0) {
        const int natural = QFontMetrics(font).averageCharWidth();
        if (natural > 0) {
            int stretch = int((100.0 * double(lf.lfWidth)) / double(natural) + 0.5);
            if (stretch < 10)   stretch = 10;
            if (stretch > 400)  stretch = 400;
            font.setStretch(stretch);
        }
    }
    return font;
}

/// Нарисовать строку в точке (x, y, z) сцены текущим цветом OpenGL.
void drawStringAt(const LOGFONT &lf, const char *text, double x, double y, double z)
{
    if (text == 0 || *text == 0)
        return;

    // Цвет и прозрачность — те же, что взял бы glBitmap: текущий цвет OpenGL.
    GLfloat color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glGetFloatv(GL_CURRENT_COLOR, color);

    glRasterPos3f(GLfloat(x), GLfloat(y), GLfloat(z));

    // Если точка вне области вывода, растровые операции игнорируются — так же
    // вела себя Windows-версия.
    GLboolean valid = GL_FALSE;
    glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, &valid);
    if (!valid)
        return;

    const QFont font = fontFromLogFont(lf);
    const QString str = QString::fromLocal8Bit(text);
    const QFontMetrics fm(font);

    const int w = fm.horizontalAdvance(str);
    const int h = fm.height();
    if (w <= 0 || h <= 0)
        return;

    // Растрируем строку белым по прозрачному: получаем маску покрытия.
    QImage mask(w, h, QImage::Format_ARGB32);
    mask.fill(Qt::transparent);
    {
        QPainter p(&mask);
        p.setFont(font);
        p.setPen(Qt::white);
        p.drawText(0, fm.ascent(), str);
    }

    // Готовим GL_RGBA: цвет — текущий цвет OpenGL, прозрачность — покрытие,
    // умноженное на прозрачность цвета. Строки переворачиваем: у QImage первая
    // строка верхняя, у glDrawPixels — нижняя.
    const unsigned char r = (unsigned char)(color[0] * 255.0f + 0.5f);
    const unsigned char g = (unsigned char)(color[1] * 255.0f + 0.5f);
    const unsigned char b = (unsigned char)(color[2] * 255.0f + 0.5f);
    const float alpha = color[3];

    std::vector<unsigned char> pixels(size_t(w) * size_t(h) * 4u, 0);
    for (int row = 0; row < h; ++row) {
        const QRgb *src = reinterpret_cast<const QRgb *>(mask.constScanLine(h - 1 - row));
        unsigned char *dst = &pixels[size_t(row) * size_t(w) * 4u];
        for (int col = 0; col < w; ++col) {
            const int a = qAlpha(src[col]);
            dst[0] = r;
            dst[1] = g;
            dst[2] = b;
            dst[3] = (unsigned char)(float(a) * alpha + 0.5f);
            dst += 4;
        }
    }

    // Смещение начала растра на 2 пикселя вниз — прежний аргумент yorig = 2.0
    // у glBitmap. Пустой glBitmap только двигает текущую точку растра.
    glBitmap(0, 0, 0.0f, 0.0f, 0.0f, -2.0f, 0);

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_PIXEL_MODE_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelZoom(1.0f, 1.0f);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);
    glPopAttrib();
}

} // namespace

/////////////////////////////////////////////////////////////////////////////
// CTmcGLText

CTmcGLText::CTmcGLText()
{
    // Начальные значения — те же, что в Windows-версии.
    nBlend = 100;
    scColorRef = RGB(256, 256, 256);
    hFont = 0;

    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = 12 * GetDeviceCaps(wglGetCurrentDC(), LOGPIXELSY) / 72;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfWeight = 400;
    lf.lfOutPrecision = 0;
    lf.lfClipPrecision = 0;
    strcpy(lf.lfFaceName, "TimesET");

    size.cx = 0;
    size.cy = 0;
}

CTmcGLText::~CTmcGLText()
{
}

BEGIN_MESSAGE_MAP(CTmcGLText, CWnd)
END_MESSAGE_MAP()

void CTmcGLText::SetFont(LOGFONT *plf1)
{
    // Шрифтом владеет Qt: достаточно запомнить описание, растр строится в
    // момент вывода строки.
    if (plf1 != 0)
        lf = *plf1;
}

void CTmcGLText::SetColor(COLORREF scColorRef1)
{
    scColorRef = scColorRef1;
}

// Формулы цвета — дословно из Windows-версии (делитель 256, прозрачность в
// сотых долях). Не менять.
void CTmcGLText::SetColorSize(void)
{
    glColor4f(((float)(GetRValue(scColorRef))) / 256,
              ((float)(GetGValue(scColorRef))) / 256,
              ((float)(GetBValue(scColorRef))) / 256,
              (float)(nBlend / 100.0));
}

void CTmcGLText::SetColorInput(void)
{
    glColor4f(((float)(GetRValue(scColorRef))) / 256,
              ((float)(GetGValue(scColorRef))) / 256,
              ((float)(GetBValue(scColorRef))) / 256,
              (float)(nBlend / 100.0));
}

void CTmcGLText::DrawString(char *ch, double x, double y, double z)
{
    drawStringAt(lf, ch, x, y, z);
}

void CTmcGLText::DrawString(CString cs, double x, double y, double z)
{
    drawStringAt(lf, (const char *)cs, x, y, z);
}

void CTmcGLText::DrawInputNum(int n, double x, double y, double z)
{
    char ch[64];
    sprintf(ch, "Inp %d", n);
    // Порядок как в Windows-версии: цвет задаётся перед выводом строки.
    SetColorInput();
    drawStringAt(lf, ch, x, y, z);
}

// Вспомогательные методы Windows-версии (растр GDI) под Linux не нужны: их
// работу целиком делает drawStringAt. Определений намеренно нет — обращений
// к ним в коде тоже нет.
