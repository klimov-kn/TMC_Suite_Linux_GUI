// Проверка чтения файла задания и расчёта диаграммы без графики.
//
// Открывает документ тем же кодом, что и программа, и печатает, что получилось:
// параметры окна графика, список графиков, число точек и первые значения, а
// также сводку по решётке излучателей (число излучателей, КНД, КИП, частота).
// Так видно, читается ли файл, созданный на Windows, и совпадают ли числа.
//
// Внимание: чтение задания заодно пересчитывает диаграмму и дописывает файл
// задания — запускать только на копии.
//
// В поставку не входит: инструмент разработки.
//
//   tmc_dn_doccheck <файл задания> [--dump]

#include <cstdio>
#include <cstring>

// Слой совместимости должен подключаться раньше заголовков Windows-версии:
// они рассчитывают, что MFC уже объявлен (в оригинале это делал stdafx.h).
#include <afxwin.h>

// c2darray.h раньше документа: tmcgroutdoc.h держит указатель на c2DArray,
// а в Windows-версии его объявление приходило из stdafx.h программы.
#include "c2darray.h"
#include "tmcgroutdoc.h"

// Печать файла так, как его видит программа: через тот же fopen, а значит,
// через обёртку текстового режима. Сразу видно, снялись ли возвраты каретки.
static void dumpLines(const char* path)
{
    FILE* f = fopen(path, "r");
    if (!f) {
        std::printf("не открывается: %s\n", path);
        return;
    }
    char line[10000];
    int n = 0;
    while (fgets(line, sizeof(line), f)) {
        std::printf("%3d |", ++n);
        for (const char* p = line; *p; ++p) {
            if (*p == '\n')      std::printf("<LF>");
            else if (*p == '\r') std::printf("<CR>");
            else if (*p == '\t') std::printf("<TAB>");
            else                 std::putchar(*p);
        }
        std::printf("|\n");
    }
    fclose(f);
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::printf("Использование: tmc_dn_doccheck <файл задания> [--dump]\n");
        return 1;
    }

    if (argc > 2 && std::strcmp(argv[2], "--dump") == 0) {
        dumpLines(argv[1]);
        return 0;
    }

    CWinApp::SetAppName("TMC_DN");

    CTMCGROUTDoc* doc = CTMCGROUTDoc::TmcCreate();
    doc->SetPathName(argv[1]);
    doc->ReadGraphParametersDefault();
    doc->ReadGraphParameters();
    doc->ReadDocFile();

    std::printf("файл           : %s\n", doc->GetPathName());
    std::printf("ошибка         : %s\n", doc->error[0] ? doc->error : "нет");
    std::printf("графиков       : %d\n", doc->grdoc.nGraph);
    std::printf("тип X / Y      : %d / %d\n", doc->grdoc.nXType, doc->grdoc.nYType);
    std::printf("Xmin..Xmax     : %g .. %g\n", doc->grdoc.Xmin, doc->grdoc.Xmax);
    std::printf("Ymin..Ymax     : %g .. %g\n", doc->grdoc.Ymin, doc->grdoc.Ymax);
    std::printf("авторазмер X/Y : %d / %d\n", (int)doc->grdoc.nAFlagX, (int)doc->grdoc.nAFlagY);
    std::printf("точки рисуются : %d\n", (int)doc->grdoc.PointDrawFlag);
    std::printf("частота, Гц    : %g\n", doc->dFreq);

    for (int i = 0; i < doc->grdoc.nGraph; ++i) {
        const TMC_GR_DOC1_DN& g = doc->grdoc.psGraph[i];
        std::printf("\nграфик %d\n", i + 1);
        std::printf("  файл решётки   : %s\n", g.szFileName ? g.szFileName : "(нет)");
        std::printf("  имя            : %s\n", g.szGrapName ? g.szGrapName : "(нет)");
        std::printf("  подпись        : %s\n", g.szGrapPodp ? g.szGrapPodp : "(нет)");
        std::printf("  вход/мода      : %d/%d -> %d/%d\n", g.nInp1, g.nMod1, g.nInp2, g.nMod2);
        std::printf("  точек          : %d\n", g.nPoint);
        std::printf("  цвет линии     : %d, тип %d, толщина %d\n",
                    g.LineColor, g.LineType, g.LineWidth);
        std::printf("  вывод на экран : %d\n", g.OutFlag);

        // Решётка излучателей: считает её c2DArray, документ держит по одной
        // на график. Печатаем сводку — так видно, что расчёт прошёл.
        if (doc->pcDirectionalPattern) {
            c2DArray& a = doc->pcDirectionalPattern[i];
            if (a.IsError()) {
                std::printf("  решётка        : ошибка: %s\n", a.GetError());
            } else {
                std::printf("  излучателей    : %d\n", a.GetnIzl());
                std::printf("  углов          : %d\n", a.GetnAngle());
                std::printf("  частота, ГГц   : %g\n", a.GetdFreqGHz());
                std::printf("  КНД, дБ        : %g\n", a.GetKnd_dB());
                if (a.IsCalculateKip())
                    std::printf("  КИП, дБ        : %g\n", a.GetKip_dB());
                std::printf("  максимум       : %g\n", a.GetdMax());
            }
        }

        for (int k = 0; k < g.nPoint && k < 3; ++k) {
            std::printf("    угол[%d] = %-12g уровень = %g\n", k,
                        g.pFreq ? g.pFreq[k] : 0.0,
                        g.pSmatr ? g.pSmatr[k].x : 0.0);
        }
    }
    return 0;
}
