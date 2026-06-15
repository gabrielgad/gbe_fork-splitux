#pragma once

#include <string>
#include <string_view>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <chrono>

#if defined(_WIN32)
    #include <process.h>   // _getpid
#else
    #include <unistd.h>    // getpid
#endif

class dbg_log
{
private:
    std::recursive_mutex f_mtx{};
    std::string filepath{};
    std::FILE *out_file{};
    const std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();

    void open();
    void write_stamp();

public:
    dbg_log(std::string_view path);
    dbg_log(std::wstring_view path);
    ~dbg_log();

    void write(const std::string &str);
    void write(const std::wstring &str);

    void write(const char* fmt, ...);
    void write(const wchar_t* fmt, ...);

    void close();

    // True when logging should actually happen: always in debug builds; in release
    // builds only when the GSE_FORCE_LOG env var is set (cached on first call).
    // PRINT_DEBUG gates its whole body on this so an inactive logger has ~zero cost
    // and no side effects.
    bool is_active();
};

// Inject the current process id before the file extension, e.g.
// "gse-foo.log" -> "gse-foo.12345.log". Games that run more than one process under
// the same account (a launcher + the shipping exe, EAC/EOS helpers, or a re-init)
// otherwise all clobber a single GSE_LOG_PATH and produce an un-splittable merged
// log. Per-pid files keep each process's view separate.
inline std::string dbg_log_inject_pid(std::string path)
{
#if defined(_WIN32)
    long pid = static_cast<long>(_getpid());
#else
    long pid = static_cast<long>(getpid());
#endif
    const std::string ins = "." + std::to_string(pid);
    auto dot = path.find_last_of('.');
    auto slash = path.find_last_of("/\\");
    if (dot != std::string::npos && (slash == std::string::npos || dot > slash)) {
        path.insert(dot, ins);
    } else {
        path += ins;
    }
    return path;
}

// Single process-wide logger, provided as an inline accessor so every translation
// unit / project that uses PRINT_DEBUG gets it WITHOUT needing to link a specific
// .cpp that defines a global (e.g. lib_steam_old uses PRINT_DEBUG but does not link
// base.cpp, and has no get_full_program_path). Log path: GSE_LOG_PATH env if set,
// else a fixed name in the process CWD; the pid is injected so multi-process games
// don't clobber one file. The file is opened lazily and only if logging actually
// activates (see dbg_log::is_active / GSE_FORCE_LOG).
inline dbg_log& dbg_logger_get()
{
    static dbg_log instance(
        []() -> std::string {
            const char* p = std::getenv("GSE_LOG_PATH");
            std::string base = (p && p[0]) ? std::string(p) : std::string("STEAM_LOG.log");
            return dbg_log_inject_pid(base);
        }());
    return instance;
}
