/*
 * WebSrv.cpp
 *
 * Manage the embedded PHP web server for WinVetSim / OpenVetSim.
 *
 * Responsibilities:
 *   - Locate the php executable (bundled or system-installed)
 *   - Launch  "php -S <addr>:<port> sim-ii/router.php"
 *   - Poll until the server responds to HTTP (health-check)
 *   - Provide a stop function for clean shutdown
 *
 * This file is part of the sim-mgr distribution.
 *
 * Copyright (c) 2021 VetSim, Cornell University College of Veterinary Medicine Ithaca, NY
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "vetsim.h"
#include <filesystem>

namespace fs = std::filesystem;

using namespace std;

// Forward declaration
static int checkURL(const string& host, const string& port, const string& page);

// ----------------------------------------------------------------
// isServerRunning
// ----------------------------------------------------------------
int isServerRunning(void)
{
	string host = PHP_SERVER_ADDR;
	string port = to_string(PHP_SERVER_PORT);
	string page = "sim-ii/hostCheck.php";
	return checkURL(host, port, page);
}

// ----------------------------------------------------------------
// stopPHPServer
//
// Send a platform-appropriate signal / command to terminate the
// PHP server process that was started by startPHPServer().
// ----------------------------------------------------------------
void
stopPHPServer(void)
{
#ifdef _WIN32
	// Windows: taskkill by window title (matches the "start" command title)
	system("taskkill /FI \"WINDOWTITLE eq WinVetSim PHP\"");
#else
	// POSIX: kill by matching command-line pattern
	// "pkill -f" sends SIGTERM to all processes whose argv matches the pattern.
	system("pkill -f 'php.*router.php'");
#endif
}

// ----------------------------------------------------------------
// findPhpPath
//
// Search well-known locations for a php executable.
// Fills phpPath[] and returns 1 on success, 0 if not found.
//
// Search order (both platforms):
//   1. Bundled copy next to this binary   ./PHP8.0/
//   2. Platform-specific system locations
// ----------------------------------------------------------------
char phpPath[FILENAME_MAX] = { 0, };

int
findPhpPath(void)
{
	// Helper: does <dir>/<name> exist as a regular file?
	auto hasExe = [](const fs::path& dir, const string& name) -> bool
	{
		fs::path candidate = dir / name;
		std::error_code ec;
		return fs::is_regular_file(candidate, ec);
	};

#ifdef _WIN32
	const string phpExe = "php.exe";
#else
	const string phpExe = "php";
#endif

	// ---- 1. Bundled copy (highest priority, same on all platforms) ----
	{
		fs::path bundled("./PHP8.0");
		if (hasExe(bundled, phpExe))
		{
			sprintf_s(phpPath, sizeof(phpPath), "%s", bundled.string().c_str());
			return 1;
		}
	}

#ifdef _WIN32
	// ---- 2. Windows: Program Files locations ----
	struct { const char* base; const char* sub; } winPaths[] = {
		{ "C:/Program Files/WinVetSim",    "PHP8.0"      },
		{ "C:/Program Files (x86)/PHP",    "v8.0"        },
		{ "C:/Program Files/PHP",          "v8.0"        },
		{ "C:/Program Files/PHP",          "v7.4"        },
		{ "C:/Program Files (x86)/PHP",    "v7.4"        },
		{ "C:/Program Files/PHP",          "v7.3"        },
		{ "C:/Program Files (x86)/PHP",    "v7.3"        },
		{ "C:/Program Files/PHP",          "v7.2"        },
		{ "C:/Program Files (x86)/PHP",    "v7.2"        },
	};
	for (auto& p : winPaths)
	{
		fs::path dir = fs::path(p.base) / p.sub;
		if (hasExe(dir, phpExe))
		{
			sprintf_s(phpPath, sizeof(phpPath), "%s", dir.string().c_str());
			return 1;
		}
	}

#else
	// ---- 2. POSIX: common install locations ----
	const char* posixPaths[] = {
		"/usr/local/bin",    // macOS Homebrew or manual install
		"/opt/homebrew/bin", // macOS Apple-Silicon Homebrew
		"/usr/bin",          // Linux apt/dnf packages
		nullptr
	};
	for (int i = 0; posixPaths[i] != nullptr; i++)
	{
		fs::path dir(posixPaths[i]);
		if (hasExe(dir, phpExe))
		{
			sprintf_s(phpPath, sizeof(phpPath), "%s", dir.string().c_str());
			return 1;
		}
	}
#endif

	printf("findPhpPath: php not found in any known location\n");
	return 0;
}

// ----------------------------------------------------------------
// startPHPServer
//
// Launch the PHP built-in web server and wait up to 1 second for
// it to start accepting connections.
// Returns 0 on success, -1 on failure.
// ----------------------------------------------------------------
int
startPHPServer(void)
{
	char commandLine[2048];
	char mbuf[300];
	int  rval = -1;

	if (isServerRunning() != 0)
	{
		sprintf_s(mbuf, sizeof(mbuf), "%s", "isServerRunning() says PHP is Already Running");
		log_message("", mbuf);
		return 0;
	}

	if (findPhpPath() == 0)
	{
		sprintf_s(mbuf, sizeof(mbuf), "%s", "findPhpPath() Fails");
		log_message("", mbuf);
		printf("Cannot find PHP\n");
		return rval;
	}

	printf("Starting PHP Server from %s\n", phpPath);

#ifdef _WIN32
	// Windows: use "start" so PHP gets its own console window and we can
	// identify it later by the window title for taskkill.
	sprintf_s(commandLine, sizeof(commandLine),
		"start \"WinVetSim PHP\" /d \"%s\" /min \"%s/%s\" -S %s:%d sim-ii/router.php",
		localConfig.html_path, phpPath, "php.exe",
		PHP_SERVER_ADDR, PHP_SERVER_PORT);
#else
	// POSIX: launch as background process, redirect output to a log file.
	sprintf_s(commandLine, sizeof(commandLine),
		"cd \"%s\" && \"%s/php\" -S %s:%d sim-ii/router.php >simlogs/php.log 2>&1 &",
		localConfig.html_path, phpPath,
		PHP_SERVER_ADDR, PHP_SERVER_PORT);
#endif

	sprintf_s(mbuf, sizeof(mbuf), "starting PHP: %s", commandLine);
	log_message("", mbuf);
	system(commandLine);

	// Poll until the server is up (up to 1 second in 10 ms steps)
	for (int checkCount = 100; checkCount > 0; checkCount--)
	{
		sim_sleep_ms(10);
		if (isServerRunning() != 0)
		{
			rval = 0;
			break;
		}
	}

	return rval;
}

// ----------------------------------------------------------------
// checkURL  (internal helper)
//
// Make a minimal HTTP GET request to host:port/page.
// Returns the number of bytes received in the response body
// (positive = server is responding), or 0 on failure.
//
// Uses only POSIX-compatible BSD sockets via platform.h shims,
// so the same code compiles on Windows (WinSock2) and macOS/Linux.
// ----------------------------------------------------------------
static int
checkURL(const string& host, const string& port, const string& page)
{
	SOCKET     Socket = INVALID_SOCKET;
	int        nDataLength;
	string     website_HTML;
	int        found = 0;

	// Build the HTTP/1.1 GET request
	string get_http =
		"GET /" + page + " HTTP/1.1\r\n"
		"Host: " + host + "\r\n"
		"Connection: close\r\n\r\n";

	WSAData wsaData;
	WSAStartup(0x0202, &wsaData);   // no-op on POSIX via platform.h

	struct addrinfo  hints;
	struct addrinfo* result = nullptr;
	struct addrinfo* ptr    = nullptr;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int dwRetval = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
	if (dwRetval != 0)
	{
		// printf("getaddrinfo failed: %d\n", dwRetval);
		WSACleanup();
		return 0;
	}

	for (ptr = result; ptr != nullptr; ptr = ptr->ai_next)
	{
		if (ptr->ai_family == AF_INET) { found = 1; break; }
	}

	if (found)
	{
		Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (Socket != INVALID_SOCKET)
		{
			if (connect(Socket, ptr->ai_addr, (int)ptr->ai_addrlen) == 0)
			{
				send(Socket, get_http.c_str(), (int)get_http.size(), 0);

				char buffer[4096];
				int  i = 0;
				while ((nDataLength = recv(Socket, buffer, sizeof(buffer) - 1, 0)) > 0)
				{
					buffer[nDataLength] = '\0';
					website_HTML += buffer;
					i += nDataLength;
				}
			}
			closesocket(Socket);
		}
	}

	freeaddrinfo(result);
	WSACleanup();   // no-op on POSIX

	return (int)website_HTML.length();
}
