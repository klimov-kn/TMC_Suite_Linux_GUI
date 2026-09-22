// mfc_wgl.h — заглушки оконного слоя OpenGL для Windows (wgl).
//
// FieldView создаёт контекст OpenGL средствами Windows: подбирает формат
// пикселей, зовёт `wglCreateContext` и `wglMakeCurrent`, а в конце кадра —
// `SwapBuffers`. В Qt всем этим владеет `QOpenGLWidget`: контекст создан до
// вызова `paintGL`, уже сделан текущим, а смена буферов происходит сама.
//
// Поэтому здесь функции есть, но ничего не делают: код Windows-версии остаётся
// нетронутым, а работу выполняет Qt. Сама отрисовка (`glBegin`, `glVertex3f`,
// `glColor4f`, матрицы) — настоящая, из системного OpenGL.
//
// Ни одна формула цвета или нормировки поля здесь не затрагивается.

#pragma once

#ifndef _WIN32

// Нужны базовые типы и CGdiObject: палитра — такой же объект рисования.
#include "mfc_gdi.h"

typedef void* HGLRC;
typedef void* HPALETTE;
// HDC объявлен в mfc_gdi.h: описатель контекста рисования нужен и обычному
// рисованию, и OpenGL.

// --- Описание формата пикселей ------------------------------------------------
// Структура заполняется кодом Windows-версии и передаётся в ChoosePixelFormat.
// Qt выбирает формат сам (см. QSurfaceFormat в виджете), поэтому поля только
// хранятся.
struct PIXELFORMATDESCRIPTOR
{
    WORD  nSize;
    WORD  nVersion;
    DWORD dwFlags;
    BYTE  iPixelType;
    BYTE  cColorBits;
    BYTE  cRedBits, cRedShift;
    BYTE  cGreenBits, cGreenShift;
    BYTE  cBlueBits, cBlueShift;
    BYTE  cAlphaBits, cAlphaShift;
    BYTE  cAccumBits;
    BYTE  cAccumRedBits, cAccumGreenBits, cAccumBlueBits, cAccumAlphaBits;
    BYTE  cDepthBits;
    BYTE  cStencilBits;
    BYTE  cAuxBuffers;
    BYTE  iLayerType;
    BYTE  bReserved;
    DWORD dwLayerMask;
    DWORD dwVisibleMask;
    DWORD dwDamageMask;
};

#define PFD_DRAW_TO_WINDOW      0x00000004
#define PFD_SUPPORT_OPENGL      0x00000020
#define PFD_DOUBLEBUFFER        0x00000001
#define PFD_TYPE_RGBA           0
#define PFD_TYPE_COLORINDEX     1
#define PFD_MAIN_PLANE          0
// Признак «формату нужна палитра». Ветка кода, которая её строит
// (CFldViewView::CreateRGBPalette), выполняется только при нём; Qt всегда даёт
// полноцветный формат, поэтому она не выполняется никогда.
#define PFD_NEED_PALETTE        0x00000008

int  ChoosePixelFormat(HDC dc, const PIXELFORMATDESCRIPTOR* pfd);
BOOL SetPixelFormat(HDC dc, int format, const PIXELFORMATDESCRIPTOR* pfd);
int  GetPixelFormat(HDC dc);
int  DescribePixelFormat(HDC dc, int format, UINT size, PIXELFORMATDESCRIPTOR* pfd);
BOOL SwapBuffers(HDC dc);

HGLRC wglCreateContext(HDC dc);
BOOL  wglMakeCurrent(HDC dc, HGLRC rc);
BOOL  wglDeleteContext(HGLRC rc);
HGLRC wglGetCurrentContext();

/// Текущий контекст рисования OpenGL. В Windows его отдавала система; здесь
/// контекстом владеет QOpenGLWidget, поэтому возвращается ненулевая метка — код
/// Windows-версии сверяет её только с NULL (fldviewview.cpp: SwapBuffers,
/// GetDeviceCaps; tmcdialogplace.cpp).
HDC   wglGetCurrentDC();

// --- Палитра -------------------------------------------------------------------
// В Windows палитра нужна была для 256-цветных режимов. Современные системы
// работают в полном цвете, поэтому эти вызовы — пустышки.
// Одна запись палитры. Код Windows-версии заполняет ими массив
// (`static PALETTEENTRY defaultPalEntry[20]`, fldviewview.cpp) и таблицу
// LOGPALETTE; поля и их порядок — как в wingdi.h.
struct PALETTEENTRY
{
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
};

// Таблица палитры. Массив palPalEntry объявлен из одного элемента, как в
// Windows: настоящий размер задаётся при выделении памяти
// (`new char[sizeof(LOGPALETTE) + n * sizeof(PALETTEENTRY)]`).
struct LOGPALETTE
{
    WORD         palVersion;
    WORD         palNumEntries;
    PALETTEENTRY palPalEntry[1];
};
typedef LOGPALETTE* PLOGPALETTE;

class CPalette : public CGdiObject
{
public:
    BOOL CreatePalette(LOGPALETTE*) { return TRUE; }
    BOOL DeleteObject() { return TRUE; }
};

#endif // !_WIN32
