// Проверка связки программ пакета.
//
// Код Windows-версии запускает соседние программы по их windows-именам
// («TMCROS.EXE», «FLDVIEW.EXE»…). Слой совместимости переводит такое имя в имя
// нашей программы рядом с текущей. Здесь проверяется именно перевод: что имя
// разрешается в существующий исполняемый файл. Сами программы не запускаются —
// они графические и не завершились бы сами.
//
// В поставку не входит.

#include <cstdio>
#include <string>

#include <sys/stat.h>

#include <afxwin.h>

namespace {

bool checkName(const char* windowsName)
{
    const std::string resolved = TmcResolveProgramName(windowsName);

    struct stat st;
    const bool exists = !resolved.empty()
                        && ::stat(resolved.c_str(), &st) == 0
                        && (st.st_mode & S_IXUSR);

    std::printf("  %-6s %-14s -> %s\n",
                exists ? "ОК" : "СБОЙ", windowsName, resolved.c_str());
    return exists;
}

} // namespace

int main()
{
    std::printf("Связка программ пакета: перевод windows-имён\n");

    const char* names[] = {
        "TMCROS.EXE", "TMCGROUT.EXE", "TMC_DN.EXE",
        "FLDVIEW.EXE", "PLANRT_H.EXE", "PLANRT_X.EXE",
    };
    const int total = int(sizeof(names) / sizeof(names[0]));

    int ok = 0;
    for (const char* name : names)
        ok += checkName(name) ? 1 : 0;

    // Внешний редактор: на Linux файл открывается тем, что назначено в системе.
    const std::string editor = TmcResolveProgramName("NOTEPAD.EXE");
    std::printf("  %-6s %-14s -> %s\n",
                editor == "xdg-open" ? "ОК" : "СБОЙ", "NOTEPAD.EXE", editor.c_str());

    std::printf("разрешилось программ: %d из %d\n", ok, total);
    return (ok == total && editor == "xdg-open") ? 0 : 1;
}
