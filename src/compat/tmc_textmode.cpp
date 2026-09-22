// tmc_textmode.cpp — текстовый режим файлов и поиск имени, как в Windows.
//
// ЗАЧЕМ. Файлы заданий и текстовые результаты TMC записаны с окончаниями строк
// CRLF. На Windows `fopen(..., "r")` открывает поток в ТЕКСТОВОМ режиме: при
// чтении пара CRLF превращается в один перевод строки, при записи наоборот.
// На Linux текстовый режим ничем не отличается от двоичного, поэтому символ
// возврата каретки доживает до разбора: пустая строка перестаёт быть пустой,
// число «20.0344\r» читается неправильно, а записанный файл отличается от
// windows-версии на байты окончаний строк — то есть перестаёт быть совместимым.
//
// Вторая задача — имя файла. Файловая система Windows не различает регистр:
// задание подключает `#include horns.h`, а на диске лежит `HORNS.H`. На ext4 это
// разные имена, и файл, созданный на Windows, не открывается.
//
// РЕШЕНИЕ. Перехватываем fopen флагом компоновщика -Wl,--wrap=fopen. Для
// ТЕКСТОВЫХ режимов (без 'b') возвращаем поток, который при чтении убирает
// возврат каретки, а при записи ставит его перед каждым переводом строки.
// ДВОИЧНЫЕ режимы идут в настоящий fopen без изменений — числовые форматы
// (S-матрицы) остаются байт в байт теми же. Имя в обоих случаях разрешается
// без учёта регистра.
//
// ПРОИЗВОДИТЕЛЬНОСТЬ. Перевод строк делается БЛОКАМИ, а не посимвольно: счётные
// ядра пишут выходной файл на каждом такте, и файлы бывают под мегабайт.
// Посимвольная обработка (fgetc/fputc на каждый байт) замедляла счёт в десятки
// раз по сравнению с Windows-версией.
//
// Это слой совместимости, а не вычисления: меняется только трактовка окончаний
// строк на границе ввода-вывода — ровно то, что делает библиотека C на Windows.

#ifndef _WIN32

#define _GNU_SOURCE
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <dirent.h>
#include <string>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>

extern "C" FILE* __real_fopen(const char* path, const char* mode);

