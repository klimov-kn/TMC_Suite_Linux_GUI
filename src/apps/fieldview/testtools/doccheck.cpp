// Проверка чтения файла поля без графики: печатает, что прочиталось.
//
// Нужна, чтобы отделять ошибки разбора файлов от ошибок отрисовки.
//
//   fieldview_doccheck <файл поля>

#include <cstdio>

// Слой совместимости должен подключаться раньше заголовков Windows-версии:
// они рассчитывают, что MFC уже объявлен (в оригинале это делал stdafx.h).
#include <afxwin.h>

#include "resource.h"
#include "fldviewdoc.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::printf("Использование: fieldview_doccheck <файл поля>\n");
        return 2;
    }

    CFldViewDoc *doc = CFldViewDoc::TmcCreate();
    doc->SetPathName(argv[1]);
    doc->ReadData();

    if (doc->cError.IsError()) {
        std::printf("Ошибка чтения: %s\n", (const char *)doc->cError.GetErrorMessage());
        return 1;
    }

    std::printf("Файл       : %s\n", (const char *)doc->GetPathName());
    std::printf("Сетка      : nX = %d, nY = %d\n", doc->GetnX(), doc->GetnY());
    std::printf("Шаг сетки  : %g\n", doc->GetdDelta());
    std::printf("Шаг по t   : nT = %d, t = %g\n", doc->GetnTCurrent(), doc->GetdTCurrent());
    std::printf("Xmin, Ymin : %g, %g\n", doc->GetdXmin(), doc->GetdYmin());
    std::printf("Данные     : %s\n", doc->GetpdSurface() ? "прочитаны" : "нет");
    return 0;
}
