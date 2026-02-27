#pragma once

/*
 * platform.h
 *
 * Cross-platform abstraction layer for WinVetSim / OpenVetSim
 *
 * Provides a single include that conditionally pulls in the right system
 * headers and defines thin compatibility shims so that the core simulation
 * code compiles unchanged on both Windows (MSVC) and POSIX platforms
 * (macOS / Linux with Clang or GCC).
 *
 * Usage: replace all of the Windows-header blocks in vetsim.h with
 *   #include "platform.h"
 *
 * This file is part of the WinVetSim / OpenVetSim distribution.
 * Copyright (c) 2021-2025 VetSim, Cornell University College of Veterinary Medicine
 * Licensed under GNU GPL v3.
 */

// ============================================================
//  Common C++ standard-library headers (all platforms)
// ============================================================
#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cstdint>
#include <cinttypes>
#include <thread>
#include <mutex>
#include <functional>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <locale>
#include <codecvt>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <stdarg.h>

// ============================================================
//  Path separator (compile-time string literal)
// ============================================================
#ifdef _WIN32
#  define PATH_SEP "\\"
#else
#  define PATH_SEP "/"
#endif

//  WINDOWS
// ============================================================
#ifdef _WIN32

#define _WIN32_WINNT _WIN32_WINNT_MAXVER
#include <WinSDKVer.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winbase.h>
#include <tlhelp32.h>
#include <direct.h>
#include <conio.h>
#include <tchar.h>
#include <strsafe.h>
#include <Lmcons.h>
#include <synchapi.h>
#include <debugapi.h>
#include <WinUser.h>
#include <stringapiset.h>
#include <sal.h>
#include <psapi.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Psapi.lib")

// --- Mutex abstraction (Windows: HANDLE) ---
using SIM_MUTEX = HANDLE;

inline SIM_MUTEX sim_create_mutex()
{
    return CreateMutex(NULL, FALSE, NULL);
}
inline void sim_lock_mutex(SIM_MUTEX m)
{
    WaitForSingleObject(m, INFINITE);
}
inline void sim_unlock_mutex(SIM_MUTEX m)
{
    ReleaseMutex(m);
}
inline void sim_close_mutex(SIM_MUTEX m)
{
    CloseHandle(m);
}

// --- Sleep ---
inline void sim_sleep_ms(unsigned int ms) { Sleep(ms); }

// --- sim_clock_gettime_tv: on Windows, clock_gettime already uses timeval* ---
// (Windows clock_gettime is our custom implementation in simutil.cpp)
#define sim_clock_gettime_tv clock_gettime

// --- Time helpers (keep MSVC variants as-is on Windows) ---
// _time64, _localtime64_s, asctime_s, _dupenv_s, strerror_s
// are all natively available on MSVC — no shims needed.

// --- Process pipe ---
// _popen / _pclose are natively available on MSVC.

// --- Socket close ---
// closesocket() is natively available on Windows via winsock2.h

// ============================================================
//  POSIX  (macOS / Linux)
// ============================================================
#else  // !_WIN32

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <fcntl.h>
#include <cerrno>

// --- Windows integer types ---
using DWORD     = uint32_t;
using WORD      = uint16_t;
using BYTE      = uint8_t;
using BOOL      = int;
using LONG      = long;
using ULONG     = unsigned long;
using LONGLONG  = int64_t;
using ULONGLONG = uint64_t;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE  1
#endif

// --- Socket compatibility ---
using SOCKET = int;
#define INVALID_SOCKET  (-1)
#define SOCKET_ERROR    (-1)

inline int  closesocket(SOCKET s) { return close(s); }
// WSAStartup / WSACleanup are no-ops on POSIX
// wVersion is set to 0x0202 so any version check against 0x0202 passes.
struct WSAData { uint16_t wVersion = 0x0202; uint16_t wHighVersion = 0x0202; };
inline int  WSAStartup(int, WSAData* w) { if (w) w->wVersion = 0x0202; return 0; }
inline void WSACleanup() {}
// ZeroMemory
#ifndef ZeroMemory
#define ZeroMemory(ptr, sz) memset((ptr), 0, (sz))
#endif

