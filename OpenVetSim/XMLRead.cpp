/*
 * XMLRead.cpp
 *
 * Parser for XML files.
 *
 * This file is part of the WinVetSim distribution (https://github.com/OpenVetSimDevelopers/sim-mgr).
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
#include "XMLRead.h"

#ifdef _WIN32
void DisplayError(LPTSTR lpszFunction);
#endif

int
XMLRead::open(const char* path)
{
    char* cptr;
    char* cptr1;
    char* cptr2;
    int length;

    depth = -1;
    fileLength = 0;
    name[0] = 0;
    value[0] = 0;
    idx = 0;

    printf("XMLRead open %s\n", path);

#ifdef _WIN32
    // ── Windows: use CreateFile / ReadFile ──────────────────────────────────
    LARGE_INTEGER filelen;
    size_t len;
    DWORD ol;
    BOOL sts;
    HANDLE hFile;

    TCHAR* tchar = new TCHAR[strlen(path) + 4];
    for (size_t i = 0; i <= strlen(path); i++)
        tchar[i] = path[i];

    hFile = CreateFile((LPCWSTR)tchar,
        GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DisplayError((LPTSTR)(TEXT("CreateFile")));
        _tprintf(TEXT("Terminal failure: unable to open file \"%s\" for read.\n"), tchar);
        delete[] tchar;
        return (-1);
    }
    sts = GetFileSizeEx(hFile, &filelen);
    if (!sts)
    {
        DisplayError((LPTSTR)(TEXT("GetFileSizeEx")));
        _tprintf(TEXT("Terminal failure: unable to get file length\"%s\".\n"), tchar);
        CloseHandle(hFile);
        delete[] tchar;
        return (-1);
    }
    len = (size_t)filelen.LowPart;
    XMLRead::xml = (char*)calloc(len + 32, 1);
    XMLRead::idx = 0;
    _tprintf(TEXT("ReadFile Request %d bytes.\n"), (int)len);

    sts = ReadFile(hFile, (LPVOID)&XMLRead::xml[0], (DWORD)len, &ol, NULL);
    if (!sts || ol != len)
    {
        DisplayError((LPTSTR)(TEXT("ReadFile")));
        _tprintf(TEXT("Terminal failure: ReadFile for \"%s\" returned %d bytes with %d expected.\n"),
            tchar, ol, (int)len);
        printf(" %.40s...\n", XMLRead::xml);
        CloseHandle(hFile);
        delete[] tchar;
        return (-1);
    }
    CloseHandle(hFile);
    delete[] tchar;
    XMLRead::fileLength = (int)ol;

#else
    // ── POSIX: use fopen / fread ─────────────────────────────────────────────
    FILE* fp = fopen(path, "rb");
    if (!fp)
    {
        fprintf(stderr, "XMLRead::open: cannot open '%s': %s\n", path, strerror(errno));
        return (-1);
    }
    fseek(fp, 0, SEEK_END);
    long flen = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (flen < 0)
    {
        fprintf(stderr, "XMLRead::open: ftell failed for '%s'\n", path);
        fclose(fp);
        return (-1);
    }
    size_t len = (size_t)flen;
    XMLRead::xml = (char*)calloc(len + 32, 1);
    XMLRead::idx = 0;
    size_t nr = fread(XMLRead::xml, 1, len, fp);
    fclose(fp);
    if (nr != len)
    {
        fprintf(stderr, "XMLRead::open: fread got %zu of %zu bytes for '%s'\n", nr, len, path);
        return (-1);
    }
    XMLRead::fileLength = (int)len;
#endif

    printf("Read is good\n");

    cptr = XMLRead::xml;
    // Strip out prolog, if present  (replace with spaces)
    cptr1 = strstr(cptr, "<?xml");
    if (!cptr1)
        cptr1 = strstr(cptr, "<?XML");
    if (cptr1)
    {
        cptr2 = strstr(cptr1, "?>");
        if (cptr2)
        {
            cptr2 += 2;
            length = (int)(cptr2 - cptr1);
            memset(cptr1, ' ', length);
        }
    }

    // Strip out all comments (replace with spaces)
    cptr1 = cptr;
    while (1)
    {
        cptr1 = strstr(cptr1, "<!--");
        if (cptr1)
        {
            cptr2 = strstr(cptr1, "-->");
            if (cptr2)
            {
                cptr2 += 3;
                length = (int)(cptr2 - cptr1);
                memset(cptr1, ' ', length);
            }
            else
                break;
        }
        else
            break;
    }

    printf("XMLRead::open complete\n");
    return (0);
}

int
XMLRead::getEntry(void)
{
    int i;
    size_t maxLen;
    char* cptr = XMLRead::xml + XMLRead::idx;
    char* cptr2;
    XMLRead::type = XML_TYPE_FILE_END;  // Default return if nothing is found

    if (XMLRead::state == xmlParseState::initial || XMLRead::state == xmlParseState::closedElement)
    {
        for (i = 0; cptr[i]; i++)
        {
            if (strncmp(&cptr[i], "</", 2) == 0)
            {
                cptr2 = &cptr[i + 2];
                for (i = 0; cptr2[i]; i++)
                {
                    if (cptr2[i] == '>' || isspace(cptr2[i]))
                        break;
                }
                strncpy_s(XMLRead::name, cptr2, size_t(i) - 1);
                XMLRead::name[i] = 0;
                XMLRead::value[0] = 0;
                XMLRead::type = XML_TYPE_END_ELEMENT;
                if (XMLRead::depth > 0)
                    XMLRead::depth--;
                XMLRead::idx = (int)(&cptr2[i] - XMLRead::xml + 1);
                XMLRead::state = xmlParseState::closedElement;
                break;
            }
            else if (cptr[i] == '<')
            {
                cptr = &cptr[i + 1];
                for (i = 0; cptr[i]; i++)
                {
                    if (cptr[i] == '>' || isspace(cptr[i]))
                    {
                        if (i > 0)
                        {
                            maxLen = (size_t)(i);
                            strncpy_s(XMLRead::name, cptr, maxLen);
                            XMLRead::name[i] = 0;
                        }
                        else
                            XMLRead::name[0] = 0;
                        XMLRead::value[0] = 0;
                        XMLRead::type = XML_TYPE_ELEMENT;
                        XMLRead::idx = (int)(&cptr[i] - XMLRead::xml + 1);
                        XMLRead::depth++;
                        XMLRead::state = xmlParseState::foundElement;
                        break;
                    }
                }
                break;
            }
        }
    }
    else if (XMLRead::state == xmlParseState::foundElement)
    {
        for (i = 0; cptr[i]; i++)
        {
            if (!isspace(cptr[i]))
            {
                cptr = &cptr[i];
                break;
            }
        }
        if (strncmp(cptr, "</", 2) == 0)
        {
            cptr2 = &cptr[2];
            for (i = 0; cptr2[i]; i++)
            {
                if (cptr2[i] == '>' || isspace(cptr2[i]))
                    break;
            }
            strncpy_s(XMLRead::name, cptr2, size_t(i));
            XMLRead::name[i] = 0;
            XMLRead::value[0] = 0;
            XMLRead::type = XML_TYPE_END_ELEMENT;
            if (XMLRead::depth > 0)
                XMLRead::depth--;
            XMLRead::idx = (int)(&cptr2[i] - XMLRead::xml + 1);
            XMLRead::state = xmlParseState::closedElement;
        }
        else if (cptr[0] == '<')
        {
            cptr++;
            for (i = 0; cptr[i]; i++)
            {
                if (cptr[i] == '>' || isspace(cptr[i]))
                {
                    strncpy_s(XMLRead::name, cptr, size_t(i));
                    XMLRead::name[i] = 0;
                    XMLRead::value[0] = 0;
                    XMLRead::type = XML_TYPE_ELEMENT;
                    XMLRead::depth++;
                    XMLRead::idx = (int)(&cptr[i] - XMLRead::xml + 1);
                    XMLRead::state = xmlParseState::foundElement;
                    break;
                }
            }
        }
        else
        {
            for (i = 0; cptr[i]; i++)
            {
                if (cptr[i] == '<')
                {
                    XMLRead::value[i] = 0;
                    XMLRead::state = xmlParseState::returnedText;
                    XMLRead::type = XML_TYPE_TEXT;
                    XMLRead::idx = (int)(&cptr[i] - XMLRead::xml);
                    break;
                }
                else
                    XMLRead::value[i] = cptr[i];
            }
        }
    }
    else if (XMLRead::state == xmlParseState::returnedText)
    {
        for (i = 0; cptr[i]; i++)
        {
            if (!isspace(cptr[i]))
            {
                cptr = &cptr[i];
                break;
            }
        }
        if (strncmp(cptr, "</", 2) == 0)
        {
            cptr2 = &cptr[2];
            for (i = 0; cptr2[i]; i++)
            {
                if (cptr2[i] == '>')
                {
                    cptr2 = &cptr2[i];
                    break;
                }
            }
            strncpy_s(XMLRead::name, &cptr[2], size_t(i));
            XMLRead::name[i + 1] = 0;
            XMLRead::value[0] = 0;
            XMLRead::type = XML_TYPE_END_ELEMENT;
            if (XMLRead::depth > 0)
                XMLRead::depth--;
            XMLRead::idx = XMLRead::idx + i;
            XMLRead::state = xmlParseState::closedElement;
        }
        else if (cptr[0] == '<')
        {
            cptr++;
            for (i = 0; cptr[i]; i++)
            {
                if (cptr[i] == '>' || isspace(cptr[i]))
                {
                    strncpy_s(XMLRead::name, cptr, size_t(i));
                    XMLRead::name[i] = 0;
                    XMLRead::value[0] = 0;
                    XMLRead::type = XML_TYPE_ELEMENT;
                    XMLRead::depth++;
                    XMLRead::idx = (int)(&cptr[i] - XMLRead::xml + 1);
                    XMLRead::state = xmlParseState::foundElement;
                    break;
                }
            }
        }
        else
        {
            XMLRead::name[0] = 0;
            XMLRead::value[0] = 0;
            XMLRead::type = XML_TYPE_NONE;
            XMLRead::idx = (int)(&cptr[i] - XMLRead::xml + 1);
            XMLRead::state = xmlParseState::initial;
        }
    }
    if (XMLRead::type == XML_TYPE_FILE_END)
        return (1);
    else
        return (0);
}

#ifdef _WIN32
void DisplayError(LPTSTR lpszFunction)
// Routine Description:
// Retrieve and output the system error message for the last-error code
{
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();
    SIZE_T length;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    length = lstrlen((LPCTSTR)lpMsgBuf);
    length += lstrlen((LPCTSTR)lpszFunction);
    length += 40;
    length *= sizeof(TCHAR);
    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, length);
    if (lpDisplayBuf)
    {
        if (FAILED(StringCchPrintf((LPTSTR)lpDisplayBuf,
            LocalSize(lpDisplayBuf) / sizeof(TCHAR),
            TEXT("%s failed with error code %d as follows:\n%s"),
            lpszFunction, dw, (LPTSTR)lpMsgBuf)))
        {
            printf("FATAL ERROR: Unable to output error code.\n");
        }
        _tprintf(TEXT("ERROR: %s\n"), (LPCTSTR)lpDisplayBuf);
        LocalFree(lpDisplayBuf);
    }
    LocalFree(lpMsgBuf);
}
#endif  // _WIN32
