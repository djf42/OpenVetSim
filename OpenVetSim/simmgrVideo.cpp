/*
 * simmgrVideo.cpp
 *
 * Video Recording Support.
 *
 * This file is part of the sim-mgr distribution (https://github.com/OpenVetSimDevelopers/sim-mgr).
 *
 * Copyright (c) 2019 VetSim, Cornell University College of Veterinary Medicine Ithaca, NY
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

#ifdef _WIN32  // This file contains Windows-only functionality

#ifdef _WIN32
#include <psapi.h>
#include <tlhelp32.h>
#endif

#pragma comment(lib, "psapi.lib")

using namespace std;

struct obsData obsd = { NULL, 0, "" };

DWORD processId;


BOOL CALLBACK EnumWindowsProcMy(HWND hwnd, LPARAM lParam)
{
	DWORD lpdwProcessId;
	GetWindowThreadProcessId(hwnd, &lpdwProcessId);
	if (lpdwProcessId == lParam)
	{
		obsd.obsWnd = hwnd;

		printf("Found OBS Process\n");
		return FALSE;
	}
	else
	{
		obsd.obsWnd = NULL;
	}
	return TRUE;
}

void
getObsHandle(const char *appName )
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	BOOL sts;
	wchar_t app[512];

	swprintf(app, 512, L"%S", appName );

	if (obsd.obsWnd == NULL)
	{
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

		if (Process32First(snapshot, &entry) == TRUE)
		{
			do
			{
				//wprintf(L"%s\n", entry.szExeFile);
				if (wcscmp(entry.szExeFile, app) == 0)
				{
					HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
					wprintf(L"Found App %ls\n", app);
					obsd.obsPid = entry.th32ProcessID; 
					sts = EnumWindows(EnumWindowsProcMy, obsd.obsPid);
					// Do stuff..

					CloseHandle(hProcess);
				}
			} while (Process32Next(snapshot, &entry) == TRUE);
		}
		CloseHandle(snapshot);
	}
}
/**
 * recordStartStop
 * @record - Start if 1, Stop if 0
 *
 *
 */
int
recordStartStop(int record)
{
	//BOOL sts;
	//wchar_t keys[32];

	getObsHandle("obs64.exe");

	if (obsd.obsWnd == NULL)
	{
		log_message("", "OBS is not running. Please Start OBS or uncheck the \"Start Video with Scenario\" box. Then start the scenario again.");
		// Attempt to start OBS

		return ( -1 );
	}
	if (record)
	{
		// Send Start
		// Signal vitals JS code to trigger recording start
	}
	else
	{
		sprintf_s(obsd.newFilename, sizeof(obsd.newFilename), "%s", simmgr_shm->logfile.vfilename);

		// Send Stop
		// Signal vitals JS code to trigger recording stop
		obsd.obsWnd = NULL;
		closeVideoCapture();
	}
	return(0);
}

off_t
getFileSize(char* fname)
{
	struct stat sbuf;
	int sts;
	off_t size = -1;

	sts = stat(fname, &sbuf);
	if (sts == 0)
	{
		size = sbuf.st_size;
	}
	return (size);
}

#include <minwinbase.h>
#include <winnt.h>
int
getLatestFile(char* rname, int bufsize, char* dir)
{
	ULARGE_INTEGER lint;
	unsigned long long fileDate = 0;
	unsigned long long latestDate = 0;

	wchar_t fname[MAX_PATH];
	swprintf_s(fname, MAX_PATH, L"%S\\simlogs\\video\\*.mp4", localConfig.html_path );
	wchar_t latestFname[MAX_PATH] = { 0, };

	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;
	if ((hFind = FindFirstFile(fname, &FindFileData)) != INVALID_HANDLE_VALUE) {
		do {
			//printf("%s\n", FindFileData.cFileName);
			lint.LowPart = FindFileData.ftLastWriteTime.dwLowDateTime;
			lint.HighPart = FindFileData.ftLastWriteTime.dwHighDateTime;
			fileDate = lint.QuadPart;
			if (fileDate > latestDate)
			{
				latestDate = fileDate;
				swprintf_s(latestFname, MAX_PATH, L"%S/%ls", dir, &FindFileData.cFileName);
			}
		} while (FindNextFile(hFind, &FindFileData));
		FindClose(hFind);
	}

	if (wcslen(latestFname) != 0)
	{
		sprintf_s(rname, bufsize, "%ls", latestFname);
		return (0);
	}
	else
	{
		return (-1);
	}
}

int
renameVideoFile(char* filename)
{
	int sts;
	char drive[10];
	char dir[1024];
	char fname[1024];
	char ext[10];
	char newFile[1024];
	char oldFile[1024];
	errno_t err;
	snprintf(oldFile, 1024, "%s", filename);
	//printf("Calling _splitpath_s(\"%s\", %p, %d, %p, %d,%p, %d,%p, %d\n", filename, drive, 1024, dir, 1024, fname, 1024, ext, 1024);
	err = _splitpath_s(filename, drive, 10, dir, 1024, fname, 1024, ext, 10);
	if (err)
	{
		printf("_splitpath_s err: %d\n", err );
		sts = err;
	}
	else
	{
		snprintf(newFile, 1024, "%s%s%s", drive, dir, obsd.newFilename);
		printf("Rename %s to %s\n", oldFile, newFile);
		sts = rename(oldFile, newFile);
	}
	return (sts);
}

#define MAX_FILE_WAIT_LOOPS 10
char smvbuf[512];


