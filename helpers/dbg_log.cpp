#include "dbg_log/dbg_log.hpp"
#include "common_helpers/common_helpers.hpp"
#include "utfcpp/utf8.h"

#include <iterator>
#include <cwchar>
#include <cstdarg>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <stdio.h>

#include "common_helpers/os_detector.h"


// Logging is compiled into release builds, but only ACTIVATES when GSE_FORCE_LOG is
// set in the environment (any non-empty value other than "0"). Debug builds always
// log. The single runtime gate lives here (and is mirrored by PRINT_DEBUG so an
// inactive logger does no work and has no side effects); every write path goes
// through open(), so when inactive out_file stays null and all writes no-op.
bool dbg_log::is_active()
{
#ifndef EMU_RELEASE_BUILD
	return true;
#else
	static int cached = -1;
	if (cached < 0) {
		const char* e = std::getenv("GSE_FORCE_LOG");
		cached = (e && e[0] && e[0] != '0') ? 1 : 0;
	}
	return cached == 1;
#endif
}

void dbg_log::open()
{
	if (is_active() && !out_file && filepath.size()) {

		// https://en.cppreference.com/w/cpp/filesystem/path/u8path
		const auto fsp = std::filesystem::u8path(filepath);
#if defined(__WINDOWS__)
		out_file = _wfopen(fsp.c_str(), L"at");
#else
		out_file = std::fopen(fsp.c_str(), "a");
#endif

	}
}

void dbg_log::write_stamp()
{
	auto elapsed = std::chrono::high_resolution_clock::now() - start_time;
	auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
	auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

	std::stringstream ss{};
	ss << "[" << duration_ms << " ms, " << duration_us << " us] ";
	auto ss_str = ss.str();

	std::fprintf(out_file, "%s", ss_str.c_str());
}

dbg_log::dbg_log(std::string_view path)
{
	// Store the path unconditionally; open() decides whether to actually open it
	// (release: only when GSE_FORCE_LOG is set). Storing a string is free.
	filepath = path;
}

dbg_log::dbg_log(std::wstring_view path)
{
	filepath = common_helpers::to_str(path);
}

dbg_log::~dbg_log()
{
	close();
}

void dbg_log::write(const std::string &str)
{
	write("%s", str.c_str());
}

void dbg_log::write(const std::wstring &str)
{
	write("%s", str.c_str());
}

void dbg_log::write(const char *fmt, ...)
{
	std::lock_guard lk(f_mtx);
	open();
	if (out_file) {
		write_stamp();

		std::va_list args;
		va_start(args, fmt);
		std::vfprintf(out_file, fmt, args);
		va_end(args);

		std::fprintf(out_file, "\n");
		std::fflush(out_file);
	}
}

void dbg_log::write(const wchar_t *fmt, ...)
{
	std::lock_guard lk(f_mtx);
	open();
	if (out_file) {
		write_stamp();

		std::va_list args;
		va_start(args, fmt);
		std::vfwprintf(out_file, fmt, args);
		va_end(args);

		std::fprintf(out_file, "\n");
		std::fflush(out_file);
	}
}

void dbg_log::close()
{
	std::lock_guard lk(f_mtx);
	if (out_file) {
		std::fprintf(out_file, "\nLog file closed\n\n");
		std::fclose(out_file);
		out_file = nullptr;
	}
}
