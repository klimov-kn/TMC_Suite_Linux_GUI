// Реализация заглушек wgl (см. mfc_wgl.h).
//
// Контекстом OpenGL владеет Qt: он создан, сделан текущим и показан на экране
// без участия этого кода. Здесь только «да, сделано», чтобы код Windows-версии
// шёл своим чередом.

#ifndef _WIN32

#include "mfc_wgl.h"

namespace {
// Один общий признак контекста: код Windows-версии хранит его в переменной и
// проверяет на ноль.
int g_dummyContext = 1;
} // namespace

int ChoosePixelFormat(HDC, const PIXELFORMATDESCRIPTOR*)
{
    // Формат поверхности задаёт Qt (QSurfaceFormat у виджета). Возвращаем
    // ненулевой номер, чтобы проверка в коде Windows-версии прошла.
    return 1;
}

BOOL SetPixelFormat(HDC, int, const PIXELFORMATDESCRIPTOR*)
{
    return TRUE;
}

int GetPixelFormat(HDC)
{
    return 1;
}

int DescribePixelFormat(HDC, int, UINT size, PIXELFORMATDESCRIPTOR* pfd)
{
    if (pfd) {
        // Заполняем тем, что реально предоставляет Qt: полноцветный буфер с
        // двойной буферизацией и буфером глубины.
        std::memset(pfd, 0, size ? size : sizeof(PIXELFORMATDESCRIPTOR));
        pfd->nSize = WORD(sizeof(PIXELFORMATDESCRIPTOR));
        pfd->nVersion = 1;
        pfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd->iPixelType = PFD_TYPE_RGBA;
        pfd->cColorBits = 32;
        pfd->cDepthBits = 24;
        pfd->iLayerType = PFD_MAIN_PLANE;
    }
    return 1;
}

BOOL SwapBuffers(HDC)
{
    // Показ готового кадра делает QOpenGLWidget после возврата из paintGL.
    return TRUE;
}

HGLRC wglCreateContext(HDC)
{
    return static_cast<HGLRC>(&g_dummyContext);
}

BOOL wglMakeCurrent(HDC, HGLRC)
{
    // Контекст уже текущий: Qt делает это перед вызовом paintGL.
    return TRUE;
}

BOOL wglDeleteContext(HGLRC)
{
    return TRUE;
}

HGLRC wglGetCurrentContext()
{
    return static_cast<HGLRC>(&g_dummyContext);
}

HDC wglGetCurrentDC()
{
    // Контекстом рисования владеет QOpenGLWidget. Отдаём ту же ненулевую метку:
    // код Windows-версии передаёт её в SwapBuffers и GetDeviceCaps и сверяет
    // только с NULL.
    return static_cast<HDC>(&g_dummyContext);
}

#endif // !_WIN32