// --- Mutex abstraction (POSIX: std::mutex*) ---
using SIM_MUTEX = std::mutex*;

inline SIM_MUTEX sim_create_mutex()   { return new std::mutex(); }
inline void      sim_lock_mutex(SIM_MUTEX m)   { if (m) m->lock(); }
inline void      sim_unlock_mutex(SIM_MUTEX m) { if (m) m->unlock(); }
inline void      sim_close_mutex(SIM_MUTEX m)  { delete m; }

// --- Sleep ---
inline void sim_sleep_ms(unsigned int ms) { usleep((useconds_t)ms * 1000u); }

// --- sprintf_s → snprintf ---
// Two overloads to handle both MSVC usage forms:
//   (1) sprintf_s(buf, size, fmt, ...)  — explicit size (4+ args)
//   (2) sprintf_s(arr, fmt, ...)        — MSVC template form that deduces N from char(&)[N]
inline int sprintf_s(char* buf, size_t sz, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, sz, fmt, ap);
    va_end(ap);
    return r;
}
template<size_t N>
inline int sprintf_s(char (&buf)[N], const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, N, fmt, ap);
    va_end(ap);
    return r;
}

// swprintf_s → swprintf
#define swprintf_s(buf, sz, fmt, ...)  swprintf(buf, sz, fmt, ##__VA_ARGS__)

// snprintf_s → snprintf
#define snprintf_s(buf, sz, cnt, fmt, ...) snprintf(buf, (sz), fmt, ##__VA_ARGS__)

// --- strerror_s → strerror_r ---
inline int strerror_s(char* buf, size_t sz, int errnum)
{
#if defined(__APPLE__) || defined(__FreeBSD__)
    // XSI-compliant strerror_r returns int
    return strerror_r(errnum, buf, sz);
#else
    // GNU strerror_r may return a char*
    char* msg = strerror_r(errnum, buf, sz);
    if (msg != buf) strncpy(buf, msg, sz - 1);
    return 0;
#endif
}
// errno_t is an int alias used in MSVC safe-function signatures
using errno_t = int;

// --- _dupenv_s → getenv ---
inline int _dupenv_s(char** pValue, size_t* pLen, const char* varname)
{
    const char* val = getenv(varname);
    if (val)
    {
        *pLen   = strlen(val) + 1;
        *pValue = static_cast<char*>(malloc(*pLen));
        if (*pValue) strcpy(*pValue, val);
    }
    else
    {
        *pValue = nullptr;
        *pLen   = 0;
    }
    return 0;
}

// --- 64-bit time helpers (MSVC extensions → standard C) ---
using __time64_t = time_t;
inline time_t _time64(time_t* t)  { return time(t); }
inline int _localtime64_s(struct tm* out, const time_t* t)
{
    return localtime_r(t, out) ? 0 : 1;
}
inline int asctime_s(char* buf, size_t /*sz*/, const struct tm* tm_info)
{
    char tmp[26];
    asctime_r(tm_info, tmp);
    strcpy(buf, tmp);  // caller is responsible for adequate buffer (26+ bytes)
    return 0;
}

// localtime_s (POSIX: localtime_r with swapped args)
inline int localtime_s(struct tm* out, const time_t* t)
{
    return localtime_r(t, out) ? 0 : 1;
}

// --- Process pipe ---
#define _popen  popen
#define _pclose pclose

// --- String comparison ---
#define _stricmp  strcasecmp
#define _wcsicmp  wcscasecmp

// --- strtok_s → strtok_r ---
#define strtok_s strtok_r

// --- fopen_s → fopen wrapper ---
inline errno_t fopen_s(FILE** fp, const char* path, const char* mode)
{
    *fp = fopen(path, mode);
    return (*fp == nullptr) ? errno : 0;
}