namespace {

// --- Поиск файла без учёта регистра -------------------------------------------
//
// Ищем только по последнему элементу пути и только когда точного имени нет.
// Если совпадений несколько, ничего не выбираем — пусть лучше будет честная
// ошибка, чем открытие не того файла.
std::string resolveIgnoringCase(const char* path)
{
    if (!path || !*path)
        return std::string();

    struct stat st;
    if (::stat(path, &st) == 0)
        return path;                       // точное имя есть — ничего не ищем

    std::string full(path);
    const std::string::size_type slash = full.find_last_of('/');
    const std::string dir  = (slash == std::string::npos) ? std::string(".")
                                                          : full.substr(0, slash);
    const std::string name = (slash == std::string::npos) ? full
                                                          : full.substr(slash + 1);
    if (name.empty())
        return full;

    DIR* d = ::opendir(dir.c_str());
    if (!d)
        return full;

    std::string found;
    int matches = 0;
    while (struct dirent* e = ::readdir(d)) {
        if (::strcasecmp(e->d_name, name.c_str()) == 0) {
            ++matches;
            found = e->d_name;
        }
    }
    ::closedir(d);

    if (matches != 1)
        return full;                       // ноль или несколько — не гадаем

    return (slash == std::string::npos) ? found : dir + "/" + found;
}

// --- Поток с переводом окончаний строк ----------------------------------------

struct TextStream
{
    FILE* raw;          // настоящий поток, открытый в двоичном режиме
    char* buffer;       // рабочий буфер для блочного обмена
    size_t bufferSize;
};

const size_t kBufferSize = 64 * 1024;

// Чтение: берём блок и выбрасываем из него возвраты каретки. Если после чистки
// не осталось ни байта (блок был из одних CR), читаем следующий блок.
ssize_t textRead(void* cookie, char* buf, size_t size)
{
    TextStream* ts = static_cast<TextStream*>(cookie);
    if (size == 0)
        return 0;

    for (;;) {
        const size_t want = size < ts->bufferSize ? size : ts->bufferSize;
        const size_t got = std::fread(ts->buffer, 1, want, ts->raw);
        if (got == 0)
            return std::ferror(ts->raw) ? -1 : 0;

        // Быстрый путь: возвратов каретки в блоке нет — отдаём как есть.
        const char* cr = static_cast<const char*>(std::memchr(ts->buffer, '\r', got));
        if (!cr) {
            std::memcpy(buf, ts->buffer, got);
            return static_cast<ssize_t>(got);
        }

        size_t out = 0;
        const char* src = ts->buffer;
        const char* end = ts->buffer + got;
        while (src < end) {
            const char* next = static_cast<const char*>(
                std::memchr(src, '\r', size_t(end - src)));
            if (!next) {
                const size_t n = size_t(end - src);
                std::memcpy(buf + out, src, n);
                out += n;
                break;
            }
            const size_t n = size_t(next - src);
            std::memcpy(buf + out, src, n);
            out += n;
            src = next + 1;                 // сам возврат каретки пропускаем
        }
        if (out > 0)
            return static_cast<ssize_t>(out);
        // Блок состоял из одних возвратов каретки — читаем следующий.
    }
}

// Запись: перед каждым переводом строки добавляем возврат каретки. Пишем
// кусками между переводами строк, а не по байту.
ssize_t textWrite(void* cookie, const char* buf, size_t size)
{
    TextStream* ts = static_cast<TextStream*>(cookie);
    size_t done = 0;

    while (done < size) {
        const char* start = buf + done;
        const size_t left = size - done;
        const char* nl = static_cast<const char*>(std::memchr(start, '\n', left));
        const size_t chunk = nl ? size_t(nl - start) : left;

        if (chunk && std::fwrite(start, 1, chunk, ts->raw) != chunk)
            return -1;
        done += chunk;

        if (nl) {
            if (std::fwrite("\r\n", 1, 2, ts->raw) != 2)
                return -1;
            done += 1;                      // сам перевод строки уже записан
        }
    }
    return static_cast<ssize_t>(size);
}

// Позиционирование отдаём настоящему потоку. Смещения в файле с CRLF отличаются
// от «логических» на число возвратов каретки, но текстовые файлы TMC читаются
// строго последовательно (fgets/fscanf), а там, где нужен точный отсчёт байтов
// (S-матрицы), файл открывается в двоичном режиме и сюда не попадает.
int textSeek(void* cookie, off64_t* offset, int whence)
{
    TextStream* ts = static_cast<TextStream*>(cookie);
    if (fseeko(ts->raw, static_cast<off_t>(*offset), whence) != 0)
        return -1;
    *offset = static_cast<off64_t>(ftello(ts->raw));
    return 0;
}

int textClose(void* cookie)
{
    TextStream* ts = static_cast<TextStream*>(cookie);
    int r = std::fclose(ts->raw);
    std::free(ts->buffer);
    std::free(ts);
    return r;
}

} // namespace

extern "C" FILE* __wrap_fopen(const char* path, const char* mode)
{
    if (mode == 0)
        return __real_fopen(path, mode);

    // Имя разрешаем так же, как это делает Windows: без учёта регистра. Это
    // касается и записи: если файл с таким именем уже есть (пусть и в другом
    // регистре), Windows пишет именно в него, а не заводит второй.
    const std::string resolved = resolveIgnoringCase(path);
    const char* realPath = resolved.empty() ? path : resolved.c_str();

    // Двоичный режим — без перевода строк.
    if (std::strchr(mode, 'b'))
        return __real_fopen(realPath, mode);

    // Текстовый режим: настоящий поток открываем двоичным, перевод строк берёт
    // на себя обёртка.
    char rawMode[8];
    std::snprintf(rawMode, sizeof(rawMode), "%sb", mode);

    FILE* raw = __real_fopen(realPath, rawMode);
    if (!raw)
        return 0;

    TextStream* ts = static_cast<TextStream*>(std::malloc(sizeof(TextStream)));
    if (!ts) {
        std::fclose(raw);
        return 0;
    }
    ts->raw = raw;
    ts->bufferSize = kBufferSize;
    ts->buffer = static_cast<char*>(std::malloc(ts->bufferSize));
    if (!ts->buffer) {
        std::fclose(raw);
        std::free(ts);
        return 0;
    }

    cookie_io_functions_t io = { textRead, textWrite, textSeek, textClose };
    FILE* f = fopencookie(ts, mode, io);
    if (!f) {
        std::fclose(raw);
        std::free(ts->buffer);
        std::free(ts);
        return 0;
    }

    // Свой буфер и у внешнего потока: иначе каждый fputs из кода превращается
    // в отдельный вызов обёртки.
    std::setvbuf(f, 0, _IOFBF, kBufferSize);
    return f;
}

#endif // !_WIN32