void
closeVideoCapture()
{
	int fsizeA = 0;
	int fsizeB = 0;
	char filename[512];
	char path[MAX_PATH];
	int loops = 0;
	int sts;

	// Wait for file to complete and then rename it.
	// 1 - Find the latest file in simlogs/video
	sprintf_s(path, MAX_PATH, "%s/%s", localConfig.html_path, "simlogs/video");
	sts = getLatestFile(filename, sizeof(filename), path);
	if (sts == 0)
	{
		// Found file
		printf("Last Video File is: %s\n", filename);

		// 2 - Wait until Stop has completed. Detected by no change in size for 2 seconds.
		fsizeA = getFileSize(filename);
		do {
			Sleep(2000);
			fsizeB = getFileSize(filename);
			if (fsizeA == fsizeB)
			{
				break;
			}
			fsizeA = fsizeB;
		} while (loops++ < MAX_FILE_WAIT_LOOPS);

		// 3 - Rename the file
		if (loops < MAX_FILE_WAIT_LOOPS)
		{
			for (loops = 0; loops < MAX_FILE_WAIT_LOOPS; loops++)
			{
				sts = renameVideoFile(filename);
				if (sts == 0)
				{
					break;
				}
				Sleep(1000);
			}
			if (sts != 0)
			{
				strerror_s(smvbuf, 512, errno);
				printf("renameVideoFile failed: %s\n", smvbuf);
			}
		}
		else
		{
			printf("File Size did not stop %d, %d\nq", fsizeA, fsizeB);
		}
	}
}

int
getVideoFileCount(void)
{
	int file_count = 0;
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;
	wchar_t name[MAX_PATH];
	swprintf_s(name, L"%S\\simlogs\\video\\*.mp4", localConfig.html_path);

	if ((hFind = FindFirstFile(name, &FindFileData)) != INVALID_HANDLE_VALUE) {
		do {
			file_count++;
		} while (FindNextFile(hFind, &FindFileData));
		FindClose(hFind);
	}

	return (file_count);
}

#else  // ! _WIN32
#include <dirent.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

static char posix_target_vfilename[256] = "";

static off_t
posix_getFileSize(const char* fname)
{
	struct stat sbuf;
	if (stat(fname, &sbuf) == 0) {
		return sbuf.st_size;
	}
	return -1;
}

static int
posix_getLatestMp4(char* rname, int bufsize, const char* dir)
{
	DIR* d = opendir(dir);
	if (d == NULL) return -1;

	time_t latestTime = 0;
	char latestName[1088] = "";
	struct dirent* entry;

	while ((entry = readdir(d)) != NULL) {
		size_t len = strlen(entry->d_name);
		if (len > 4 && strcasecmp(entry->d_name + len - 4, ".mp4") == 0) {
			char fullpath[1088];
			snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entry->d_name);
			struct stat sbuf;
			if (stat(fullpath, &sbuf) == 0 && sbuf.st_mtime > latestTime) {
				latestTime = sbuf.st_mtime;
				snprintf(latestName, sizeof(latestName), "%s", fullpath);
			}
		}
	}
	closedir(d);

	if (strlen(latestName) > 0) {
		snprintf(rname, bufsize, "%s", latestName);
		return 0;
	}
	return -1;
}

static void*
posix_closeVideoCapture_thread(void* /*arg*/)
{
	char videoDir[1088];
	snprintf(videoDir, sizeof(videoDir), "%s/simlogs/video", localConfig.html_path);

	// Wait up to 30s for OBS to finish writing the file
	char filename[1088] = "";
	for (int wait = 0; wait < 30; wait++) {
		if (posix_getLatestMp4(filename, sizeof(filename), videoDir) == 0) {
			break;
		}
		sleep(1);
	}

	if (strlen(filename) == 0) {
		printf("closeVideoCapture: no mp4 file found\n");
		return NULL;
	}

	printf("Last Video File is: %s\n", filename);

	// Wait until file size stops changing (recording complete)
	off_t fsizeA = posix_getFileSize(filename);
	int loops = 0;
	do {
		sleep(2);
		off_t fsizeB = posix_getFileSize(filename);
		if (fsizeA == fsizeB) break;
		fsizeA = fsizeB;
	} while (loops++ < 10);

	// Rename to the expected target filename so debrief player can find it
	char newFile[1088];
	snprintf(newFile, sizeof(newFile), "%s/%s", videoDir, posix_target_vfilename);
	printf("Rename %s to %s\n", filename, newFile);
	if (rename(filename, newFile) != 0) {
		perror("renameVideoFile failed");
	}

	return NULL;
}

// Count .mp4 files in simlogs/video so start_scenario() can detect when
// OBS has created a new recording file.
int
getVideoFileCount(void)
{
	int file_count = 0;
	char path[1088];
	snprintf(path, sizeof(path), "%s/simlogs/video", localConfig.html_path);

	DIR* dir = opendir(path);
	if (dir == NULL) {
		return 0;
	}

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		size_t len = strlen(entry->d_name);
		if (len > 4 && strcasecmp(entry->d_name + len - 4, ".mp4") == 0) {
			file_count++;
		}
	}
	closedir(dir);
	return file_count;
}

// OBS recording is triggered by the JS WebSocket layer.
// On stop, save the target filename and launch a background thread to
// wait for OBS to finish writing, then rename the file so the debrief
// player can find it by the expected name.
int
recordStartStop(int record)
{
	if (!record) {
		// Save target filename before the scenario state changes
		snprintf(posix_target_vfilename, sizeof(posix_target_vfilename),
				 "%s", simmgr_shm->logfile.vfilename);

		pthread_t tid;
		pthread_create(&tid, NULL, posix_closeVideoCapture_thread, NULL);
		pthread_detach(tid);
	}
	return 0;
}

#endif  // _WIN32
