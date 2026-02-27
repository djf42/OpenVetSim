/*
 * key.cpp
 *
 * Windows Registry Keys access.
 *
 * This file is part of the WinVetSim distribution (https://github.com/OpenVetSimDevelopers/sim-mgr).
 *
 * Copyright (c) 2021-2023 VetSim, Cornell University College of Veterinary Medicine Ithaca, NY
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

/*
 * Configuration parameters are now kept in the WinVetSim html directory, in the file winvetsim.ini
 * This file provides a means to transfer previously set Registry keys to the .ini file.
 * 1 - Defaults are set at program start
 * 2 - Values are read from the Registry  (Windows only)
 * 3 - Values are read from winvetsim.ini
 * 4 - If a value for a setting is found in the .ini file, it is used.
 * 5 - If an entry is missing the entry, the .ini file is updated to add it.
 */

#include "vetsim.h"
#include <stdio.h>
#include "ini.h"

using namespace std;

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

// ─────────────────────────────────────────────────────────────────────────────
// Windows-only: Registry helper functions
// ─────────────────────────────────────────────────────────────────────────────
#ifdef _WIN32

WCHAR achValue[MAX_VALUE_NAME];

void QueryKey(HKEY hKey)
{
	WCHAR achKey[MAX_KEY_LENGTH];
	DWORD cbName;
	WCHAR achClass[MAX_PATH] = TEXT("");
	DWORD cchClassName = MAX_PATH;
	DWORD cSubKeys = 0;
	DWORD cbMaxSubKey;
	DWORD cchMaxClass;
	DWORD cValues;
	DWORD cchMaxValue;
	DWORD cbMaxValueData;
	DWORD cbSecurityDescriptor;
	FILETIME ftLastWriteTime;
	DWORD i, retCode;
	DWORD cchValue = MAX_VALUE_NAME;
	retCode = RegQueryInfoKey(
		hKey, achClass, &cchClassName, NULL,
		&cSubKeys, &cbMaxSubKey, &cchMaxClass,
		&cValues, &cchMaxValue, &cbMaxValueData,
		&cbSecurityDescriptor, &ftLastWriteTime);
	wprintf(L"RegQueryInfoKey() returns %u\n", retCode);
	if (cSubKeys)
	{
		wprintf(L"\nNumber of subkeys: %d\n", cSubKeys);
		for (i = 0; i < cSubKeys; i++)
		{
			cbName = MAX_KEY_LENGTH;
			retCode = RegEnumKeyEx(hKey, i, achKey, &cbName, NULL, NULL, NULL, &ftLastWriteTime);
			if (retCode == ERROR_SUCCESS)
				wprintf(L"(%d) %s\n", i + 1, achKey);
		}
	}
	else
		wprintf(L"No subkeys to be enumerated!\n");

	if (cValues)
	{
		wprintf(L"\nNumber of values: %d\n", cValues);
		for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++)
		{
			cchValue = MAX_VALUE_NAME;
			achValue[0] = '\0';
			retCode = RegEnumValue(hKey, i, achValue, &cchValue, NULL, NULL, NULL, NULL);
			if (retCode == ERROR_SUCCESS)
				wprintf(L"(%d) %s\n", i + 1, achValue);
		}
	}
	else
		wprintf(L"No values to be enumerated!\n");
}

BOOL writeStringInRegistry(HKEY hKeyParent, LPCWSTR subkey, LPCSTR valueName, char* strData, int len)
{
	DWORD Ret;
	HKEY hKey;
	LPBYTE bptr = (LPBYTE)strData;
	Ret = RegOpenKeyEx(hKeyParent, subkey, 0, KEY_WRITE, &hKey);
	if (Ret == ERROR_SUCCESS)
	{
		if (ERROR_SUCCESS != RegSetValueExA(hKey, valueName, 0, REG_SZ, bptr, len))
		{
			RegCloseKey(hKey);
			return FALSE;
		}
		RegCloseKey(hKey);
		return TRUE;
	}
	return FALSE;
}