// --- mbstowcs_s → mbstowcs ---
inline errno_t mbstowcs_s(size_t* pNumConverted, wchar_t* wcstr,
                           size_t /*sizeInWords*/, const char* mbstr, size_t count)
{
    size_t n = mbstowcs(wcstr, mbstr, count);
    if (pNumConverted) *pNumConverted = n;
    return (n == (size_t)-1) ? 1 : 0;
}

// --- wcscat_s → wcscat ---
#define wcscat_s(dst, sz, src)  wcscat(dst, src)

// --- wcscpy_s → wcscpy ---
#define wcscpy_s(dst, sz, src)  wcscpy(dst, src)

// --- GetLastError / error string (stub using errno) ---
inline uint32_t GetLastError() { return (uint32_t)errno; }
inline std::string GetLastErrorAsString()
{
    return std::string(strerror(errno));
}

// --- Windows format specifier for size_t (%Iu → %zu) ---
// These are only used in printf calls in main.cpp; replace at source.

// --- WinSock type aliases used in networking code ---
using WSADATA    = WSAData;             // simstatus.cpp uses WSADATA
using SOCKADDR_IN = struct sockaddr_in; // simstatus.cpp uses SOCKADDR_IN
using LPSOCKADDR = struct sockaddr*;    // used in bind() casts
inline uint32_t WSAGetLastError() { return (uint32_t)errno; }
#ifndef MAKEWORD
#define MAKEWORD(lo, hi) ((uint16_t)(((uint8_t)(lo)) | (((uint16_t)((uint8_t)(hi))) << 8)))
#endif

// --- String helpers ---
#define _strdup strdup  // MSVC extension → POSIX

// --- getenv_s → getenv wrapper ---
inline int getenv_s(size_t* pLen, char* buf, size_t bufLen, const char* varname)
{
    const char* val = getenv(varname);
    if (!val) { if (pLen) *pLen = 0; if (buf && bufLen > 0) buf[0] = '\0'; return 0; }
    size_t n = strlen(val) + 1;
    if (pLen) *pLen = n;
    if (buf && bufLen >= n) { strcpy(buf, val); return 0; }
    return 1; // buffer too small
}

// --- MultiByteToWideChar stub (used in scenario_xml.cpp for length calculation) ---
#define CP_UTF8 65001
inline int MultiByteToWideChar(int /*cp*/, int /*flags*/,
    const char* src, int srcLen, wchar_t* dst, int dstLen)
{
    if (srcLen < 0) srcLen = (int)strlen(src);
    if (!dst || dstLen == 0) return (int)mbstowcs(nullptr, src, (size_t)srcLen) + 1;
    return (int)mbstowcs(dst, src, (size_t)dstLen);
}

// --- MessageBox stub (console fallback) ---
#define MB_OK          0
#define MB_ICONSTOP    0
inline int MessageBox(void* /*hwnd*/, const wchar_t* text, const wchar_t* /*caption*/, int /*type*/)
{
    // Convert wchar_t to narrow for console output
    char buf[512];
    wcstombs(buf, text, sizeof(buf) - 1);
    fprintf(stderr, "[MessageBox] %s\n", buf);
    return 0;
}
// Narrow-string overload
inline int MessageBoxA(void* /*hwnd*/, const char* text, const char* /*caption*/, int /*type*/)
{
    fprintf(stderr, "[MessageBox] %s\n", text);
    return 0;
}
inline int MessageBoxW(void* /*hwnd*/, const wchar_t* text, const wchar_t* /*caption*/, int /*type*/)
{
    char buf[512];
    wcstombs(buf, text, sizeof(buf) - 1);
    fprintf(stderr, "[MessageBox] %s\n", buf);
    return 0;
}

// --- File / directory existence ---
inline bool sim_dir_exists(const char* path)
{
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}
inline void sim_mkdir(const char* path)
{
    mkdir(path, 0755);
}

// --- ExitProcess → exit ---
#define ExitProcess(code) exit(code)

