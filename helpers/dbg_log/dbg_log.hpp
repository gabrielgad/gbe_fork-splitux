#pragma once

#include <string>
#include <string_view>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <chrono>

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

// Single process-wide logger, provided as an inline accessor so every translation
// unit / project that uses PRINT_DEBUG gets it WITHOUT needing to link a specific
// .cpp that defines a global (e.g. lib_steam_old uses PRINT_DEBUG but does not link
// base.cpp, and has no get_full_program_path). Log path: GSE_LOG_PATH env if set,
// else a fixed name in the process CWD. The file is opened lazily and only if
// logging actually activates (see dbg_log::is_active / GSE_FORCE_LOG).
inline dbg_log& dbg_logger_get()
{
    static dbg_log instance(
        []() -> std::string {
            const char* p = std::getenv("GSE_LOG_PATH");
            return (p && p[0]) ? std::string(p) : std::string("STEAM_LOG.log");
        }());
    return instance;
}
