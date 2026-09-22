// planrt_compat.cpp — то, чего оболочкам счётных ядер не хватает в слое
// совместимости (см. planrt_prelude.h и src/apps/planarrt_h/NEEDS.md).
//
// Здесь нет Qt: файл входит в счётное ядро программы, которое собирается и
// проверяется без графики. Всё, что должно дойти до интерфейса (окно с
// вопросом, просьба закрыть главное окно), уходит через обработчики —
// Qt-слой ставит свои при старте, а без него работают безопасные значения по
// умолчанию, годные для прогона из консоли.

#include <chrono>
#include <cstdio>

#include "planrt_prelude.h"

// --- WIN.INI ------------------------------------------------------------------
DWORD TmcGetWinIniProfileString(const char* /*section*/, const char* /*entry*/,
                                const char* defaultValue, char* buffer, DWORD size)
{
    if (!buffer || size == 0)
        return 0;

    const char* src = defaultValue ? defaultValue : "";
    DWORD n = 0;
    while (src[n] && n + 1 < size) {
        buffer[n] = src[n];
        n++;
    }
    buffer[n] = '\0';
    return n;
}

// --- Время --------------------------------------------------------------------
DWORD TmcGetTickCount(void)
{
    using namespace std::chrono;
    static const steady_clock::time_point start = steady_clock::now();
    return (DWORD)duration_cast<milliseconds>(steady_clock::now() - start).count();
}

// --- Обработчики, которые ставит Qt-слой --------------------------------------
namespace {

TmcPlanRtMessageBoxFn s_messageBox = 0;
TmcPlanRtCloseFn      s_closeFrame = 0;

} // namespace

void TmcPlanRtSetMessageBoxHandler(TmcPlanRtMessageBoxFn fn) { s_messageBox = fn; }
void TmcPlanRtSetCloseHandler(TmcPlanRtCloseFn fn)           { s_closeFrame = fn; }

// Окно сообщения с кнопками переехало в общий слой совместимости
// (src/compat/mfc_dialog.cpp): там оно показывает обычное окно Qt, а без
// графики — например, в средстве проверки счёта — сообщает в поток ошибок и
// отвечает так же, как пакетный режим Windows-версии.

void TmcPostFrameClose(UINT message)
{
    if (s_closeFrame)
        s_closeFrame(message);
}