int WriteInRegistry(HKEY hKeyParent, LPCWSTR subkey, LPCWSTR valueName, DWORD data)
{
	DWORD Ret;
	HKEY hKey;
	Ret = RegOpenKeyEx(hKeyParent, subkey, 0, KEY_WRITE, &hKey);
	if (Ret == ERROR_SUCCESS)
	{
		if (ERROR_SUCCESS != RegSetValueEx(hKey, valueName, 0, REG_DWORD,
			reinterpret_cast<BYTE*>(&data), sizeof(data)))
		{
			RegCloseKey(hKey);
			return FALSE;
		}
		RegCloseKey(hKey);
		return TRUE;
	}
	return FALSE;
}

DWORD CreateRegistryKey(HKEY hKeyParent, LPCWSTR subkey)
{
	DWORD dwDisposition;
	HKEY  hKey;
	DWORD Ret;
	Ret = RegCreateKeyEx(hKeyParent, subkey, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
	if (Ret != ERROR_SUCCESS)
		printf("Error opening or creating key.\n");
	else
		RegCloseKey(hKey);
	return Ret;
}

DWORD readDwordValueRegistry(HKEY hKeyParent, LPCWSTR subkey, LPCWSTR valueName, DWORD* readData)
{
	HKEY hKey;
	DWORD Ret;
	Ret = RegOpenKeyExW(hKeyParent, subkey, 0, KEY_READ, &hKey);
	if (Ret == ERROR_SUCCESS)
	{
		DWORD data;
		DWORD len = sizeof(DWORD);
		Ret = RegQueryValueExW(hKey, valueName, NULL, NULL, (LPBYTE)(&data), &len);
		if (Ret == ERROR_SUCCESS)
			(*readData) = data;
		RegCloseKey(hKey);
	}
	return Ret;
}

DWORD readStringFromRegistry(HKEY hKeyParent, LPCWSTR subkey, LPCSTR valueName, char* readData, DWORD len)
{
	HKEY hKey;
	DWORD readDataLen = len;
	DWORD Ret = RegOpenKeyEx(hKeyParent, subkey, 0, KEY_READ, &hKey);
	if (Ret == ERROR_SUCCESS)
	{
		Ret = RegQueryValueExA(hKey, valueName, NULL, NULL, (BYTE*)readData, &readDataLen);
		RegCloseKey(hKey);
	}
	return (Ret);
}

HKEY whichKey = HKEY_CURRENT_USER;

void
readSubKeys(void)
{
	DWORD data;
	DWORD Ret;
	char stringBuf[FILENAME_SIZE];
	int len;

	Ret = readDwordValueRegistry(whichKey, L"SOFTWARE\\WinVetSim", L"PulsePortNum", &data);
	if (Ret == ERROR_SUCCESS)
		localConfig.port_pulse = data;

	Ret = readDwordValueRegistry(whichKey, L"SOFTWARE\\WinVetSim", L"StatusPortNum", &data);
	if (Ret == ERROR_SUCCESS)
		localConfig.port_status = data;

	Ret = readDwordValueRegistry(whichKey, L"SOFTWARE\\WinVetSim", L"ServerPortNum", &data);
	if (Ret == ERROR_SUCCESS)
		localConfig.php_server_port = data;

	Ret = readStringFromRegistry(whichKey, L"SOFTWARE\\WinVetSim", "ServerAddress", stringBuf, STR_SIZE);
	if (Ret == ERROR_SUCCESS)
		sprintf_s(localConfig.php_server_addr, sizeof(localConfig.php_server_addr), "%s", stringBuf);

	Ret = readStringFromRegistry(whichKey, L"SOFTWARE\\WinVetSim", "LogName", stringBuf, STR_SIZE);
	if (Ret == ERROR_SUCCESS)
		sprintf_s(localConfig.log_name, sizeof(localConfig.log_name), "%s", stringBuf);

	Ret = readStringFromRegistry(whichKey, L"SOFTWARE\\WinVetSim", "HTML_Path", stringBuf, STR_SIZE);
	if (Ret == ERROR_FILE_NOT_FOUND)
	{
		len = (int)strlen(localConfig.html_path);
		Ret = writeStringInRegistry(whichKey, L"SOFTWARE\\WinVetSim", "HTML_Path", localConfig.html_path, len);
	}
	else if (Ret == ERROR_SUCCESS)
		sprintf_s(localConfig.html_path, sizeof(localConfig.html_path), "%s", stringBuf);
}

#endif  // _WIN32

// ─────────────────────────────────────────────────────────────────────────────
// Cross-platform: INI output helper
// ─────────────────────────────────────────────────────────────────────────────

void outputData(mINI::INIStructure const& ini)
{
	for (auto const& it : ini)
	{
		auto const& section = it.first;
		auto const& collection = it.second;
		std::cout << "[" << section << "]" << std::endl;
		for (auto const& it2 : collection)
		{
			auto const& key = it2.first;
			auto const& value = it2.second;
			std::cout << key << "=" << value << std::endl;
		}
		std::cout << std::endl;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// getKeys() — reads Registry (Windows) then INI file (all platforms)
// ─────────────────────────────────────────────────────────────────────────────
int getKeys()
{
	int rval = 0;
	bool ret;
	char errorstr[256];
	errno_t et;

#ifdef _WIN32
	HKEY theKey;
	LPCTSTR strKeyName = L"SOFTWARE\\WinVetSim";

	// Check first for Private Key
	long sts = RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\WinVetSim", 0, KEY_READ, &theKey);
	if (ERROR_SUCCESS == sts)
	{
		rval = 1;
		whichKey = HKEY_CURRENT_USER;
	}
	else if (ERROR_NO_MATCH == sts || ERROR_FILE_NOT_FOUND == sts)
	{
		long sts2 = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WinVetSim", 0, KEY_READ, &theKey);
		if (ERROR_SUCCESS == sts2)
		{
			rval = 1;
			whichKey = HKEY_LOCAL_MACHINE;
		}
		else if (ERROR_NO_MATCH == sts2 || ERROR_FILE_NOT_FOUND == sts2)
		{
			rval = -1;
		}
	}
	else
	{
		rval = -1;
	}

	if (rval >= 0)
	{
		RegCloseKey(theKey);
		readSubKeys();
	}
#endif  // _WIN32

	// Now process the ini file (cross-platform)
	char iniFileName[512];
	snprintf(iniFileName, sizeof(iniFileName), "%s/%s", HTML_PATH, "winvetsim.ini");

	mINI::INIFile file(iniFileName);
	mINI::INIStructure ini;
	ret = file.read(ini);
	if (ret != true)
	{
		et = strerror_s(errorstr, sizeof(errorstr), errno);
		printf("INI Read Fails (%s), Try to create.\n", errorstr);
		std::ofstream fileWriteStream(iniFileName);
		if (fileWriteStream.is_open())
		{
			fileWriteStream << "; Configuration file for WinVetSim" << std::endl;
			fileWriteStream << "[Server]" << std::endl;
			fileWriteStream << "serverAddress = " << PHP_SERVER_ADDR << std::endl;
			fileWriteStream << "serverPort = " << PHP_SERVER_PORT << std::endl;
			fileWriteStream << "" << std::endl;
			fileWriteStream << "[Listeners]" << std::endl;
			fileWriteStream << "pulsePort = " << PORT_PULSE << std::endl;
			fileWriteStream << "statusPort = " << PORT_STATUS << std::endl;
			ret = file.read(ini);
			if (ret != true)
			{
				et = strerror_s(errorstr, sizeof(errorstr), errno);
				printf("INI Read fails after create (%s)\n", errorstr);
			}
		}
		else
		{
			et = strerror_s(errorstr, sizeof(errorstr), errno);
			printf("INI is_open() fails (%s)\n", errorstr);
		}
	}
	if (ret == true)
	{
		outputData(ini);
		if (ini["Server"]["serverPort"].length() > 0)
			localConfig.php_server_port = atoi((const char*)ini["Server"]["serverPort"].c_str());
		if (ini["Server"]["serverAddress"].length() > 0)
			snprintf(localConfig.php_server_addr, sizeof(localConfig.php_server_addr),
				"%s", ini["Server"]["serverAddress"].c_str());
		if (ini["Listeners"]["pulsePort"].length() > 0)
			localConfig.port_pulse = atoi((const char*)ini["Listeners"]["pulsePort"].c_str());
		if (ini["Listeners"]["statusPort"].length() > 0)
			localConfig.port_status = atoi((const char*)ini["Listeners"]["statusPort"].c_str());
		printf("Data from INI: Server %s:%d, Pulse %d, Status %d\n",
			localConfig.php_server_addr,
			localConfig.php_server_port,
			localConfig.port_pulse,
			localConfig.port_status);
	}
	return (rval);
}