// --- Windows TEXT() and _tprintf() ---
// On POSIX, TEXT() is a no-op (strings are already narrow) and _tprintf = printf.
#ifndef TEXT
#define TEXT(x) x
#endif
#define _tprintf printf
#define _tprintf_s printf

// --- Windows thread priority stubs (POSIX: scheduling handled by OS) ---
inline void* GetCurrentThread() { return nullptr; }
inline bool  SetThreadPriority(void* /*hThread*/, int /*nPriority*/) { return true; }
inline int   GetThreadPriority(void* /*hThread*/) { return 0; }
#define THREAD_PRIORITY_TIME_CRITICAL  15
#define THREAD_PRIORITY_NORMAL          0

// --- _gcvt_s: convert double to string (base 10 only) ---
inline int _gcvt_s(char* buf, size_t sz, double val, int digits)
{
    return snprintf(buf, sz, "%.*g", digits, val);
}

// --- _i64toa_s: convert int64 to string (base 10 only) ---
inline int _i64toa_s(int64_t val, char* buf, size_t sz, int /*radix*/)
{
    return snprintf(buf, sz, "%" PRId64, val);
}

// --- GetTickCount64: milliseconds since an arbitrary start point ---
// Used for msec_time_update(); CLOCK_MONOTONIC never wraps during a session.
inline uint64_t GetTickCount64()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000));
}

// --- _kbhit / _getch: Windows console keyboard polling ---
// On POSIX, graceful exit is handled via SIGTERM (Ctrl-C), so these become no-ops.
inline int _kbhit() { return 0; }
inline int _getch()  { return 0; }

// --- _itoa_s / _ltoa_s → snprintf ---
// MSVC signature: _itoa_s(val, buf, bufSize, radix)
// We only support base 10 (radix=10) since that's the only usage.
inline int _itoa_s(int val, char* buf, size_t sz, int /*radix*/)
{
    return snprintf(buf, sz, "%d", val);
}
inline int _ltoa_s(long val, char* buf, size_t sz, int /*radix*/)
{
    return snprintf(buf, sz, "%ld", val);
}

// --- sscanf_s → sscanf ---
// MSVC sscanf_s requires buffer-size args after each %s/%c; on POSIX just use sscanf.
#define sscanf_s sscanf

// --- strncpy_s: explicit-size form and MSVC template array-deduction form ---
inline errno_t strncpy_s(char* dst, size_t dstSz, const char* src, size_t count)
{
    size_t n = (count < dstSz - 1) ? count : dstSz - 1;
    strncpy(dst, src, n);
    dst[n] = '\0';
    return 0;
}
template<size_t N>
inline errno_t strncpy_s(char (&dst)[N], const char* src, size_t count)
{
    size_t n = (count < N - 1) ? count : N - 1;
    strncpy(dst, src, n);
    dst[n] = '\0';
    return 0;
}

// --- sim_clock_gettime_tv: clock_gettime that fills a struct timeval ---
// scenario.cpp uses timeval-based timing arithmetic (tv_usec), so we provide
// a helper that calls native clock_gettime and converts timespec → timeval.
#include <time.h>
inline int sim_clock_gettime_tv(int clk, struct timeval* tv)
{
    struct timespec ts;
    int r = clock_gettime((clockid_t)clk, &ts);
    tv->tv_sec  = ts.tv_sec;
    tv->tv_usec = (suseconds_t)(ts.tv_nsec / 1000);
    return r;
}

#endif  // _WIN32

// ============================================================
//  Platform-neutral helpers (available on both platforms)
// ============================================================

// On Windows, sim_dir_exists / sim_mkdir are defined using Win32 API.
#ifdef _WIN32
inline bool sim_dir_exists(const char* path)
{
    DWORD ftyp = GetFileAttributesA(path);
    return (ftyp != INVALID_FILE_ATTRIBUTES && (ftyp & FILE_ATTRIBUTE_DIRECTORY));
}
inline void sim_mkdir(const char* path)
{
    CreateDirectoryA(path, NULL);
}
#endif
