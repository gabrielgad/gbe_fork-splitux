#pragma once

#include <string>
#include <string_view>
#include <mutex>
#include <cstdio>
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
