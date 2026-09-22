// runcheck.cpp — проверка счёта без графики (PlanarRT_H и PlanarRT_X).
//
// Создаёт документ, читает файл задания (.tpl), выполняет один шаг расчёта и
// печатает номер шага, сообщение об ошибке ядра и имена выходных файлов с их
// размерами. Нужна, чтобы отделять ошибки расчёта от ошибок интерфейса: если
// здесь всё хорошо, а в окне пусто — виновата графика, и наоборот.
//
// Расчёт вызывается тем же кодом, что и в программе, — CPlanRT_HDoc::RunStep().
// Ни одной формулы этот файл не содержит.
//
// Запуск:  planarrt_h_runcheck <файл.tpl> [номер шага]

#include <sys/stat.h>

#include <cstdio>
#include <cstring>

#include "resource.h"        // номера диалогов: их ждут заголовки Windows-версии
#include "mainfrm.h"
#include "planrt_hdoc.h"
#include "pl_iofor.h"

#ifndef TMC_PLANRT_APP
  #define TMC_PLANRT_APP "PlanarRT"
#endif

namespace {

/// Печатает имя выходного файла и его размер (или «нет файла»).
void ReportFile(const char* title, const char* path)
{
    if (!path || !*path) {
        std::printf("  %-14s (имя не задано)\n", title);
        return;
    }
    struct stat st;
    if (stat(path, &st) == 0)
        std::printf("  %-14s %s  (%lld байт)\n", title, path, (long long)st.st_size);
    else
        std::printf("  %-14s %s  (файла нет)\n", title, path);
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::fprintf(stderr, "Использование: %s <файл.tpl> [номер шага]\n", argv[0]);
        return 2;
    }

    // Настройки программы (внешние просмотрщики, форматы вывода) на Windows
    // лежали в реестре, здесь — в ~/.config/TMC_Suite/<программа>.conf.
    CWinApp::SetAppName(TMC_PLANRT_APP);

    // Код, перенесённый из Windows-версии, пишет ход счёта в строку состояния
    // главного окна (PutTrace/PutStatistics). Без окна писать некуда, но сама
    // рамка должна существовать: иначе PutTrace обратится в пустоту.
    CMainFrame frame;
    AfxGetApp()->m_pMainWnd = &frame;

    CPlanRT_HDoc* pDoc = CPlanRT_HDoc::TmcCreate();
    pDoc->SetPathName(argv[1]);
    pDoc->SetTitle(argv[1]);

    // Шаг по умолчанию — первый, как при открытии задания в программе.
    int wanted = (argc > 2) ? atoi(argv[2]) : 1;
    pDoc->SetFirstStep();
    for (int i = 1; i < wanted; i++)
        pDoc->SkipStep();

    std::printf("Задание:        %s\n", argv[1]);
    std::printf("Шагов в задании: %d\n", pDoc->GetRectNode().GetNStepMax(pDoc->GetPathName()));

    pDoc->ReadData();
    std::printf("После чтения:    шаг %d; ошибка ядра: \"%s\"\n",
                pDoc->GetnStep(), (const char*)pDoc->GetErrorMessage());

    pDoc->RunStep();
    std::printf("После расчёта:   шаг %d; ошибка ядра: \"%s\"\n",
                pDoc->GetnStep(), (const char*)pDoc->GetErrorMessage());

    CTmcRTH_IndanOutput* pOut = pDoc->GetOutput();
    std::printf("Выходные файлы:\n");
    if (pOut) {
        ReportFile("сигнал:", pOut->csFileName);
        ReportFile("поле:", pOut->csFileNameField);
        ReportFile("S-матрица:", pOut->csFileNameSMatrix);
    } else {
        std::printf("  (ядро не отдало описание вывода)\n");
    }

    // Ядро сообщает «ошибок нет» строкой "Error:no" — так же, как в
    // Windows-версии; любая другая строка означает настоящую ошибку.
    const char* msg = (const char*)pDoc->GetErrorMessage();
    const int nErr = (msg && *msg && std::strcmp(msg, "Error:no") != 0) ? 1 : 0;
    delete pDoc;
    AfxGetApp()->m_pMainWnd = 0;
    return nErr;
}
