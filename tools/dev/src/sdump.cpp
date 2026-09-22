// Печать матрицы рассеяния из файла .s — тем же кодом, каким её читают
// программы пакета (read_S_matrix_element из библиотеки sfile95).
//
// Нужен, чтобы сверять результаты разных сборок по числам, а не по байтам:
// сигналы во времени (.t) совпадают побайтно, а S-матрица считается из них
// через комплексные функции, и последний разряд double может отличаться от
// системы к системе.
//
// В поставку не входит: инструмент разработки.
//
//   sdump <файл.s> [число входов]
//
// Печатает по строке на элемент: S<i><j>, частота, действительная и мнимая
// части, модуль и модуль в децибелах.

#include <afxwin.h>

#include <proc_s.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::fprintf(stderr,
            "sdump — печать матрицы рассеяния из файла .s\n"
            "  sdump <файл.s> [число входов]\n");
        return 2;
    }

    char *path = argv[1];
    const int inputs = (argc > 2) ? std::atoi(argv[2]) : 4;

    std::printf("файл: %s\n", path);
    std::printf("%-6s %14s %22s %22s %14s %10s\n",
                "эл-т", "частота", "Re", "Im", "модуль", "дБ");

    int printed = 0;
    for (int i = 1; i <= inputs; ++i) {
        for (int j = 1; j <= inputs; ++j) {
            int in1 = i, in2 = j, mod1 = 1, mod2 = 1, nPoint = 0;
            _complex *sss = NULL;
            double *f1 = NULL;

            if (read_S_matrix_element(&in1, &mod1, &in2, &mod2, path,
                                      &sss, &f1, &nPoint) != 0)
                continue;
            if (sss == NULL || f1 == NULL || nPoint <= 0)
                continue;

            for (int k = 0; k < nPoint; ++k) {
                const double re = sss[k].x;
                const double im = sss[k].y;
                const double mag = std::sqrt(re * re + im * im);
                const double db = (mag > 0.0) ? 20.0 * std::log10(mag) : -999.0;
                char name[16];
                std::snprintf(name, sizeof(name), "S%d%d", i, j);
                std::printf("%-6s %14.6f %22.17g %22.17g %14.9f %10.4f\n",
                            name, f1[k], re, im, mag, db);
                ++printed;
            }
        }
    }

    if (printed == 0) {
        std::fprintf(stderr, "ни одного элемента не прочитано "
                             "(проверьте число входов)\n");
        return 1;
    }
    return 0;
}
