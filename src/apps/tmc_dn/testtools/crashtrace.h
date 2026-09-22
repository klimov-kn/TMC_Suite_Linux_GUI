// crashtrace.h — печать стека при падении. Только для средств разработки.
//
// В целевой системе отладчика может не быть, а падение надо увидеть здесь и
// сейчас. Обработчик сигнала печатает стек через backtrace() и помечает, в
// каком потоке случилось падение: расчёт диаграммы идёт в отдельном потоке, и
// это первое, что нужно знать.
//
// Цели, которые это подключают, собираются с `-g -rdynamic`: без -rdynamic
// имена функций из статических библиотек в стек не попадут.

#pragma once

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <execinfo.h>
#include <unistd.h>
#include <pthread.h>

namespace tmc_crashtrace {

inline pthread_t& mainThread()
{
    static pthread_t t = pthread_self();
    return t;
}

inline void handler(int sig)
{
    const char* name = (sig == SIGSEGV) ? "SIGSEGV"
                     : (sig == SIGABRT) ? "SIGABRT"
                     : (sig == SIGBUS)  ? "SIGBUS"
                     : (sig == SIGFPE)  ? "SIGFPE" : "сигнал";
    std::fprintf(stderr, "\n=== ПАДЕНИЕ: %s ===\n", name);
    std::fprintf(stderr, "поток: %s\n",
                 pthread_equal(pthread_self(), mainThread()) ? "главный (интерфейс)"
                                                             : "ФОНОВЫЙ (расчёт)");
    void* frames[64];
    const int n = backtrace(frames, 64);
    std::fflush(stderr);
    // Именно backtrace_symbols_fd, а не backtrace_symbols: второй выделяет
    // память, а если падение случилось из-за порчи кучи, malloc уже не работает
    // и стек напечатать не удастся.
    backtrace_symbols_fd(frames, n, 2);
    _exit(139);
}

/// Поставить обработчик. Звать из main() до создания окон.
inline void install()
{
    mainThread() = pthread_self();
    std::signal(SIGSEGV, handler);
    std::signal(SIGABRT, handler);
    std::signal(SIGBUS,  handler);
    std::signal(SIGFPE,  handler);
}

} // namespace tmc_crashtrace
